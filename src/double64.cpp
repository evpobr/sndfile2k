/*
** Copyright (C) 1999-2015 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#define DOUBLE64_READ double64_le_read
#define DOUBLE64_WRITE double64_le_write
#elif CPU_IS_BIG_ENDIAN
#define DOUBLE64_READ double64_be_read
#define DOUBLE64_WRITE double64_be_write
#endif

/* A 32 number which will not overflow when multiplied by sizeof (double). */
#define SENSIBLE_LEN (0x8000000)

/*
 * Processor floating point capabilities. double64_get_capability () returns one of the
 * latter three values.
 */

enum
{
    DOUBLE_UNKNOWN = 0x00,
    DOUBLE_CAN_RW_LE = 0x23,
    DOUBLE_CAN_RW_BE = 0x34,
    DOUBLE_BROKEN_LE = 0x45,
    DOUBLE_BROKEN_BE = 0x56
};

/*
 * Prototypes for private functions.
 */

static size_t host_read_d2s(SndFile *psf, short *ptr, size_t len);
static size_t host_read_d2i(SndFile *psf, int *ptr, size_t len);
static size_t host_read_d2f(SndFile *psf, float *ptr, size_t len);
static size_t host_read_d(SndFile *psf, double *ptr, size_t len);

static size_t host_write_s2d(SndFile *psf, const short *ptr, size_t len);
static size_t host_write_i2d(SndFile *psf, const int *ptr, size_t len);
static size_t host_write_f2d(SndFile *psf, const float *ptr, size_t len);
static size_t host_write_d(SndFile *psf, const double *ptr, size_t len);

static void double64_peak_update(SndFile *psf, const double *buffer, size_t count,
                                 size_t size_t);

static int double64_get_capability(SndFile *psf);

static size_t replace_read_d2s(SndFile *psf, short *ptr, size_t len);
static size_t replace_read_d2i(SndFile *psf, int *ptr, size_t len);
static size_t replace_read_d2f(SndFile *psf, float *ptr, size_t len);
static size_t replace_read_d(SndFile *psf, double *ptr, size_t len);

static size_t replace_write_s2d(SndFile *psf, const short *ptr, size_t len);
static size_t replace_write_i2d(SndFile *psf, const int *ptr, size_t len);
static size_t replace_write_f2d(SndFile *psf, const float *ptr, size_t len);
static size_t replace_write_d(SndFile *psf, const double *ptr, size_t len);

static void d2bd_read(double *buffer, size_t count);
static void bd2d_write(double *buffer, size_t count);

/*
 * Exported functions.
 */

