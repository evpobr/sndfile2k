/*
** Copyright (C) 1999-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2004-2005 David Viens <davidv@plogue.com>
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
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <inttypes.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"
#include "wavlike.h"

#include <algorithm>

#define RIFF_MARKER (MAKE_MARKER('R', 'I', 'F', 'F'))
#define RIFX_MARKER (MAKE_MARKER('R', 'I', 'F', 'X'))
#define WAVE_MARKER (MAKE_MARKER('W', 'A', 'V', 'E'))
#define fmt_MARKER (MAKE_MARKER('f', 'm', 't', ' '))
#define fact_MARKER (MAKE_MARKER('f', 'a', 'c', 't'))

#define cue_MARKER (MAKE_MARKER('c', 'u', 'e', ' '))
#define slnt_MARKER (MAKE_MARKER('s', 'l', 'n', 't'))
#define wavl_MARKER (MAKE_MARKER('w', 'a', 'v', 'l'))
#define plst_MARKER (MAKE_MARKER('p', 'l', 's', 't'))
#define smpl_MARKER (MAKE_MARKER('s', 'm', 'p', 'l'))
#define iXML_MARKER (MAKE_MARKER('i', 'X', 'M', 'L'))
#define levl_MARKER (MAKE_MARKER('l', 'e', 'v', 'l'))
#define MEXT_MARKER (MAKE_MARKER('M', 'E', 'X', 'T'))
#define acid_MARKER (MAKE_MARKER('a', 'c', 'i', 'd'))
#define strc_MARKER (MAKE_MARKER('s', 't', 'r', 'c'))
#define afsp_MARKER (MAKE_MARKER('a', 'f', 's', 'p'))
#define clm_MARKER (MAKE_MARKER('c', 'l', 'm', ' '))
#define elmo_MARKER (MAKE_MARKER('e', 'l', 'm', 'o'))
#define FLLR_MARKER (MAKE_MARKER('F', 'L', 'L', 'R'))

#define minf_MARKER (MAKE_MARKER('m', 'i', 'n', 'f'))
#define elm1_MARKER (MAKE_MARKER('e', 'l', 'm', '1'))
#define regn_MARKER (MAKE_MARKER('r', 'e', 'g', 'n'))
#define ovwf_MARKER (MAKE_MARKER('o', 'v', 'w', 'f'))
#define umid_MARKER (MAKE_MARKER('u', 'm', 'i', 'd'))
#define SyLp_MARKER (MAKE_MARKER('S', 'y', 'L', 'p'))
#define Cr8r_MARKER (MAKE_MARKER('C', 'r', '8', 'r'))
#define JUNK_MARKER (MAKE_MARKER('J', 'U', 'N', 'K'))
#define PMX_MARKER (MAKE_MARKER('_', 'P', 'M', 'X'))
#define inst_MARKER (MAKE_MARKER('i', 'n', 's', 't'))
#define AFAn_MARKER (MAKE_MARKER('A', 'F', 'A', 'n'))

/* Weird WAVPACK marker which can show up at the start of the DATA section. */
#define wvpk_MARKER (MAKE_MARKER('w', 'v', 'p', 'k'))
#define OggS_MARKER (MAKE_MARKER('O', 'g', 'g', 'S'))

#define WAVLIKE_PEAK_CHUNK_SIZE(ch) (2 * sizeof(int) + ch * (sizeof(float) + sizeof(int)))

enum
{
    HAVE_RIFF = 0x01,
    HAVE_WAVE = 0x02,
    HAVE_fmt = 0x04,
    HAVE_fact = 0x08,
    HAVE_PEAK = 0x10,
    HAVE_data = 0x20,
    HAVE_other = 0x80000000
};

/* known WAVEFORMATEXTENSIBLE GUIDS */
static const EXT_SUBFORMAT MSGUID_SUBTYPE_PCM = {
    0x00000001, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

#if 0
static const EXT_SUBFORMAT MSGUID_SUBTYPE_MS_ADPCM =
{
	0x00000002, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 }
};
#endif

static const EXT_SUBFORMAT MSGUID_SUBTYPE_IEEE_FLOAT = {
    0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_ALAW = {
    0x00000006, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_MULAW = {
    0x00000007, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};

/*
 * the next two are from
 * http://dream.cs.bath.ac.uk/researchdev/wave-ex/bformat.html
 */
static const EXT_SUBFORMAT MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_PCM = {
    0x00000001, 0x0721, 0x11d3, {0x86, 0x44, 0xC8, 0xC1, 0xCA, 0x00, 0x00, 0x00}};

static const EXT_SUBFORMAT MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT = {
    0x00000003, 0x0721, 0x11d3, {0x86, 0x44, 0xC8, 0xC1, 0xCA, 0x00, 0x00, 0x00}};

#if 0
/* maybe interesting one day to read the following through sf_read_raw */
/* http://www.bath.ac.uk/~masrwd/pvocex/pvocex.html */
static const EXT_SUBFORMAT MSGUID_SUBTYPE_PVOCEX =
{
	0x8312B9C2, 0x2E6E, 0x11d4, { 0xA8, 0x24, 0xDE, 0x5B, 0x96, 0xC3, 0xAB, 0x21 }
};
#endif

static int wav_read_header(SF_PRIVATE *psf, int *blockalign, int *framesperblock);
static int wav_write_header(SF_PRIVATE *psf, int calc_length);

static int wav_write_tailer(SF_PRIVATE *psf);
static size_t wav_command(SF_PRIVATE *psf, int command, void *data, size_t datasize);
static int wav_close(SF_PRIVATE *psf);

static int wav_read_smpl_chunk(SF_PRIVATE *psf, uint32_t chunklen);
static int wav_read_acid_chunk(SF_PRIVATE *psf, uint32_t chunklen);

static int wav_set_chunk(SF_PRIVATE *psf, const SF_CHUNK_INFO *chunk_info);
static SF_CHUNK_ITERATOR *wav_next_chunk_iterator(SF_PRIVATE *psf, SF_CHUNK_ITERATOR *iterator);
static int wav_get_chunk_size(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator,
                              SF_CHUNK_INFO *chunk_info);
static int wav_get_chunk_data(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator,
                              SF_CHUNK_INFO *chunk_info);

int wav_open(SF_PRIVATE *psf)
{
    WAVLIKE_PRIVATE *wpriv;
    int format, subformat, error, blockalign = 0, framesperblock = 0;

    if ((wpriv = (WAVLIKE_PRIVATE *)calloc(1, sizeof(WAVLIKE_PRIVATE))) == NULL)
        return SFE_MALLOC_FAILED;
    psf->m_container_data = wpriv;

    wpriv->wavex_ambisonic = SF_AMBISONIC_NONE;
    psf->m_strings.flags = SF_STR_ALLOW_START | SF_STR_ALLOW_END;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = wav_read_header(psf, &blockalign, &framesperblock)))
            return error;

        psf->next_chunk_iterator = wav_next_chunk_iterator;
        psf->get_chunk_size = wav_get_chunk_size;
        psf->get_chunk_data = wav_get_chunk_data;
    };

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        wpriv->wavex_ambisonic = SF_AMBISONIC_NONE;

        format = SF_CONTAINER(psf->sf.format);
        if (format != SF_FORMAT_WAV && format != SF_FORMAT_WAVEX)
            return SFE_BAD_OPEN_FORMAT;

        psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

        /* RIFF WAVs are little-endian, RIFX WAVs are big-endian, default to little */
        psf->m_endian = SF_ENDIAN(psf->sf.format);
        if (CPU_IS_BIG_ENDIAN && psf->m_endian == SF_ENDIAN_CPU)
            psf->m_endian = SF_ENDIAN_BIG;
        else if (psf->m_endian != SF_ENDIAN_BIG)
            psf->m_endian = SF_ENDIAN_LITTLE;

        if (psf->m_mode != SFM_RDWR || psf->m_filelength < 44)
        {
            psf->m_filelength = 0;
            psf->m_datalength = 0;
            psf->m_dataoffset = 0;
            psf->sf.frames = 0;
        };

        if (subformat == SF_FORMAT_IMA_ADPCM || subformat == SF_FORMAT_MS_ADPCM)
        {
            blockalign = wavlike_srate2blocksize(psf->sf.samplerate * psf->sf.channels);
            framesperblock = -1; /* Corrected later. */
        };

        /*
		 * By default, add the peak chunk to floating point files. Default behaviour
		 * can be switched off using sf_command (SFC_SET_PEAK_CHUNK, SF_FALSE).
		 */
        if (psf->m_mode == SFM_WRITE &&
            (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE))
        {
            psf->m_peak_info = std::make_unique<PEAK_INFO>(psf->sf.channels);
        };

        psf->write_header = wav_write_header;
        psf->set_chunk = wav_set_chunk;
    };

    psf->container_close = wav_close;
    psf->on_command = wav_command;

    switch (subformat)
    {
    case SF_FORMAT_PCM_U8:
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

    case SF_FORMAT_G721_32:
        error = g72x_init(psf);
        break;

    case SF_FORMAT_NMS_ADPCM_16:
    case SF_FORMAT_NMS_ADPCM_24:
    case SF_FORMAT_NMS_ADPCM_32:
        error = nms_adpcm_init(psf);
        break;

    case SF_FORMAT_GSM610:
        error = gsm610_init(psf);
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    if (psf->m_mode == SFM_WRITE || (psf->m_mode == SFM_RDWR && psf->m_filelength == 0))
        return psf->write_header(psf, SF_FALSE);

    return error;
}

