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
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

/*
 * Macros to handle big/little endian issues.
 */

#define DOTSND_MARKER (MAKE_MARKER('.', 's', 'n', 'd'))
#define DNSDOT_MARKER (MAKE_MARKER('d', 'n', 's', '.'))

#define AU_DATA_OFFSET (24)

/*
 * Known AU file encoding types.
 */

enum
{
    /* 8-bit u-law samples */
    AU_ENCODING_ULAW_8 = 1,
    /* 8-bit linear samples */
    AU_ENCODING_PCM_8 = 2,
    /* 16-bit linear samples */
    AU_ENCODING_PCM_16 = 3,
    /* 24-bit linear samples */
    AU_ENCODING_PCM_24 = 4,
    /* 32-bit linear samples */
    AU_ENCODING_PCM_32 = 5,

    /* floating-point samples */
    AU_ENCODING_FLOAT = 6,
    /* double-precision float samples */
    AU_ENCODING_DOUBLE = 7,
    /* fragmented sampled data */
    AU_ENCODING_INDIRECT = 8,
    /* ? */
    AU_ENCODING_NESTED = 9,
    /* DSP program */
    AU_ENCODING_DSP_CORE = 10,
    /* 8-bit fixed-point samples */
    AU_ENCODING_DSP_DATA_8 = 11,
    /* 16-bit fixed-point samples */
    AU_ENCODING_DSP_DATA_16 = 12,
    /* 24-bit fixed-point samples */
    AU_ENCODING_DSP_DATA_24 = 13,
    /* 32-bit fixed-point samples */
    AU_ENCODING_DSP_DATA_32 = 14,

    /* non-audio display data */
    AU_ENCODING_DISPLAY = 16,
    /* ? */
    AU_ENCODING_MULAW_SQUELCH = 17,
    /* 16-bit linear with emphasis */
    AU_ENCODING_EMPHASIZED = 18,
    /* 16-bit linear with compression (NEXT) */
    AU_ENCODING_NEXT = 19,
    /* A combination of the two above */
    AU_ENCODING_COMPRESSED_EMPHASIZED = 20,
    /* Music Kit DSP commands */
    AU_ENCODING_DSP_COMMANDS = 21,
    /* ? */
    AU_ENCODING_DSP_COMMANDS_SAMPLES = 22,

    /* G721 32 kbs ADPCM - 4 bits per sample. */
    AU_ENCODING_ADPCM_G721_32 = 23,
    /* G722 64 kbs ADPCM */
    AU_ENCODING_ADPCM_G722 = 24,
    /* G723 24 kbs ADPCM - 3 bits per sample. */
    AU_ENCODING_ADPCM_G723_24 = 25,
    /* G723 40 kbs ADPCM - 5 bits per sample. */
    AU_ENCODING_ADPCM_G723_40 = 26,

    AU_ENCODING_ALAW_8 = 27
};

/*
 * Typedefs.
 */

typedef struct
{
    int dataoffset;
    int datasize;
    int encoding;
    int samplerate;
    int channels;
} AU_FMT;

/*
 * Private static functions.
 */

static int au_close(SndFile *psf);

static int au_format_to_encoding(int format);

static int au_write_header(SndFile *psf, int calc_length);
static int au_read_header(SndFile *psf);

/*
 * Public function.
 */

int au_open(SndFile *psf)
{
    int error = 0;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = au_read_header(psf)))
            return error;
    };

    if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_AU)
        return SFE_BAD_OPEN_FORMAT;

    int subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        psf->m_endian = SF_ENDIAN(psf->sf.format);
        if (CPU_IS_LITTLE_ENDIAN && psf->m_endian == SF_ENDIAN_CPU)
        {
            psf->m_endian = SF_ENDIAN_LITTLE;
        }
        else if (psf->m_endian != SF_ENDIAN_LITTLE)
        {
            psf->m_endian = SF_ENDIAN_BIG;
        }

        if (au_write_header(psf, SF_FALSE))
        {
            return psf->m_error;
        }

        psf->write_header = au_write_header;
    };

    psf->container_close = au_close;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    switch (subformat)
    {
    case SF_FORMAT_ULAW: /* 8-bit Ulaw encoding. */
        ulaw_init(psf);
        break;

    case SF_FORMAT_PCM_S8: /* 8-bit linear PCM. */
        error = pcm_init(psf);
        break;

    case SF_FORMAT_PCM_16: /* 16-bit linear PCM. */
    case SF_FORMAT_PCM_24: /* 24-bit linear PCM */
    case SF_FORMAT_PCM_32: /* 32-bit linear PCM. */
        error = pcm_init(psf);
        break;

    case SF_FORMAT_ALAW: /* 8-bit Alaw encoding. */
        alaw_init(psf);
        break;

    case SF_FORMAT_FLOAT: /* 32-bit floats. */
        error = float32_init(psf);
        break;

    case SF_FORMAT_DOUBLE: /* 64-bit double precision floats. */
        error = double64_init(psf);
        break;

    case SF_FORMAT_G721_32:
        error = g72x_init(psf);
        psf->sf.seekable = SF_FALSE;
        break;

    case SF_FORMAT_G723_24:
        error = g72x_init(psf);
        psf->sf.seekable = SF_FALSE;
        break;

    case SF_FORMAT_G723_40:
        error = g72x_init(psf);
        psf->sf.seekable = SF_FALSE;
        break;

    default:
        break;
    };

    return error;
}

