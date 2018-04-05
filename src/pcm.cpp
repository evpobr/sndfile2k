/*
** Copyright (C) 1999-2016 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#include <math.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"
#include "shift.h"

/*
 * Need to be able to handle 3 byte (24 bit) integers. So defined a
 * type and use SIZEOF_TRIBYTE instead of (tribyte).
 */

typedef void tribyte;

#define SIZEOF_TRIBYTE (3)

static size_t pcm_read_sc2s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t pcm_read_uc2s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t pcm_read_bes2s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t pcm_read_les2s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t pcm_read_bet2s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t pcm_read_let2s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t pcm_read_bei2s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t pcm_read_lei2s(SF_PRIVATE *psf, short *ptr, size_t len);

static size_t pcm_read_sc2i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t pcm_read_uc2i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t pcm_read_bes2i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t pcm_read_les2i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t pcm_read_bet2i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t pcm_read_let2i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t pcm_read_bei2i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t pcm_read_lei2i(SF_PRIVATE *psf, int *ptr, size_t len);

static size_t pcm_read_sc2f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t pcm_read_uc2f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t pcm_read_bes2f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t pcm_read_les2f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t pcm_read_bet2f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t pcm_read_let2f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t pcm_read_bei2f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t pcm_read_lei2f(SF_PRIVATE *psf, float *ptr, size_t len);

static size_t pcm_read_sc2d(SF_PRIVATE *psf, double *ptr, size_t len);
static size_t pcm_read_uc2d(SF_PRIVATE *psf, double *ptr, size_t len);
static size_t pcm_read_bes2d(SF_PRIVATE *psf, double *ptr, size_t len);
static size_t pcm_read_les2d(SF_PRIVATE *psf, double *ptr, size_t len);
static size_t pcm_read_bet2d(SF_PRIVATE *psf, double *ptr, size_t len);
static size_t pcm_read_let2d(SF_PRIVATE *psf, double *ptr, size_t len);
static size_t pcm_read_bei2d(SF_PRIVATE *psf, double *ptr, size_t len);
static size_t pcm_read_lei2d(SF_PRIVATE *psf, double *ptr, size_t len);

static size_t pcm_write_s2sc(SF_PRIVATE *psf, const short *ptr, size_t len);
static size_t pcm_write_s2uc(SF_PRIVATE *psf, const short *ptr, size_t len);
static size_t pcm_write_s2bes(SF_PRIVATE *psf, const short *ptr, size_t len);
static size_t pcm_write_s2les(SF_PRIVATE *psf, const short *ptr, size_t len);
static size_t pcm_write_s2bet(SF_PRIVATE *psf, const short *ptr, size_t len);
static size_t pcm_write_s2let(SF_PRIVATE *psf, const short *ptr, size_t len);
static size_t pcm_write_s2bei(SF_PRIVATE *psf, const short *ptr, size_t len);
static size_t pcm_write_s2lei(SF_PRIVATE *psf, const short *ptr, size_t len);

static size_t pcm_write_i2sc(SF_PRIVATE *psf, const int *ptr, size_t len);
static size_t pcm_write_i2uc(SF_PRIVATE *psf, const int *ptr, size_t len);
static size_t pcm_write_i2bes(SF_PRIVATE *psf, const int *ptr, size_t len);
static size_t pcm_write_i2les(SF_PRIVATE *psf, const int *ptr, size_t len);
static size_t pcm_write_i2bet(SF_PRIVATE *psf, const int *ptr, size_t len);
static size_t pcm_write_i2let(SF_PRIVATE *psf, const int *ptr, size_t len);
static size_t pcm_write_i2bei(SF_PRIVATE *psf, const int *ptr, size_t len);
static size_t pcm_write_i2lei(SF_PRIVATE *psf, const int *ptr, size_t len);

static size_t pcm_write_f2sc(SF_PRIVATE *psf, const float *ptr, size_t len);
static size_t pcm_write_f2uc(SF_PRIVATE *psf, const float *ptr, size_t len);
static size_t pcm_write_f2bes(SF_PRIVATE *psf, const float *ptr, size_t len);
static size_t pcm_write_f2les(SF_PRIVATE *psf, const float *ptr, size_t len);
static size_t pcm_write_f2bet(SF_PRIVATE *psf, const float *ptr, size_t len);
static size_t pcm_write_f2let(SF_PRIVATE *psf, const float *ptr, size_t len);
static size_t pcm_write_f2bei(SF_PRIVATE *psf, const float *ptr, size_t len);
static size_t pcm_write_f2lei(SF_PRIVATE *psf, const float *ptr, size_t len);

static size_t pcm_write_d2sc(SF_PRIVATE *psf, const double *ptr, size_t len);
static size_t pcm_write_d2uc(SF_PRIVATE *psf, const double *ptr, size_t len);
static size_t pcm_write_d2bes(SF_PRIVATE *psf, const double *ptr, size_t len);
static size_t pcm_write_d2les(SF_PRIVATE *psf, const double *ptr, size_t len);
static size_t pcm_write_d2bet(SF_PRIVATE *psf, const double *ptr, size_t len);
static size_t pcm_write_d2let(SF_PRIVATE *psf, const double *ptr, size_t len);
static size_t pcm_write_d2bei(SF_PRIVATE *psf, const double *ptr, size_t len);
static size_t pcm_write_d2lei(SF_PRIVATE *psf, const double *ptr, size_t len);

