/*
** Copyright (C) 2005-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <math.h>

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"
#include "chanmap.h"

#include <algorithm>
#include <memory>

/*
 * Macros to handle big/little endian issues.
 */

#define aac_MARKER MAKE_MARKER('a', 'a', 'c', ' ')
#define alac_MARKER MAKE_MARKER('a', 'l', 'a', 'c')
#define alaw_MARKER MAKE_MARKER('a', 'l', 'a', 'w')
#define caff_MARKER MAKE_MARKER('c', 'a', 'f', 'f')
#define chan_MARKER MAKE_MARKER('c', 'h', 'a', 'n')
#define data_MARKER MAKE_MARKER('d', 'a', 't', 'a')
#define desc_MARKER MAKE_MARKER('d', 'e', 's', 'c')
#define edct_MARKER MAKE_MARKER('e', 'd', 'c', 't')
#define free_MARKER MAKE_MARKER('f', 'r', 'e', 'e')
#define ima4_MARKER MAKE_MARKER('i', 'm', 'a', '4')
#define info_MARKER MAKE_MARKER('i', 'n', 'f', 'o')
#define inst_MARKER MAKE_MARKER('i', 'n', 's', 't')
#define kuki_MARKER MAKE_MARKER('k', 'u', 'k', 'i')
#define lpcm_MARKER MAKE_MARKER('l', 'p', 'c', 'm')
#define mark_MARKER MAKE_MARKER('m', 'a', 'r', 'k')
#define midi_MARKER MAKE_MARKER('m', 'i', 'd', 'i')
#define mp1_MARKER MAKE_MARKER('.', 'm', 'p', '1')
#define mp2_MARKER MAKE_MARKER('.', 'm', 'p', '2')
#define mp3_MARKER MAKE_MARKER('.', 'm', 'p', '3')
#define ovvw_MARKER MAKE_MARKER('o', 'v', 'v', 'w')
#define pakt_MARKER MAKE_MARKER('p', 'a', 'k', 't')
#define peak_MARKER MAKE_MARKER('p', 'e', 'a', 'k')
#define regn_MARKER MAKE_MARKER('r', 'e', 'g', 'n')
#define strg_MARKER MAKE_MARKER('s', 't', 'r', 'g')
#define umid_MARKER MAKE_MARKER('u', 'm', 'i', 'd')
#define uuid_MARKER MAKE_MARKER('u', 'u', 'i', 'd')
#define ulaw_MARKER MAKE_MARKER('u', 'l', 'a', 'w')
#define MAC3_MARKER MAKE_MARKER('M', 'A', 'C', '3')
#define MAC6_MARKER MAKE_MARKER('M', 'A', 'C', '6')

#define CAF_PEAK_CHUNK_SIZE(ch) ((int)(sizeof(int) + ch * (sizeof(float) + 8)))

#define SFE_CAF_NOT_CAF (666)
#define SFE_CAF_NO_DESC (667)
#define SFE_CAF_BAD_PEAK (668)

struct DESC_CHUNK
{
    uint8_t srate[8];
    uint32_t fmt_id;
    uint32_t fmt_flags;
    uint32_t pkt_bytes;
    uint32_t frames_per_packet;
    uint32_t channels_per_frame;
    uint32_t bits_per_chan;
};

struct CAF_PRIVATE
{
    int chanmap_tag;

	struct ALAC_DECODER_INFO alac;
};

/*
 * Private static functions.
 */

static int caf_close(SndFile *psf);
static int caf_read_header(SndFile *psf);
static int caf_write_header(SndFile *psf, int calc_length);
static int caf_write_tailer(SndFile *psf);
static size_t caf_command(SndFile *psf, int command, void *data, size_t datasize);
static int caf_read_chanmap(SndFile *psf, sf_count_t chunk_size);
static int caf_read_strings(SndFile *psf, sf_count_t chunk_size);
static void caf_write_strings(SndFile *psf, int location);

static int caf_set_chunk(SndFile *psf, const SF_CHUNK_INFO *chunk_info);
static SF_CHUNK_ITERATOR *caf_next_chunk_iterator(SndFile *psf, SF_CHUNK_ITERATOR *iterator);
static int caf_get_chunk_size(SndFile *psf, const SF_CHUNK_ITERATOR *iterator,
                              SF_CHUNK_INFO *chunk_info);