static int au_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        au_write_header(psf, SF_TRUE);

    return 0;
}

static int au_write_header(SndFile *psf, int calc_length)
{
    sf_count_t current = psf->ftell();

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
        if (psf->m_dataend)
        {
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;
        }
    };

    int encoding = au_format_to_encoding(SF_CODEC(psf->sf.format));
    if (!encoding)
        return (psf->m_error = SFE_BAD_OPEN_FORMAT);

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;

    psf->fseek(0, SEEK_SET);

    /*
     * AU format files allow a datalength value of -1 if the datalength
     * is not know at the time the header is written.
     * Also use this value of -1 if the datalength > 2 gigabytes.
     */
    
    int datalength;

    if (psf->m_datalength < 0 || psf->m_datalength > 0x7FFFFFFF)
        datalength = -1;
    else
        datalength = (int)(psf->m_datalength & 0x7FFFFFFF);

    if (psf->m_endian == SF_ENDIAN_BIG)
    {
        psf->binheader_writef("Em4", BHWm(DOTSND_MARKER), BHW4(AU_DATA_OFFSET));
        psf->binheader_writef("E4444", BHW4(datalength), BHW4(encoding),
                             BHW4(psf->sf.samplerate), BHW4(psf->sf.channels));
    }
    else if (psf->m_endian == SF_ENDIAN_LITTLE)
    {
        psf->binheader_writef("em4", BHWm(DNSDOT_MARKER), BHW4(AU_DATA_OFFSET));
        psf->binheader_writef("e4444", BHW4(datalength), BHW4(encoding),
                             BHW4(psf->sf.samplerate), BHW4(psf->sf.channels));
    }
    else
    {
        return (psf->m_error = SFE_BAD_OPEN_FORMAT);
    }

    /* Header construction complete so write it out. */
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int au_format_to_encoding(int format)
{
    switch (format)
    {
    case SF_FORMAT_PCM_S8:
        return AU_ENCODING_PCM_8;
    case SF_FORMAT_PCM_16:
        return AU_ENCODING_PCM_16;
    case SF_FORMAT_PCM_24:
        return AU_ENCODING_PCM_24;
    case SF_FORMAT_PCM_32:
        return AU_ENCODING_PCM_32;

    case SF_FORMAT_FLOAT:
        return AU_ENCODING_FLOAT;
    case SF_FORMAT_DOUBLE:
        return AU_ENCODING_DOUBLE;

    case SF_FORMAT_ULAW:
        return AU_ENCODING_ULAW_8;
    case SF_FORMAT_ALAW:
        return AU_ENCODING_ALAW_8;

    case SF_FORMAT_G721_32:
        return AU_ENCODING_ADPCM_G721_32;
    case SF_FORMAT_G723_24:
        return AU_ENCODING_ADPCM_G723_24;
    case SF_FORMAT_G723_40:
        return AU_ENCODING_ADPCM_G723_40;

    default:
        break;
    };
    return 0;
}

