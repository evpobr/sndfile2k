/*
** Copyright (C) 2002-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

/*
 * Information on how to decode and encode this file was obtained in a PDF
 * file which I found on http://www.wotsit.org/.
 * Also did a lot of testing with GNU Octave but do not have access to
 * Matlab (tm) and so could not test it there.
 */

#define MATL_MARKER (MAKE_MARKER('M', 'A', 'T', 'L'))

#define IM_MARKER (('I' << 8) + 'M')
#define MI_MARKER (('M' << 8) + 'I')

enum
{
    MAT5_TYPE_SCHAR = 0x1,
    MAT5_TYPE_UCHAR = 0x2,
    MAT5_TYPE_INT16 = 0x3,
    MAT5_TYPE_UINT16 = 0x4,
    MAT5_TYPE_INT32 = 0x5,
    MAT5_TYPE_UINT32 = 0x6,
    MAT5_TYPE_FLOAT = 0x7,
    MAT5_TYPE_DOUBLE = 0x9,
    MAT5_TYPE_ARRAY = 0xE,

    MAT5_TYPE_COMP_USHORT = 0x00020004,
    MAT5_TYPE_COMP_UINT = 0x00040006
};

typedef struct
{
    sf_count_t size;
    int rows, cols;
    char name[32];
} MAT5_MATRIX;

static int mat5_close(SndFile *psf);

static int mat5_write_header(SndFile *psf, int calc_length);
static int mat5_read_header(SndFile *psf);

int mat5_open(SndFile *psf)
{
    int subformat, error = 0;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = mat5_read_header(psf)))
            return error;
    };

    if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_MAT5)
        return SFE_BAD_OPEN_FORMAT;

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        psf->m_endian = SF_ENDIAN(psf->sf.format);
        if (CPU_IS_LITTLE_ENDIAN && (psf->m_endian == SF_ENDIAN_CPU || psf->m_endian == 0))
            psf->m_endian = SF_ENDIAN_LITTLE;
        else if (CPU_IS_BIG_ENDIAN && (psf->m_endian == SF_ENDIAN_CPU || psf->m_endian == 0))
            psf->m_endian = SF_ENDIAN_BIG;

        if ((error = mat5_write_header(psf, SF_FALSE)))
            return error;

        psf->write_header = mat5_write_header;
    };

    psf->container_close = mat5_close;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    switch (subformat)
    {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_32:
        error = pcm_init(psf);
        break;

    case SF_FORMAT_FLOAT:
        error = float32_init(psf);
        break;

    case SF_FORMAT_DOUBLE:
        error = double64_init(psf);
        break;

    default:
        break;
    };

    return error;
}

static int mat5_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        mat5_write_header(psf, SF_TRUE);

    return 0;
} /* mat5_close */

/*------------------------------------------------------------------------------
*/