static int caf_get_chunk_data(SndFile *psf, const SF_CHUNK_ITERATOR *iterator,
                              SF_CHUNK_INFO *chunk_info);

int caf_open(SndFile *psf)
{
    struct CAF_PRIVATE *pcaf;
    int subformat, format, error = 0;

    if ((psf->m_container_data = calloc(1, sizeof(struct CAF_PRIVATE))) == NULL)
        return SFE_MALLOC_FAILED;

    pcaf = (struct CAF_PRIVATE *)psf->m_container_data;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = caf_read_header(psf)))
            return error;

        psf->next_chunk_iterator = caf_next_chunk_iterator;
        psf->get_chunk_size = caf_get_chunk_size;
        psf->get_chunk_data = caf_get_chunk_data;
    };

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        format = SF_CONTAINER(psf->sf.format);
        if (format != SF_FORMAT_CAF)
            return SFE_BAD_OPEN_FORMAT;

        psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

        if (psf->m_mode != SFM_RDWR || psf->m_filelength < 44)
        {
            psf->m_filelength = 0;
            psf->m_datalength = 0;
            psf->m_dataoffset = 0;
            psf->sf.frames = 0;
        };

        psf->m_strings.flags = SF_STR_ALLOW_START | SF_STR_ALLOW_END;

        /*
         * By default, add the peak chunk to floating point files. Default behaviour
         * can be switched off using sf_command (SFC_SET_PEAK_CHUNK, SF_FALSE).
         */
        if (psf->m_mode == SFM_WRITE &&
            (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE))
        {
            psf->m_peak_info = std::unique_ptr<PEAK_INFO>(new PEAK_INFO(psf->sf.channels));
        };

        if ((error = caf_write_header(psf, SF_FALSE)) != 0)
            return error;

        psf->write_header = caf_write_header;
        psf->set_chunk = caf_set_chunk;
    };

    psf->container_close = caf_close;
    psf->on_command = caf_command;

    switch (subformat)
    {
    case SF_FORMAT_PCM_S8:
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

    case SF_FORMAT_ALAC_16:
    case SF_FORMAT_ALAC_20:
    case SF_FORMAT_ALAC_24:
    case SF_FORMAT_ALAC_32:
        if (psf->m_mode == SFM_READ)
            /* Only pass the ALAC_DECODER_INFO in read mode. */
            error = alac_init(psf, &pcaf->alac);
        else
            error = alac_init(psf, NULL);
        break;

    default:
        return SFE_UNSUPPORTED_ENCODING;
    };

    return error;
}

static int caf_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        caf_write_tailer(psf);
        caf_write_header(psf, SF_TRUE);
    };

    return 0;
}

static size_t caf_command(SndFile *psf, int command, void *UNUSED(data), size_t UNUSED(datasize))
{
	struct CAF_PRIVATE *pcaf;

    if ((pcaf = (struct CAF_PRIVATE *)psf->m_container_data) == NULL)
        return SFE_INTERNAL;

    switch (command)
    {
    case SFC_SET_CHANNEL_MAP_INFO:
        pcaf->chanmap_tag = aiff_caf_find_channel_layout_tag(psf->m_channel_map.data(), psf->sf.channels);
        return (pcaf->chanmap_tag != 0);

    default:
        break;
    };

    return 0;
}

