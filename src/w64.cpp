/*
** Copyright (C) 1999-2018 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <ctype.h>
#include <time.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"
#include "wavlike.h"

#include <algorithm>

/*
 * W64 files use 16 byte markers as opposed to the four byte marker of
 * WAV files.
 * For comparison purposes, an integer is required, so make an integer
 * hash for the 16 bytes using MAKE_HASH16 macro, but also create a 16
 * byte array containing the complete 16 bytes required when writing the
 * header.
 */

#define MAKE_HASH16(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, xa, xb, xc, xd, xe, xf)             \
    ((x0) ^ ((x1) << 1) ^ ((x2) << 2) ^ ((x3) << 3) ^ ((x4) << 4) ^ ((x5) << 5) ^ ((x6) << 6) ^ \
     ((x7) << 7) ^ ((x8) << 8) ^ ((x9) << 9) ^ ((xa) << 10) ^ ((xb) << 11) ^ ((xc) << 12) ^     \
     ((xd) << 13) ^ ((xe) << 14) ^ ((xf) << 15))

#define MAKE_MARKER16(name, x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, xa, xb, xc, xd, xe, xf) \
    static unsigned char name[16] = {(x0), (x1), (x2), (x3), (x4), (x5), (x6), (x7),        \
                                     (x8), (x9), (xa), (xb), (xc), (xd), (xe), (xf)}

#define riff_HASH16                                                                             \
    MAKE_HASH16('r', 'i', 'f', 'f', 0x2E, 0x91, 0xCF, 0x11, 0xA5, 0xD6, 0x28, 0xDB, 0x04, 0xC1, \
                0x00, 0x00)

#define wave_HASH16                                                                             \
    MAKE_HASH16('w', 'a', 'v', 'e', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, \
                0xDB, 0x8A)

#define fmt_HASH16                                                                              \
    MAKE_HASH16('f', 'm', 't', ' ', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, \
                0xDB, 0x8A)

#define fact_HASH16                                                                             \
    MAKE_HASH16('f', 'a', 'c', 't', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, \
                0xDB, 0x8A)

#define data_HASH16                                                                             \
    MAKE_HASH16('d', 'a', 't', 'a', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4F, 0x8E, \
                0xDB, 0x8A)

#define ACID_HASH16                                                                           \
    MAKE_HASH16(0x6D, 0x07, 0x1C, 0xEA, 0xA3, 0xEF, 0x78, 0x4C, 0x90, 0x57, 0x7F, 0x79, 0xEE, \
                0x25, 0x2A, 0xAE)

#define levl_HASH16                                                                           \
    MAKE_HASH16(0x6c, 0x65, 0x76, 0x6c, 0xf3, 0xac, 0xd3, 0x11, 0xd1, 0x8c, 0x00, 0xC0, 0x4F, \
                0x8E, 0xDB, 0x8A)

#define list_HASH16                                                                           \
    MAKE_HASH16(0x6C, 0x69, 0x73, 0x74, 0x2F, 0x91, 0xCF, 0x11, 0xA5, 0xD6, 0x28, 0xDB, 0x04, \
                0xC1, 0x00, 0x00)

#define junk_HASH16                                                                           \
    MAKE_HASH16(0x6A, 0x75, 0x6E, 0x6b, 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0, 0x4f, \
                0x8E, 0xDB, 0x8A)

#define bext_HASH16                                                                           \
    MAKE_HASH16(0x62, 0x65, 0x78, 0x74, 0xf3, 0xac, 0xd3, 0xaa, 0xd1, 0x8c, 0x00, 0xC0, 0x4F, \
                0x8E, 0xDB, 0x8A)

#define MARKER_HASH16                                                                         \
    MAKE_HASH16(0x56, 0x62, 0xf7, 0xab, 0x2d, 0x39, 0xd2, 0x11, 0x86, 0xc7, 0x00, 0xc0, 0x4f, \
                0x8e, 0xdb, 0x8a)

#define SUMLIST_HASH16                                                                        \
    MAKE_HASH16(0xBC, 0x94, 0x5F, 0x92, 0x5A, 0x52, 0xD2, 0x11, 0x86, 0xDC, 0x00, 0xC0, 0x4F, \
                0x8E, 0xDB, 0x8A)

MAKE_MARKER16(riff_MARKER16, 'r', 'i', 'f', 'f', 0x2E, 0x91, 0xCF, 0x11, 0xA5, 0xD6, 0x28, 0xDB,
              0x04, 0xC1, 0x00, 0x00);

MAKE_MARKER16(wave_MARKER16, 'w', 'a', 'v', 'e', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0,
              0x4F, 0x8E, 0xDB, 0x8A);