enum
{
    /* Char type for 8 bit files. */
    SF_CHARS_SIGNED = 200,
    SF_CHARS_UNSIGNED = 201
};

int pcm_init(SF_PRIVATE *psf)
{
    int chars = 0;

    if (psf->bytewidth == 0 || psf->sf.channels == 0)
    {
        psf->log_printf("pcm_init : internal error : bytewitdh = %d, channels = %d\n",
                        psf->bytewidth, psf->sf.channels);
        return SFE_INTERNAL;
    };

    psf->blockwidth = psf->bytewidth * psf->sf.channels;

    if ((SF_CODEC(psf->sf.format)) == SF_FORMAT_PCM_S8)
        chars = SF_CHARS_SIGNED;
    else if ((SF_CODEC(psf->sf.format)) == SF_FORMAT_PCM_U8)
        chars = SF_CHARS_UNSIGNED;

    if (CPU_IS_BIG_ENDIAN)
        psf->data_endswap = (psf->endian == SF_ENDIAN_BIG) ? SF_FALSE : SF_TRUE;
    else
        psf->data_endswap = (psf->endian == SF_ENDIAN_LITTLE) ? SF_FALSE : SF_TRUE;

    if (psf->file.mode == SFM_READ || psf->file.mode == SFM_RDWR)
    {
        switch (psf->bytewidth * 0x10000 + psf->endian + chars)
        {
        case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_SIGNED):
        case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_SIGNED):
            psf->read_short = pcm_read_sc2s;
            psf->read_int = pcm_read_sc2i;
            psf->read_float = pcm_read_sc2f;
            psf->read_double = pcm_read_sc2d;
            break;
        case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_UNSIGNED):
        case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_UNSIGNED):
            psf->read_short = pcm_read_uc2s;
            psf->read_int = pcm_read_uc2i;
            psf->read_float = pcm_read_uc2f;
            psf->read_double = pcm_read_uc2d;
            break;

        case (2 * 0x10000 + SF_ENDIAN_BIG):
            psf->read_short = pcm_read_bes2s;
            psf->read_int = pcm_read_bes2i;
            psf->read_float = pcm_read_bes2f;
            psf->read_double = pcm_read_bes2d;
            break;
        case (3 * 0x10000 + SF_ENDIAN_BIG):
            psf->read_short = pcm_read_bet2s;
            psf->read_int = pcm_read_bet2i;
            psf->read_float = pcm_read_bet2f;
            psf->read_double = pcm_read_bet2d;
            break;
        case (4 * 0x10000 + SF_ENDIAN_BIG):

            psf->read_short = pcm_read_bei2s;
            psf->read_int = pcm_read_bei2i;
            psf->read_float = pcm_read_bei2f;
            psf->read_double = pcm_read_bei2d;
            break;

        case (2 * 0x10000 + SF_ENDIAN_LITTLE):
            psf->read_short = pcm_read_les2s;
            psf->read_int = pcm_read_les2i;
            psf->read_float = pcm_read_les2f;
            psf->read_double = pcm_read_les2d;
            break;
        case (3 * 0x10000 + SF_ENDIAN_LITTLE):
            psf->read_short = pcm_read_let2s;
            psf->read_int = pcm_read_let2i;
            psf->read_float = pcm_read_let2f;
            psf->read_double = pcm_read_let2d;
            break;
        case (4 * 0x10000 + SF_ENDIAN_LITTLE):
            psf->read_short = pcm_read_lei2s;
            psf->read_int = pcm_read_lei2i;
            psf->read_float = pcm_read_lei2f;
            psf->read_double = pcm_read_lei2d;
            break;
        default:
            psf->log_printf("pcm.c returning SFE_UNIMPLEMENTED\nbytewidth "
                            "%d    endian %d\n",
                            psf->bytewidth, psf->endian);
            return SFE_UNIMPLEMENTED;
        };
    };

    if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
    {
        switch (psf->bytewidth * 0x10000 + psf->endian + chars)
        {
        case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_SIGNED):
        case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_SIGNED):
            psf->write_short = pcm_write_s2sc;
            psf->write_int = pcm_write_i2sc;
            psf->write_float = pcm_write_f2sc;
            psf->write_double = pcm_write_d2sc;
            break;
        case (0x10000 + SF_ENDIAN_BIG + SF_CHARS_UNSIGNED):
        case (0x10000 + SF_ENDIAN_LITTLE + SF_CHARS_UNSIGNED):
            psf->write_short = pcm_write_s2uc;
            psf->write_int = pcm_write_i2uc;
            psf->write_float = pcm_write_f2uc;
            psf->write_double = pcm_write_d2uc;
            break;

        case (2 * 0x10000 + SF_ENDIAN_BIG):
            psf->write_short = pcm_write_s2bes;
            psf->write_int = pcm_write_i2bes;
            psf->write_float = pcm_write_f2bes;
            psf->write_double = pcm_write_d2bes;
            break;

        case (3 * 0x10000 + SF_ENDIAN_BIG):
            psf->write_short = pcm_write_s2bet;
            psf->write_int = pcm_write_i2bet;
            psf->write_float = pcm_write_f2bet;
            psf->write_double = pcm_write_d2bet;
            break;

        case (4 * 0x10000 + SF_ENDIAN_BIG):
            psf->write_short = pcm_write_s2bei;
            psf->write_int = pcm_write_i2bei;
            psf->write_float = pcm_write_f2bei;
            psf->write_double = pcm_write_d2bei;
            break;

        case (2 * 0x10000 + SF_ENDIAN_LITTLE):
            psf->write_short = pcm_write_s2les;
            psf->write_int = pcm_write_i2les;
            psf->write_float = pcm_write_f2les;
            psf->write_double = pcm_write_d2les;
            break;

        case (3 * 0x10000 + SF_ENDIAN_LITTLE):
            psf->write_short = pcm_write_s2let;
            psf->write_int = pcm_write_i2let;
            psf->write_float = pcm_write_f2let;
            psf->write_double = pcm_write_d2let;
            break;

        case (4 * 0x10000 + SF_ENDIAN_LITTLE):
            psf->write_short = pcm_write_s2lei;
            psf->write_int = pcm_write_i2lei;
            psf->write_float = pcm_write_f2lei;
            psf->write_double = pcm_write_d2lei;
            break;

        default:
            psf->log_printf("pcm.c returning SFE_UNIMPLEMENTED\nbytewidth "
                            "%d    endian %d\n",
                            psf->bytewidth, psf->endian);
            return SFE_UNIMPLEMENTED;
        };
    };

    if (psf->filelength > psf->dataoffset)
        psf->datalength =
            (psf->dataend > 0) ? psf->dataend - psf->dataoffset : psf->filelength - psf->dataoffset;
    else
        psf->datalength = 0;

    psf->sf.frames = psf->blockwidth > 0 ? psf->datalength / psf->blockwidth : 0;

    return 0;
}