static int decode_desc_chunk(SndFile *psf, const struct DESC_CHUNK *desc)
{
    int format = SF_FORMAT_CAF;

    psf->sf.channels = desc->channels_per_frame;

    if (desc->fmt_id == alac_MARKER)
    {
		struct CAF_PRIVATE *pcaf;

        if ((pcaf = (struct CAF_PRIVATE *)psf->m_container_data) != NULL)
        {
            switch (desc->fmt_flags)
            {
            case 1:
                pcaf->alac.bits_per_sample = 16;
                format |= SF_FORMAT_ALAC_16;
                break;
            case 2:
                pcaf->alac.bits_per_sample = 20;
                format |= SF_FORMAT_ALAC_20;
                break;
            case 3:
                pcaf->alac.bits_per_sample = 24;
                format |= SF_FORMAT_ALAC_24;
                break;
            case 4:
                pcaf->alac.bits_per_sample = 32;
                format |= SF_FORMAT_ALAC_32;
                break;
            default:
                psf->log_printf("Bad ALAC format flag value of %d\n", desc->fmt_flags);
            };

            pcaf->alac.frames_per_packet = desc->frames_per_packet;
        };

        return format;
    };

    format |= psf->m_endian == SF_ENDIAN_LITTLE ? SF_ENDIAN_LITTLE : 0;

    if (desc->fmt_id == lpcm_MARKER && desc->fmt_flags & 1)
    {
        /* Floating point data. */
        if (desc->bits_per_chan == 32 && desc->pkt_bytes == 4 * desc->channels_per_frame)
        {
            psf->m_bytewidth = 4;
            return format | SF_FORMAT_FLOAT;
        };
        if (desc->bits_per_chan == 64 && desc->pkt_bytes == 8 * desc->channels_per_frame)
        {
            psf->m_bytewidth = 8;
            return format | SF_FORMAT_DOUBLE;
        };
    };

    if (desc->fmt_id == lpcm_MARKER && (desc->fmt_flags & 1) == 0)
    {
        /* Integer data. */
        if (desc->bits_per_chan == 32 && desc->pkt_bytes == 4 * desc->channels_per_frame)
        {
            psf->m_bytewidth = 4;
            return format | SF_FORMAT_PCM_32;
        };
        if (desc->bits_per_chan == 24 && desc->pkt_bytes == 3 * desc->channels_per_frame)
        {
            psf->m_bytewidth = 3;
            return format | SF_FORMAT_PCM_24;
        };
        if (desc->bits_per_chan == 16 && desc->pkt_bytes == 2 * desc->channels_per_frame)
        {
            psf->m_bytewidth = 2;
            return format | SF_FORMAT_PCM_16;
        };
        if (desc->bits_per_chan == 8 && desc->pkt_bytes == 1 * desc->channels_per_frame)
        {
            psf->m_bytewidth = 1;
            return format | SF_FORMAT_PCM_S8;
        };
    };

    if (desc->fmt_id == alaw_MARKER && desc->bits_per_chan == 8)
    {
        psf->m_bytewidth = 1;
        return format | SF_FORMAT_ALAW;
    };

    if (desc->fmt_id == ulaw_MARKER && desc->bits_per_chan == 8)
    {
        psf->m_bytewidth = 1;
        return format | SF_FORMAT_ULAW;
    };

    psf->log_printf("**** Unknown format identifier.\n");

    return 0;
}