static int mat5_write_header(SndFile *psf, int calc_length)
{
    static const char *filename =
        "MATLAB 5.0 MAT-file, written by " PACKAGE_NAME "-" PACKAGE_VERSION ", ";
    static const char *sr_name = "samplerate\0\0\0\0\0\0\0\0\0\0\0";
    static const char *wd_name = "wavedata\0";
    char buffer[256];
    sf_count_t current, datasize;
    int encoding;

    current = psf->ftell();

    if (calc_length)
    {
        psf->fseek(0, SEEK_END);
        psf->m_filelength = psf->ftell();
        psf->fseek(0, SEEK_SET);

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
        if (psf->m_dataend)
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;

        psf->sf.frames = psf->m_datalength / (psf->m_bytewidth * psf->sf.channels);
    };

    switch (SF_CODEC(psf->sf.format))
    {
    case SF_FORMAT_PCM_U8:
        encoding = MAT5_TYPE_UCHAR;
        break;

    case SF_FORMAT_PCM_16:
        encoding = MAT5_TYPE_INT16;
        break;

    case SF_FORMAT_PCM_32:
        encoding = MAT5_TYPE_INT32;
        break;

    case SF_FORMAT_FLOAT:
        encoding = MAT5_TYPE_FLOAT;
        break;

    case SF_FORMAT_DOUBLE:
        encoding = MAT5_TYPE_DOUBLE;
        break;

    default:
        return SFE_BAD_OPEN_FORMAT;
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    psf_get_date_str(buffer, sizeof(buffer));
    psf->binheader_writef("bb", BHWv(filename), BHWz(strlen(filename)), BHWv(buffer),
                         BHWz(strlen(buffer) + 1));

    memset(buffer, ' ', 124 - psf->m_header.indx);
    psf->binheader_writef("b", BHWv(buffer), BHWz(124 - psf->m_header.indx));

    psf->m_rwf_endian = psf->m_endian;

    if (psf->m_rwf_endian == SF_ENDIAN_BIG)
        psf->binheader_writef("2b", BHW2(0x0100), BHWv("MI"), BHWz(2));
    else
        psf->binheader_writef("2b", BHW2(0x0100), BHWv("IM"), BHWz(2));

    psf->binheader_writef("444444", BHW4(MAT5_TYPE_ARRAY), BHW4(64), BHW4(MAT5_TYPE_UINT32),
                         BHW4(8), BHW4(6), BHW4(0));
    psf->binheader_writef("4444", BHW4(MAT5_TYPE_INT32), BHW4(8), BHW4(1), BHW4(1));
    psf->binheader_writef("44b", BHW4(MAT5_TYPE_SCHAR), BHW4(strlen(sr_name)), BHWv(sr_name),
                         BHWz(16));

    if (psf->sf.samplerate > 0xFFFF)
    {
        psf->binheader_writef("44", BHW4(MAT5_TYPE_COMP_UINT), BHW4(psf->sf.samplerate));
    }
    else
    {
        unsigned short samplerate = psf->sf.samplerate;

        psf->binheader_writef("422", BHW4(MAT5_TYPE_COMP_USHORT), BHW2(samplerate), BHW2(0));
    };

    datasize = psf->sf.frames * psf->sf.channels * psf->m_bytewidth;

    psf->binheader_writef("t484444", BHW4(MAT5_TYPE_ARRAY), BHW8(datasize + 64),
                         BHW4(MAT5_TYPE_UINT32), BHW4(8), BHW4(6), BHW4(0));
    psf->binheader_writef("t4448", BHW4(MAT5_TYPE_INT32), BHW4(8), BHW4(psf->sf.channels),
                         BHW8(psf->sf.frames));
    psf->binheader_writef("44b", BHW4(MAT5_TYPE_SCHAR), BHW4(strlen(wd_name)), BHWv(wd_name),
                         BHWz(strlen(wd_name)));

    datasize = psf->sf.frames * psf->sf.channels * psf->m_bytewidth;
    if (datasize > 0x7FFFFFFF)
        datasize = 0x7FFFFFFF;

    psf->binheader_writef("t48", BHW4(encoding), BHW8(datasize));

    /* Header construction complete so write it out. */
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
} /* mat5_write_header */

static int mat5_read_header(SndFile *psf)
{
    char buffer[256], name[32];
    short version, endian;
    int type, flags1, flags2, rows, cols;
    unsigned size;
    int have_samplerate = 1;

	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("b", buffer, 124);

    buffer[125] = 0;

    if (strlen(buffer) >= 124)
        return SFE_UNIMPLEMENTED;

    if (strstr(buffer, "MATLAB 5.0 MAT-file") == buffer)
        psf->log_printf("%s\n", buffer);

    psf->binheader_readf("E22", &version, &endian);

    if (endian == MI_MARKER)
    {
        psf->m_endian = psf->m_rwf_endian = SF_ENDIAN_BIG;
        if (CPU_IS_LITTLE_ENDIAN)
            version = ENDSWAP_16(version);
    }
    else if (endian == IM_MARKER)
    {
        psf->m_endian = psf->m_rwf_endian = SF_ENDIAN_LITTLE;
        if (CPU_IS_BIG_ENDIAN)
            version = ENDSWAP_16(version);
    }
    else
        return SFE_MAT5_BAD_ENDIAN;

    if ((CPU_IS_LITTLE_ENDIAN && endian == IM_MARKER) || (CPU_IS_BIG_ENDIAN && endian == MI_MARKER))
        version = ENDSWAP_16(version);

    psf->log_printf("Version : 0x%04X\n", version);
    psf->log_printf("Endian  : 0x%04X => %s\n", endian,
                   (psf->m_endian == SF_ENDIAN_LITTLE) ? "Little" : "Big");

    /*========================================================*/
    psf->binheader_readf("44", &type, &size);
    psf->log_printf("Block\n Type : %X    Size : %d\n", type, size);

    if (type != MAT5_TYPE_ARRAY)
        return SFE_MAT5_NO_BLOCK;

    psf->binheader_readf("44", &type, &size);
    psf->log_printf("    Type : %X    Size : %d\n", type, size);

    if (type != MAT5_TYPE_UINT32)
        return SFE_MAT5_NO_BLOCK;

    psf->binheader_readf("44", &flags1, &flags2);
    psf->log_printf("    Flg1 : %X    Flg2 : %d\n", flags1, flags2);

    psf->binheader_readf("44", &type, &size);
    psf->log_printf("    Type : %X    Size : %d\n", type, size);

    if (type != MAT5_TYPE_INT32)
        return SFE_MAT5_NO_BLOCK;

    psf->binheader_readf("44", &rows, &cols);
    psf->log_printf("    Rows : %d    Cols : %d\n", rows, cols);

    if (rows != 1 || cols != 1)
    {
        if (psf->sf.samplerate == 0)
            psf->sf.samplerate = 44100;
        have_samplerate = 0;
    }
    psf->binheader_readf("4", &type);

    if (type == MAT5_TYPE_SCHAR)
    {
        psf->binheader_readf("4", &size);
        psf->log_printf("    Type : %X    Size : %d\n", type, size);
        if (size > SIGNED_SIZEOF(name) - 1)
        {
            psf->log_printf("Error : Bad name length.\n");
            return SFE_MAT5_NO_BLOCK;
        };

        psf->binheader_readf("b", name, size);
		name[size] = 0;
		psf->binheader_seekf((8 - (size % 8)) % 8, SF_SEEK_CUR);
    }
    else if ((type & 0xFFFF) == MAT5_TYPE_SCHAR)
    {
        size = type >> 16;
        if (size > 4)
        {
            psf->log_printf("Error : Bad name length.\n");
            return SFE_MAT5_NO_BLOCK;
        };

        psf->log_printf("    Type : %X\n", type);
        psf->binheader_readf("4", &name);
        name[size] = 0;
    }
    else
        return SFE_MAT5_NO_BLOCK;

    psf->log_printf("    Name : %s\n", name);

    /*-----------------------------------------*/

    psf->binheader_readf("44", &type, &size);

    if (!have_samplerate)
        goto skip_samplerate;

    switch (type)
    {
    case MAT5_TYPE_DOUBLE:
    {
        double samplerate;

        psf->binheader_readf("d", &samplerate);
        snprintf(name, sizeof(name), "%f\n", samplerate);
        psf->log_printf("    Val  : %s\n", name);

        psf->sf.samplerate = lrint(samplerate);
    };
    break;

    case MAT5_TYPE_COMP_USHORT:
    {
        unsigned short samplerate;

		psf->binheader_seekf(-4, SF_SEEK_CUR);
        psf->binheader_readf("2", &samplerate);
		psf->binheader_seekf(2, SF_SEEK_CUR);
        psf->log_printf("    Val  : %u\n", samplerate);
        psf->sf.samplerate = samplerate;
    }
    break;

    case MAT5_TYPE_COMP_UINT:
        psf->log_printf("    Val  : %u\n", size);
        psf->sf.samplerate = size;
        break;

    default:
        psf->log_printf("    Type : %X    Size : %d  ***\n", type, size);
        return SFE_MAT5_SAMPLE_RATE;
    };

    /*-----------------------------------------*/

    psf->binheader_readf("44", &type, &size);
    psf->log_printf(" Type : %X    Size : %d\n", type, size);

    if (type != MAT5_TYPE_ARRAY)
        return SFE_MAT5_NO_BLOCK;

    psf->binheader_readf("44", &type, &size);
    psf->log_printf("    Type : %X    Size : %d\n", type, size);

    if (type != MAT5_TYPE_UINT32)
        return SFE_MAT5_NO_BLOCK;

    psf->binheader_readf("44", &flags1, &flags2);
    psf->log_printf("    Flg1 : %X    Flg2 : %d\n", flags1, flags2);

    psf->binheader_readf("44", &type, &size);
    psf->log_printf("    Type : %X    Size : %d\n", type, size);

    if (type != MAT5_TYPE_INT32)
        return SFE_MAT5_NO_BLOCK;

    psf->binheader_readf("44", &rows, &cols);
    psf->log_printf("    Rows : %X    Cols : %d\n", rows, cols);

    psf->binheader_readf("4", &type);

    if (type == MAT5_TYPE_SCHAR)
    {
        psf->binheader_readf("4", &size);
        psf->log_printf("    Type : %X    Size : %d\n", type, size);
        if (size > SIGNED_SIZEOF(name) - 1)
        {
            psf->log_printf("Error : Bad name length.\n");
            return SFE_MAT5_NO_BLOCK;
        };

        psf->binheader_readf("b", name, size);
		psf->binheader_seekf((8 - (size % 8)) % 8, SF_SEEK_CUR);
        name[size] = 0;
    }
    else if ((type & 0xFFFF) == MAT5_TYPE_SCHAR)
    {
        size = type >> 16;
        if (size > 4)
        {
            psf->log_printf("Error : Bad name length.\n");
            return SFE_MAT5_NO_BLOCK;
        };

        psf->log_printf("    Type : %X\n", type);
        psf->binheader_readf("4", &name);
        name[size] = 0;
    }
    else
        return SFE_MAT5_NO_BLOCK;

    psf->log_printf("    Name : %s\n", name);

    psf->binheader_readf("44", &type, &size);
    psf->log_printf("    Type : %X    Size : %d\n", type, size);

skip_samplerate:
    /*++++++++++++++++++++++++++++++++++++++++++++++++++*/

    if (rows == 0 && cols == 0)
    {
        psf->log_printf("*** Error : zero channel count.\n");
        return SFE_CHANNEL_COUNT_ZERO;
    };

    psf->sf.channels = rows;
    psf->sf.frames = cols;

    psf->sf.format = psf->m_endian | SF_FORMAT_MAT5;

    switch (type)
    {
    case MAT5_TYPE_DOUBLE:
        psf->log_printf("Data type : double\n");
        psf->sf.format |= SF_FORMAT_DOUBLE;
        psf->m_bytewidth = 8;
        break;

    case MAT5_TYPE_FLOAT:
        psf->log_printf("Data type : float\n");
        psf->sf.format |= SF_FORMAT_FLOAT;
        psf->m_bytewidth = 4;
        break;

    case MAT5_TYPE_INT32:
        psf->log_printf("Data type : 32 bit PCM\n");
        psf->sf.format |= SF_FORMAT_PCM_32;
        psf->m_bytewidth = 4;
        break;

    case MAT5_TYPE_INT16:
        psf->log_printf("Data type : 16 bit PCM\n");
        psf->sf.format |= SF_FORMAT_PCM_16;
        psf->m_bytewidth = 2;
        break;

    case MAT5_TYPE_UCHAR:
        psf->log_printf("Data type : unsigned 8 bit PCM\n");
        psf->sf.format |= SF_FORMAT_PCM_U8;
        psf->m_bytewidth = 1;
        break;

    default:
        psf->log_printf("*** Error : Bad marker %08X\n", type);
        return SFE_UNIMPLEMENTED;
    };

    psf->m_dataoffset = psf->ftell();
    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

    return 0;
} /* mat5_read_header */