static inline void sc2s_array(signed char *src, size_t count, short *dest)
{
    while (count)
    {
        count--;
        dest[count] = ((uint16_t)src[count]) << 8;
    };
}

static inline void uc2s_array(unsigned char *src, size_t count, short *dest)
{
    while (count)
    {
        count--;
        dest[count] = (((uint32_t)src[count]) - 0x80) << 8;
    };
}

static inline void let2s_array(tribyte *src, size_t count, short *dest)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        dest[count] = LET2H_16_PTR(ucptr);
    };
}

static inline void bet2s_array(tribyte *src, size_t count, short *dest)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        dest[count] = BET2H_16_PTR(ucptr);
    };
}

static inline void lei2s_array(int *src, size_t count, short *dest)
{
    int value;

    while (count)
    {
        count--;
        value = LE2H_32(src[count]);
        dest[count] = value >> 16;
    };
}

static inline void bei2s_array(int *src, size_t count, short *dest)
{
    int value;

    while (count)
    {
        count--;
        value = BE2H_32(src[count]);
        dest[count] = value >> 16;
    };
}

static inline void sc2i_array(signed char *src, size_t count, int *dest)
{
    while (count)
    {
        count--;
        dest[count] = arith_shift_left((int)src[count], 24);
    };
}

static inline void uc2i_array(unsigned char *src, size_t count, int *dest)
{
    while (count)
    {
        count--;
        dest[count] = arith_shift_left(((int)src[count]) - 128, 24);
    };
}

static inline void bes2i_array(short *src, size_t count, int *dest)
{
    short value;

    while (count)
    {
        count--;
        value = BE2H_16(src[count]);
        dest[count] = arith_shift_left(value, 16);
    };
}

static inline void les2i_array(short *src, size_t count, int *dest)
{
    short value;

    while (count)
    {
        count--;
        value = LE2H_16(src[count]);
        dest[count] = arith_shift_left(value, 16);
    };
}

static inline void bet2i_array(tribyte *src, size_t count, int *dest)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        dest[count] = psf_get_be24(ucptr, 0);
    };
}

static inline void let2i_array(tribyte *src, size_t count, int *dest)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        dest[count] = psf_get_le24(ucptr, 0);
    };
}

static inline void sc2f_array(signed char *src, size_t count, float *dest, float normfact)
{
    while (count)
    {
        count--;
        dest[count] = ((float)src[count]) * normfact;
    }
}

static inline void uc2f_array(unsigned char *src, size_t count, float *dest, float normfact)
{
    while (count)
    {
        count--;
        dest[count] = (((int)src[count]) - 128) * normfact;
    }
}

static inline void les2f_array(short *src, size_t count, float *dest, float normfact)
{
    short value;

    while (count)
    {
        count--;
        value = src[count];
        value = LE2H_16(value);
        dest[count] = ((float)value) * normfact;
    };
}

static inline void bes2f_array(short *src, size_t count, float *dest, float normfact)
{
    short value;

    while (count)
    {
        count--;
        value = src[count];
        value = BE2H_16(value);
        dest[count] = ((float)value) * normfact;
    };
}

static inline void let2f_array(tribyte *src, size_t count, float *dest, float normfact)
{
    unsigned char *ucptr;
    int value;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        value = psf_get_le24(ucptr, 0);
        dest[count] = ((float)value) * normfact;
    };
}

static inline void bet2f_array(tribyte *src, size_t count, float *dest, float normfact)
{
    unsigned char *ucptr;
    int value;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        value = psf_get_be24(ucptr, 0);
        dest[count] = ((float)value) * normfact;
    };
}

static inline void lei2f_array(int *src, size_t count, float *dest, float normfact)
{
    int value;

    while (count)
    {
        count--;
        value = src[count];
        value = LE2H_32(value);
        dest[count] = ((float)value) * normfact;
    };
}

static inline void bei2f_array(int *src, size_t count, float *dest, float normfact)
{
    int value;

    while (count)
    {
        count--;
        value = src[count];
        value = BE2H_32(value);
        dest[count] = ((float)value) * normfact;
    };
}