static int caf_read_header(SndFile *psf)
{
	struct CAF_PRIVATE *pcaf;
    BUF_UNION ubuf;
	struct DESC_CHUNK desc;
    sf_count_t chunk_size;
    double srate;
    short version, flags;
    int marker, k, have_data = 0, error;

    if ((pcaf = (struct CAF_PRIVATE *)psf->m_container_data) == NULL)
        return SFE_INTERNAL;

    memset(&desc, 0, sizeof(desc));

    /* Set position to start of file to begin reading header. */
	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("mE2E2", &marker, &version, &flags);
    psf->log_printf("%M\n  Version : %d\n  Flags   : %x\n", marker, version, flags);
    if (marker != caff_MARKER)
        return SFE_CAF_NOT_CAF;

    psf->binheader_readf("mE8b", &marker, &chunk_size, ubuf.ucbuf, 8);
    srate = double64_be_read(ubuf.ucbuf);
    snprintf(ubuf.cbuf, sizeof(ubuf.cbuf), "%5.3f", srate);
    psf->log_printf("%M : %D\n  Sample rate  : %s\n", marker, chunk_size, ubuf.cbuf);
    if (marker != desc_MARKER)
        return SFE_CAF_NO_DESC;

    if (chunk_size < SIGNED_SIZEOF(struct DESC_CHUNK))
    {
        psf->log_printf("**** Chunk size too small. Should be > 32 bytes.\n");
        return SFE_MALFORMED_FILE;
    };

    psf->sf.samplerate = lrint(srate);

    psf->binheader_readf("mE44444", &desc.fmt_id, &desc.fmt_flags, &desc.pkt_bytes,
                        &desc.frames_per_packet, &desc.channels_per_frame, &desc.bits_per_chan);
    psf->log_printf(
                   "  Format id    : %M\n  Format flags : %x\n  Bytes / packet   : %u\n"
                   "  Frames / packet  : %u\n  Channels / frame : %u\n  Bits / channel   "
                   ": %u\n",
                   desc.fmt_id, desc.fmt_flags, desc.pkt_bytes, desc.frames_per_packet,
                   desc.channels_per_frame, desc.bits_per_chan);

    if (desc.channels_per_frame > SF_MAX_CHANNELS)
    {
        psf->log_printf("**** Bad channels per frame value %u.\n", desc.channels_per_frame);
        return SFE_MALFORMED_FILE;
    };

	if (chunk_size > SIGNED_SIZEOF(struct DESC_CHUNK))
		psf->binheader_seekf(chunk_size - sizeof(struct DESC_CHUNK), SF_SEEK_CUR);

    psf->sf.channels = desc.channels_per_frame;

    while (1)
    {
        marker = 0;
        chunk_size = 0;

        psf->binheader_readf("mE8", &marker, &chunk_size);
        if (marker == 0)
        {
            sf_count_t pos = psf->ftell();
            psf->log_printf("Have 0 marker at position %D (0x%x).\n", pos, pos);
            break;
        };
        if (chunk_size < 0)
        {
            psf->log_printf("%M : %D *** Should be >= 0 ***\n", marker, chunk_size);
            break;
        };
        if (chunk_size > psf->m_filelength)
            break;

        psf_store_read_chunk_u32(&psf->m_rchunks, marker, psf->ftell(), chunk_size);

        switch (marker)
        {
        case peak_MARKER:
            psf->log_printf("%M : %D\n", marker, chunk_size);
            if (chunk_size != CAF_PEAK_CHUNK_SIZE(psf->sf.channels))
            {
				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
                psf->log_printf("*** File PEAK chunk %D should be %d.\n", chunk_size,
                               CAF_PEAK_CHUNK_SIZE(psf->sf.channels));
                return SFE_CAF_BAD_PEAK;
            };

            psf->m_peak_info = std::unique_ptr<PEAK_INFO>(new PEAK_INFO(psf->sf.channels));

            /* read in rest of PEAK chunk. */
            psf->binheader_readf("E4", &(psf->m_peak_info->edit_number));
            psf->log_printf("  edit count : %d\n", psf->m_peak_info->edit_number);

            psf->log_printf("     Ch   Position       Value\n");
            for (k = 0; k < psf->sf.channels; k++)
            {
                sf_count_t position;
                float value;

                psf->binheader_readf("Ef8", &value, &position);
                psf->m_peak_info->peaks[k].value = value;
                psf->m_peak_info->peaks[k].position = position;

                snprintf(ubuf.cbuf, sizeof(ubuf.cbuf), "    %2d   %-12" PRId64 "   %g\n", k,
                         position, value);
                psf->log_printf(ubuf.cbuf);
            };

            psf->m_peak_info->peak_loc = SF_PEAK_START;
            break;

        case chan_MARKER:
            if (chunk_size < 12)
            {
                psf->log_printf("%M : %D (should be >= 12)\n", marker, chunk_size);
				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
                break;
            }

            psf->log_printf("%M : %D\n", marker, chunk_size);

            if ((error = caf_read_chanmap(psf, chunk_size)))
                return error;
            break;

        case free_MARKER:
            psf->log_printf("%M : %D\n", marker, chunk_size);
			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        case data_MARKER:
            psf->binheader_readf("E4", &k);
            if (chunk_size == -1)
            {
                psf->log_printf("%M : -1\n");
                chunk_size = psf->m_filelength - psf->m_header.indx;
            }
            else if (psf->m_filelength > 0 && chunk_size > psf->m_filelength - psf->m_header.indx + 10)
            {
                psf->log_printf("%M : %D (should be %D)\n", marker, chunk_size,
                               psf->m_filelength - psf->m_header.indx - 8);
                psf->m_datalength = psf->m_filelength - psf->m_header.indx - 8;
            }
            else
            {
                psf->log_printf("%M : %D\n", marker, chunk_size);
                /* Subtract the 4 bytes of the 'edit' field above. */
                psf->m_datalength = chunk_size - 4;
            };

            psf->log_printf("  edit : %u\n", k);

            psf->m_dataoffset = psf->m_header.indx;
            if (psf->m_datalength + psf->m_dataoffset < psf->m_filelength)
                psf->m_dataend = psf->m_datalength + psf->m_dataoffset;

			psf->binheader_seekf(psf->m_datalength, SF_SEEK_CUR);
            have_data = 1;
            break;

        case kuki_MARKER:
            psf->log_printf("%M : %D\n", marker, chunk_size);
            pcaf->alac.kuki_offset = psf->ftell() - 12;
			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        case pakt_MARKER:
            if (chunk_size < 24)
            {
                psf->log_printf("%M : %D (should be > 24)\n", marker, chunk_size);
                return SFE_MALFORMED_FILE;
            }
            else if (chunk_size > psf->m_filelength - psf->m_header.indx)
            {
                psf->log_printf("%M : %D (should be < %D)\n", marker, chunk_size,
                               psf->m_filelength - psf->m_header.indx);
                return SFE_MALFORMED_FILE;
            }
            else
                psf->log_printf("%M : %D\n", marker, chunk_size);

            psf->binheader_readf("E8844", &pcaf->alac.packets, &pcaf->alac.valid_frames,
                                &pcaf->alac.priming_frames, &pcaf->alac.remainder_frames);

            psf->log_printf(
                           "  Packets          : %D\n"
                           "  Valid frames     : %D\n"
                           "  Priming frames   : %d\n"
                           "  Remainder frames : %d\n",
                           pcaf->alac.packets, pcaf->alac.valid_frames, pcaf->alac.priming_frames,
                           pcaf->alac.remainder_frames);

            if (pcaf->alac.packets == 0 && pcaf->alac.valid_frames == 0 &&
                pcaf->alac.priming_frames == 0 && pcaf->alac.remainder_frames == 0)
                psf->log_printf("*** 'pakt' chunk header is all zero.\n");

            pcaf->alac.pakt_offset = psf->ftell() - 12;
			psf->binheader_seekf(chunk_size - 24, SF_SEEK_CUR);
            break;

        case info_MARKER:
            if (chunk_size < 4)
            {
                psf->log_printf("%M : %D (should be > 4)\n", marker, chunk_size);
                return SFE_MALFORMED_FILE;
            }
            else if (chunk_size > psf->m_filelength - psf->m_header.indx)
            {
                psf->log_printf("%M : %D (should be < %z)\n", marker, chunk_size,
                               psf->m_filelength - psf->m_header.indx);
                return SFE_MALFORMED_FILE;
            };
            psf->log_printf("%M : %D\n", marker, chunk_size);
            if (chunk_size > 4)
                caf_read_strings(psf, chunk_size - 4);
            break;

        default:
            psf->log_printf("%M : %D (skipped)\n", marker, chunk_size);
			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;
        };

        if (marker != data_MARKER && chunk_size >= 0xffffff00)
            break;

        if (!psf->sf.seekable && have_data)
            break;

        if (psf->ftell() >= psf->m_filelength - SIGNED_SIZEOF(chunk_size))
        {
            psf->log_printf("End\n");
            break;
        };
    };

    if (have_data == 0)
    {
        psf->log_printf("**** Error, could not find 'data' chunk.\n");
        return SFE_MALFORMED_FILE;
    };

    psf->m_endian = (desc.fmt_flags & 2) ? SF_ENDIAN_LITTLE : SF_ENDIAN_BIG;

    psf->fseek(psf->m_dataoffset, SEEK_SET);

    if ((psf->sf.format = decode_desc_chunk(psf, &desc)) == 0)
        return SFE_UNSUPPORTED_ENCODING;

    if (psf->m_bytewidth > 0)
        psf->sf.frames = psf->m_datalength / psf->m_bytewidth;

    return 0;
}