static int wav_read_header(SF_PRIVATE *psf, int *blockalign, int *framesperblock)
{
    WAVLIKE_PRIVATE *wpriv;
    WAV_FMT *wav_fmt;
    FACT_CHUNK fact_chunk;
    uint32_t marker, chunk_size = 0, RIFFsize = 0, done = 0;
    int parsestage = 0, error, format = 0;

    if (psf->m_filelength > INT64_C(0xffffffff))
        psf->log_printf("Warning : filelength > 0xffffffff. This is bad!!!!\n");

    if ((wpriv = (WAVLIKE_PRIVATE *)psf->m_container_data) == NULL)
        return SFE_INTERNAL;
    wav_fmt = &wpriv->wav_fmt;

    /* Set position to start of file to begin reading header. */
	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("m", &marker);
	psf->binheader_seekf(-4, SF_SEEK_CUR);
    psf->m_header.indx = 0;

    /*
	 * RIFX signifies big-endian format for all header and data  to prevent
	 * lots of code copying here, we'll set the psf->rwf_endian flag once here,
	 * and never specify endian-ness for all other header ops/
	 */
    psf->m_rwf_endian = (marker == RIFF_MARKER) ? SF_ENDIAN_LITTLE : SF_ENDIAN_BIG;

    while (!done)
    {
        size_t jump = chunk_size & 1;

        marker = chunk_size = 0;
		psf->binheader_seekf(jump, SF_SEEK_CUR);
        psf->binheader_readf("m4", &marker, &chunk_size);
        if (marker == 0)
        {
            sf_count_t pos = psf->ftell();
            psf->log_printf("Have 0 marker at position %D (0x%x).\n", pos, pos);
            break;
        };

        psf_store_read_chunk_u32(&psf->m_rchunks, marker, psf->ftell(), chunk_size);

        switch (marker)
        {
        case RIFF_MARKER:
        case RIFX_MARKER:
            if (parsestage)
                return SFE_WAV_NO_RIFF;

            parsestage |= HAVE_RIFF;

            RIFFsize = chunk_size;

            if (psf->m_filelength < RIFFsize + 2 * SIGNED_SIZEOF(marker))
            {
                if (marker == RIFF_MARKER)
                    psf->log_printf("RIFF : %u (should be %D)\n", RIFFsize,
                                   psf->m_filelength - 2 * SIGNED_SIZEOF(marker));
                else
                    psf->log_printf("RIFX : %u (should be %D)\n", RIFFsize,
                                   psf->m_filelength - 2 * SIGNED_SIZEOF(marker));

                RIFFsize = psf->m_filelength - 2 * SIGNED_SIZEOF(RIFFsize);
            }
            else
            {
                if (marker == RIFF_MARKER)
                    psf->log_printf("RIFF : %u\n", RIFFsize);
                else
                    psf->log_printf("RIFX : %u\n", RIFFsize);
            };

            psf->binheader_readf("m", &marker);
            if (marker != WAVE_MARKER)
                return SFE_WAV_NO_WAVE;
            parsestage |= HAVE_WAVE;
            psf->log_printf("WAVE\n");
            chunk_size = 0;
            break;

        case fmt_MARKER:
            if ((parsestage & (HAVE_RIFF | HAVE_WAVE)) != (HAVE_RIFF | HAVE_WAVE))
                return SFE_WAV_NO_FMT;

            /* If this file has a SECOND fmt chunk, I don't want to know about it. */
            if (parsestage & HAVE_fmt)
                break;

            parsestage |= HAVE_fmt;

            psf->log_printf("fmt  : %d\n", chunk_size);

            if ((error = wavlike_read_fmt_chunk(psf, chunk_size)))
                return error;

            format = wav_fmt->format;
            break;

        case data_MARKER:
            if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) !=
                (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
                return SFE_WAV_NO_DATA;

            if (psf->m_mode == SFM_RDWR && (parsestage & HAVE_other) != 0)
                return SFE_RDWR_BAD_HEADER;

            parsestage |= HAVE_data;

            psf->m_datalength = chunk_size;
            if (psf->m_datalength & 1)
                psf->log_printf("*** 'data' chunk should be an even number "
                                    "of bytes in length.\n");

            psf->m_dataoffset = psf->ftell();

            if (psf->m_dataoffset > 0)
            {
                if (chunk_size == 0 && RIFFsize == 8 && psf->m_filelength > 44)
                {
                    psf->log_printf("*** Looks like a WAV file which "
                                        "wasn't closed properly. Fixing it.\n");
                    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
                };

                if (psf->m_datalength > psf->m_filelength - psf->m_dataoffset)
                {
                    psf->log_printf("data : %D (should be %D)\n", psf->m_datalength,
                                   psf->m_filelength - psf->m_dataoffset);
                    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
                }
                else
                    psf->log_printf("data : %D\n", psf->m_datalength);

                /* Only set dataend if there really is data at the end. */
                if (psf->m_datalength + psf->m_dataoffset < psf->m_filelength)
                    psf->m_dataend = psf->m_datalength + psf->m_dataoffset;

                psf->m_datalength += chunk_size & 1;
                chunk_size = 0;
            };

            if (!psf->sf.seekable || psf->m_dataoffset < 0)
                break;

            /* Seek past data and continue reading header. */
            psf->fseek(psf->m_datalength, SEEK_CUR);

            if (psf->ftell() != psf->m_datalength + psf->m_dataoffset)
                psf->log_printf("*** psf_fseek past end error ***\n");
            break;

        case fact_MARKER:
            if ((parsestage & (HAVE_RIFF | HAVE_WAVE)) != (HAVE_RIFF | HAVE_WAVE))
                return SFE_WAV_BAD_FACT;

            parsestage |= HAVE_fact;

            if ((parsestage & HAVE_fmt) != HAVE_fmt)
                psf->log_printf("*** Should have 'fmt ' chunk before 'fact'\n");

            psf->binheader_readf("4", &(fact_chunk.frames));

            if (chunk_size > SIGNED_SIZEOF(fact_chunk))
				psf->binheader_seekf(chunk_size - sizeof(fact_chunk), SF_SEEK_CUR);

            if (chunk_size)
                psf->log_printf("%M : %u\n", marker, chunk_size);
            else
                psf->log_printf("%M : %u (should not be zero)\n", marker, chunk_size);

            psf->log_printf("  frames  : %d\n", fact_chunk.frames);
            break;

        case PEAK_MARKER:
            if ((parsestage & (HAVE_RIFF | HAVE_WAVE | HAVE_fmt)) !=
                (HAVE_RIFF | HAVE_WAVE | HAVE_fmt))
                return SFE_WAV_PEAK_B4_FMT;

            parsestage |= HAVE_PEAK;

            psf->log_printf("%M : %u\n", marker, chunk_size);
            if ((error = wavlike_read_peak_chunk(psf, chunk_size)) != 0)
                return error;
            psf->m_peak_info->peak_loc =
                ((parsestage & HAVE_data) == 0) ? SF_PEAK_START : SF_PEAK_END;
            break;

        case cue_MARKER:
            parsestage |= HAVE_other;

            {
                uint32_t thisread, bytesread, cue_count, position, offset;
                int id, chunk_id, chunk_start, block_start, cue_index;

                bytesread = psf->binheader_readf("4", &cue_count);
                psf->log_printf("%M : %u\n", marker, chunk_size);

                if (cue_count > 1000)
                {
                    psf->log_printf("  Count : %u (skipping)\n", cue_count);
					psf->binheader_seekf((cue_count > 20 ? 20 : cue_count) * 24, SF_SEEK_CUR);
                    break;
                };

                psf->log_printf("  Count : %d\n", cue_count);

				psf->m_cues.resize(static_cast<size_t>(cue_count));
                cue_index = 0;

                while (cue_count)
                {
                    if ((thisread = psf->binheader_readf("e44m444", &id, &position, &chunk_id,
                                                        &chunk_start, &block_start, &offset)) == 0)
                        break;
                    bytesread += thisread;

                    psf->log_printf(
                                   "   Cue ID : %2d"
                                   "  Pos : %5u  Chunk : %M"
                                   "  Chk Start : %d  Blk Start : %d"
                                   "  Offset : %5d\n",
                                   id, position, chunk_id, chunk_start, block_start, offset);
					psf->m_cues[cue_index].indx = id;
					psf->m_cues[cue_index].position = position;
					psf->m_cues[cue_index].fcc_chunk = chunk_id;
					psf->m_cues[cue_index].chunk_start = chunk_start;
					psf->m_cues[cue_index].block_start = block_start;
					psf->m_cues[cue_index].sample_offset = offset;
					psf->m_cues[cue_index].name[0] = '\0';
                    cue_count--;
                    cue_index++;
                };

                if (bytesread != chunk_size)
                {
                    psf->log_printf("**** Chunk size weirdness (%d != %d)\n", chunk_size,
                                   bytesread);
					psf->binheader_seekf(chunk_size - bytesread, SF_SEEK_CUR);
                };
            };
            break;

        case smpl_MARKER:
            parsestage |= HAVE_other;

            psf->log_printf("smpl : %u\n", chunk_size);

            if ((error = wav_read_smpl_chunk(psf, chunk_size)))
                return error;
            break;

        case acid_MARKER:
            parsestage |= HAVE_other;

            psf->log_printf("acid : %u\n", chunk_size);

            if ((error = wav_read_acid_chunk(psf, chunk_size)))
                return error;
            break;

        case INFO_MARKER:
        case LIST_MARKER:
            parsestage |= HAVE_other;

            if ((error = wavlike_subchunk_parse(psf, marker, chunk_size)) != 0)
                return error;
            break;

        case PAD_MARKER:
            /*
			 * We can eat into a 'PAD ' chunk if we need to.
			 * parsestage |= HAVE_other ;
			 */
            psf->log_printf("%M : %u\n", marker, chunk_size);
			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        case cart_MARKER:
        case iXML_MARKER: /* See http://en.wikipedia.org/wiki/IXML */
        case strc_MARKER: /* Multiple of 32 bytes. */
        case afsp_MARKER:
        case clm_MARKER:
        case elmo_MARKER:
        case levl_MARKER:
        case plst_MARKER:
        case minf_MARKER:
        case elm1_MARKER:
        case regn_MARKER:
        case ovwf_MARKER:
        case inst_MARKER:
        case AFAn_MARKER:
        case umid_MARKER:
        case SyLp_MARKER:
        case Cr8r_MARKER:
        case JUNK_MARKER:
        case PMX_MARKER:
        case DISP_MARKER:
        case MEXT_MARKER:
        case FLLR_MARKER:
        case bext_MARKER:
            psf->log_printf("%M : %u\n", marker, chunk_size);
			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        default:
            if (chunk_size >= 0xffff0000)
            {
                done = SF_TRUE;
                psf->log_printf(
                               "*** Unknown chunk marker (%X) at position "
                               "%D with length %u. Exiting parser.\n",
                               marker, psf->ftell() - 8, chunk_size);
                break;
            };

            if (psf_isprint((marker >> 24) & 0xFF) && psf_isprint((marker >> 16) & 0xFF) &&
                psf_isprint((marker >> 8) & 0xFF) && psf_isprint(marker & 0xFF))
            {
                psf->log_printf("*** %M : %u (unknown marker)\n", marker, chunk_size);
				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
                break;
            };
            if (psf->ftell() & 0x03)
            {
                psf->log_printf("  Unknown chunk marker at position %D. Resynching.\n",
                               psf->ftell() - 8);
				psf->binheader_seekf(-3, SF_SEEK_CUR);
                /* File is too messed up so we prevent editing in RDWR mode here. */
                parsestage |= HAVE_other;
                break;
            };
            psf->log_printf(
                           "*** Unknown chunk marker (%X) at position %D. "
                           "Exiting parser.\n",
                           marker, psf->ftell() - 8);
            done = SF_TRUE;
            break;
        };

        if (chunk_size >= psf->m_filelength)
        {
            psf->log_printf("*** Chunk size %u > file length %D. Exiting parser.\n", chunk_size,
                           psf->m_filelength);
            break;
        };

        if (!psf->sf.seekable && (parsestage & HAVE_data))
            break;

        if (psf->ftell() >= psf->m_filelength - SIGNED_SIZEOF(chunk_size))
        {
            psf->log_printf("End\n");
            break;
        };
    }; /* while (1) */

    if (psf->m_dataoffset <= 0)
        return SFE_WAV_NO_DATA;

    if (psf->sf.channels < 1)
        return SFE_CHANNEL_COUNT_ZERO;

    if (psf->sf.channels > SF_MAX_CHANNELS)
        return SFE_CHANNEL_COUNT;

    if (format != WAVE_FORMAT_PCM && (parsestage & HAVE_fact) == 0)
        psf->log_printf("**** All non-PCM format files should have a 'fact' chunk.\n");

    /* WAVs can be little or big endian */
    psf->m_endian = psf->m_rwf_endian;

    psf->fseek(psf->m_dataoffset, SEEK_SET);

    /*
     * Check for 'wvpk' at the start of the DATA section. Not able to
     * handle this.
     */
    psf->binheader_readf("4", &marker);
    if (marker == wvpk_MARKER || marker == OggS_MARKER)
        return SFE_WAV_WVPK_DATA;

    /* Seek to start of DATA section. */
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
    case WAVE_FORMAT_EXTENSIBLE:
        if (psf->sf.format == (SF_FORMAT_WAVEX | SF_FORMAT_MS_ADPCM))
        {
            *blockalign = wav_fmt->msadpcm.blockalign;
            *framesperblock = wav_fmt->msadpcm.samplesperblock;
        };
        break;

    case WAVE_FORMAT_NMS_VBXADPCM:
        *blockalign = wav_fmt->min.blockalign;
        *framesperblock = 160;
        switch (wav_fmt->min.bitwidth)
        {
        case 2:
            psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_NMS_ADPCM_16;
            break;
        case 3:
            psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_NMS_ADPCM_24;
            break;
        case 4:
            psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_NMS_ADPCM_32;
            break;

        default:
            return SFE_UNIMPLEMENTED;
        }
        break;

    case WAVE_FORMAT_PCM:
        psf->sf.format = SF_FORMAT_WAV | u_bitwidth_to_subformat(psf->m_bytewidth * 8);
        break;

    case WAVE_FORMAT_MULAW:
    case IBM_FORMAT_MULAW:
        psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_ULAW);
        break;

    case WAVE_FORMAT_ALAW:
    case IBM_FORMAT_ALAW:
        psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_ALAW);
        break;

    case WAVE_FORMAT_MS_ADPCM:
        psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_MS_ADPCM);
        *blockalign = wav_fmt->msadpcm.blockalign;
        *framesperblock = wav_fmt->msadpcm.samplesperblock;
        break;

    case WAVE_FORMAT_IMA_ADPCM:
        psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_IMA_ADPCM);
        *blockalign = wav_fmt->ima.blockalign;
        *framesperblock = wav_fmt->ima.samplesperblock;
        break;

    case WAVE_FORMAT_GSM610:
        psf->sf.format = (SF_FORMAT_WAV | SF_FORMAT_GSM610);
        break;

    case WAVE_FORMAT_IEEE_FLOAT:
        psf->sf.format = SF_FORMAT_WAV;
        psf->sf.format |= (psf->m_bytewidth == 8) ? SF_FORMAT_DOUBLE : SF_FORMAT_FLOAT;
        break;

    case WAVE_FORMAT_G721_ADPCM:
        psf->sf.format = SF_FORMAT_WAV | SF_FORMAT_G721_32;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    if (wpriv->fmt_is_broken)
        wavlike_analyze(psf);

    /* Only set the format endian-ness if its non-standard big-endian. */
    if (psf->m_endian == SF_ENDIAN_BIG)
        psf->sf.format |= SF_ENDIAN_BIG;

    return 0;
}