int double64_init(SndFile *psf)
{
    static int double64_caps;

    if (psf->sf.channels < 1 || psf->sf.channels > SF_MAX_CHANNELS)
    {
        psf->log_printf("double64_init : internal error : channels = %d\n", psf->sf.channels);
        return SFE_INTERNAL;
    };

    double64_caps = double64_get_capability(psf);

    psf->m_blockwidth = sizeof(double) * psf->sf.channels;

    if (psf->m_mode == SFM_READ || psf->m_mode == SFM_RDWR)
    {
        switch (psf->m_endian + double64_caps)
        {
        case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = host_read_d2s;
            psf->read_int = host_read_d2i;
            psf->read_float = host_read_d2f;
            psf->read_double = host_read_d;
            break;

        case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = host_read_d2s;
            psf->read_int = host_read_d2i;
            psf->read_float = host_read_d2f;
            psf->read_double = host_read_d;
            break;

        case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_LE):
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = host_read_d2s;
            psf->read_int = host_read_d2i;
            psf->read_float = host_read_d2f;
            psf->read_double = host_read_d;
            break;

        case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = host_read_d2s;
            psf->read_int = host_read_d2i;
            psf->read_float = host_read_d2f;
            psf->read_double = host_read_d;
            break;

        /* When the CPU is not IEEE compatible. */
        case (SF_ENDIAN_BIG + DOUBLE_BROKEN_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = replace_read_d2s;
            psf->read_int = replace_read_d2i;
            psf->read_float = replace_read_d2f;
            psf->read_double = replace_read_d;
            break;

        case (SF_ENDIAN_LITTLE + DOUBLE_BROKEN_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->read_short = replace_read_d2s;
            psf->read_int = replace_read_d2i;
            psf->read_float = replace_read_d2f;
            psf->read_double = replace_read_d;
            break;

        case (SF_ENDIAN_BIG + DOUBLE_BROKEN_LE):
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = replace_read_d2s;
            psf->read_int = replace_read_d2i;
            psf->read_float = replace_read_d2f;
            psf->read_double = replace_read_d;
            break;

        case (SF_ENDIAN_LITTLE + DOUBLE_BROKEN_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->read_short = replace_read_d2s;
            psf->read_int = replace_read_d2i;
            psf->read_float = replace_read_d2f;
            psf->read_double = replace_read_d;
            break;

        default:
            break;
        };
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        switch (psf->m_endian + double64_caps)
        {
        case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = host_write_s2d;
            psf->write_int = host_write_i2d;
            psf->write_float = host_write_f2d;
            psf->write_double = host_write_d;
            break;

        case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = host_write_s2d;
            psf->write_int = host_write_i2d;
            psf->write_float = host_write_f2d;
            psf->write_double = host_write_d;
            break;

        case (SF_ENDIAN_BIG + DOUBLE_CAN_RW_LE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = host_write_s2d;
            psf->write_int = host_write_i2d;
            psf->write_float = host_write_f2d;
            psf->write_double = host_write_d;
            break;

        case (SF_ENDIAN_LITTLE + DOUBLE_CAN_RW_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = host_write_s2d;
            psf->write_int = host_write_i2d;
            psf->write_float = host_write_f2d;
            psf->write_double = host_write_d;
            break;

        /* When the CPU is not IEEE compatible. */
        case (SF_ENDIAN_LITTLE + DOUBLE_BROKEN_LE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = replace_write_s2d;
            psf->write_int = replace_write_i2d;
            psf->write_float = replace_write_f2d;
            psf->write_double = replace_write_d;
            break;

        case (SF_ENDIAN_BIG + DOUBLE_BROKEN_BE):
            psf->m_data_endswap = SF_FALSE;
            psf->write_short = replace_write_s2d;
            psf->write_int = replace_write_i2d;
            psf->write_float = replace_write_f2d;
            psf->write_double = replace_write_d;
            break;

        case (SF_ENDIAN_BIG + DOUBLE_BROKEN_LE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = replace_write_s2d;
            psf->write_int = replace_write_i2d;
            psf->write_float = replace_write_f2d;
            psf->write_double = replace_write_d;
            break;

        case (SF_ENDIAN_LITTLE + DOUBLE_BROKEN_BE):
            psf->m_data_endswap = SF_TRUE;
            psf->write_short = replace_write_s2d;
            psf->write_int = replace_write_i2d;
            psf->write_float = replace_write_f2d;
            psf->write_double = replace_write_d;
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

    psf->sf.frames = psf->m_datalength / psf->m_blockwidth;

    return 0;
}

/*----------------------------------------------------------------------------
** From : http://www.hpcf.cam.ac.uk/fp_formats.html
**
** 64 bit double precision layout (big endian)
** 	  Sign				bit 0
** 	  Exponent			bits 1-11
** 	  Mantissa			bits 12-63
** 	  Exponent Offset	1023
**
**            double             single
**
** +INF     7FF0000000000000     7F800000
** -INF     FFF0000000000000     FF800000
**  NaN     7FF0000000000001     7F800001
**                to               to
**          7FFFFFFFFFFFFFFF     7FFFFFFF
**                and              and
**          FFF0000000000001     FF800001
**                to               to
**          FFFFFFFFFFFFFFFF     FFFFFFFF
** +OVER    7FEFFFFFFFFFFFFF     7F7FFFFF
** -OVER    FFEFFFFFFFFFFFFF     FF7FFFFF
** +UNDER   0010000000000000     00800000
** -UNDER   8010000000000000     80800000
*/

double double64_be_read(const unsigned char *cptr)
{
    int exponent, negative, upper, lower;
    double dvalue;

    negative = (cptr[0] & 0x80) ? 1 : 0;
    exponent = ((cptr[0] & 0x7F) << 4) | ((cptr[1] >> 4) & 0xF);

    /* Might not have a 64 bit long, so load the mantissa into a double. */
    upper = (((cptr[1] & 0xF) << 24) | (cptr[2] << 16) | (cptr[3] << 8) | cptr[4]);
    lower = (cptr[5] << 16) | (cptr[6] << 8) | cptr[7];

    if (exponent == 0 && upper == 0 && lower == 0)
        return 0.0;

    dvalue = upper + lower / ((double)0x1000000);
    dvalue += 0x10000000;

    exponent = exponent - 0x3FF;

    dvalue = dvalue / ((double)0x10000000);

    if (negative)
        dvalue *= -1;

    if (exponent > 0)
        dvalue *= pow(2.0, exponent);
    else if (exponent < 0)
        dvalue /= pow(2.0, abs(exponent));

    return dvalue;
}

double double64_le_read(const unsigned char *cptr)
{
    int exponent, negative, upper, lower;
    double dvalue;

    negative = (cptr[7] & 0x80) ? 1 : 0;
    exponent = ((cptr[7] & 0x7F) << 4) | ((cptr[6] >> 4) & 0xF);

    /* Might not have a 64 bit long, so load the mantissa into a double. */
    upper = ((cptr[6] & 0xF) << 24) | (cptr[5] << 16) | (cptr[4] << 8) | cptr[3];
    lower = (cptr[2] << 16) | (cptr[1] << 8) | cptr[0];

    if (exponent == 0 && upper == 0 && lower == 0)
        return 0.0;

    dvalue = upper + lower / ((double)0x1000000);
    dvalue += 0x10000000;

    exponent = exponent - 0x3FF;

    dvalue = dvalue / ((double)0x10000000);

    if (negative)
        dvalue *= -1;

    if (exponent > 0)
        dvalue *= pow(2.0, exponent);
    else if (exponent < 0)
        dvalue /= pow(2.0, abs(exponent));

    return dvalue;
}

void double64_be_write(double in, unsigned char *out)
{
    int exponent, mantissa;

    memset(out, 0, sizeof(double));

    if (fabs(in) < 1e-30)
        return;

    if (in < 0.0)
    {
        in *= -1.0;
        out[0] |= 0x80;
    };

    in = frexp(in, &exponent);

    exponent += 1022;

    out[0] |= (exponent >> 4) & 0x7F;
    out[1] |= (exponent << 4) & 0xF0;

    in *= 0x20000000;
    mantissa = lrint(floor(in));

    out[1] |= (mantissa >> 24) & 0xF;
    out[2] = (mantissa >> 16) & 0xFF;
    out[3] = (mantissa >> 8) & 0xFF;
    out[4] = mantissa & 0xFF;

    in = fmod(in, 1.0);
    in *= 0x1000000;
    mantissa = lrint(floor(in));

    out[5] = (mantissa >> 16) & 0xFF;
    out[6] = (mantissa >> 8) & 0xFF;
    out[7] = mantissa & 0xFF;

    return;
}

void double64_le_write(double in, unsigned char *out)
{
    int exponent, mantissa;

    memset(out, 0, sizeof(double));

    if (fabs(in) < 1e-30)
        return;

    if (in < 0.0)
    {
        in *= -1.0;
        out[7] |= 0x80;
    };

    in = frexp(in, &exponent);

    exponent += 1022;

    out[7] |= (exponent >> 4) & 0x7F;
    out[6] |= (exponent << 4) & 0xF0;

    in *= 0x20000000;
    mantissa = lrint(floor(in));

    out[6] |= (mantissa >> 24) & 0xF;
    out[5] = (mantissa >> 16) & 0xFF;
    out[4] = (mantissa >> 8) & 0xFF;
    out[3] = mantissa & 0xFF;

    in = fmod(in, 1.0);
    in *= 0x1000000;
    mantissa = lrint(floor(in));

    out[2] = (mantissa >> 16) & 0xFF;
    out[1] = (mantissa >> 8) & 0xFF;
    out[0] = mantissa & 0xFF;

    return;
}

static void double64_peak_update(SndFile *psf, const double *buffer, size_t count, size_t indx)
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

static int double64_get_capability(SndFile *psf)
{
    union
    {
        double d;
        unsigned char c[8];
    } data;

    data.d = 1.234567890123456789; /* Some abitrary value. */

    if (!psf->m_ieee_replace)
    {
        /* If this test is true ints and floats are compatible and little endian. */
        if (data.c[0] == 0xfb && data.c[1] == 0x59 && data.c[2] == 0x8c && data.c[3] == 0x42 &&
            data.c[4] == 0xca && data.c[5] == 0xc0 && data.c[6] == 0xf3 && data.c[7] == 0x3f)
            return DOUBLE_CAN_RW_LE;

        /* If this test is true ints and floats are compatible and big endian. */
        if (data.c[0] == 0x3f && data.c[1] == 0xf3 && data.c[2] == 0xc0 && data.c[3] == 0xca &&
            data.c[4] == 0x42 && data.c[5] == 0x8c && data.c[6] == 0x59 && data.c[7] == 0xfb)
            return DOUBLE_CAN_RW_BE;
    };

    /* Doubles are broken. Don't expect reading or writing to be fast. */
    psf->log_printf("Using IEEE replacement code for double.\n");

    return (CPU_IS_LITTLE_ENDIAN) ? DOUBLE_BROKEN_LE : DOUBLE_BROKEN_BE;
}

static void d2s_array(const double *src, size_t count, short *dest, double scale)
{
    while (count)
    {
        count--;
        dest[count] = (short)lrint(scale * src[count]);
    };
}

static void d2s_clip_array(const double *src, size_t count, short *dest, double scale)
{
    while (count)
    {
        count--;
        double tmp = scale * src[count];

        if (CPU_CLIPS_POSITIVE == 0 && tmp > 32767.0)
            dest[count] = SHRT_MAX;
        else if (CPU_CLIPS_NEGATIVE == 0 && tmp < -32768.0)
            dest[count] = SHRT_MIN;
        else
            dest[count] = (short)lrint(tmp);
    };
}

static void d2i_array(const double *src, size_t count, int *dest, double scale)
{
    while (count)
    {
        count--;
        dest[count] = lrint(scale * src[count]);
    };
}

static void d2i_clip_array(const double *src, size_t count, int *dest, double scale)
{
    while (count)
    {
        count--;
        float tmp = (float)(scale * src[count]);

        if (CPU_CLIPS_POSITIVE == 0 && tmp > (1.0 * INT_MAX))
            dest[count] = INT_MAX;
        else if (CPU_CLIPS_NEGATIVE == 0 && tmp < (-1.0 * INT_MAX))
            dest[count] = INT_MIN;
        else
            dest[count] = lrint(tmp);
    };
}

static inline void d2f_array(const double *src, size_t count, float *dest)
{
    while (count)
    {
        count--;
        dest[count] = (float)src[count];
    };
}

static inline void s2d_array(const short *src, double *dest, size_t count, double scale)
{
    while (count)
    {
        count--;
        dest[count] = scale * src[count];
    };
}

static inline void i2d_array(const int *src, double *dest, size_t count, double scale)
{
    while (count)
    {
        count--;
        dest[count] = scale * src[count];
    };
}

static inline void f2d_array(const float *src, double *dest, size_t count)
{
    while (count)
    {
        count--;
        dest[count] = src[count];
    };
}

static size_t host_read_d2s(SndFile *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, size_t, short *, double);
    size_t bufferlen, readcount;
    size_t total = 0;
    double scale;

    convert = (psf->m_add_clipping) ? d2s_clip_array : d2s_array;
    bufferlen = ARRAY_LEN(ubuf.dbuf);
    scale = (psf->m_float_int_mult == 0) ? 1.0 : 0x7FFF / psf->m_float_max;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.dbuf, sizeof(double), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, readcount);

        convert(ubuf.dbuf, readcount, ptr + total, scale);
        total += readcount;
        len -= readcount;
        if (readcount < bufferlen)
            break;
    };

    return total;
}

static size_t host_read_d2i(SndFile *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, size_t, int *, double);
    size_t bufferlen, readcount;
    size_t total = 0;
    double scale;

    convert = (psf->m_add_clipping) ? d2i_clip_array : d2i_array;
    bufferlen = ARRAY_LEN(ubuf.dbuf);
    scale = (psf->m_float_int_mult == 0) ? 1.0 : 0x7FFFFFFF / psf->m_float_max;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.dbuf, sizeof(double), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        convert(ubuf.dbuf, readcount, ptr + total, scale);
        total += readcount;
        len -= readcount;
        if (readcount < bufferlen)
            break;
    };

    return total;
}

static size_t host_read_d2f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.dbuf, sizeof(double), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        d2f_array(ubuf.dbuf, readcount, ptr + total);
        total += readcount;
        len -= readcount;
        if (readcount < bufferlen)
            break;
    };

    return total;
}

static size_t host_read_d(SndFile *psf, double *ptr, size_t len)
{
    size_t bufferlen;
    size_t readcount, total = 0;

    readcount = psf->fread(ptr, sizeof(double), len);

    if (psf->m_data_endswap != SF_TRUE)
        return readcount;

    /* If the read length was sensible, endswap output in one go. */
    if (readcount < SENSIBLE_LEN)
    {
        endswap_double_array(ptr, readcount);
        return readcount;
    };

    bufferlen = SENSIBLE_LEN;
    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;

        endswap_double_array(ptr + total, bufferlen);

        total += bufferlen;
        len -= bufferlen;
    };

    return total;
}