static int caf_write_header(SndFile *psf, int calc_length)
{
    BUF_UNION ubuf;
	struct CAF_PRIVATE *pcaf;
	struct DESC_CHUNK desc;
    sf_count_t current;
    uint32_t uk;
    int subformat, append_free_block = SF_TRUE;

    if ((pcaf = (struct CAF_PRIVATE *)psf->m_container_data) == NULL)
        return SFE_INTERNAL;

    memset(&desc, 0, sizeof(desc));

    current = psf->ftell();

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

        if (psf->m_dataend)
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;

        if (psf->m_bytewidth > 0)
            psf->sf.frames = psf->m_datalength / (psf->m_bytewidth * psf->sf.channels);
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    /* 'caff' marker, version and flags. */
    psf->binheader_writef("Em22", BHWm(caff_MARKER), BHW2(1), BHW2(0));

    /* 'desc' marker and chunk size. */
    psf->binheader_writef("Em8", BHWm(desc_MARKER), BHW8((sf_count_t)(sizeof(struct DESC_CHUNK))));

    double64_be_write(1.0 * psf->sf.samplerate, ubuf.ucbuf);
    psf->binheader_writef("b", BHWv(ubuf.ucbuf), BHWz(8));

    subformat = SF_CODEC(psf->sf.format);

    psf->m_endian = SF_ENDIAN(psf->sf.format);

    if (CPU_IS_BIG_ENDIAN && (psf->m_endian == 0 || psf->m_endian == SF_ENDIAN_CPU))
        psf->m_endian = SF_ENDIAN_BIG;
    else if (CPU_IS_LITTLE_ENDIAN &&
             (psf->m_endian == SF_ENDIAN_LITTLE || psf->m_endian == SF_ENDIAN_CPU))
        psf->m_endian = SF_ENDIAN_LITTLE;

    if (psf->m_endian == SF_ENDIAN_LITTLE)
        desc.fmt_flags = 2;
    else
        psf->m_endian = SF_ENDIAN_BIG;

    /* initial section (same for all, it appears) */
    switch (subformat)
    {
    case SF_FORMAT_PCM_S8:
        desc.fmt_id = lpcm_MARKER;
        psf->m_bytewidth = 1;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 8;
        break;

    case SF_FORMAT_PCM_16:
        desc.fmt_id = lpcm_MARKER;
        psf->m_bytewidth = 2;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 16;
        break;

    case SF_FORMAT_PCM_24:
        psf->m_bytewidth = 3;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 24;
        desc.fmt_id = lpcm_MARKER;
        break;

    case SF_FORMAT_PCM_32:
        desc.fmt_id = lpcm_MARKER;
        psf->m_bytewidth = 4;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 32;
        break;

    case SF_FORMAT_FLOAT:
        desc.fmt_id = lpcm_MARKER;
        desc.fmt_flags |= 1;
        psf->m_bytewidth = 4;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 32;
        break;

    case SF_FORMAT_DOUBLE:
        desc.fmt_id = lpcm_MARKER;
        desc.fmt_flags |= 1;
        psf->m_bytewidth = 8;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 64;
        break;

    case SF_FORMAT_ALAW:
        desc.fmt_id = alaw_MARKER;
        psf->m_bytewidth = 1;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 8;
        break;

    case SF_FORMAT_ULAW:
        desc.fmt_id = ulaw_MARKER;
        psf->m_bytewidth = 1;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.frames_per_packet = 1;
        desc.channels_per_frame = psf->sf.channels;
        desc.bits_per_chan = 8;
        break;

    case SF_FORMAT_ALAC_16:
    case SF_FORMAT_ALAC_20:
    case SF_FORMAT_ALAC_24:
    case SF_FORMAT_ALAC_32:
        desc.fmt_id = alac_MARKER;
        desc.pkt_bytes = psf->m_bytewidth * psf->sf.channels;
        desc.channels_per_frame = psf->sf.channels;
        alac_get_desc_chunk_items(subformat, &desc.fmt_flags, &desc.frames_per_packet);
        append_free_block = SF_FALSE;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    psf->binheader_writef("mE44444", BHWm(desc.fmt_id), BHW4(desc.fmt_flags),
                         BHW4(desc.pkt_bytes), BHW4(desc.frames_per_packet),
                         BHW4(desc.channels_per_frame), BHW4(desc.bits_per_chan));

    caf_write_strings(psf, SF_STR_LOCATE_START);

    if (psf->m_peak_info != NULL)
    {
        int k;
        psf->binheader_writef("Em84", BHWm(peak_MARKER),
                             BHW8((sf_count_t)CAF_PEAK_CHUNK_SIZE(psf->sf.channels)),
                             BHW4(psf->m_peak_info->edit_number));
        for (k = 0; k < psf->sf.channels; k++)
            psf->binheader_writef("Ef8", BHWf((float)psf->m_peak_info->peaks[k].value),
                                 BHW8(psf->m_peak_info->peaks[k].position));
    };

    if (!psf->m_channel_map.empty() && pcaf->chanmap_tag)
        psf->binheader_writef("Em8444", BHWm(chan_MARKER), BHW8((sf_count_t)12),
                             BHW4(pcaf->chanmap_tag), BHW4(0), BHW4(0));

    /* Write custom headers. */
    for (uk = 0; uk < psf->m_wchunks.used; uk++)
        psf->binheader_writef("m44b", BHWm((int)psf->m_wchunks.chunks[uk].mark32), BHW4(0),
                             BHW4(psf->m_wchunks.chunks[uk].len), BHWv(psf->m_wchunks.chunks[uk].data),
                             BHWz(psf->m_wchunks.chunks[uk].len));

    if (append_free_block)
    {
        /* Add free chunk so that the actual audio data starts at a multiple 0x1000. */
        sf_count_t free_len = 0x1000 - psf->m_header.indx - 16 - 12;
        while (free_len < 0)
            free_len += 0x1000;
        psf->binheader_writef("Em8z", BHWm(free_MARKER), BHW8(free_len), BHWz(free_len));
    };

    psf->binheader_writef("Em84", BHWm(data_MARKER), BHW8(psf->m_datalength + 4), BHW4(0));

    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);
    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;
    if (current < psf->m_dataoffset)
        psf->fseek(psf->m_dataoffset, SEEK_SET);
    else if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int caf_write_tailer(SndFile *psf)
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

    if (psf->m_strings.flags & SF_STR_LOCATE_END)
        caf_write_strings(psf, SF_STR_LOCATE_END);

    /* Write the tailer. */
    if (psf->m_header.indx > 0)
        psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    return 0;
}