static int wav_write_fmt_chunk(SF_PRIVATE *psf)
{
    int subformat, fmt_size, add_fact_chunk = 0;

    subformat = SF_CODEC(psf->sf.format);

    switch (subformat)
    {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("4224", BHW4(fmt_size), BHW2(WAVE_FORMAT_PCM),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth */
        psf->binheader_writef("22", BHW2(psf->m_bytewidth * psf->sf.channels),
                             BHW2(psf->m_bytewidth * 8));
        break;

    case SF_FORMAT_FLOAT:
    case SF_FORMAT_DOUBLE:
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("4224", BHW4(fmt_size), BHW2(WAVE_FORMAT_IEEE_FLOAT),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth */
        psf->binheader_writef("22", BHW2(psf->m_bytewidth * psf->sf.channels),
                             BHW2(psf->m_bytewidth * 8));

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_ULAW:
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("4224", BHW4(fmt_size), BHW2(WAVE_FORMAT_MULAW),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth, extrabytes */
        psf->binheader_writef("222", BHW2(psf->m_bytewidth * psf->sf.channels), BHW2(8), BHW2(0));

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_ALAW:
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("4224", BHW4(fmt_size), BHW2(WAVE_FORMAT_ALAW),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth, extrabytes */
        psf->binheader_writef("222", BHW2(psf->m_bytewidth * psf->sf.channels), BHW2(8), BHW2(0));

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_IMA_ADPCM:
    {
        int blockalign, framesperblock, bytespersec;

        blockalign = wavlike_srate2blocksize(psf->sf.samplerate * psf->sf.channels);
        framesperblock = 2 * (blockalign - 4 * psf->sf.channels) / psf->sf.channels + 1;
        bytespersec = (psf->sf.samplerate * blockalign) / framesperblock;

        /* fmt chunk. */
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2;

        /* fmt : size, WAV format type, channels, samplerate, bytespersec */
        psf->binheader_writef("42244", BHW4(fmt_size), BHW2(WAVE_FORMAT_IMA_ADPCM),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate), BHW4(bytespersec));

        /* fmt : blockalign, bitwidth, extrabytes, framesperblock. */
        psf->binheader_writef("2222", BHW2(blockalign), BHW2(4), BHW2(2), BHW2(framesperblock));
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
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + extrabytes;

        /* fmt : size, WAV format type, channels. */
        psf->binheader_writef("422", BHW4(fmt_size), BHW2(WAVE_FORMAT_MS_ADPCM),
                             BHW2(psf->sf.channels));

        /* fmt : samplerate, bytespersec. */
        psf->binheader_writef("44", BHW4(psf->sf.samplerate), BHW4(bytespersec));

        /* fmt : blockalign, bitwidth, extrabytes, framesperblock. */
        psf->binheader_writef("22222", BHW2(blockalign), BHW2(4), BHW2(extrabytes),
                             BHW2(framesperblock), BHW2(7));

        wavlike_msadpcm_write_adapt_coeffs(psf);
    };

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_G721_32:
        /* fmt chunk. */
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2;

        /* fmt : size, WAV format type, channels, samplerate, bytespersec */
        psf->binheader_writef("42244", BHW4(fmt_size), BHW2(WAVE_FORMAT_G721_ADPCM),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate),
                             BHW4(psf->sf.samplerate * psf->sf.channels / 2));

        /* fmt : blockalign, bitwidth, extrabytes, auxblocksize. */
        psf->binheader_writef("2222", BHW2(64), BHW2(4), BHW2(2), BHW2(0));

        add_fact_chunk = SF_TRUE;
        break;

    case SF_FORMAT_NMS_ADPCM_16:
    case SF_FORMAT_NMS_ADPCM_24:
    case SF_FORMAT_NMS_ADPCM_32:
    {
        int bytespersec, blockalign, bitwidth;

        bitwidth = subformat == SF_FORMAT_NMS_ADPCM_16 ? 2 : subformat == SF_FORMAT_NMS_ADPCM_24 ? 3 : 4;
        blockalign = 20 * bitwidth + 2;
        bytespersec = psf->sf.samplerate * blockalign / 160;

        /* fmt chunk. */
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("4224", BHW4(fmt_size), BHW2(WAVE_FORMAT_NMS_VBXADPCM), BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec, blockalign, bitwidth */
        psf->binheader_writef("422", BHW4(bytespersec), BHW2(blockalign), BHW2(bitwidth));

        add_fact_chunk = SF_TRUE;
        break;
    }

    case SF_FORMAT_GSM610:
    {
        int blockalign, framesperblock, bytespersec;

        blockalign = WAVLIKE_GSM610_BLOCKSIZE;
        framesperblock = WAVLIKE_GSM610_SAMPLES;
        bytespersec = (psf->sf.samplerate * blockalign) / framesperblock;

        /* fmt chunk. */
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2;

        /* fmt : size, WAV format type, channels. */
        psf->binheader_writef("422", BHW4(fmt_size), BHW2(WAVE_FORMAT_GSM610),
                             BHW2(psf->sf.channels));

        /* fmt : samplerate, bytespersec. */
        psf->binheader_writef("44", BHW4(psf->sf.samplerate), BHW4(bytespersec));

        /* fmt : blockalign, bitwidth, extrabytes, framesperblock. */
        psf->binheader_writef("2222", BHW2(blockalign), BHW2(0), BHW2(2), BHW2(framesperblock));
    };

        add_fact_chunk = SF_TRUE;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    if (add_fact_chunk)
        psf->binheader_writef("tm48", BHWm(fact_MARKER), BHW4(4), BHW8(psf->sf.frames));

    return 0;
}

static int wavex_write_fmt_chunk(SF_PRIVATE *psf)
{
    WAVLIKE_PRIVATE *wpriv;
    int subformat, fmt_size;

    if ((wpriv = (WAVLIKE_PRIVATE *)psf->m_container_data) == NULL)
        return SFE_INTERNAL;

    subformat = SF_CODEC(psf->sf.format);

    /* initial section (same for all, it appears) */
    switch (subformat)
    {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
    case SF_FORMAT_FLOAT:
    case SF_FORMAT_DOUBLE:
    case SF_FORMAT_ULAW:
    case SF_FORMAT_ALAW:
        fmt_size = 2 + 2 + 4 + 4 + 2 + 2 + 2 + 2 + 4 + 4 + 2 + 2 + 8;

        /* fmt : format, channels, samplerate */
        psf->binheader_writef("4224", BHW4(fmt_size), BHW2(WAVE_FORMAT_EXTENSIBLE),
                             BHW2(psf->sf.channels), BHW4(psf->sf.samplerate));
        /*  fmt : bytespersec */
        psf->binheader_writef("4",
                             BHW4(psf->sf.samplerate * psf->m_bytewidth * psf->sf.channels));
        /*  fmt : blockalign, bitwidth */
        psf->binheader_writef("22", BHW2(psf->m_bytewidth * psf->sf.channels),
                             BHW2(psf->m_bytewidth * 8));

        /* cbSize 22 is sizeof (WAVEFORMATEXTENSIBLE) - sizeof (WAVEFORMATEX) */
        psf->binheader_writef("2", BHW2(22));

        /* wValidBitsPerSample, for our use same as bitwidth as we use it fully */
        psf->binheader_writef("2", BHW2(psf->m_bytewidth * 8));

        /*
		 * For an Ambisonic file set the channel mask to zero.
		 * Otherwise use a default based on the channel count.
		 */
        if (wpriv->wavex_ambisonic != SF_AMBISONIC_NONE)
            psf->binheader_writef("4", BHW4(0));
        else if (wpriv->wavex_channelmask != 0)
            psf->binheader_writef("4", BHW4(wpriv->wavex_channelmask));
        else
        {
            /*
			 * Ok some liberty is taken here to use the most commonly used channel masks
			 * instead of "no mapping". If you really want to use "no mapping" for 8 channels and less
			 * please don't use wavex. (otherwise we'll have to create a new SF_COMMAND)
			 */
            switch (psf->sf.channels)
            {
            case 1: /* center channel mono */
                psf->binheader_writef("4", BHW4(0x4));
                break;

            case 2: /* front left and right */
                psf->binheader_writef("4", BHW4(0x1 | 0x2));
                break;

            case 4: /* Quad */
                psf->binheader_writef("4", BHW4(0x1 | 0x2 | 0x10 | 0x20));
                break;

            case 6: /* 5.1 */
                psf->binheader_writef("4", BHW4(0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x20));
                break;

            case 8: /* 7.1 */
                psf->binheader_writef("4",
                                     BHW4(0x1 | 0x2 | 0x4 | 0x8 | 0x10 | 0x20 | 0x40 | 0x80));
                break;

            default: /* 0 when in doubt , use direct out, ie NO mapping*/
                psf->binheader_writef("4", BHW4(0x0));
                break;
            };
        };
        break;

    case SF_FORMAT_MS_ADPCM: /* Todo, GUID exists might have different header as per wav_write_header */
    default:
        return SFE_UNIMPLEMENTED;
    };

    /* GUID section, different for each */

    switch (subformat)
    {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
        wavlike_write_guid(psf, wpriv->wavex_ambisonic == SF_AMBISONIC_NONE
                                    ? &MSGUID_SUBTYPE_PCM
                                    : &MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_PCM);
        break;

    case SF_FORMAT_FLOAT:
    case SF_FORMAT_DOUBLE:
        wavlike_write_guid(psf, wpriv->wavex_ambisonic == SF_AMBISONIC_NONE
                                    ? &MSGUID_SUBTYPE_IEEE_FLOAT
                                    : &MSGUID_SUBTYPE_AMBISONIC_B_FORMAT_IEEE_FLOAT);
        break;

    case SF_FORMAT_ULAW:
        wavlike_write_guid(psf, &MSGUID_SUBTYPE_MULAW);
        break;

    case SF_FORMAT_ALAW:
        wavlike_write_guid(psf, &MSGUID_SUBTYPE_ALAW);
        break;

#if 0
	/* This is dead code due to return in previous switch statement. */
	case SF_FORMAT_MS_ADPCM: /* todo, GUID exists */
		wavlike_write_guid(psf, &MSGUID_SUBTYPE_MS_ADPCM);
		break;
		return SFE_UNIMPLEMENTED;
#endif

    default:
        return SFE_UNIMPLEMENTED;
    };

    psf->binheader_writef("tm48", BHWm(fact_MARKER), BHW4(4), BHW8(psf->sf.frames));

    return 0;
}

static int wav_write_header(SF_PRIVATE *psf, int calc_length)
{
    sf_count_t current;
    int error, has_data = SF_FALSE;

    current = psf->ftell();

    if (current > psf->m_dataoffset)
        has_data = SF_TRUE;

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

        if (psf->m_dataend)
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;
        else if (psf->m_bytewidth > 0 && psf->sf.seekable == SF_TRUE)
            psf->m_datalength = psf->sf.frames * psf->m_bytewidth * psf->sf.channels;
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    /*
	 * RIFX signifies big-endian format for all header and data.
	 * To prevent lots of code copying here, we'll set the psf->rwf_endian flag
	 * once here, and never specify endian-ness for all other header operations.
	 */

    /* RIFF/RIFX marker, length, WAVE and 'fmt ' markers. */

    if (psf->m_endian == SF_ENDIAN_LITTLE)
        psf->binheader_writef("etm8", BHWm(RIFF_MARKER),
                             BHW8((psf->m_filelength < 8) ? 8 : psf->m_filelength - 8));
    else
        psf->binheader_writef("Etm8", BHWm(RIFX_MARKER),
                             BHW8((psf->m_filelength < 8) ? 8 : psf->m_filelength - 8));

    /* WAVE and 'fmt ' markers. */
    psf->binheader_writef("mm", BHWm(WAVE_MARKER), BHWm(fmt_MARKER));

    /* Write the 'fmt ' chunk. */
    switch (SF_CONTAINER(psf->sf.format))
    {
    case SF_FORMAT_WAV:
        if ((error = wav_write_fmt_chunk(psf)) != 0)
            return error;
        break;

    case SF_FORMAT_WAVEX:
        if ((error = wavex_write_fmt_chunk(psf)) != 0)
            return error;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    /* The LIST/INFO chunk. */
    if (psf->m_strings.flags & SF_STR_LOCATE_START)
        wavlike_write_strings(psf, SF_STR_LOCATE_START);

    if (psf->m_peak_info != NULL && psf->m_peak_info->peak_loc == SF_PEAK_START)
        wavlike_write_peak_chunk(psf);

    if (!psf->m_cues.empty())
    {
        psf->binheader_writef("em44", BHWm(cue_MARKER), BHW4(4 + psf->m_cues.size() * 6 * 4),
                             BHW4(psf->m_cues.size()));

		for (auto &cue: psf->m_cues) {
			psf->binheader_writef("e44m444", BHW4(cue.indx),
								 BHW4(cue.position),
								 BHWm(cue.fcc_chunk),
								 BHW4(cue.chunk_start),
								 BHW4(cue.block_start),
								 BHW4(cue.sample_offset));
		}
    };

    if (psf->m_instrument != NULL)
    {
        int tmp;
        double dtune = (double)(0x40000000) / 25.0;

        psf->binheader_writef("m4", BHWm(smpl_MARKER),
                             BHW4(9 * 4 + psf->m_instrument->loop_count * 6 * 4));
        psf->binheader_writef("44", BHW4(0), BHW4(0)); /* Manufacturer zero is everyone */
        tmp = (int)(1.0e9 / psf->sf.samplerate); /* Sample period in nano seconds */
        psf->binheader_writef("44", BHW4(tmp), BHW4(psf->m_instrument->basenote));
        tmp = (uint32_t)(psf->m_instrument->detune * dtune + 0.5);
        psf->binheader_writef("4", BHW4(tmp));
        psf->binheader_writef("44", BHW4(0), BHW4(0)); /* SMTPE format */
        psf->binheader_writef("44", BHW4(psf->m_instrument->loop_count), BHW4(0));

        for (tmp = 0; tmp < psf->m_instrument->loop_count; tmp++)
        {
            int type;

            type = psf->m_instrument->loops[tmp].mode;
            type = (type == SF_LOOP_FORWARD
                        ? 0
                        : type == SF_LOOP_BACKWARD ? 2 : type == SF_LOOP_ALTERNATING ? 1 : 32);

            psf->binheader_writef("44", BHW4(tmp), BHW4(type));
            psf->binheader_writef("44", BHW4(psf->m_instrument->loops[tmp].start),
                                 BHW4(psf->m_instrument->loops[tmp].end - 1));
            psf->binheader_writef("44", BHW4(0), BHW4(psf->m_instrument->loops[tmp].count));
        };
    };

    /* Write custom headers. */
    if (psf->m_wchunks.used > 0)
        wavlike_write_custom_chunks(psf);

    if (psf->m_header.indx + 16 < psf->m_dataoffset)
    {
        /* Add PAD data if necessary. */
        size_t k = psf->m_dataoffset - (psf->m_header.indx + 16);
        psf->binheader_writef("m4z", BHWm(PAD_MARKER), BHW4(k), BHWz(k));
    };

    psf->binheader_writef("tm8", BHWm(data_MARKER), BHW8(psf->m_datalength));
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);
    if (psf->m_error)
        return psf->m_error;

    if (has_data && psf->m_dataoffset != psf->m_header.indx)
    {
        psf->log_printf("Oooops : has_data && psf->dataoffset != psf->header.indx\n");
        return psf->m_error = SFE_INTERNAL;
    };

    psf->m_dataoffset = psf->m_header.indx;

    if (!has_data)
        psf->fseek(psf->m_dataoffset, SEEK_SET);
    else if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int wav_write_tailer(SF_PRIVATE *psf)
{
    /* Reset the current header buffer length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;

    if (psf->m_bytewidth > 0 && psf->sf.seekable == SF_TRUE)
    {
        psf->m_datalength = psf->sf.frames * psf->m_bytewidth * psf->sf.channels;
        psf->m_dataend = psf->m_dataoffset + psf->m_datalength;
    };

    if (psf->m_dataend > 0)
        psf->fseek(psf->m_dataend, SEEK_SET);
    else
        psf->m_dataend = psf->fseek(0, SEEK_END);

    if (psf->m_dataend & 1)
        psf->binheader_writef("z", BHWz(1));

    /* Add a PEAK chunk if requested. */
    if (psf->m_peak_info != NULL && psf->m_peak_info->peak_loc == SF_PEAK_END)
        wavlike_write_peak_chunk(psf);

    if (psf->m_strings.flags & SF_STR_LOCATE_END)
        wavlike_write_strings(psf, SF_STR_LOCATE_END);

    /* Write the tailer. */
    if (psf->m_header.indx > 0)
        psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    return 0;
}

static int wav_close(SF_PRIVATE *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        wav_write_tailer(psf);

        if (psf->m_mode == SFM_RDWR)
        {
            sf_count_t current = psf->ftell();

            /*
			**	If the mode is RDWR and the current position is less than the
			**	filelength, truncate the file.
			*/

            if (current < psf->m_filelength)
            {
                psf->ftruncate(current);
                psf->m_filelength = current;
            };
        };

        psf->write_header(psf, SF_TRUE);
    };

    return 0;
}

static size_t wav_command(SF_PRIVATE *psf, int command, void *UNUSED(data), size_t datasize)
{
    WAVLIKE_PRIVATE *wpriv;

    if ((wpriv = (WAVLIKE_PRIVATE *)psf->m_container_data) == NULL)
        return SFE_INTERNAL;

    switch (command)
    {
    case SFC_WAVEX_SET_AMBISONIC:
        if ((SF_CONTAINER(psf->sf.format)) == SF_FORMAT_WAVEX)
        {
            if (datasize == SF_AMBISONIC_NONE)
                wpriv->wavex_ambisonic = SF_AMBISONIC_NONE;
            else if (datasize == SF_AMBISONIC_B_FORMAT)
                wpriv->wavex_ambisonic = SF_AMBISONIC_B_FORMAT;
            else
                return 0;
        };
        return wpriv->wavex_ambisonic;

    case SFC_WAVEX_GET_AMBISONIC:
        return wpriv->wavex_ambisonic;

    case SFC_SET_CHANNEL_MAP_INFO:
        wpriv->wavex_channelmask = wavlike_gen_channel_mask(psf->m_channel_map.data(), psf->sf.channels);
        return (wpriv->wavex_channelmask != 0);

    default:
        break;
    };

    return 0;
}

static int wav_read_smpl_chunk(SF_PRIVATE *psf, uint32_t chunklen)
{
    char buffer[512];
    uint32_t thisread, bytesread = 0, dword, sampler_data, loop_count;
    uint32_t note, start, end, type = -1, count;
    int j, k;

    chunklen += (chunklen & 1);

    bytesread += psf->binheader_readf("4", &dword);
    psf->log_printf("  Manufacturer : %X\n", dword);

    bytesread += psf->binheader_readf("4", &dword);
    psf->log_printf("  Product      : %u\n", dword);

    bytesread += psf->binheader_readf("4", &dword);
    psf->log_printf("  Period       : %u nsec\n", dword);

    bytesread += psf->binheader_readf("4", &note);
    psf->log_printf("  Midi Note    : %u\n", note);

    bytesread += psf->binheader_readf("4", &dword);
    if (dword != 0)
    {
        snprintf(buffer, sizeof(buffer), "%f", (1.0 * 0x80000000) / ((uint32_t)dword));
        psf->log_printf("  Pitch Fract. : %s\n", buffer);
    }
    else
        psf->log_printf("  Pitch Fract. : 0\n");

    bytesread += psf->binheader_readf("4", &dword);
    psf->log_printf("  SMPTE Format : %u\n", dword);

    bytesread += psf->binheader_readf("4", &dword);
    snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d %02d", (dword >> 24) & 0x7F,
             (dword >> 16) & 0x7F, (dword >> 8) & 0x7F, dword & 0x7F);
    psf->log_printf("  SMPTE Offset : %s\n", buffer);

    bytesread += psf->binheader_readf("4", &loop_count);
    psf->log_printf("  Loop Count   : %u\n", loop_count);

    if (loop_count == 0 && chunklen == bytesread)
        return 0;

    /*
	 * Sampler Data holds the number of data bytes after the CUE chunks which
	 * is not actually CUE data. Display value after CUE data.
	 */
    bytesread += psf->binheader_readf("4", &sampler_data);

    if ((psf->m_instrument = psf_instrument_alloc()) == NULL)
        return SFE_MALLOC_FAILED;

    psf->m_instrument->loop_count = loop_count;

    for (j = 0; loop_count > 0 && chunklen - bytesread >= 24; j++)
    {
        if ((thisread = psf->binheader_readf("4", &dword)) == 0)
            break;
        bytesread += thisread;
        psf->log_printf("    Cue ID : %2u", dword);

        bytesread += psf->binheader_readf("4", &type);
        psf->log_printf("  Type : %2u", type);

        bytesread += psf->binheader_readf("4", &start);
        psf->log_printf("  Start : %5u", start);

        bytesread += psf->binheader_readf("4", &end);
        psf->log_printf("  End : %5u", end);

        bytesread += psf->binheader_readf("4", &dword);
        psf->log_printf("  Fraction : %5u", dword);

        bytesread += psf->binheader_readf("4", &count);
        psf->log_printf("  Count : %5u\n", count);

        if (j < ARRAY_LEN(psf->m_instrument->loops))
        {
            psf->m_instrument->loops[j].start = start;
            psf->m_instrument->loops[j].end = end + 1;
            psf->m_instrument->loops[j].count = count;

            switch (type)
            {
            case 0:
                psf->m_instrument->loops[j].mode = SF_LOOP_FORWARD;
                break;
            case 1:
                psf->m_instrument->loops[j].mode = SF_LOOP_ALTERNATING;
                break;
            case 2:
                psf->m_instrument->loops[j].mode = SF_LOOP_BACKWARD;
                break;
            default:
                psf->m_instrument->loops[j].mode = SF_LOOP_NONE;
                break;
            };
        };

        loop_count--;
    };

    if (chunklen - bytesread == 0)
    {
        if (sampler_data != 0)
            psf->log_printf("  Sampler Data : %u (should be 0)\n", sampler_data);
        else
            psf->log_printf("  Sampler Data : %u\n", sampler_data);
    }
    else
    {
        if (sampler_data != chunklen - bytesread)
        {
            psf->log_printf("  Sampler Data : %u (should have been %u)\n", sampler_data,
                           chunklen - bytesread);
            sampler_data = chunklen - bytesread;
        }
        else
        {
            psf->log_printf("  Sampler Data : %u\n", sampler_data);
        }

        psf->log_printf("      ");
        for (k = 0; k < (int)sampler_data; k++)
        {
            char ch;

            if (k > 0 && (k % 20) == 0)
                psf->log_printf("\n      ");

            if ((thisread = psf->binheader_readf("1", &ch)) == 0)
                break;
            bytesread += thisread;
            psf->log_printf("%02X ", ch & 0xFF);
        };

        psf->log_printf("\n");
    };

    psf->m_instrument->basenote = note;
    psf->m_instrument->gain = 1;
    psf->m_instrument->velocity_lo = psf->m_instrument->key_lo = 0;
    psf->m_instrument->velocity_hi = psf->m_instrument->key_hi = 127;

    return 0;
}

/*
 * The acid chunk goes a little something like this:
 *
 * 4 bytes          'acid'
 * 4 bytes (int)     length of chunk starting at next byte
 *
 * 4 bytes (int)     type of file:
 *        this appears to be a bit mask,however some combinations
 *        are probably impossible and/or qualified as "errors"
 *
 *        0x01 On: One Shot         Off: Loop
 *        0x02 On: Root note is Set Off: No root
 *        0x04 On: Stretch is On,   Off: Strech is OFF
 *        0x08 On: Disk Based       Off: Ram based
 *        0x10 On: ??????????       Off: ????????? (Acidizer puts that ON)
 *
 * 2 bytes (short)      root note
 *        if type 0x10 is OFF : [C,C#,(...),B] -> [0x30 to 0x3B]
 *        if type 0x10 is ON  : [C,C#,(...),B] -> [0x3C to 0x47]
 *         (both types fit on same MIDI pitch albeit different octaves, so who cares)
 *
 * 2 bytes (short)      ??? always set to 0x8000
 * 4 bytes (float)      ??? seems to be always 0
 * 4 bytes (int)        number of beats
 * 2 bytes (short)      meter denominator   //always 4 in SF/ACID
 * 2 bytes (short)      meter numerator     //always 4 in SF/ACID
 *                      //are we sure about the order?? usually its num/denom
 * 4 bytes (float)      tempo
 *
 */

static int wav_read_acid_chunk(SF_PRIVATE *psf, uint32_t chunklen)
{
    char buffer[512];
    uint32_t bytesread = 0;
    int beats, flags;
    short rootnote, q1, meter_denom, meter_numer;
    float q2, tempo;

    chunklen += (chunklen & 1);

    bytesread += psf->binheader_readf("422f", &flags, &rootnote, &q1, &q2);

    snprintf(buffer, sizeof(buffer), "%f", q2);

    psf->log_printf("  Flags     : 0x%04x (%s,%s,%s,%s,%s)\n", flags,
                   (flags & 0x01) ? "OneShot" : "Loop",
                   (flags & 0x02) ? "RootNoteValid" : "RootNoteInvalid",
                   (flags & 0x04) ? "StretchOn" : "StretchOff",
                   (flags & 0x08) ? "DiskBased" : "RAMBased", (flags & 0x10) ? "??On" : "??Off");

    psf->log_printf("  Root note : 0x%x\n  ????      : 0x%04x\n  ????      : %s\n", rootnote,
                   q1, buffer);

    bytesread += psf->binheader_readf("422f", &beats, &meter_denom, &meter_numer, &tempo);
    snprintf(buffer, sizeof(buffer), "%f", tempo);
    psf->log_printf("  Beats     : %d\n  Meter     : %d/%d\n  Tempo     : %s\n", beats,
                   meter_numer, meter_denom, buffer);

	psf->binheader_seekf(chunklen - bytesread, SF_SEEK_CUR);

    if ((psf->m_loop_info = (SF_LOOP_INFO *)calloc(1, sizeof(SF_LOOP_INFO))) == NULL)
        return SFE_MALLOC_FAILED;

    psf->m_loop_info->time_sig_num = meter_numer;
    psf->m_loop_info->time_sig_den = meter_denom;
    psf->m_loop_info->loop_mode = (flags & 0x01) ? SF_LOOP_NONE : SF_LOOP_FORWARD;
    psf->m_loop_info->num_beats = beats;
    psf->m_loop_info->bpm = tempo;
    psf->m_loop_info->root_key = (flags & 0x02) ? rootnote : -1;

    return 0;
}

static int wav_set_chunk(SF_PRIVATE *psf, const SF_CHUNK_INFO *chunk_info)
{
    return psf_save_write_chunk(&psf->m_wchunks, chunk_info);
}

static SF_CHUNK_ITERATOR *wav_next_chunk_iterator(SF_PRIVATE *psf, SF_CHUNK_ITERATOR *iterator)
{
    return psf_next_chunk_iterator(&psf->m_rchunks, iterator);
}

static int wav_get_chunk_size(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator,
                              SF_CHUNK_INFO *chunk_info)
{
    int indx;

    if ((indx = psf_find_read_chunk_iterator(&psf->m_rchunks, iterator)) < 0)
        return SFE_UNKNOWN_CHUNK;

    chunk_info->datalen = psf->m_rchunks.chunks[indx].len;

    return SFE_NO_ERROR;
}

static int wav_get_chunk_data(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator,
                              SF_CHUNK_INFO *chunk_info)
{
    int indx;
    sf_count_t pos;

    if ((indx = psf_find_read_chunk_iterator(&psf->m_rchunks, iterator)) < 0)
        return SFE_UNKNOWN_CHUNK;

    if (chunk_info->data == NULL)
        return SFE_BAD_CHUNK_DATA_PTR;

    chunk_info->id_size = psf->m_rchunks.chunks[indx].id_size;
    memcpy(chunk_info->id, psf->m_rchunks.chunks[indx].id,
           sizeof(chunk_info->id) / sizeof(*chunk_info->id));

    pos = psf->ftell();
    psf->fseek(psf->m_rchunks.chunks[indx].offset, SEEK_SET);
    psf->fread(chunk_info->data, std::min(chunk_info->datalen, psf->m_rchunks.chunks[indx].len), 1);
    psf->fseek(pos, SEEK_SET);

    return SFE_NO_ERROR;
}