static size_t host_write_s2d(SndFile *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    double scale;

    scale = (psf->m_scale_int_float == 0) ? 1.0 : 1.0 / 0x8000;
    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;

        s2d_array(ptr + total, ubuf.dbuf, bufferlen, scale);

        if (psf->m_peak_info)
            double64_peak_update(psf, ubuf.dbuf, bufferlen, total / psf->sf.channels);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t host_write_i2d(SndFile *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    double scale;

    scale = (psf->m_scale_int_float == 0) ? 1.0 : 1.0 / (8.0 * 0x10000000);
    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2d_array(ptr + total, ubuf.dbuf, bufferlen, scale);

        if (psf->m_peak_info)
            double64_peak_update(psf, ubuf.dbuf, bufferlen, total / psf->sf.channels);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t host_write_f2d(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        f2d_array(ptr + total, ubuf.dbuf, bufferlen);

        if (psf->m_peak_info)
            double64_peak_update(psf, ubuf.dbuf, bufferlen, total / psf->sf.channels);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t host_write_d(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    if (psf->m_peak_info)
        double64_peak_update(psf, ptr, len, 0);

    if (psf->m_data_endswap != SF_TRUE)
        return psf->fwrite(ptr, sizeof(double), len);

    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;

        endswap_double_copy(ubuf.dbuf, ptr + total, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_read_d2s(SndFile *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double scale;

    bufferlen = ARRAY_LEN(ubuf.dbuf);
    scale = (psf->m_float_int_mult == 0) ? 1.0 : 0x7FFF / psf->m_float_max;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.dbuf, sizeof(double), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        d2bd_read(ubuf.dbuf, bufferlen);

        d2s_array(ubuf.dbuf, readcount, ptr + total, scale);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_read_d2i(SndFile *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double scale;

    bufferlen = ARRAY_LEN(ubuf.dbuf);
    scale = (psf->m_float_int_mult == 0) ? 1.0 : 0x7FFFFFFF / psf->m_float_max;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.dbuf, sizeof(double), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        d2bd_read(ubuf.dbuf, bufferlen);

        d2i_array(ubuf.dbuf, readcount, ptr + total, scale);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_read_d2f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.dbuf, sizeof(double), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        d2bd_read(ubuf.dbuf, bufferlen);

        memcpy(ptr + total, ubuf.dbuf, bufferlen * sizeof(double));

        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_read_d(SndFile *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    /* FIXME : This is probably nowhere near optimal. */
    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.dbuf, sizeof(double), bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, readcount);

        d2bd_read(ubuf.dbuf, readcount);

        memcpy(ptr + total, ubuf.dbuf, readcount * sizeof(double));

        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t replace_write_s2d(SndFile *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    double scale;

    scale = (psf->m_scale_int_float == 0) ? 1.0 : 1.0 / 0x8000;
    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2d_array(ptr + total, ubuf.dbuf, bufferlen, scale);

        if (psf->m_peak_info)
            double64_peak_update(psf, ubuf.dbuf, bufferlen, total / psf->sf.channels);

        bd2d_write(ubuf.dbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_write_i2d(SndFile *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;
    double scale;

    scale = (psf->m_scale_int_float == 0) ? 1.0 : 1.0 / (8.0 * 0x10000000);
    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2d_array(ptr + total, ubuf.dbuf, bufferlen, scale);

        if (psf->m_peak_info)
            double64_peak_update(psf, ubuf.dbuf, bufferlen, total / psf->sf.channels);

        bd2d_write(ubuf.dbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_write_f2d(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        f2d_array(ptr + total, ubuf.dbuf, bufferlen);

        bd2d_write(ubuf.dbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t replace_write_d(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    /* FIXME : This is probably nowhere near optimal. */
    if (psf->m_peak_info)
        double64_peak_update(psf, ptr, len, 0);

    bufferlen = ARRAY_LEN(ubuf.dbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;

        memcpy(ubuf.dbuf, ptr + total, bufferlen * sizeof(double));

        bd2d_write(ubuf.dbuf, bufferlen);

        if (psf->m_data_endswap == SF_TRUE)
            endswap_double_array(ubuf.dbuf, bufferlen);

        writecount = psf->fwrite(ubuf.dbuf, sizeof(double), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2bd_read(double *buffer, size_t count)
{
    while (count)
    {
        count--;
        buffer[count] = DOUBLE64_READ((unsigned char *)(buffer + count));
    };
}

static void bd2d_write(double *buffer, size_t count)
{
    while (count)
    {
        count--;
        DOUBLE64_WRITE(buffer[count], (unsigned char *)(buffer + count));
    };
}
