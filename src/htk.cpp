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

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

#define SFE_HTK_BAD_FILE_LEN (1666)
#define SFE_HTK_NOT_WAVEFORM (1667)

static int htk_close(SndFile *psf);

static int htk_write_header(SndFile *psf, int calc_length);
static int htk_read_header(SndFile *psf);

int htk_open(SndFile *psf)
{
    int subformat;
    int error = 0;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = htk_read_header(psf)))
            return error;
    };

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_HTK)
            return SFE_BAD_OPEN_FORMAT;

        psf->m_endian = SF_ENDIAN_BIG;

        if (htk_write_header(psf, SF_FALSE))
            return psf->m_error;

        psf->write_header = htk_write_header;
    };

    psf->container_close = htk_close;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    switch (subformat)
    {
    case SF_FORMAT_PCM_16: /* 16-bit linear PCM. */
        error = pcm_init(psf);
        break;

    default:
        break;
    };

    return error;
}

static int htk_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        htk_write_header(psf, SF_TRUE);

    return 0;
}

static int htk_write_header(SndFile *psf, int calc_length)
{
    sf_count_t current;
    int sample_count, sample_period;

    current = psf->ftell();

    if (calc_length)
        psf->m_filelength = psf->get_filelen();

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    if (psf->m_filelength > 12)
        sample_count = (psf->m_filelength - 12) / 2;
    else
        sample_count = 0;

    sample_period = 10000000 / psf->sf.samplerate;

    psf->binheader_writef("E444", BHW4(sample_count), BHW4(sample_period), BHW4(0x20000));

    /* Header construction complete so write it out. */
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

/*
** Found the following info in a comment block within Bill Schottstaedt's
** sndlib library.
**
** HTK format files consist of a contiguous sequence of samples preceded by a
** header. Each sample is a vector of either 2-byte integers or 4-byte floats.
** 2-byte integers are used for compressed forms as described below and for
** vector quantised data as described later in section 5.11. HTK format data
** files can also be used to store speech waveforms as described in section 5.8.
**
** The HTK file format header is 12 bytes long and contains the following data
**   nSamples   -- number of samples in file (4-byte integer)
**   sampPeriod -- sample period in 100ns units (4-byte integer)
**   sampSize   -- number of bytes per sample (2-byte integer)
**   parmKind   -- a code indicating the sample kind (2-byte integer)
**
** The parameter kind  consists of a 6 bit code representing the basic
** parameter kind plus additional bits for each of the possible qualifiers.
** The basic parameter kind codes are
**
**  0    WAVEFORM    sampled waveform
**  1    LPC         linear prediction filter coefficients
**  2    LPREFC      linear prediction reflection coefficients
**  3    LPCEPSTRA   LPC cepstral coefficients
**  4    LPDELCEP    LPC cepstra plus delta coefficients
**  5    IREFC       LPC reflection coef in 16 bit integer format
**  6    MFCC        mel-frequency cepstral coefficients
**  7    FBANK       log mel-filter bank channel outputs
**  8    MELSPEC     linear mel-filter bank channel outputs
**  9    USER        user defined sample kind
**  10   DISCRETE    vector quantised data
**
** and the bit-encoding for the qualifiers (in octal) is
**   _E   000100      has energy
**   _N   000200      absolute energy suppressed
**   _D   000400      has delta coefficients
**   _A   001000      has acceleration coefficients
**   _C   002000      is compressed
**   _Z   004000      has zero mean static coef.
**   _K   010000      has CRC checksum
**   _O   020000      has 0'th cepstral coef.
*/

static int htk_read_header(SndFile *psf)
{
    int sample_count, sample_period, marker;

	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("E444", &sample_count, &sample_period, &marker);

    if (2 * sample_count + 12 != psf->m_filelength)
        return SFE_HTK_BAD_FILE_LEN;

    if (marker != 0x20000)
        return SFE_HTK_NOT_WAVEFORM;

    psf->sf.channels = 1;

    if (sample_period > 0)
    {
        psf->sf.samplerate = 10000000 / sample_period;
        psf->log_printf(
                       "HTK Waveform file\n  Sample Count  : %d\n  Sample "
                       "Period : %d => %d Hz\n",
                       sample_count, sample_period, psf->sf.samplerate);
    }
    else
    {
        psf->sf.samplerate = 16000;
        psf->log_printf(
                       "HTK Waveform file\n  Sample Count  : %d\n  Sample "
                       "Period : %d (should be > 0) => Guessed sample "
                       "rate %d Hz\n",
                       sample_count, sample_period, psf->sf.samplerate);
    };

    psf->sf.format = SF_FORMAT_HTK | SF_FORMAT_PCM_16;
    psf->m_bytewidth = 2;

    /* HTK always has a 12 byte header. */
    psf->m_dataoffset = 12;
    psf->m_endian = SF_ENDIAN_BIG;

    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

    psf->m_blockwidth = psf->sf.channels * psf->m_bytewidth;

    if (!psf->sf.frames && psf->m_blockwidth)
        psf->sf.frames = (psf->m_filelength - psf->m_dataoffset) / psf->m_blockwidth;

    return 0;
}
