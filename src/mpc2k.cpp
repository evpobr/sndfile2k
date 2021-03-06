/*
** Copyright (C) 2008-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
**	Info from Olivier Tristan <o.tristan@ultimatesoundbank.com>
**
**	HEADER
**	2 magic bytes: 1 and 4.
**	17 char for the name of the sample.
**	3 bytes: level, tune and channels (0 for channels is mono while 1 is stereo)
**	4 uint32: sampleStart, loopEnd, sampleFrames and loopLength
**	1 byte: loopMode (0 no loop, 1 forward looping)
**	1 byte: number of beat in loop
**	1 uint16: sampleRate
**
**	DATA
**	Data are always non compressed 16 bits interleaved
*/

#define HEADER_LENGTH (42) /* Sum of above data fields. */
#define HEADER_NAME_LEN (17) /* Length of name string. */

#define SFE_MPC_NO_MARKER (666)

static int mpc2k_close(SndFile *psf);

static int mpc2k_write_header(SndFile *psf, int calc_length);
static int mpc2k_read_header(SndFile *psf);

int mpc2k_open(SndFile *psf)
{
    int error = 0;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = mpc2k_read_header(psf)))
            return error;
    };

    if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_MPC2K)
        return SFE_BAD_OPEN_FORMAT;

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if (mpc2k_write_header(psf, SF_FALSE))
            return psf->m_error;

        psf->write_header = mpc2k_write_header;
    };

    psf->container_close = mpc2k_close;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    error = pcm_init(psf);

    return error;
}

static int mpc2k_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        mpc2k_write_header(psf, SF_TRUE);

    return 0;
}

static int mpc2k_write_header(SndFile *psf, int calc_length)
{
    char sample_name[HEADER_NAME_LEN + 1];
    sf_count_t current;

    current = psf->ftell();

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_dataoffset = HEADER_LENGTH;
        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

        psf->sf.frames = psf->m_datalength / (psf->m_bytewidth * psf->sf.channels);
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;

    psf->fseek(0, SEEK_SET);

    snprintf(sample_name, sizeof(sample_name), "%s                    ", psf->m_path);

    psf->binheader_writef("e11b", BHW1(1), BHW1(4), BHWv(sample_name), BHWz(HEADER_NAME_LEN));
    psf->binheader_writef("e111", BHW1(100), BHW1(0), BHW1((psf->sf.channels - 1) & 1));
    psf->binheader_writef("et4888", BHW4(0), BHW8(psf->sf.frames), BHW8(psf->sf.frames),
                         BHW8(psf->sf.frames));
    psf->binheader_writef("e112", BHW1(0), BHW1(1), BHW2((uint16_t)psf->sf.samplerate));

    /* Always 16 bit little endian data. */
    psf->m_bytewidth = 2;
    psf->m_endian = SF_ENDIAN_LITTLE;

    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int mpc2k_read_header(SndFile *psf)
{
    char sample_name[HEADER_NAME_LEN + 1];
    unsigned char bytes[4];
    uint32_t sample_start, loop_end, sample_frames, loop_length;
    uint16_t sample_rate;

	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("ebb", bytes, 2, sample_name, make_size_t(HEADER_NAME_LEN));

    if (bytes[0] != 1 || bytes[1] != 4)
        return SFE_MPC_NO_MARKER;

    sample_name[HEADER_NAME_LEN] = 0;

    psf->log_printf("MPC2000\n  Name         : %s\n", sample_name);

    psf->binheader_readf("eb4444", bytes, 3, &sample_start, &loop_end, &sample_frames,
                        &loop_length);

    psf->sf.channels = bytes[2] ? 2 : 1;

    psf->log_printf("  Level        : %d\n  Tune         : %d\n  Stereo       : %s\n", bytes[0],
                   bytes[1], bytes[2] ? "Yes" : "No");

    psf->log_printf(
                   "  Sample start : %d\n  Loop end     : %d\n  Frames    "
                   "   : %d\n  Length       : %d\n",
                   sample_start, loop_end, sample_frames, loop_length);

    psf->binheader_readf("eb2", bytes, 2, &sample_rate);

    psf->log_printf("  Loop mode    : %s\n  Beats        : %d\n  Sample rate  : %d\nEnd\n",
                   bytes[0] ? "None" : "Fwd", bytes[1], sample_rate);

    psf->sf.samplerate = sample_rate;

    psf->sf.format = SF_FORMAT_MPC2K | SF_FORMAT_PCM_16;

    psf->m_dataoffset = psf->ftell();

    /* Always 16 bit little endian data. */
    psf->m_bytewidth = 2;
    psf->m_endian = SF_ENDIAN_LITTLE;

    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
    psf->m_blockwidth = psf->sf.channels * psf->m_bytewidth;
    psf->sf.frames = psf->m_datalength / psf->m_blockwidth;

    psf->sf.frames = (psf->m_filelength - psf->m_dataoffset) / psf->m_blockwidth;

    return 0;
}
