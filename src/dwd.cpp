/*
** Copyright (C) 2002-2012 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#if (ENABLE_EXPERIMENTAL_CODE == 0)

int dwd_open(SF_PRIVATE *psf)
{
    if (psf)
        return SFE_UNIMPLEMENTED;
    return 0;
}

#else

/*
 * Macros to handle big/little endian issues.
 */

#define SFE_DWD_NO_DWD (1666)
#define SFE_DWD_BAND_BIT_WIDTH (1667)
#define SFE_DWD_COMPRESSION (1668)

#define DWD_IDENTIFIER "DiamondWare Digitized\n\0\x1a"
#define DWD_IDENTIFIER_LEN (24)

#define DWD_HEADER_LEN (57)

/*
 * Typedefs.
 */

/*
 * Private static functions.
 */

static int dwd_read_header(SF_PRIVATE *psf);

static int dwd_close(SF_PRIVATE *psf);

/*
 * Public function.
 */

int dwd_open(SF_PRIVATE *psf)
{
    int error = 0;

    if (psf->file_mode == SFM_READ || (psf->file_mode == SFM_RDWR && psf->filelength > 0))
    {
        if ((error = dwd_read_header(psf)))
            return error;
    };

    if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_DWD)
        return SFE_BAD_OPEN_FORMAT;

    if (psf->file_mode == SFM_WRITE || psf->file_mode == SFM_RDWR)
    {
        /*-psf->endian = SF_ENDIAN (psf->sf.format) ;
		if (CPU_IS_LITTLE_ENDIAN && psf->endian == SF_ENDIAN_CPU)
			psf->endian = SF_ENDIAN_LITTLE ;
		else if (psf->endian != SF_ENDIAN_LITTLE)
			psf->endian = SF_ENDIAN_BIG ;

		if (! (encoding = dwd_write_header (psf, SF_FALSE)))
			return psf->error ;

		psf->write_header = dwd_write_header ;
		-*/
    };

    psf->container_close = dwd_close;

    /*-psf->blockwidth = psf->bytewidth * psf->sf.channels ;-*/

    return error;
}

static int dwd_close(SF_PRIVATE *UNUSED(psf))
{
    return 0;
}

/* This struct contains all the fields of interest om the DWD header, but does not
** do so in the same order and layout as the actual file, header.
** No assumptions are made about the packing of this struct.
*/
typedef struct
{
    unsigned char major, minor, compression, channels, bitwidth;
    unsigned short srate, maxval;
    unsigned int id, datalen, frames, offset;
} DWD_HEADER;

static int dwd_read_header(SF_PRIVATE *psf)
{
    BUF_UNION ubuf;
    DWD_HEADER dwdh;

    memset(ubuf.cbuf, 0, sizeof(ubuf.cbuf));
    /* Set position to start of file to begin reading header. */
    psf->binheader_readf("pb", 0, ubuf.cbuf, DWD_IDENTIFIER_LEN);

    if (memcmp(ubuf.cbuf, DWD_IDENTIFIER, DWD_IDENTIFIER_LEN) != 0)
        return SFE_DWD_NO_DWD;

    psf->log_printf("Read only : DiamondWare Digitized (.dwd)\n", ubuf.cbuf);

    psf->binheader_readf("11", &dwdh.major, &dwdh.minor);
    psf->binheader_readf("e4j1", &dwdh.id, 1, &dwdh.compression);
    psf->binheader_readf("e211", &dwdh.srate, &dwdh.channels, &dwdh.bitwidth);
    psf->binheader_readf("e24", &dwdh.maxval, &dwdh.datalen);
    psf->binheader_readf("e44", &dwdh.frames, &dwdh.offset);

    psf->log_printf("  Version Major : %d\n  Version Minor : %d\n  Unique ID     : %08X\n",
                   dwdh.major, dwdh.minor, dwdh.id);
    psf->log_printf("  Compression   : %d => ", dwdh.compression);

    if (dwdh.compression != 0)
    {
        psf->log_printf("Unsupported compression\n");
        return SFE_DWD_COMPRESSION;
    }
    else
    {
        psf->log_printf("None\n");
    }

    psf->log_printf(
                   "  Sample Rate   : %d\n  Channels      : %d\n"
                   "  Bit Width     : %d\n",
                   dwdh.srate, dwdh.channels, dwdh.bitwidth);

    switch (dwdh.bitwidth)
    {
    case 8:
        psf->sf.format = SF_FORMAT_DWD | SF_FORMAT_PCM_S8;
        psf->bytewidth = 1;
        break;

    case 16:
        psf->sf.format = SF_FORMAT_DWD | SF_FORMAT_PCM_16;
        psf->bytewidth = 2;
        break;

    default:
        psf->log_printf("*** Bad bit width %d\n", dwdh.bitwidth);
        return SFE_DWD_BAND_BIT_WIDTH;
    };

    if (psf->filelength != dwdh.offset + dwdh.datalen)
    {
        psf->log_printf("  Data Length   : %d (should be %D)\n", dwdh.datalen,
                       psf->filelength - dwdh.offset);
        dwdh.datalen = (unsigned int)(psf->filelength - dwdh.offset);
    }
    else
        psf->log_printf("  Data Length   : %d\n", dwdh.datalen);

    psf->log_printf("  Max Value     : %d\n", dwdh.maxval);
    psf->log_printf("  Frames        : %d\n", dwdh.frames);
    psf->log_printf("  Data Offset   : %d\n", dwdh.offset);

    psf->datalength = dwdh.datalen;
    psf->dataoffset = dwdh.offset;

    psf->endian = SF_ENDIAN_LITTLE;

    psf->sf.samplerate = dwdh.srate;
    psf->sf.channels = dwdh.channels;
    psf->sf.sections = 1;

    return pcm_init(psf);
}

#endif