static inline void sc2d_array(signed char *src, size_t count, double *dest, double normfact)
{
    while (count)
    {
        count--;
        dest[count] = ((double)src[count]) * normfact;
    }
}

static inline void uc2d_array(unsigned char *src, size_t count, double *dest, double normfact)
{
    while (count)
    {
        count--;
        dest[count] = (((int)src[count]) - 128) * normfact;
    }
}

static inline void les2d_array(short *src, size_t count, double *dest, double normfact)
{
    short value;

    while (count)
    {
        count--;
        value = src[count];
        value = LE2H_16(value);
        dest[count] = ((double)value) * normfact;
    };
}

static inline void bes2d_array(short *src, size_t count, double *dest, double normfact)
{
    short value;

    while (count)
    {
        count--;
        value = src[count];
        value = BE2H_16(value);
        dest[count] = ((double)value) * normfact;
    };
}

static inline void let2d_array(tribyte *src, size_t count, double *dest, double normfact)
{
    unsigned char *ucptr;
    int value;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        value = psf_get_le24(ucptr, 0);
        dest[count] = ((double)value) * normfact;
    };
}

static inline void bet2d_array(tribyte *src, size_t count, double *dest, double normfact)
{
    unsigned char *ucptr;
    int value;

    ucptr = ((unsigned char *)src) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        value = psf_get_be24(ucptr, 0);
        dest[count] = ((double)value) * normfact;
    };
}

static inline void lei2d_array(int *src, size_t count, double *dest, double normfact)
{
    int value;

    while (count)
    {
        count--;
        value = src[count];
        value = LE2H_32(value);
        dest[count] = ((double)value) * normfact;
    };
}

static inline void bei2d_array(int *src, size_t count, double *dest, double normfact)
{
    int value;

    while (count)
    {
        count--;
        value = src[count];
        value = BE2H_32(value);
        dest[count] = ((double)value) * normfact;
    };
}

static inline void s2sc_array(const short *src, signed char *dest, size_t count)
{
    while (count)
    {
        count--;
        dest[count] = src[count] >> 8;
    }
}

static inline void s2uc_array(const short *src, unsigned char *dest, size_t count)
{
    while (count)
    {
        count--;
        dest[count] = (src[count] >> 8) + 0x80;
    }
}

static inline void s2let_array(const short *src, tribyte *dest, size_t count)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)dest) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        ucptr[0] = 0;
        ucptr[1] = (unsigned char)src[count];
        ucptr[2] = (unsigned char)(src[count] >> 8);
    };
}

static inline void s2bet_array(const short *src, tribyte *dest, size_t count)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)dest) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        ucptr[2] = 0;
        ucptr[1] = (unsigned char)src[count];
        ucptr[0] = (unsigned char)(src[count] >> 8);
    };
}

static inline void s2lei_array(const short *src, int *dest, size_t count)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)dest) + 4 * count;
    while (count)
    {
        count--;
        ucptr -= 4;
        ucptr[0] = 0;
        ucptr[1] = 0;
        ucptr[2] = (unsigned char)src[count];
        ucptr[3] = (unsigned char)(src[count] >> 8);
    };
}

static inline void s2bei_array(const short *src, int *dest, size_t count)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)dest) + 4 * count;
    while (count)
    {
        count--;
        ucptr -= 4;
        ucptr[0] = src[count] >> 8;
        ucptr[1] = (unsigned char)src[count];
        ucptr[2] = 0;
        ucptr[3] = 0;
    };
}

static inline void i2sc_array(const int *src, signed char *dest, size_t count)
{
    while (count)
    {
        count--;
        dest[count] = (src[count] >> 24);
    }
}

static inline void i2uc_array(const int *src, unsigned char *dest, size_t count)
{
    while (count)
    {
        count--;
        dest[count] = ((src[count] >> 24) + 128);
    }
}

static inline void i2bes_array(const int *src, short *dest, size_t count)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)dest) + 2 * count;
    while (count)
    {
        count--;
        ucptr -= 2;
        ucptr[0] = src[count] >> 24;
        ucptr[1] = src[count] >> 16;
    };
}

static inline void i2les_array(const int *src, short *dest, size_t count)
{
    unsigned char *ucptr;

    ucptr = ((unsigned char *)dest) + 2 * count;
    while (count)
    {
        count--;
        ucptr -= 2;
        ucptr[0] = src[count] >> 16;
        ucptr[1] = src[count] >> 24;
    };
}

static inline void i2let_array(const int *src, tribyte *dest, size_t count)
{
    unsigned char *ucptr;
    int value;

    ucptr = ((unsigned char *)dest) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        value = src[count] >> 8;
        ucptr[0] = value;
        ucptr[1] = value >> 8;
        ucptr[2] = value >> 16;
    };
}

static inline void i2bet_array(const int *src, tribyte *dest, size_t count)
{
    unsigned char *ucptr;
    int value;

    ucptr = ((unsigned char *)dest) + 3 * count;
    while (count)
    {
        count--;
        ucptr -= 3;
        value = src[count] >> 8;
        ucptr[2] = value;
        ucptr[1] = value >> 8;
        ucptr[0] = value >> 16;
    };
}

static size_t pcm_read_sc2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        sc2s_array(ubuf.scbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_uc2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, sizeof(unsigned char), bufferlen);
        uc2s_array(ubuf.ucbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bes2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    size_t total;

    total = psf->fread(ptr, sizeof(short), len);
    if (CPU_IS_LITTLE_ENDIAN)
        endswap_short_array(ptr, len);

    return total;
}

