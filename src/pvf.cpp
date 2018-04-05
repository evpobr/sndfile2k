/*
** Copyright (C) 2002-2016 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#define PVF1_MARKER (MAKE_MARKER('P', 'V', 'F', '1'))

static int pvf_close(SF_PRIVATE *psf);

static int pvf_write_header(SF_PRIVATE *psf, int calc_length);
static int pvf_read_header(SF_PRIVATE *psf);

int pvf_open(SF_PRIVATE *psf)
{
    int subformat;
    int error = 0;

    if (psf->file.mode == SFM_READ || (psf->file.mode == SFM_RDWR && psf->filelength > 0))
    {
        if ((error = pvf_read_header(psf)))
            return error;
    };

    subformat = SF_CODEC(psf->sf.format);

    if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
    {
        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_PVF)
            return SFE_BAD_OPEN_FORMAT;

        psf->endian = SF_ENDIAN_BIG;

        if (pvf_write_header(psf, SF_FALSE))
            return psf->error;

        psf->write_header = pvf_write_header;
    };

    psf->container_close = pvf_close;

    psf->blockwidth = psf->bytewidth * psf->sf.channels;

    switch (subformat)
    {
    case SF_FORMAT_PCM_S8: /* 8-bit linear PCM. */
    case SF_FORMAT_PCM_16: /* 16-bit linear PCM. */
    case SF_FORMAT_PCM_32: /* 32-bit linear PCM. */
        error = pcm_init(psf);
        break;

    default:
        break;
    };

    return error;
}

static int pvf_close(SF_PRIVATE *UNUSED(psf))
{
    return 0;
}

static int pvf_write_header(SF_PRIVATE *psf, int UNUSED(calc_length))
{
    sf_count_t current;

    if (psf->file.pipeoffset > 0)
        return 0;

    current = psf->ftell();

    /* Reset the current header length to zero. */
    psf->header.ptr[0] = 0;
    psf->header.indx = 0;

    if (psf->file.is_pipe == SF_FALSE)
        psf->fseek(0, SEEK_SET);

    snprintf((char *)psf->header.ptr, psf->header.len, "PVF1\n%d %d %d\n", psf->sf.channels,
             psf->sf.samplerate, psf->bytewidth * 8);

    psf->header.indx = strlen((char *)psf->header.ptr);

    /* Header construction complete so write it out. */
    psf->fwrite(psf->header.ptr, psf->header.indx, 1);

    if (psf->error)
        return psf->error;

    psf->dataoffset = psf->header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->error;
}

static int pvf_read_header(SF_PRIVATE *psf)
{
    char buffer[32];
    int marker, channels, samplerate, bitwidth;

	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("m", &marker);
	psf->binheader_seekf(1, SF_SEEK_CUR);
    psf->log_printf("%M\n", marker);

    if (marker != PVF1_MARKER)
        return SFE_PVF_NO_PVF1;

    /* Grab characters up until a newline which is replaced by an EOS. */
    psf->binheader_readf("G", buffer, sizeof(buffer));

    if (sscanf(buffer, "%d %d %d", &channels, &samplerate, &bitwidth) != 3)
        return SFE_PVF_BAD_HEADER;

    psf->log_printf(" Channels    : %d\n Sample rate : %d\n Bit width   : %d\n", channels,
                   samplerate, bitwidth);

    psf->sf.channels = channels;
    psf->sf.samplerate = samplerate;

    switch (bitwidth)
    {
    case 8:
        psf->sf.format = SF_FORMAT_PVF | SF_FORMAT_PCM_S8;
        psf->bytewidth = 1;
        break;

    case 16:
        psf->sf.format = SF_FORMAT_PVF | SF_FORMAT_PCM_16;
        psf->bytewidth = 2;
        break;
    case 32:
        psf->sf.format = SF_FORMAT_PVF | SF_FORMAT_PCM_32;
        psf->bytewidth = 4;
        break;

    default:
        return SFE_PVF_BAD_BITWIDTH;
    };

    psf->dataoffset = psf->ftell();
    psf->log_printf(" Data Offset : %D\n", psf->dataoffset);

    psf->endian = SF_ENDIAN_BIG;

    psf->datalength = psf->filelength - psf->dataoffset;
    psf->blockwidth = psf->sf.channels * psf->bytewidth;

    if (!psf->sf.frames && psf->blockwidth)
        psf->sf.frames = (psf->filelength - psf->dataoffset) / psf->blockwidth;

    return 0;
}
