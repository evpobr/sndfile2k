/*
** Copyright (C) 2004-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <string.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

#define TWOBIT_MARKER (MAKE_MARKER('2', 'B', 'I', 'T'))
#define AVR_HDR_SIZE (128)

#define SFE_AVR_X (666)

/*
 * From: hyc@hanauma.Jpl.Nasa.Gov (Howard Chu)
 *
 * A lot of PD software exists to play Mac .snd files on the ST. One other
 * format that seems pretty popular (used by a number of commercial packages)
 * is the AVR format (from Audio Visual Research). This format has a 128 byte
 * header that looks like this (its actually packed, but thats not portable):
 */

typedef struct
{
    /* 2BIT */
    int marker;
    /* null-padded sample name */
    char name[8];
    /* 0 = mono, 0xffff = stereo */
    short mono;
    /* 8 = 8 bit, 16 = 16 bit */
    short rez;
    /* 0 = unsigned, 0xffff = signed */
    short sign;
    /* 0 = no loop, 0xffff = looping sample */
    short loop;
    /* 0xffff = no MIDI note assigned,  */
    /* 0xffXX = single key note assignment */
    /* 0xLLHH = key split, low/hi note */
    short midi;
    /* sample frequency in hertz */
    int srate;
    /* sample length in bytes or words (see rez) */
    int frames;
    /* offset to start of loop in bytes or words. */
    /* set to zero if unused */
    int lbeg;
    /* offset to end of loop in bytes or words. */
    /* set to sample length if unused */
    int lend;
    /* Reserved, MIDI keyboard split */
    short res1;
    /* Reserved, sample compression */
    short res2;
    /* Reserved */
    short res3;
    /* Additional filename space, used if (name[7] != 0) */
    char ext[20];
    /* User defined. Typically ASCII message */
    char user[64];
} AVR_HEADER;

/*
 * Private static functions.
 */

static int avr_close(SndFile *psf);

static int avr_read_header(SndFile *psf);
static int avr_write_header(SndFile *psf, int calc_length);

/*
 * Public function.
 */

int avr_open(SndFile *psf)
{
    int error = 0;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = avr_read_header(psf)))
            return error;
    };

    if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_AVR)
        return SFE_BAD_OPEN_FORMAT;

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        psf->m_endian = SF_ENDIAN_BIG;

        if (avr_write_header(psf, SF_FALSE))
            return psf->m_error;

        psf->write_header = avr_write_header;
    };

    psf->container_close = avr_close;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    error = pcm_init(psf);

    return error;
}

static int avr_read_header(SndFile *psf)
{
    AVR_HEADER hdr;

    memset(&hdr, 0, sizeof(hdr));

	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("mb", &hdr.marker, &hdr.name, sizeof(hdr.name));
    psf->log_printf("%M\n", hdr.marker);

    if (hdr.marker != TWOBIT_MARKER)
        return SFE_AVR_X;

    psf->log_printf("  Name        : %s\n", hdr.name);

    psf->binheader_readf("E22222", &hdr.mono, &hdr.rez, &hdr.sign, &hdr.loop, &hdr.midi);

    psf->sf.channels = (hdr.mono & 1) + 1;

    psf->log_printf("  Channels    : %d\n  Bit width   : %d\n  Signed      : %s\n",
                   (hdr.mono & 1) + 1, hdr.rez, hdr.sign ? "yes" : "no");

    switch ((hdr.rez << 16) + (hdr.sign & 1))
    {
    case ((8 << 16) + 0):
        psf->sf.format = SF_FORMAT_AVR | SF_FORMAT_PCM_U8;
        psf->m_bytewidth = 1;
        break;

    case ((8 << 16) + 1):
        psf->sf.format = SF_FORMAT_AVR | SF_FORMAT_PCM_S8;
        psf->m_bytewidth = 1;
        break;

    case ((16 << 16) + 1):
        psf->sf.format = SF_FORMAT_AVR | SF_FORMAT_PCM_16;
        psf->m_bytewidth = 2;
        break;

    default:
        psf->log_printf("Error : bad rez/sign combination.\n");
        return SFE_AVR_X;
    };

    psf->binheader_readf("E4444", &hdr.srate, &hdr.frames, &hdr.lbeg, &hdr.lend);

    psf->sf.frames = hdr.frames;
    psf->sf.samplerate = hdr.srate;

    psf->log_printf("  Frames      : %D\n", psf->sf.frames);
    psf->log_printf("  Sample rate : %d\n", psf->sf.samplerate);

    psf->binheader_readf("E222", &hdr.res1, &hdr.res2, &hdr.res3);
    psf->binheader_readf("bb", hdr.ext, sizeof(hdr.ext), hdr.user, sizeof(hdr.user));

    psf->log_printf("  Ext         : %s\n  User        : %s\n", hdr.ext, hdr.user);

    psf->m_endian = SF_ENDIAN_BIG;

    psf->m_dataoffset = AVR_HDR_SIZE;
    psf->m_datalength = hdr.frames * (hdr.rez / 8);

	if (psf->ftell() != psf->m_dataoffset)
		psf->binheader_seekf(psf->m_dataoffset - psf->ftell(), SF_SEEK_CUR);

    psf->m_blockwidth = psf->sf.channels * psf->m_bytewidth;

    if (psf->sf.frames == 0 && psf->m_blockwidth)
        psf->sf.frames = (psf->m_filelength - psf->m_dataoffset) / psf->m_blockwidth;

    return 0;
}

static int avr_write_header(SndFile *psf, int calc_length)
{
    sf_count_t current;
    int sign;

    current = psf->ftell();

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

    psf->binheader_writef("Emz22", BHWm(TWOBIT_MARKER), BHWz(8),
                         BHW2(psf->sf.channels == 2 ? 0xFFFF : 0), BHW2(psf->m_bytewidth * 8));

    sign = ((SF_CODEC(psf->sf.format)) == SF_FORMAT_PCM_U8) ? 0 : 0xFFFF;

    psf->binheader_writef("E222", BHW2(sign), BHW2(0), BHW2(0xFFFF));
    psf->binheader_writef("E4444", BHW4(psf->sf.samplerate), BHW4(psf->sf.frames), BHW4(0),
                         BHW4(0));

    psf->binheader_writef("E222zz", BHW2(0), BHW2(0), BHW2(0), BHWz(20), BHWz(64));

    /* Header construction complete so write it out. */
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int avr_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        avr_write_header(psf, SF_TRUE);

    return 0;
}