static int caf_read_chanmap(SndFile *psf, sf_count_t chunk_size)
{
    const AIFF_CAF_CHANNEL_MAP *map_info;
    unsigned channel_bitmap, channel_decriptions, bytesread;
    int layout_tag;

    bytesread =
        psf->binheader_readf("E444", &layout_tag, &channel_bitmap, &channel_decriptions);

    map_info = aiff_caf_of_channel_layout_tag(layout_tag);

    psf->log_printf("  Tag    : %x\n", layout_tag);
    if (map_info)
        psf->log_printf("  Layout : %s\n", map_info->name);

	if (bytesread < chunk_size)
		psf->binheader_seekf(chunk_size - bytesread, SF_SEEK_CUR);

    if (map_info && map_info->channel_map != NULL)
    {
        size_t chanmap_size =
            std::min(psf->sf.channels, layout_tag & 0xff) * sizeof(psf->m_channel_map[0]);

        psf->m_channel_map.resize(chanmap_size);
        memcpy(psf->m_channel_map.data(), map_info->channel_map, sizeof(int) * chanmap_size);
    };

    return 0;
}

static uint32_t string_hash32(const char *str)
{
    uint32_t hash = 0x87654321;

    while (str[0])
    {
        hash = hash * 333 + str[0];
        str++;
    };

    return hash;
}