static int au_read_header(SndFile *psf)
{
    AU_FMT au_fmt;
    int marker;

    memset(&au_fmt, 0, sizeof(au_fmt));
	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("m", &marker);
    psf->log_printf("%M\n", marker);

    if (marker == DOTSND_MARKER)
    {
        psf->m_endian = SF_ENDIAN_BIG;

        psf->binheader_readf("E44444", &(au_fmt.dataoffset), &(au_fmt.datasize),
                            &(au_fmt.encoding), &(au_fmt.samplerate), &(au_fmt.channels));
    }
    else if (marker == DNSDOT_MARKER)
    {
        psf->m_endian = SF_ENDIAN_LITTLE;
        psf->binheader_readf("e44444", &(au_fmt.dataoffset), &(au_fmt.datasize),
                            &(au_fmt.encoding), &(au_fmt.samplerate), &(au_fmt.channels));
    }
    else
    {
        return SFE_AU_NO_DOTSND;
    }

    psf->log_printf("  Data Offset : %d\n", au_fmt.dataoffset);

    if (au_fmt.datasize == -1 || au_fmt.dataoffset + au_fmt.datasize == psf->m_filelength)
    {
        psf->log_printf("  Data Size   : %d\n", au_fmt.datasize);
    }
    else if (au_fmt.dataoffset + au_fmt.datasize < psf->m_filelength)
    {
        psf->m_filelength = au_fmt.dataoffset + au_fmt.datasize;
        psf->log_printf("  Data Size   : %d\n", au_fmt.datasize);
    }
    else
    {
        int dword = psf->m_filelength - au_fmt.dataoffset;
        psf->log_printf("  Data Size   : %d (should be %d)\n", au_fmt.datasize, dword);
        au_fmt.datasize = dword;
    };

    psf->m_dataoffset = au_fmt.dataoffset;
    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

	if (psf->ftell() < psf->m_dataoffset)
		psf->binheader_seekf(psf->m_dataoffset - psf->ftell(), SF_SEEK_CUR);

    psf->sf.samplerate = au_fmt.samplerate;
    psf->sf.channels = au_fmt.channels;

    /* Only fill in type major. */
    if (psf->m_endian == SF_ENDIAN_BIG)
        psf->sf.format = SF_FORMAT_AU;
    else if (psf->m_endian == SF_ENDIAN_LITTLE)
        psf->sf.format = SF_ENDIAN_LITTLE | SF_FORMAT_AU;

    psf->log_printf("  Encoding    : %d => ", au_fmt.encoding);

    psf->sf.format = SF_ENDIAN(psf->sf.format);

    switch (au_fmt.encoding)
    {
    case AU_ENCODING_ULAW_8:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_ULAW;
        psf->m_bytewidth = 1; /* Before decoding */
        psf->log_printf("8-bit ISDN u-law\n");
        break;

    case AU_ENCODING_PCM_8:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_S8;
        psf->m_bytewidth = 1;
        psf->log_printf("8-bit linear PCM\n");
        break;

    case AU_ENCODING_PCM_16:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_16;
        psf->m_bytewidth = 2;
        psf->log_printf("16-bit linear PCM\n");
        break;

    case AU_ENCODING_PCM_24:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_24;
        psf->m_bytewidth = 3;
        psf->log_printf("24-bit linear PCM\n");
        break;

    case AU_ENCODING_PCM_32:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_PCM_32;
        psf->m_bytewidth = 4;
        psf->log_printf("32-bit linear PCM\n");
        break;

    case AU_ENCODING_FLOAT:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_FLOAT;
        psf->m_bytewidth = 4;
        psf->log_printf("32-bit float\n");
        break;

    case AU_ENCODING_DOUBLE:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_DOUBLE;
        psf->m_bytewidth = 8;
        psf->log_printf("64-bit double precision float\n");
        break;

    case AU_ENCODING_ALAW_8:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_ALAW;
        psf->m_bytewidth = 1; /* Before decoding */
        psf->log_printf("8-bit ISDN A-law\n");
        break;

    case AU_ENCODING_ADPCM_G721_32:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_G721_32;
        psf->m_bytewidth = 0;
        psf->log_printf("G721 32kbs ADPCM\n");
        break;

    case AU_ENCODING_ADPCM_G723_24:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_G723_24;
        psf->m_bytewidth = 0;
        psf->log_printf("G723 24kbs ADPCM\n");
        break;

    case AU_ENCODING_ADPCM_G723_40:
        psf->sf.format |= SF_FORMAT_AU | SF_FORMAT_G723_40;
        psf->m_bytewidth = 0;
        psf->log_printf("G723 40kbs ADPCM\n");
        break;

    case AU_ENCODING_ADPCM_G722:
        psf->log_printf("G722 64 kbs ADPCM (unsupported)\n");
        break;

    case AU_ENCODING_NEXT:
        psf->log_printf("Weird NeXT encoding format (unsupported)\n");
        break;

    default:
        psf->log_printf("Unknown!!\n");
        break;
    };

    psf->log_printf("  Sample Rate : %d\n", au_fmt.samplerate);
    if (au_fmt.channels < 1)
    {
        psf->log_printf("  Channels    : %d  **** should be >= 1\n", au_fmt.channels);
        return SFE_CHANNEL_COUNT_ZERO;
    }
    else if (au_fmt.channels > SF_MAX_CHANNELS)
    {
        psf->log_printf("  Channels    : %d  **** should be <= %d\n", au_fmt.channels, SF_MAX_CHANNELS);
        return SFE_CHANNEL_COUNT;
    };

    psf->log_printf("  Channels    : %d\n", au_fmt.channels);

    psf->m_blockwidth = psf->sf.channels * psf->m_bytewidth;

    if (!psf->sf.frames && psf->m_blockwidth)
        psf->sf.frames = (psf->m_filelength - psf->m_dataoffset) / psf->m_blockwidth;

    return 0;
}
