/*
** Copyright (C) 1999-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

#if CPU_IS_LITTLE_ENDIAN
#define FLOAT32_READ float32_le_read
#define FLOAT32_WRITE float32_le_write
#elif CPU_IS_BIG_ENDIAN
#define FLOAT32_READ float32_be_read
#define FLOAT32_WRITE float32_be_write
#endif

/*--------------------------------------------------------------------------------------------
 * Processor floating point capabilities. float32_get_capability () returns one of the
 * latter four values.
 */

enum
{
    FLOAT_UNKNOWN = 0x00,
    FLOAT_CAN_RW_LE = 0x12,
    FLOAT_CAN_RW_BE = 0x23,
    FLOAT_BROKEN_LE = 0x34,
    FLOAT_BROKEN_BE = 0x45
};

/*--------------------------------------------------------------------------------------------
 * Prototypes for private functions.
 */

static size_t host_read_f2s(SndFile *psf, short *ptr, size_t len);
static size_t host_read_f2i(SndFile *psf, int *ptr, size_t len);
static size_t host_read_f(SndFile *psf, float *ptr, size_t len);
static size_t host_read_f2d(SndFile *psf, double *ptr, size_t len);

static size_t host_write_s2f(SndFile *psf, const short *ptr, size_t len);
static size_t host_write_i2f(SndFile *psf, const int *ptr, size_t len);
static size_t host_write_f(SndFile *psf, const float *ptr, size_t len);
static size_t host_write_d2f(SndFile *psf, const double *ptr, size_t len);

static void float32_peak_update(SndFile *psf, const float *buffer, size_t count, size_t indx);

static size_t replace_read_f2s(SndFile *psf, short *ptr, size_t len);
static size_t replace_read_f2i(SndFile *psf, int *ptr, size_t len);
static size_t replace_read_f(SndFile *psf, float *ptr, size_t len);
static size_t replace_read_f2d(SndFile *psf, double *ptr, size_t len);

static size_t replace_write_s2f(SndFile *psf, const short *ptr, size_t len);
static size_t replace_write_i2f(SndFile *psf, const int *ptr, size_t len);
static size_t replace_write_f(SndFile *psf, const float *ptr, size_t len);
static size_t replace_write_d2f(SndFile *psf, const double *ptr, size_t len);

static void bf2f_array(float *buffer, size_t count);
static void f2bf_array(float *buffer, size_t count);

static int float32_get_capability(SndFile *psf);

/*
 * Exported functions.
 */