static int caf_read_strings(SndFile *psf, sf_count_t chunk_size)
{
    char *buf;
    char *key, *value;
    uint32_t count, hash;

    if ((buf = (char *)malloc(chunk_size + 1)) == NULL)
        return (psf->m_error = SFE_MALLOC_FAILED);

    psf->binheader_readf("E4b", &count, buf, make_size_t(chunk_size));
    psf->log_printf(" count: %u\n", count);

    /* Force terminate `buf` to make sure. */
    buf[chunk_size] = 0;

    for (key = buf; key < buf + chunk_size;)
    {
        value = key + strlen(key) + 1;
        if (value > buf + chunk_size)
            break;
        psf->log_printf("   %-12s : %s\n", key, value);

        hash = string_hash32(key);
        switch (hash)
        {
        case 0xC4861943: /* 'title' */
            psf->store_string(SF_STR_TITLE, value);
            break;
        case 0xAD47A394: /* 'software' */
            psf->store_string(SF_STR_SOFTWARE, value);
            break;
        case 0x5D178E2A: /* 'copyright' */
            psf->store_string(SF_STR_COPYRIGHT, value);
            break;
        case 0x60E4D0C8: /* 'artist' */
            psf->store_string(SF_STR_ARTIST, value);
            break;
        case 0x83B5D16A: /* 'genre' */
            psf->store_string(SF_STR_GENRE, value);
            break;
        case 0x15E5FC88: /* 'comment' */
        case 0x7C297D5B: /* 'comments' */
            psf->store_string(SF_STR_COMMENT, value);
            break;
        case 0x24A7C347: /* 'tracknumber' */
            psf->store_string(SF_STR_TRACKNUMBER, value);
            break;
        case 0x50A31EB7: /* 'date' */
            psf->store_string(SF_STR_DATE, value);
            break;
        case 0x6583545A: /* 'album' */
            psf->store_string(SF_STR_ALBUM, value);
            break;
        case 0xE7C64B6C: /* 'license' */
            psf->store_string(SF_STR_LICENSE, value);
            break;
        default:
            psf->log_printf(" Unhandled hash 0x%x : /* '%s' */\n", hash, key);
            break;
        };

        key = value + strlen(value) + 1;
    };

    free(buf);

    return 0;
}