MAKE_MARKER16(fmt_MARKER16, 'f', 'm', 't', ' ', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0,
              0x4F, 0x8E, 0xDB, 0x8A);

MAKE_MARKER16(fact_MARKER16, 'f', 'a', 'c', 't', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0,
              0x4F, 0x8E, 0xDB, 0x8A);

MAKE_MARKER16(data_MARKER16, 'd', 'a', 't', 'a', 0xF3, 0xAC, 0xD3, 0x11, 0x8C, 0xD1, 0x00, 0xC0,
              0x4F, 0x8E, 0xDB, 0x8A);

enum
{
    HAVE_riff = 0x01,
    HAVE_wave = 0x02,
    HAVE_fmt = 0x04,
    HAVE_fact = 0x08,
    HAVE_data = 0x20
};

static int w64_read_header(SndFile *psf, int *blockalign, int *framesperblock);
static int w64_write_header(SndFile *psf, int calc_length);
static int w64_close(SndFile *psf);

int w64_open(SndFile *psf)
{
    WAVLIKE_PRIVATE *wpriv;
    int subformat, error, blockalign = 0, framesperblock = 0;

    if ((wpriv = (WAVLIKE_PRIVATE *)calloc(1, sizeof(WAVLIKE_PRIVATE))) == NULL)
        return SFE_MALLOC_FAILED;
    psf->m_container_data = wpriv;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = w64_read_header(psf, &blockalign, &framesperblock)))
            return error;
    };

    if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_W64)
        return SFE_BAD_OPEN_FORMAT;

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        psf->m_endian = SF_ENDIAN_LITTLE; /* All W64 files are little endian. */

        psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

        if (subformat == SF_FORMAT_IMA_ADPCM || subformat == SF_FORMAT_MS_ADPCM)
        {
            blockalign = wavlike_srate2blocksize(psf->sf.samplerate * psf->sf.channels);
            framesperblock = -1;

            /*
			 * At this point we don't know the file length so set it stupidly high, but not
			 * so high that it triggers undefined behaviour whan something is added to it.
			 */
            psf->m_filelength = SF_COUNT_MAX - 10000;
            psf->m_datalength = psf->m_filelength;
            if (psf->sf.frames <= 0)
                psf->sf.frames =
                    (psf->m_blockwidth) ? psf->m_filelength / psf->m_blockwidth : psf->m_filelength;
        };

        if ((error = w64_write_header(psf, SF_FALSE)))
            return error;

        psf->write_header = w64_write_header;
    };

    psf->container_close = w64_close;

    switch (subformat)
    {
    case SF_FORMAT_PCM_U8:
        error = pcm_init(psf);
        break;

    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
        error = pcm_init(psf);
        break;

    case SF_FORMAT_ULAW:
        error = ulaw_init(psf);
        break;

    case SF_FORMAT_ALAW:
        error = alaw_init(psf);
        break;

    case SF_FORMAT_FLOAT:
        error = float32_init(psf);
        break;

    case SF_FORMAT_DOUBLE:
        error = double64_init(psf);
        break;

    case SF_FORMAT_IMA_ADPCM:
        error = wavlike_ima_init(psf, blockalign, framesperblock);
        break;

    case SF_FORMAT_MS_ADPCM:
        error = wavlike_msadpcm_init(psf, blockalign, framesperblock);
        break;

    case SF_FORMAT_GSM610:
        error = gsm610_init(psf);
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    return error;
}