int float32_init(SndFile *psf)
{
    static int float_caps;

    if (psf->sf.channels < 1)
    {
        psf->log_printf("float32_init : internal error : channels = %d\n", psf->sf.channels);
        return SFE_INTERNAL;
    };

    float_caps = float32_get_capability(psf);

    psf->m_blockwidth = sizeof(float) * psf->sf.channels;

    if (psf->m_mode == SFM_READ || psf->m_mode == SFM_RDWR)
    {
        switch (psf->m_endian + float_caps)
        {
        case (SF_ENDIAN_BIG + FLOAT_CAN_RW_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = host_read_f2s;
            psf->read_int = host_read_f2i;
            psf->read_float = host_read_f;
            psf->read_double = host_read_f2d;
            break;

        case (SF_ENDIAN_LITTLE + FLOAT_CAN_RW_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = host_read_f2s;
            psf->read_int = host_read_f2i;
            psf->read_float = host_read_f;
            psf->read_double = host_read_f2d;
            break;

        case (SF_ENDIAN_BIG + FLOAT_CAN_RW_LE):
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = host_read_f2s;
            psf->read_int = host_read_f2i;
            psf->read_float = host_read_f;
            psf->read_double = host_read_f2d;
            break;

        case (SF_ENDIAN_LITTLE + FLOAT_CAN_RW_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = host_read_f2s;
            psf->read_int = host_read_f2i;
            psf->read_float = host_read_f;
            psf->read_double = host_read_f2d;
            break;

        case (SF_ENDIAN_BIG + FLOAT_BROKEN_LE):
            /* When the CPU is not IEEE compatible. */
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = replace_read_f2s;
            psf->read_int = replace_read_f2i;
            psf->read_float = replace_read_f;
            psf->read_double = replace_read_f2d;
            break;

        case (SF_ENDIAN_LITTLE + FLOAT_BROKEN_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = replace_read_f2s;
            psf->read_int = replace_read_f2i;
            psf->read_float = replace_read_f;
            psf->read_double = replace_read_f2d;
            break;

        case (SF_ENDIAN_BIG + FLOAT_BROKEN_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = replace_read_f2s;
            psf->read_int = replace_read_f2i;
            psf->read_float = replace_read_f;
            psf->read_double = replace_read_f2d;
            break;

        case (SF_ENDIAN_LITTLE + FLOAT_BROKEN_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = replace_read_f2s;
            psf->read_int = replace_read_f2i;
            psf->read_float = replace_read_f;
            psf->read_double = replace_read_f2d;
            break;

        default:
            break;
        };
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        switch (psf->m_endian + float_caps)
        {
        case (SF_ENDIAN_LITTLE + FLOAT_CAN_RW_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = host_write_s2f;
            psf->write_int = host_write_i2f;
            psf->write_float = host_write_f;
            psf->write_double = host_write_d2f;
            break;

        case (SF_ENDIAN_BIG + FLOAT_CAN_RW_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = host_write_s2f;
            psf->write_int = host_write_i2f;
            psf->write_float = host_write_f;
            psf->write_double = host_write_d2f;
            break;

        case (SF_ENDIAN_BIG + FLOAT_CAN_RW_LE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = host_write_s2f;
            psf->write_int = host_write_i2f;
            psf->write_float = host_write_f;
            psf->write_double = host_write_d2f;
            break;

        case (SF_ENDIAN_LITTLE + FLOAT_CAN_RW_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = host_write_s2f;
            psf->write_int = host_write_i2f;
            psf->write_float = host_write_f;
            psf->write_double = host_write_d2f;
            break;

        /* When the CPU is not IEEE compatible. */
        case (SF_ENDIAN_BIG + FLOAT_BROKEN_LE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = replace_write_s2f;
            psf->write_int = replace_write_i2f;
            psf->write_float = replace_write_f;
            psf->write_double = replace_write_d2f;
            break;

        case (SF_ENDIAN_LITTLE + FLOAT_BROKEN_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = replace_write_s2f;
            psf->write_int = replace_write_i2f;
            psf->write_float = replace_write_f;
            psf->write_double = replace_write_d2f;
            break;

        case (SF_ENDIAN_BIG + FLOAT_BROKEN_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = replace_write_s2f;
            psf->write_int = replace_write_i2f;
            psf->write_float = replace_write_f;
            psf->write_double = replace_write_d2f;
            break;

        case (SF_ENDIAN_LITTLE + FLOAT_BROKEN_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = replace_write_s2f;
            psf->write_int = replace_write_i2f;
            psf->write_float = replace_write_f;
            psf->write_double = replace_write_d2f;
            break;

        default:
            break;
        };
    };

    if (psf->m_filelength > psf->m_dataoffset)
    {
        psf->m_datalength =
            (psf->m_dataend > 0) ? psf->m_dataend - psf->m_dataoffset : psf->m_filelength - psf->m_dataoffset;
    }
    else
        psf->m_datalength = 0;

    psf->sf.frames = psf->m_blockwidth > 0 ? psf->m_datalength / psf->m_blockwidth : 0;

    return 0;
}

float float32_be_read(const unsigned char *cptr)
{
    int exponent, mantissa, negative;
    float fvalue;

    negative = cptr[0] & 0x80;
    exponent = ((cptr[0] & 0x7F) << 1) | ((cptr[1] & 0x80) ? 1 : 0);
    mantissa = ((cptr[1] & 0x7F) << 16) | (cptr[2] << 8) | (cptr[3]);

    if (!(exponent || mantissa))
        return 0.0;

    mantissa |= 0x800000;
    exponent = exponent ? exponent - 127 : 0;

    fvalue = (float)(mantissa ? ((float)mantissa) / ((float)0x800000) : 0.0);

    if (negative)
        fvalue *= -1;

    if (exponent > 0)
        fvalue *= (float)pow(2.0, exponent);
    else if (exponent < 0)
        fvalue /= (float)pow(2.0, abs(exponent));

    return fvalue;
}

float float32_le_read(const unsigned char *cptr)
{
    int exponent, mantissa, negative;
    float fvalue;

    negative = cptr[3] & 0x80;
    exponent = ((cptr[3] & 0x7F) << 1) | ((cptr[2] & 0x80) ? 1 : 0);
    mantissa = ((cptr[2] & 0x7F) << 16) | (cptr[1] << 8) | (cptr[0]);

    if (!(exponent || mantissa))
        return 0.0;

    mantissa |= 0x800000;
    exponent = exponent ? exponent - 127 : 0;

    fvalue = (float)(mantissa ? ((float)mantissa) / ((float)0x800000) : 0.0);

    if (negative)
        fvalue *= -1;

    if (exponent > 0)
        fvalue *= (float)pow(2.0, exponent);
    else if (exponent < 0)
        fvalue /= (float)pow(2.0, abs(exponent));

    return fvalue;
}

void float32_le_write(float in, unsigned char *out)
{
    int exponent, mantissa, negative = 0;

    memset(out, 0, sizeof(int));

    if (fabs(in) < 1e-30)
        return;

    if (in < 0.0)
    {
        in *= -1.0;
        negative = 1;
    };

    in = (float)frexp(in, &exponent);

    exponent += 126;

    in *= (float)0x1000000;
    mantissa = (((int)in) & 0x7FFFFF);

    if (negative)
        out[3] |= 0x80;

    if (exponent & 0x01)
        out[2] |= 0x80;

    out[0] = mantissa & 0xFF;
    out[1] = (mantissa >> 8) & 0xFF;
    out[2] |= (mantissa >> 16) & 0x7F;
    out[3] |= (exponent >> 1) & 0x7F;

    return;
}

void float32_be_write(float in, unsigned char *out)
{
    int exponent, mantissa, negative = 0;

    memset(out, 0, sizeof(int));

    if (fabs(in) < 1e-30)
        return;

    if (in < 0.0)
    {
        in *= -1.0;
        negative = 1;
    };

    in = (float)frexp(in, &exponent);

    exponent += 126;

    in *= (float)0x1000000;
    mantissa = (((int)in) & 0x7FFFFF);

    if (negative)
        out[0] |= 0x80;

    if (exponent & 0x01)
        out[1] |= 0x80;

    out[3] = mantissa & 0xFF;
    out[2] = (mantissa >> 8) & 0xFF;
    out[1] |= (mantissa >> 16) & 0x7F;
    out[0] |= (exponent >> 1) & 0x7F;

    return;
}

static void float32_peak_update(SndFile *psf, const float *buffer, size_t count, size_t indx)
{
    int chan;
    size_t k, position;
    float fmaxval;

    for (chan = 0; chan < psf->sf.channels; chan++)
    {
        fmaxval = (float)fabs(buffer[chan]);
        position = 0;
        for (k = chan; k < count; k += psf->sf.channels)
            if (fmaxval < fabs(buffer[k]))
            {
                fmaxval = (float)fabs(buffer[k]);
                position = k;
            };

        if (fmaxval > psf->m_peak_info->peaks[chan].value)
        {
            psf->m_peak_info->peaks[chan].value = fmaxval;
            psf->m_peak_info->peaks[chan].position =
                psf->m_write_current + indx + (position / psf->sf.channels);
        };
    };

    return;
}

static int float32_get_capability(SndFile *psf)
{
    union
    {
        float f;
        int i;
        unsigned char c[4];
    } data;

    data.f = (float)1.23456789; /* Some abitrary value. */

    if (!psf->m_ieee_replace)
    {
        /* If this test is true ints and floats are compatible and little endian. */
        if (data.c[0] == 0x52 && data.c[1] == 0x06 && data.c[2] == 0x9e && data.c[3] == 0x3f)
            return FLOAT_CAN_RW_LE;

        /* If this test is true ints and floats are compatible and big endian. */
        if (data.c[3] == 0x52 && data.c[2] == 0x06 && data.c[1] == 0x9e && data.c[0] == 0x3f)
            return FLOAT_CAN_RW_BE;
    };

    /* Floats are broken. Don't expect reading or writing to be fast. */
    psf->log_printf("Using IEEE replacement code for float.\n");

    return (CPU_IS_LITTLE_ENDIAN) ? FLOAT_BROKEN_LE : FLOAT_BROKEN_BE;
}

static void f2s_array(const float *src, size_t count, short *dest, float scale)
{
    while (count)
    {
        count--;
        dest[count] = (short)lrintf(scale * src[count]);
    };
}

static void f2s_clip_array(const float *src, size_t count, short *dest, float scale)
{
    while (count)
    {
        count--;
        float tmp = scale * src[count];

        if (CPU_CLIPS_POSITIVE == 0 && tmp > 32767.0)
            dest[count] = SHRT_MAX;
        else if (CPU_CLIPS_NEGATIVE == 0 && tmp < -32768.0)
            dest[count] = SHRT_MIN;
        else
            dest[count] = (short)lrintf(tmp);
    };
}

static inline void f2i_array(const float *src, size_t count, int *dest, float scale)
{
    while (count)
    {
        count--;
        dest[count] = lrintf(scale * src[count]);
    };
}

static inline void f2i_clip_array(const float *src, size_t count, int *dest, float scale)
{
    while (count)
    {
        count--;
        float tmp = scale * src[count];

        if (CPU_CLIPS_POSITIVE == 0 && tmp > (1.0 * INT_MAX))
            dest[count] = INT_MAX;
        else if (CPU_CLIPS_NEGATIVE == 0 && tmp < (-1.0 * INT_MAX))
            dest[count] = INT_MIN;
        else
            dest[count] = lrintf(tmp);
    };
}

static inline void f2d_array(const float *src, size_t count, double *dest)
{
    while (count)
    {
        count--;
        dest[count] = src[count];
    };
}

static inline void s2f_array(const short *src, float *dest, size_t count, float scale)
{
    while (count)
    {
        count--;
        dest[count] = scale * src[count];
    };
}

static inline void i2f_array(const int *src, float *dest, size_t count, float scale)
{
    while (count)
    {
        count--;
        dest[count] = scale * src[count];
    };
}

static inline void d2f_array(const double *src, float *dest, size_t count)
{
    while (count)
    {
        count--;
        dest[count] = (float)src[count];
    };
}

static size_t host_read_f2s(SndFile *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, size_t, short *, float);
    size_t bufferlen, readcount;
    size_t total = 0;
    float scale;

    convert = (psf->m_add_clipping) ? f2s_clip_array : f2s_array;
    bufferlen = ARRAY_LEN(ubuf.fbuf);
    scale = (float)((psf->m_float_int_mult == 0) ? 1.0 : 0x7FFF / psf->m_float_max);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        /* Fix me : Need lef2s_array */
        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, readcount);

        convert(ubuf.fbuf, readcount, ptr + total, scale);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t host_read_f2i(SndFile *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, size_t, int *, float);
    size_t bufferlen, readcount;
    size_t total = 0;
    float scale;

    convert = (psf->m_add_clipping) ? f2i_clip_array : f2i_array;
    bufferlen = ARRAY_LEN(ubuf.fbuf);
    scale = (float)((psf->m_float_int_mult == 0) ? 1.0 : 0x7FFFFFFF / psf->m_float_max);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        convert(ubuf.fbuf, readcount, ptr + total, scale);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t host_read_f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    if (psf->m_data_endswap != SF_TRUE)
        return psf->fread(ptr, sizeof(float), len);

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        endswap_int_copy((int *)(ptr + total), ubuf.ibuf, readcount);

        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t host_read_f2d(SndFile *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        /* Fix me : Need lef2d_array */
        f2d_array(ubuf.fbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t host_write_s2f(SndFile *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    float scale;

    /* Erik */
    scale = (float)((psf->m_scale_int_float == 0) ? 1.0 : 1.0 / 0x8000);
    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2f_array(ptr + total, ubuf.fbuf, bufferlen, scale);

        if (psf->m_peak_info)
            float32_peak_update(psf, ubuf.fbuf, bufferlen, total / psf->sf.channels);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t host_write_i2f(SndFile *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    float scale;

    scale = (float)((psf->m_scale_int_float == 0) ? 1.0 : 1.0 / (8.0 * 0x10000000));
    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2f_array(ptr + total, ubuf.fbuf, bufferlen, scale);

        if (psf->m_peak_info)
            float32_peak_update(psf, ubuf.fbuf, bufferlen, total / psf->sf.channels);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t host_write_f(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    if (psf->m_peak_info)
        float32_peak_update(psf, ptr, len, 0);

    if (psf->m_data_endswap != SF_TRUE)
        return psf->fwrite(ptr, sizeof(float), len);

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;

        endswap_int_copy(ubuf.ibuf, (const int *)(ptr + total), bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t host_write_d2f(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;

        d2f_array(ptr + total, ubuf.fbuf, bufferlen);

        if (psf->m_peak_info)
            float32_peak_update(psf, ubuf.fbuf, bufferlen, total / psf->sf.channels);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_read_f2s(SndFile *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float scale;

    bufferlen = ARRAY_LEN(ubuf.fbuf);
    scale = (float)((psf->m_float_int_mult == 0) ? 1.0 : 0x7FFF / psf->m_float_max);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        bf2f_array(ubuf.fbuf, bufferlen);

        f2s_array(ubuf.fbuf, readcount, ptr + total, scale);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_read_f2i(SndFile *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float scale;

    bufferlen = ARRAY_LEN(ubuf.fbuf);
    scale = (float)((psf->m_float_int_mult == 0) ? 1.0 : 0x7FFF / psf->m_float_max);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        bf2f_array(ubuf.fbuf, bufferlen);

        f2i_array(ubuf.fbuf, readcount, ptr + total, scale);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_read_f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    /* FIX THIS */

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        bf2f_array(ubuf.fbuf, bufferlen);

        memcpy(ptr + total, ubuf.fbuf, bufferlen * sizeof(float));

        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_read_f2d(SndFile *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.fbuf, sizeof(float), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        bf2f_array(ubuf.fbuf, bufferlen);

        f2d_array(ubuf.fbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_write_s2f(SndFile *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    float scale;

    scale = (float)((psf->m_scale_int_float == 0) ? 1.0 : 1.0 / 0x8000);
    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2f_array(ptr + total, ubuf.fbuf, bufferlen, scale);

        if (psf->m_peak_info)
            float32_peak_update(psf, ubuf.fbuf, bufferlen, total / psf->sf.channels);

        f2bf_array(ubuf.fbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_write_i2f(SndFile *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    float scale;

    scale = (float)((psf->m_scale_int_float == 0) ? 1.0 : 1.0 / (8.0 * 0x10000000));
    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2f_array(ptr + total, ubuf.fbuf, bufferlen, scale);

        if (psf->m_peak_info)
            float32_peak_update(psf, ubuf.fbuf, bufferlen, total / psf->sf.channels);

        f2bf_array(ubuf.fbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_write_f(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    /* FIX THIS */
    if (psf->m_peak_info)
        float32_peak_update(psf, ptr, len, 0);

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;

        memcpy(ubuf.fbuf, ptr + total, bufferlen * sizeof(float));

        f2bf_array(ubuf.fbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_write_d2f(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.fbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        d2f_array(ptr + total, ubuf.fbuf, bufferlen);

        if (psf->m_peak_info)
            float32_peak_update(psf, ubuf.fbuf, bufferlen, total / psf->sf.channels);

        f2bf_array(ubuf.fbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_int_array(ubuf.ibuf, bufferlen);

        writecount = psf->fwrite(ubuf.fbuf, sizeof(float), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void bf2f_array(float *buffer, size_t count)
{
    while (count)
    {
        count--;
        buffer[count] = FLOAT32_READ((unsigned char *)(buffer + count));
    };
}

static void f2bf_array(float *buffer, size_t count)
{
    while (count)
    {
        count--;
        FLOAT32_WRITE(buffer[count], (unsigned char *)(buffer + count));
    };
}