struct put_buffer
{
    uint32_t index;
    char s[16 * 1024];
};

static uint32_t put_key_value(struct put_buffer *buf, const char *key, const char *value)
{
    uint32_t written;

    if (buf->index + strlen(key) + strlen(value) + 2 > sizeof(buf->s))
        return 0;

    written =
        snprintf(buf->s + buf->index, sizeof(buf->s) - buf->index, "%s%c%s%c", key, 0, value, 0);

    if (buf->index + written >= sizeof(buf->s))
        return 0;

    buf->index += written;
    return 1;
}

static void caf_write_strings(SndFile *psf, int location)
{
    struct put_buffer buf;
    const char *cptr;
    uint32_t k, string_count = 0;

    memset(&buf, 0, sizeof(buf));

    for (k = 0; k < SF_MAX_STRINGS; k++)
    {
        if (psf->m_strings.data[k].type == 0)
            break;

        if (psf->m_strings.data[k].flags != location)
            continue;

        if ((cptr = psf->get_string(psf->m_strings.data[k].type)) == NULL)
            continue;

        switch (psf->m_strings.data[k].type)
        {
        case SF_STR_TITLE:
            string_count += put_key_value(&buf, "title", cptr);
            break;
        case SF_STR_COPYRIGHT:
            string_count += put_key_value(&buf, "copyright", cptr);
            break;
        case SF_STR_SOFTWARE:
            string_count += put_key_value(&buf, "software", cptr);
            break;
        case SF_STR_ARTIST:
            string_count += put_key_value(&buf, "artist", cptr);
            break;
        case SF_STR_COMMENT:
            string_count += put_key_value(&buf, "comment", cptr);
            break;
        case SF_STR_DATE:
            string_count += put_key_value(&buf, "date", cptr);
            break;
        case SF_STR_ALBUM:
            string_count += put_key_value(&buf, "album", cptr);
            break;
        case SF_STR_LICENSE:
            string_count += put_key_value(&buf, "license", cptr);
            break;
        case SF_STR_TRACKNUMBER:
            string_count += put_key_value(&buf, "tracknumber", cptr);
            break;
        case SF_STR_GENRE:
            string_count += put_key_value(&buf, "genre", cptr);
            break;

        default:
            break;
        };
    };

    if (string_count == 0 || buf.index == 0)
        return;

    psf->binheader_writef("Em84b", BHWm(info_MARKER), BHW8(buf.index + 4), BHW4(string_count),
                         BHWv(buf.s), BHWz(buf.index));
}

static int caf_set_chunk(SndFile *psf, const SF_CHUNK_INFO *chunk_info)
{
    return psf_save_write_chunk(&psf->m_wchunks, chunk_info);
}

static SF_CHUNK_ITERATOR *caf_next_chunk_iterator(SndFile *psf, SF_CHUNK_ITERATOR *iterator)
{
    return psf_next_chunk_iterator(&psf->m_rchunks, iterator);
}

static int caf_get_chunk_size(SndFile *psf, const SF_CHUNK_ITERATOR *iterator,
                              SF_CHUNK_INFO *chunk_info)
{
    int indx;

    if ((indx = psf_find_read_chunk_iterator(&psf->m_rchunks, iterator)) < 0)
        return SFE_UNKNOWN_CHUNK;

    chunk_info->datalen = psf->m_rchunks.chunks[indx].len;

    return SFE_NO_ERROR;
}

static int caf_get_chunk_data(SndFile *psf, const SF_CHUNK_ITERATOR *iterator,
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