static int w64_read_header(SndFile *psf, int *blockalign, int *framesperblock)
{
    WAVLIKE_PRIVATE *wpriv;
    WAV_FMT *wav_fmt;
    int dword = 0, marker, format = 0;
    sf_count_t chunk_size, bytesread = 0;
    int parsestage = 0, error, done = 0;

    if ((wpriv = (WAVLIKE_PRIVATE *)psf->m_container_data) == NULL)
        return SFE_INTERNAL;
    wav_fmt = &wpriv->wav_fmt;

    /* Set position to start of file to begin reading header. */
	psf->binheader_seekf(0, SF_SEEK_SET);

    while (!done)
    {
        /* Each new chunk must start on an 8 byte boundary, so jump if needed. */
        if (psf->m_header.indx & 0x7)
			psf->binheader_seekf(8 - (psf->m_header.indx & 0x7), SF_SEEK_CUR);

        /* Generate hash of 16 byte marker. */
        marker = chunk_size = 0;
        bytesread = psf->binheader_readf("eh8", &marker, &chunk_size);
        if (bytesread == 0)
            break;
        switch (marker)
        {
        case riff_HASH16:
            if (parsestage)
                return SFE_W64_NO_RIFF;

            if (psf->m_filelength != chunk_size)
                psf->log_printf("riff : %D (should be %D)\n", chunk_size, psf->m_filelength);
            else
                psf->log_printf("riff : %D\n", chunk_size);

            parsestage |= HAVE_riff;

            bytesread += psf->binheader_readf("h", &marker);
            if (marker == wave_HASH16)
            {
                if ((parsestage & HAVE_riff) != HAVE_riff)
                    return SFE_W64_NO_WAVE;
                psf->log_printf("wave\n");
                parsestage |= HAVE_wave;
            };
            chunk_size = 0;
            break;

        case ACID_HASH16:
            psf->log_printf("Looks like an ACID file. Exiting.\n");
            return SFE_UNIMPLEMENTED;

        case fmt_HASH16:
            if ((parsestage & (HAVE_riff | HAVE_wave)) != (HAVE_riff | HAVE_wave))
                return SFE_WAV_NO_FMT;

            psf->log_printf(" fmt : %D\n", chunk_size);

            /* size of 16 byte marker and 8 byte chunk_size value. */
            chunk_size -= 24;

            if ((error = wavlike_read_fmt_chunk(psf, (int)chunk_size)))
                return error;

			if (chunk_size % 8)
				psf->binheader_seekf(8 - (chunk_size % 8), SF_SEEK_CUR);

            format = wav_fmt->format;
            parsestage |= HAVE_fmt;
            chunk_size = 0;
            break;

        case fact_HASH16:
        {
            sf_count_t frames;

            psf->binheader_readf("e8", &frames);
            psf->log_printf("fact : %D\n  frames : %D\n", chunk_size, frames);
        };
            chunk_size = 0;
            break;

        case data_HASH16:
            if ((parsestage & (HAVE_riff | HAVE_wave | HAVE_fmt)) !=
                (HAVE_riff | HAVE_wave | HAVE_fmt))
                return SFE_W64_NO_DATA;

            psf->m_dataoffset = psf->ftell();
            psf->m_datalength = std::min(chunk_size - 24, psf->m_filelength - psf->m_dataoffset);

            if (chunk_size % 8)
                chunk_size += 8 - (chunk_size % 8);

            psf->log_printf("data : %D\n", chunk_size);

            parsestage |= HAVE_data;

            if (!psf->sf.seekable)
                break;

            /* Seek past data and continue reading header. */
            psf->fseek(chunk_size, SEEK_CUR);
            chunk_size = 0;
            break;

        case levl_HASH16:
            psf->log_printf("levl : %D\n", chunk_size);
            chunk_size -= 24;
            break;

        case list_HASH16:
            psf->log_printf("list : %D\n", chunk_size);
            chunk_size -= 24;
            break;

        case junk_HASH16:
            psf->log_printf("junk : %D\n", chunk_size);
            chunk_size -= 24;
            break;

        case bext_HASH16:
            psf->log_printf("bext : %D\n", chunk_size);
            chunk_size -= 24;
            break;

        case MARKER_HASH16:
            psf->log_printf("marker : %D\n", chunk_size);
            chunk_size -= 24;
            break;

        case SUMLIST_HASH16:
            psf->log_printf("summary list : %D\n", chunk_size);
            chunk_size -= 24;
            break;

        default:
            psf->log_printf(
                           "*** Unknown chunk marker (%X) at position %D "
                           "with length %D. Exiting parser.\n",
                           marker, psf->ftell() - 8, chunk_size);
            done = SF_TRUE;
            break;
        };

        if (chunk_size >= psf->m_filelength)
        {
            psf->log_printf("*** Chunk size %u > file length %D. Exiting parser.\n", chunk_size,
                           psf->m_filelength);
            break;
        };

        if (psf->sf.seekable == 0 && (parsestage & HAVE_data))
            break;

        if (psf->ftell() >= (psf->m_filelength - (2 * SIGNED_SIZEOF(dword))))
            break;

        if (chunk_size > 0 && chunk_size < 0xffff0000)
        {
            dword = chunk_size;
			psf->binheader_seekf(dword - 24, SF_SEEK_CUR);
        };
    };

    if (psf->m_dataoffset <= 0)
        return SFE_W64_NO_DATA;

    if (psf->sf.channels < 1)
        return SFE_CHANNEL_COUNT_ZERO;

    if (psf->sf.channels > SF_MAX_CHANNELS)
        return SFE_CHANNEL_COUNT;

    psf->m_endian = SF_ENDIAN_LITTLE; /* All W64 files are little endian. */

    if (psf->ftell() != psf->m_dataoffset)
        psf->fseek(psf->m_dataoffset, SEEK_SET);

    if (psf->m_blockwidth)
    {
        if (psf->m_filelength - psf->m_dataoffset < psf->m_datalength)
            psf->sf.frames = (psf->m_filelength - psf->m_dataoffset) / psf->m_blockwidth;
        else
            psf->sf.frames = psf->m_datalength / psf->m_blockwidth;
    };

    switch (format)
    {
    case WAVE_FORMAT_PCM:
    case WAVE_FORMAT_EXTENSIBLE:
        /* extensible might be FLOAT, MULAW, etc as well! */
        psf->sf.format = SF_FORMAT_W64 | u_bitwidth_to_subformat(psf->m_bytewidth * 8);
        break;

    case WAVE_FORMAT_MULAW:
        psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_ULAW);
        break;

    case WAVE_FORMAT_ALAW:
        psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_ALAW);
        break;

    case WAVE_FORMAT_MS_ADPCM:
        psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_MS_ADPCM);
        *blockalign = wav_fmt->msadpcm.blockalign;
        *framesperblock = wav_fmt->msadpcm.samplesperblock;
        break;

    case WAVE_FORMAT_IMA_ADPCM:
        psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_IMA_ADPCM);
        *blockalign = wav_fmt->ima.blockalign;
        *framesperblock = wav_fmt->ima.samplesperblock;
        break;

    case WAVE_FORMAT_GSM610:
        psf->sf.format = (SF_FORMAT_W64 | SF_FORMAT_GSM610);
        break;

    case WAVE_FORMAT_IEEE_FLOAT:
        psf->sf.format = SF_FORMAT_W64;
        psf->sf.format |= (psf->m_bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    return 0;
}

