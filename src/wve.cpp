/*
** Copyright (C) 2002-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2007 Reuben Thomas
** Copyright (C) 2018 evpobr <evpobr@github.com>
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

#define ALAW_MARKER MAKE_MARKER('A', 'L', 'a', 'w')
#define SOUN_MARKER MAKE_MARKER('S', 'o', 'u', 'n')
#define DFIL_MARKER MAKE_MARKER('d', 'F', 'i', 'l')
#define ESSN_MARKER MAKE_MARKER('e', '*', '*', '\0')
#define PSION_VERSION ((unsigned short)3856)
#define PSION_DATAOFFSET (0x20)

static int wve_read_header(SndFile *psf);
static int wve_write_header(SndFile *psf, int calc_length);
static int wve_close(SndFile *psf);

int wve_open(SndFile *psf)
{
    int error = 0;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = wve_read_header(psf)))
            return error;
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_WVE)
            return SFE_BAD_OPEN_FORMAT;

        psf->m_endian = SF_ENDIAN_BIG;

        if ((error = wve_write_header(psf, SF_FALSE)))
            return error;

        psf->write_header = wve_write_header;
    };

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    psf->container_close = wve_close;

    error = alaw_init(psf);

    return error;
}

static int wve_read_header(SndFile *psf)
{
    /* Set position to start of file to begin reading header. */
    uint32_t marker;
	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("m", &marker);
    if (marker != ALAW_MARKER)
    {
        psf->log_printf("Could not find '%M'\n", ALAW_MARKER);
        return SFE_WVE_NOT_WVE;
    };

    psf->binheader_readf("m", &marker);
    if (marker != SOUN_MARKER)
    {
        psf->log_printf("Could not find '%M'\n", SOUN_MARKER);
        return SFE_WVE_NOT_WVE;
    };

    psf->binheader_readf("m", &marker);
    if (marker != DFIL_MARKER)
    {
        psf->log_printf("Could not find '%M'\n", DFIL_MARKER);
        return SFE_WVE_NOT_WVE;
    };

    psf->binheader_readf("m", &marker);
    if (marker != ESSN_MARKER)
    {
        psf->log_printf("Could not find '%M'\n", ESSN_MARKER);
        return SFE_WVE_NOT_WVE;
    };

    uint16_t version;
    psf->binheader_readf("E2", &version);

    psf->log_printf("Psion Palmtop Alaw (.wve)\n"
                        "  Sample Rate : 8000\n"
                        "  Channels    : 1\n"
                        "  Encoding    : A-law\n");

    if (version != PSION_VERSION)
        psf->log_printf("Psion version %d should be %d\n", version, PSION_VERSION);

    uint32_t datalength;
    psf->binheader_readf("E4", &datalength);
    psf->m_dataoffset = PSION_DATAOFFSET;
    if (datalength != psf->m_filelength - psf->m_dataoffset)
    {
        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
        psf->log_printf("Data length %d should be %D\n", datalength, psf->m_datalength);
    }
    else
    {
        psf->m_datalength = datalength;
    }

    uint16_t padding, repeats, trash;
    psf->binheader_readf("E22222", &padding, &repeats, &trash, &trash, &trash);

    psf->sf.format = SF_FORMAT_WVE | SF_FORMAT_ALAW;
    psf->sf.samplerate = 8000;
    psf->sf.frames = psf->m_datalength;
    psf->sf.channels = 1;

    return SFE_NO_ERROR;
}

static int wve_write_header(SndFile *psf, int calc_length)
{
    sf_count_t current = current = psf->ftell();

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
        if (psf->m_dataend)
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;

        psf->sf.frames = psf->m_datalength / (psf->m_bytewidth * psf->sf.channels);
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    /* Write header. */
    uint32_t datalen = (uint32_t)psf->m_datalength;
    psf->binheader_writef("Emmmm", BHWm(ALAW_MARKER), BHWm(SOUN_MARKER), BHWm(DFIL_MARKER),
                         BHWm(ESSN_MARKER));
    psf->binheader_writef("E2422222", BHW2(PSION_VERSION), BHW4(datalen), BHW2(0), BHW2(0),
                         BHW2(0), BHW2(0), BHW2(0));
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->sf.channels != 1)
        return SFE_CHANNEL_COUNT;

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int wve_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        /*
		 * Now we know for certain the length of the file we can re-write
		 * the header.
		 */
        wve_write_header(psf, SF_TRUE);
    };

    return 0;
}