static size_t pcm_read_les2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    size_t total;

    total = psf->fread(ptr, sizeof(short), len);
    if (CPU_IS_BIG_ENDIAN)
        endswap_short_array(ptr, len);

    return total;
}

static size_t pcm_read_bet2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        bet2s_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_let2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        let2s_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bei2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ibuf, sizeof(int), bufferlen);
        bei2s_array(ubuf.ibuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_lei2s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ibuf, sizeof(int), bufferlen);
        lei2s_array(ubuf.ibuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_sc2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        sc2i_array(ubuf.scbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_uc2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, sizeof(unsigned char), bufferlen);
        uc2i_array(ubuf.ucbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bes2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        bes2i_array(ubuf.sbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_les2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        les2i_array(ubuf.sbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bet2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        bet2i_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_let2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        let2i_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bei2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    size_t total;

    total = psf->fread(ptr, sizeof(int), len);
    if (CPU_IS_LITTLE_ENDIAN)
        endswap_int_array(ptr, len);

    return total;
}

static size_t pcm_read_lei2i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    size_t total;

    total = psf->fread(ptr, sizeof(int), len);
    if (CPU_IS_BIG_ENDIAN)
        endswap_int_array(ptr, len);

    return total;
}

static size_t pcm_read_sc2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x80) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        sc2f_array(ubuf.scbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_uc2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x80) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, sizeof(unsigned char), bufferlen);
        uc2f_array(ubuf.ucbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bes2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x8000) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        bes2f_array(ubuf.sbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_les2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x8000) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        les2f_array(ubuf.sbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bet2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    /* Special normfactor because tribyte value is read into an int. */
    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x80000000) : 1.0 / 256.0);

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        bet2f_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_let2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    /* Special normfactor because tribyte value is read into an int. */
    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x80000000) : 1.0 / 256.0);

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        let2f_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bei2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x80000000) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ibuf, sizeof(int), bufferlen);
        bei2f_array(ubuf.ibuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_lei2f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    normfact = (float)((psf->norm_float == SF_TRUE) ? 1.0 / ((float)0x80000000) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ibuf, sizeof(int), bufferlen);
        lei2f_array(ubuf.ibuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_sc2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x80) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        sc2d_array(ubuf.scbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_uc2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x80) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, sizeof(unsigned char), bufferlen);
        uc2d_array(ubuf.ucbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bes2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x8000) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        bes2d_array(ubuf.sbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_les2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x8000) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        les2d_array(ubuf.sbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bet2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x80000000) : 1.0 / 256.0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        bet2d_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_let2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    /* Special normfactor because tribyte value is read into an int. */
    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x80000000) : 1.0 / 256.0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        let2d_array((tribyte *)(ubuf.ucbuf), readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_bei2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x80000000) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ibuf, sizeof(int), bufferlen);
        bei2d_array(ubuf.ibuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_read_lei2d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    normfact = (psf->norm_double == SF_TRUE) ? 1.0 / ((double)0x80000000) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.ibuf, sizeof(int), bufferlen);
        lei2d_array(ubuf.ibuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t pcm_write_s2sc(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2sc_array(ptr + total, ubuf.scbuf, bufferlen);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_s2uc(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2uc_array(ptr + total, ubuf.ucbuf, bufferlen);
        writecount = psf->fwrite(ubuf.ucbuf, sizeof(unsigned char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_s2bes(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    if (CPU_IS_BIG_ENDIAN)
        return psf->fwrite(ptr, sizeof(short), len);
    else
        bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        endswap_short_copy(ubuf.sbuf, ptr + total, bufferlen);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_s2les(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    if (CPU_IS_LITTLE_ENDIAN)
        return psf->fwrite(ptr, sizeof(short), len);

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        endswap_short_copy(ubuf.sbuf, ptr + total, bufferlen);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_s2bet(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2bet_array(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_s2let(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2let_array(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_s2bei(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2bei_array(ptr + total, ubuf.ibuf, bufferlen);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_s2lei(SF_PRIVATE *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2lei_array(ptr + total, ubuf.ibuf, bufferlen);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2sc(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2sc_array(ptr + total, ubuf.scbuf, bufferlen);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2uc(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2uc_array(ptr + total, ubuf.ucbuf, bufferlen);
        writecount = psf->fwrite(ubuf.ucbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2bes(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2bes_array(ptr + total, ubuf.sbuf, bufferlen);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2les(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2les_array(ptr + total, ubuf.sbuf, bufferlen);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2bet(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2bet_array(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2let(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2let_array(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2bei(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    if (CPU_IS_BIG_ENDIAN)
        return psf->fwrite(ptr, sizeof(int), len);

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        endswap_int_copy(ubuf.ibuf, ptr + total, bufferlen);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t pcm_write_i2lei(SF_PRIVATE *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    size_t bufferlen, writecount;
    size_t total = 0;

    if (CPU_IS_LITTLE_ENDIAN)
        return psf->fwrite(ptr, sizeof(int), len);

    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        endswap_int_copy(ubuf.ibuf, ptr + total, bufferlen);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2sc_array(const float *src, signed char *dest, int count, int normalize)
{
    float normfact;

    normfact = (float)(normalize ? (1.0 * 0x7F) : 1.0);

    while (count)
    {
        count--;
        dest[count] = (char)lrintf(src[count] * normfact);
    };
}

static void f2sc_clip_array(const float *src, signed char *dest, int count, int normalize)
{
    float normfact, scaled_value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : (1.0 * 0x1000000));

    while (count)
    {
        count--;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            dest[count] = 127;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            dest[count] = -128;
            continue;
        };

        dest[count] = lrintf(scaled_value) >> 24;
    };
}

static size_t pcm_write_f2sc(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, signed char *, int, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2sc_clip_array : f2sc_array;
    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.scbuf, bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2uc_array(const float *src, unsigned char *dest, size_t count, int normalize)
{
    float normfact;

    normfact = (float)(normalize ? (1.0 * 0x7F) : 1.0);

    while (count)
    {
        count--;
        dest[count] = (unsigned char)lrintf(src[count] * normfact) + 128;
    };
}

static void f2uc_clip_array(const float *src, unsigned char *dest, size_t count, int normalize)
{
    float normfact, scaled_value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : (1.0 * 0x1000000));

    while (count)
    {
        count--;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            dest[count] = 0xFF;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            dest[count] = 0;
            continue;
        };

        dest[count] = (lrintf(scaled_value) >> 24) + 128;
    };
}

static size_t pcm_write_f2uc(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, unsigned char *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2uc_clip_array : f2uc_array;
    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.ucbuf, bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.ucbuf, sizeof(unsigned char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2bes_array(const float *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact;
    short value;

    normfact = (float)(normalize ? (1.0 * 0x7FFF) : 1.0);
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        value = (short)lrintf(src[count] * normfact);
        ucptr[1] = (unsigned char)value;
        ucptr[0] = value >> 8;
    };
}

static void f2bes_clip_array(const float *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact, scaled_value;
    int value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : (1.0 * 0x10000));
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[1] = 0xFF;
            ucptr[0] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[1] = 0x00;
            ucptr[0] = 0x80;
            continue;
        };

        value = lrintf(scaled_value);
        ucptr[1] = value >> 16;
        ucptr[0] = value >> 24;
    };
}

static size_t pcm_write_f2bes(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, short *t, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2bes_clip_array : f2bes_array;
    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.sbuf, bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2les_array(const float *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact;
    int value;

    normfact = (float)(normalize ? (1.0 * 0x7FFF) : 1.0);
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        value = lrintf(src[count] * normfact);
        ucptr[0] = value;
        ucptr[1] = value >> 8;
    };
}

static void f2les_clip_array(const float *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact, scaled_value;
    int value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : (1.0 * 0x10000));
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[0] = 0xFF;
            ucptr[1] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x00;
            ucptr[1] = 0x80;
            continue;
        };

        value = lrintf(scaled_value);
        ucptr[0] = value >> 16;
        ucptr[1] = value >> 24;
    };
}

static size_t pcm_write_f2les(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, short *t, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2les_clip_array : f2les_array;
    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.sbuf, bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2let_array(const float *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact;
    int value;

    normfact = (float)(normalize ? (1.0 * 0x7FFFFF) : 1.0);
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        value = lrintf(src[count] * normfact);
        ucptr[0] = value;
        ucptr[1] = value >> 8;
        ucptr[2] = value >> 16;
    };
}

static void f2let_clip_array(const float *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact, scaled_value;
    int value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : (1.0 * 0x100));
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[0] = 0xFF;
            ucptr[1] = 0xFF;
            ucptr[2] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x00;
            ucptr[1] = 0x00;
            ucptr[2] = 0x80;
            continue;
        };

        value = lrintf(scaled_value);
        ucptr[0] = value >> 8;
        ucptr[1] = value >> 16;
        ucptr[2] = value >> 24;
    };
}

static size_t pcm_write_f2let(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, tribyte *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2let_clip_array : f2let_array;
    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2bet_array(const float *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact;
    int value;

    normfact = (float)(normalize ? (1.0 * 0x7FFFFF) : 1.0);
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        value = lrintf(src[count] * normfact);
        ucptr[0] = value >> 16;
        ucptr[1] = value >> 8;
        ucptr[2] = value;
    };
}

static void f2bet_clip_array(const float *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact, scaled_value;
    int value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : (1.0 * 0x100));
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[0] = 0x7F;
            ucptr[1] = 0xFF;
            ucptr[2] = 0xFF;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x80;
            ucptr[1] = 0x00;
            ucptr[2] = 0x00;
            continue;
        };

        value = lrint(scaled_value);
        ucptr[0] = value >> 24;
        ucptr[1] = value >> 16;
        ucptr[2] = value >> 8;
    };
}

static size_t pcm_write_f2bet(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, tribyte *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2bet_clip_array : f2bet_array;
    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2bei_array(const float *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact;
    int value;

    normfact = (float)(normalize ? (1.0 * 0x7FFFFFFF) : 1.0);
    ucptr = ((unsigned char *)dest) + 4 * count;
    while (count)
    {
        count--;
        ucptr -= 4;
        value = lrintf(src[count] * normfact);
        ucptr[0] = value >> 24;
        ucptr[1] = value >> 16;
        ucptr[2] = value >> 8;
        ucptr[3] = value;
    };
}

static void f2bei_clip_array(const float *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact, scaled_value;
    int value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : 1.0);
    ucptr = ((unsigned char *)dest) + 4 * count;

    while (count)
    {
        count--;
        ucptr -= 4;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= 1.0 * 0x7FFFFFFF)
        {
            ucptr[0] = 0x7F;
            ucptr[1] = 0xFF;
            ucptr[2] = 0xFF;
            ucptr[3] = 0xFF;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x80;
            ucptr[1] = 0x00;
            ucptr[2] = 0x00;
            ucptr[3] = 0x00;
            continue;
        };

        value = lrintf(scaled_value);
        ucptr[0] = value >> 24;
        ucptr[1] = value >> 16;
        ucptr[2] = value >> 8;
        ucptr[3] = value;
    };
}

static size_t pcm_write_f2bei(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, int *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2bei_clip_array : f2bei_array;
    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.ibuf, bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void f2lei_array(const float *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact;
    int value;

    normfact = (float)(normalize ? (1.0 * 0x7FFFFFFF) : 1.0);
    ucptr = ((unsigned char *)dest) + 4 * count;

    while (count)
    {
        count--;
        ucptr -= 4;
        value = lrintf(src[count] * normfact);
        ucptr[0] = value;
        ucptr[1] = value >> 8;
        ucptr[2] = value >> 16;
        ucptr[3] = value >> 24;
    };
}

static void f2lei_clip_array(const float *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    float normfact, scaled_value;
    int value;

    normfact = (float)(normalize ? (8.0 * 0x10000000) : 1.0);
    ucptr = ((unsigned char *)dest) + 4 * count;

    while (count)
    {
        count--;
        ucptr -= 4;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[0] = 0xFF;
            ucptr[1] = 0xFF;
            ucptr[2] = 0xFF;
            ucptr[3] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x00;
            ucptr[1] = 0x00;
            ucptr[2] = 0x00;
            ucptr[3] = 0x80;
            continue;
        };

        value = lrintf(scaled_value);
        ucptr[0] = value;
        ucptr[1] = value >> 8;
        ucptr[2] = value >> 16;
        ucptr[3] = value >> 24;
    };
}

static size_t pcm_write_f2lei(SF_PRIVATE *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const float *, int *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? f2lei_clip_array : f2lei_array;
    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.ibuf, bufferlen, psf->norm_float);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2sc_array(const double *src, signed char *dest, size_t count, int normalize)
{
    double normfact;

    normfact = normalize ? (1.0 * 0x7F) : 1.0;

    while (count)
    {
        count--;
        dest[count] = (char)lrint(src[count] * normfact);
    };
}

static void d2sc_clip_array(const double *src, signed char *dest, size_t count, int normalize)
{
    double normfact, scaled_value;

    normfact = normalize ? (8.0 * 0x10000000) : (1.0 * 0x1000000);

    while (count)
    {
        count--;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            dest[count] = 127;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            dest[count] = -128;
            continue;
        };

        dest[count] = lrintf(scaled_value) >> 24;
    };
}

static size_t pcm_write_d2sc(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, signed char *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2sc_clip_array : d2sc_array;
    bufferlen = ARRAY_LEN(ubuf.scbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.scbuf, bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2uc_array(const double *src, unsigned char *dest, size_t count, int normalize)
{
    double normfact;

    normfact = normalize ? (1.0 * 0x7F) : 1.0;

    while (count)
    {
        count--;
        dest[count] = (unsigned char)lrint(src[count] * normfact) + 128;
    };
}

static void d2uc_clip_array(const double *src, unsigned char *dest, size_t count, int normalize)
{
    double normfact, scaled_value;

    normfact = normalize ? (8.0 * 0x10000000) : (1.0 * 0x1000000);

    while (count)
    {
        count--;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            dest[count] = 255;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            dest[count] = 0;
            continue;
        };

        dest[count] = (lrint(src[count] * normfact) >> 24) + 128;
    };
}

static size_t pcm_write_d2uc(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, unsigned char *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2uc_clip_array : d2uc_array;
    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.ucbuf, bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.ucbuf, sizeof(unsigned char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2bes_array(const double *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    short value;
    double normfact;

    normfact = normalize ? (1.0 * 0x7FFF) : 1.0;
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        value = (short)lrint(src[count] * normfact);
        ucptr[1] = (unsigned char)value;
        ucptr[0] = value >> 8;
    };
}

static void d2bes_clip_array(const double *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    double normfact, scaled_value;
    int value;

    normfact = normalize ? (8.0 * 0x10000000) : (1.0 * 0x10000);
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[1] = 0xFF;
            ucptr[0] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[1] = 0x00;
            ucptr[0] = 0x80;
            continue;
        };

        value = lrint(scaled_value);
        ucptr[1] = value >> 16;
        ucptr[0] = value >> 24;
    };
}

static size_t pcm_write_d2bes(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, short *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2bes_clip_array : d2bes_array;
    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.sbuf, bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2les_array(const double *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    short value;
    double normfact;

    normfact = normalize ? (1.0 * 0x7FFF) : 1.0;
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        value = (short)lrint(src[count] * normfact);
        ucptr[0] = (unsigned char)value;
        ucptr[1] = value >> 8;
    };
}

static void d2les_clip_array(const double *src, short *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact, scaled_value;

    normfact = normalize ? (8.0 * 0x10000000) : (1.0 * 0x10000);
    ucptr = ((unsigned char *)dest) + 2 * count;

    while (count)
    {
        count--;
        ucptr -= 2;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[0] = 0xFF;
            ucptr[1] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x00;
            ucptr[1] = 0x80;
            continue;
        };

        value = lrint(scaled_value);
        ucptr[0] = value >> 16;
        ucptr[1] = value >> 24;
    };
}

static size_t pcm_write_d2les(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, short *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2les_clip_array : d2les_array;
    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.sbuf, bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2let_array(const double *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact;

    normfact = normalize ? (1.0 * 0x7FFFFF) : 1.0;
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        value = lrint(src[count] * normfact);
        ucptr[0] = value;
        ucptr[1] = value >> 8;
        ucptr[2] = value >> 16;
    };
}

static void d2let_clip_array(const double *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact, scaled_value;

    normfact = normalize ? (8.0 * 0x10000000) : (1.0 * 0x100);
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[0] = 0xFF;
            ucptr[1] = 0xFF;
            ucptr[2] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x00;
            ucptr[1] = 0x00;
            ucptr[2] = 0x80;
            continue;
        };

        value = lrint(scaled_value);
        ucptr[0] = value >> 8;
        ucptr[1] = value >> 16;
        ucptr[2] = value >> 24;
    };
}

static size_t pcm_write_d2let(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, tribyte *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2let_clip_array : d2let_array;
    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2bet_array(const double *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact;

    normfact = normalize ? (1.0 * 0x7FFFFF) : 1.0;
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        value = lrint(src[count] * normfact);
        ucptr[2] = value;
        ucptr[1] = value >> 8;
        ucptr[0] = value >> 16;
    };
}

static void d2bet_clip_array(const double *src, tribyte *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact, scaled_value;

    normfact = normalize ? (8.0 * 0x10000000) : (1.0 * 0x100);
    ucptr = ((unsigned char *)dest) + 3 * count;

    while (count)
    {
        count--;
        ucptr -= 3;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[2] = 0xFF;
            ucptr[1] = 0xFF;
            ucptr[0] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[2] = 0x00;
            ucptr[1] = 0x00;
            ucptr[0] = 0x80;
            continue;
        };

        value = lrint(scaled_value);
        ucptr[2] = value >> 8;
        ucptr[1] = value >> 16;
        ucptr[0] = value >> 24;
    };
}

static size_t pcm_write_d2bet(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, tribyte *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2bet_clip_array : d2bet_array;
    bufferlen = sizeof(ubuf.ucbuf) / SIZEOF_TRIBYTE;

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, (tribyte *)(ubuf.ucbuf), bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.ucbuf, SIZEOF_TRIBYTE, bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2bei_array(const double *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact;

    normfact = normalize ? (1.0 * 0x7FFFFFFF) : 1.0;
    ucptr = ((unsigned char *)dest) + 4 * count;

    while (count)
    {
        count--;
        ucptr -= 4;
        value = lrint(src[count] * normfact);
        ucptr[0] = value >> 24;
        ucptr[1] = value >> 16;
        ucptr[2] = value >> 8;
        ucptr[3] = value;
    };
}

static void d2bei_clip_array(const double *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact, scaled_value;

    normfact = normalize ? (8.0 * 0x10000000) : 1.0;
    ucptr = ((unsigned char *)dest) + 4 * count;

    while (count)
    {
        count--;
        ucptr -= 4;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[3] = 0xFF;
            ucptr[2] = 0xFF;
            ucptr[1] = 0xFF;
            ucptr[0] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[3] = 0x00;
            ucptr[2] = 0x00;
            ucptr[1] = 0x00;
            ucptr[0] = 0x80;
            continue;
        };

        value = lrint(scaled_value);
        ucptr[0] = value >> 24;
        ucptr[1] = value >> 16;
        ucptr[2] = value >> 8;
        ucptr[3] = value;
    };
}

static size_t pcm_write_d2bei(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, int *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2bei_clip_array : d2bei_array;
    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.ibuf, bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void d2lei_array(const double *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact;

    normfact = normalize ? (1.0 * 0x7FFFFFFF) : 1.0;
    ucptr = ((unsigned char *)dest) + 4 * count;

    while (count)
    {
        count--;
        ucptr -= 4;
        value = lrint(src[count] * normfact);
        ucptr[0] = value;
        ucptr[1] = value >> 8;
        ucptr[2] = value >> 16;
        ucptr[3] = value >> 24;
    };
}

static void d2lei_clip_array(const double *src, int *dest, size_t count, int normalize)
{
    unsigned char *ucptr;
    int value;
    double normfact, scaled_value;

    normfact = normalize ? (8.0 * 0x10000000) : 1.0;
    ucptr = ((unsigned char *)dest) + 4 * count;

    while (count)
    {
        count--;
        ucptr -= 4;
        scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            ucptr[0] = 0xFF;
            ucptr[1] = 0xFF;
            ucptr[2] = 0xFF;
            ucptr[3] = 0x7F;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            ucptr[0] = 0x00;
            ucptr[1] = 0x00;
            ucptr[2] = 0x00;
            ucptr[3] = 0x80;
            continue;
        };

        value = lrint(scaled_value);
        ucptr[0] = value;
        ucptr[1] = value >> 8;
        ucptr[2] = value >> 16;
        ucptr[3] = value >> 24;
    };
}

static size_t pcm_write_d2lei(SF_PRIVATE *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    void (*convert)(const double *, int *, size_t, int);
    size_t bufferlen, writecount;
    size_t total = 0;

    convert = (psf->add_clipping) ? d2lei_clip_array : d2lei_array;
    bufferlen = ARRAY_LEN(ubuf.ibuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        convert(ptr + total, ubuf.ibuf, bufferlen, psf->norm_double);
        writecount = psf->fwrite(ubuf.ibuf, sizeof(int), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}