static int w64_write_header(SndFile *psf, int calc_length)
{
    sf_count_t fmt_size, current;
    size_t fmt_pad = 0;
    int subformat, add_fact_chunk = SF_FALSE;

    current = psf->ftell();

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
        if (psf->m_dataend)
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;

        if (psf->m_bytewidth)
            psf->sf.frames = psf->m_datalength / (psf->m_bytewidth * psf->sf.channels);
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    /* riff marker, length, wave and 'fmt ' markers. */
    psf->binheader_writef("eh8hh", BHWh(riff_MARKER16), BHW8(psf->m_filelength),
                         BHWh(wave_MARKER16), BHWh(fmt_MARKER16));

    subformat = SF_CODEC(psf->sf.format);

    switch (subformat)
    {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
        fmt_size = 24 + 2 + 2 + 4 + 4 + 2 + 2;
        fmt_pad = (size_t)((fmt_size & 0x7) ? 8 - (fmt_size & 0x7) : 0);
        fmt_size += fmt_pad;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("e8224", BHW8(fmt_size), BHW2(WAVE_FORMAT_PCM),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("e4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth */
        psf->binheader_writef("e22", BHW2(psf->m_bytewidth * psf->sf.channels),
                             BHW2(psf->m_bytewidth * 8));
        break;

    case SF_FORMAT_FLOAT:
    case SF_FORMAT_DOUBLE:
        fmt_size = 24 + 2 + 2 + 4 + 4 + 2 + 2;
        fmt_pad = (size_t)((fmt_size & 0x7) ? 8 - (fmt_size & 0x7) : 0);
        fmt_size += fmt_pad;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("e8224", BHW8(fmt_size), BHW2(WAVE_FORMAT_IEEE_FLOAT),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("e4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth */
        psf->binheader_writef("e22", BHW2(psf->m_bytewidth * psf->sf.channels),
                             BHW2(psf->m_bytewidth * 8));

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_ULAW:
        fmt_size = 24 + 2 + 2 + 4 + 4 + 2 + 2;
        fmt_pad = (size_t)((fmt_size & 0x7) ? 8 - (fmt_size & 0x7) : 0);
        fmt_size += fmt_pad;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("e8224", BHW8(fmt_size), BHW2(WAVE_FORMAT_MULAW),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("e4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth */
        psf->binheader_writef("e22", BHW2(psf->m_bytewidth * psf->sf.channels), BHW2(8));

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_ALAW:
        fmt_size = 24 + 2 + 2 + 4 + 4 + 2 + 2;
        fmt_pad = (size_t)((fmt_size & 0x7) ? 8 - (fmt_size & 0x7) : 0);
        fmt_size += fmt_pad;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("e8224", BHW8(fmt_size), BHW2(WAVE_FORMAT_ALAW),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("e4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth */
        psf->binheader_writef("e22", BHW2(psf->m_bytewidth * psf->sf.channels), BHW2(8));

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_IMA_ADPCM:
    {
        int blockalign, framesperblock, bytespersec;

        blockalign = wavlike_srate2blocksize(psf->sf.samplerate * psf->sf.channels);
        framesperblock = 2 * (blockalign - 4 * psf->sf.channels) / psf->sf.channels + 1;
        bytespersec = (psf->sf.samplerate * blockalign) / framesperblock;

        /* fmt chunk. */
        fmt_size = 24 + 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2;
        fmt_pad = (size_t)((fmt_size & 0x7) ? 8 - (fmt_size & 0x7) : 0);
        fmt_size += fmt_pad;

        /* fmt : size, WAV format type, channels. */
        psf->binheader_writef("e822", BHW8(fmt_size), BHW2(WAVE_FORMAT_IMA_ADPCM),
                             BHW2(psf->sf.channels));

        /* fmt : samplerate, bytespersec. */
        psf->binheader_writef("e44", BHW4(psf->sf.samplerate), BHW4(bytespersec));

        /* fmt : blockalign, bitwidth, extrabytes, framesperblock. */
        psf->binheader_writef("e2222", BHW2(blockalign), BHW2(4), BHW2(2),
                             BHW2(framesperblock));
    };

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_MS_ADPCM:
    {
        int blockalign, framesperblock, bytespersec, extrabytes;

        blockalign = wavlike_srate2blocksize(psf->sf.samplerate * psf->sf.channels);
        framesperblock = 2 + 2 * (blockalign - 7 * psf->sf.channels) / psf->sf.channels;
        bytespersec = (psf->sf.samplerate * blockalign) / framesperblock;

        /* fmt chunk. */
        extrabytes = 2 + 2 + WAVLIKE_MSADPCM_ADAPT_COEFF_COUNT * (2 + 2);
        fmt_size = 24 + 2 + 2 + 4 + 4 + 2 + 2 + 2 + extrabytes;
        fmt_pad = (size_t)((fmt_size & 0x7) ? 8 - (fmt_size & 0x7) : 0);
        fmt_size += fmt_pad;

        /* fmt : size, W64 format type, channels. */
        psf->binheader_writef("e822", BHW8(fmt_size), BHW2(WAVE_FORMAT_MS_ADPCM),
                             BHW2(psf->sf.channels));

        /* fmt : samplerate, bytespersec. */
        psf->binheader_writef("e44", BHW4(psf->sf.samplerate), BHW4(bytespersec));

        /* fmt : blockalign, bitwidth, extrabytes, framesperblock. */
        psf->binheader_writef("e22222", BHW2(blockalign), BHW2(4), BHW2(extrabytes),
                             BHW2(framesperblock), BHW2(7));

        wavlike_msadpcm_write_adapt_coeffs(psf);
    };

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_GSM610:
    {
        int bytespersec;

        bytespersec = (psf->sf.samplerate * WAVLIKE_GSM610_BLOCKSIZE) / WAVLIKE_GSM610_SAMPLES;

        /* fmt chunk. */
        fmt_size = 24 + 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2;
        fmt_pad = (size_t)((fmt_size & 0x7) ? 8 - (fmt_size & 0x7) : 0);
        fmt_size += fmt_pad;

        /* fmt : size, WAV format type, channels. */
        psf->binheader_writef("e822", BHW8(fmt_size), BHW2(WAVE_FORMAT_GSM610),
                             BHW2(psf->sf.channels));

        /* fmt : samplerate, bytespersec. */
        psf->binheader_writef("e44", BHW4(psf->sf.samplerate), BHW4(bytespersec));

        /* fmt : blockalign, bitwidth, extrabytes, framesperblock. */
        psf->binheader_writef("e2222", BHW2(WAVLIKE_GSM610_BLOCKSIZE), BHW2(0), BHW2(2),
                             BHW2(WAVLIKE_GSM610_SAMPLES));
    };

        add_fact_chunk = SF_TRUE;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    /* Pad to 8 bytes with zeros. */
    if (fmt_pad > 0)
        psf->binheader_writef("z", BHWz(fmt_pad));

    if (add_fact_chunk)
        psf->binheader_writef("eh88", BHWh(fact_MARKER16), BHW8((sf_count_t)(16 + 8 + 8)),
                             BHW8(psf->sf.frames));

    psf->binheader_writef("eh8", BHWh(data_MARKER16), BHW8(psf->m_datalength + 24));
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int w64_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        w64_write_header(psf, SF_TRUE);

    return 0;
}
