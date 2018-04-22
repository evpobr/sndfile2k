/*
** Copyright (C) 1999-2018 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2005 David Viens <davidv@plogue.com>
** Copyright (C) 2018 evpobr <evpobr@gmail.com>
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
#include <time.h>
#include <ctype.h>

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

#define FORM_MARKER (MAKE_MARKER('F', 'O', 'R', 'M'))
#define AIFF_MARKER (MAKE_MARKER('A', 'I', 'F', 'F'))
#define AIFC_MARKER (MAKE_MARKER('A', 'I', 'F', 'C'))
#define COMM_MARKER (MAKE_MARKER('C', 'O', 'M', 'M'))
#define SSND_MARKER (MAKE_MARKER('S', 'S', 'N', 'D'))
#define MARK_MARKER (MAKE_MARKER('M', 'A', 'R', 'K'))
#define INST_MARKER (MAKE_MARKER('I', 'N', 'S', 'T'))
#define APPL_MARKER (MAKE_MARKER('A', 'P', 'P', 'L'))
#define CHAN_MARKER (MAKE_MARKER('C', 'H', 'A', 'N'))

#define c_MARKER (MAKE_MARKER('(', 'c', ')', ' '))
#define NAME_MARKER (MAKE_MARKER('N', 'A', 'M', 'E'))
#define AUTH_MARKER (MAKE_MARKER('A', 'U', 'T', 'H'))
#define ANNO_MARKER (MAKE_MARKER('A', 'N', 'N', 'O'))
#define COMT_MARKER (MAKE_MARKER('C', 'O', 'M', 'T'))
#define FVER_MARKER (MAKE_MARKER('F', 'V', 'E', 'R'))
#define SFX_MARKER (MAKE_MARKER('S', 'F', 'X', '!'))

#define PEAK_MARKER (MAKE_MARKER('P', 'E', 'A', 'K'))
#define basc_MARKER (MAKE_MARKER('b', 'a', 's', 'c'))

/* Supported AIFC encodings.*/
#define NONE_MARKER (MAKE_MARKER('N', 'O', 'N', 'E'))
#define sowt_MARKER (MAKE_MARKER('s', 'o', 'w', 't'))
#define twos_MARKER (MAKE_MARKER('t', 'w', 'o', 's'))
#define raw_MARKER (MAKE_MARKER('r', 'a', 'w', ' '))
#define in24_MARKER (MAKE_MARKER('i', 'n', '2', '4'))
#define ni24_MARKER (MAKE_MARKER('4', '2', 'n', '1'))
#define in32_MARKER (MAKE_MARKER('i', 'n', '3', '2'))
#define ni32_MARKER (MAKE_MARKER('2', '3', 'n', 'i'))

#define fl32_MARKER (MAKE_MARKER('f', 'l', '3', '2'))
#define FL32_MARKER (MAKE_MARKER('F', 'L', '3', '2'))
#define fl64_MARKER (MAKE_MARKER('f', 'l', '6', '4'))
#define FL64_MARKER (MAKE_MARKER('F', 'L', '6', '4'))

#define ulaw_MARKER (MAKE_MARKER('u', 'l', 'a', 'w'))
#define ULAW_MARKER (MAKE_MARKER('U', 'L', 'A', 'W'))
#define alaw_MARKER (MAKE_MARKER('a', 'l', 'a', 'w'))
#define ALAW_MARKER (MAKE_MARKER('A', 'L', 'A', 'W'))

#define DWVW_MARKER (MAKE_MARKER('D', 'W', 'V', 'W'))
#define GSM_MARKER (MAKE_MARKER('G', 'S', 'M', ' '))
#define ima4_MARKER (MAKE_MARKER('i', 'm', 'a', '4'))

/*
 * This value is officially assigned to Mega Nerd Pty Ltd by Apple
 * Corportation as the Application marker for libsndfile.
 *
 * See : http://developer.apple.com/faq/datatype.html
 */
#define m3ga_MARKER (MAKE_MARKER('m', '3', 'g', 'a'))

/* Unsupported AIFC encodings.*/

#define MAC3_MARKER (MAKE_MARKER('M', 'A', 'C', '3'))
#define MAC6_MARKER (MAKE_MARKER('M', 'A', 'C', '6'))
#define ADP4_MARKER (MAKE_MARKER('A', 'D', 'P', '4'))

/* Predfined chunk sizes. */
const uint32_t SIZEOF_AIFF_COMM = 18;
const uint32_t SIZEOF_AIFC_COMM_MIN = 22;
const uint32_t SIZEOF_AIFC_COMM = 24;
const uint32_t SIZEOF_SSND_CHUNK = 8;
const uint32_t SIZEOF_INST_CHUNK = 20;

/* Is it constant? */

/* AIFC/IMA4 defines. */
const int AIFC_IMA4_BLOCK_LEN = 34;
const int AIFC_IMA4_SAMPLES_PER_BLOCK = 64;

#define AIFF_PEAK_CHUNK_SIZE(ch) (2 * sizeof(int) + ch * (sizeof(float) + sizeof(int)))

/*
 * Typedefs for file chunks.
 */

enum AIFF_CHUNK_TYPE
{
    HAVE_FORM = 0x01,
    HAVE_AIFF = 0x02,
    HAVE_AIFC = 0x04,
    HAVE_FVER = 0x08,
    HAVE_COMM = 0x10,
    HAVE_SSND = 0x20
};

struct COMM_CHUNK
{
    uint32_t size;
    int16_t numChannels;
    uint32_t numSampleFrames;
    int16_t sampleSize;
    uint8_t sampleRate[10];
    uint32_t encoding;
    char zero_bytes[2];
};

struct SSND_CHUNK
{
    uint32_t offset;
    uint32_t blocksize;
};

struct INST_LOOP
{
    int16_t playMode;
    uint16_t beginLoop;
    uint16_t endLoop;
};

struct INST_CHUNK
{
    int8_t baseNote; /* all notes are MIDI note numbers */
    int8_t detune; /* cents off, only -50 to +50 are significant */
    int8_t lowNote;
    int8_t highNote;
    int8_t lowVelocity; /* 1 to 127 */
    int8_t highVelocity; /* 1 to 127 */
    int16_t gain; /* in dB, 0 is normal */
    struct INST_LOOP sustain_loop;
    struct INST_LOOP release_loop;
};

enum basc_SCALE
{
    basc_SCALE_MINOR = 1,
    basc_SCALE_MAJOR,
    basc_SCALE_NEITHER,
    basc_SCALE_BOTH
};

enum basc_TYPE
{
    basc_TYPE_LOOP = 0,
    basc_TYPE_ONE_SHOT
};

struct basc_CHUNK
{
    uint32_t version;
    uint32_t numBeats;
    uint16_t rootNote;
    uint16_t scaleType;
    uint16_t sigNumerator;
    uint16_t sigDenominator;
    uint16_t loopType;
};

struct MARK_ID_POS
{
    uint16_t markerID;
    uint32_t position;
};

struct AIFF_PRIVATE
{
    sf_count_t comm_offset;
    sf_count_t ssnd_offset;

    int32_t chanmap_tag;

    struct MARK_ID_POS *markstr;
};

/*
 * Private static functions.
 */

static int aiff_close(SF_PRIVATE *psf);

static int tenbytefloat2int(uint8_t *bytes);
static void uint2tenbytefloat(uint32_t num, uint8_t *bytes);

static int aiff_read_comm_chunk(SF_PRIVATE *psf, struct COMM_CHUNK *comm_fmt);

static int aiff_read_header(SF_PRIVATE *psf, struct COMM_CHUNK *comm_fmt);

static int aiff_write_header(SF_PRIVATE *psf, int calc_length);
static int aiff_write_tailer(SF_PRIVATE *psf);
static void aiff_write_strings(SF_PRIVATE *psf, int location);

static size_t aiff_command(SF_PRIVATE *psf, int command, void *data, size_t datasize);

static const char *get_loop_mode_str(int16_t mode);

static int16_t get_loop_mode(int16_t mode);

static int aiff_read_basc_chunk(SF_PRIVATE *psf, int);

static int aiff_read_chanmap(SF_PRIVATE *psf, unsigned dword);

static uint32_t marker_to_position(const struct MARK_ID_POS *m, uint16_t n, int marksize);

static int aiff_set_chunk(SF_PRIVATE *psf, const SF_CHUNK_INFO *chunk_info);
static SF_CHUNK_ITERATOR *aiff_next_chunk_iterator(SF_PRIVATE *psf, SF_CHUNK_ITERATOR *iterator);
static int aiff_get_chunk_size(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator,
                               SF_CHUNK_INFO *chunk_info);
static int aiff_get_chunk_data(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator,
                               SF_CHUNK_INFO *chunk_info);

/*
** Public function.
*/

int aiff_open(SF_PRIVATE *psf)
{
    struct COMM_CHUNK comm_fmt;
    int error;

    memset(&comm_fmt, 0, sizeof(comm_fmt));

    int subformat = SF_CODEC(psf->sf.format);

    if ((psf->m_container_data = calloc(1, sizeof(struct AIFF_PRIVATE))) == NULL)
        return SFE_MALLOC_FAILED;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = aiff_read_header(psf, &comm_fmt)))
            return error;

        psf->next_chunk_iterator = aiff_next_chunk_iterator;
        psf->get_chunk_size = aiff_get_chunk_size;
        psf->get_chunk_data = aiff_get_chunk_data;

        psf->fseek(psf->m_dataoffset, SEEK_SET);
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_AIFF)
            return SFE_BAD_OPEN_FORMAT;

        if (psf->m_mode == SFM_WRITE &&
            (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE))
        {
            psf->m_peak_info = std::make_unique<PEAK_INFO>(psf->sf.channels);
        };

        if (psf->m_mode != SFM_RDWR || psf->m_filelength < 40)
        {
            psf->m_filelength = 0;
            psf->m_datalength = 0;
            psf->m_dataoffset = 0;
            psf->sf.frames = 0;
        };

        psf->m_strings.flags = SF_STR_ALLOW_START | SF_STR_ALLOW_END;

        if ((error = aiff_write_header(psf, SF_FALSE)))
            return error;

        psf->write_header = aiff_write_header;
        psf->set_chunk = aiff_set_chunk;
    };

    psf->container_close = aiff_close;
    psf->command = aiff_command;

    switch (SF_CODEC(psf->sf.format))
    {
    case SF_FORMAT_PCM_U8:
        error = pcm_init(psf);
        break;

    case SF_FORMAT_PCM_S8:
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

    /* Lite remove start */
    case SF_FORMAT_FLOAT:
        error = float32_init(psf);
        break;

    case SF_FORMAT_DOUBLE:
        error = double64_init(psf);
        break;

    case SF_FORMAT_DWVW_12:
        if (psf->sf.frames > comm_fmt.numSampleFrames)
            psf->sf.frames = comm_fmt.numSampleFrames;
        break;

    case SF_FORMAT_DWVW_16:
        error = dwvw_init(psf, 16);
        if (psf->sf.frames > comm_fmt.numSampleFrames)
            psf->sf.frames = comm_fmt.numSampleFrames;
        break;

    case SF_FORMAT_DWVW_24:
        error = dwvw_init(psf, 24);
        if (psf->sf.frames > comm_fmt.numSampleFrames)
            psf->sf.frames = comm_fmt.numSampleFrames;
        break;

    case SF_FORMAT_DWVW_N:
        if (psf->m_mode != SFM_READ)
        {
            error = SFE_DWVW_BAD_BITWIDTH;
            break;
        };
        if (comm_fmt.sampleSize >= 8 && comm_fmt.sampleSize < 24)
        {
            error = dwvw_init(psf, comm_fmt.sampleSize);
            if (psf->sf.frames > comm_fmt.numSampleFrames)
                psf->sf.frames = comm_fmt.numSampleFrames;
            break;
        };
        psf->log_printf("AIFC/DWVW : Bad bitwidth %d\n", comm_fmt.sampleSize);
        error = SFE_DWVW_BAD_BITWIDTH;
        break;

    case SF_FORMAT_IMA_ADPCM:
        /*
         * IMA ADPCM encoded AIFF files always have a block length
         * of 34 which decodes to 64 samples.
         */
        error = aiff_ima_init(psf, AIFC_IMA4_BLOCK_LEN, AIFC_IMA4_SAMPLES_PER_BLOCK);
        break;

    case SF_FORMAT_GSM610:
        error = gsm610_init(psf);
        if (psf->sf.frames > comm_fmt.numSampleFrames)
            psf->sf.frames = comm_fmt.numSampleFrames;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    if (psf->m_mode != SFM_WRITE && psf->sf.frames - comm_fmt.numSampleFrames != 0)
    {
        psf->log_printf("*** Frame count read from 'COMM' chunk (%u) not equal "
                        "to frame count\n"
                        "*** calculated from length of 'SSND' chunk (%u).\n",
                        comm_fmt.numSampleFrames, (uint32_t)psf->sf.frames);
    };

    return error;
}

/*
 * Private functions.
 */

/* This function ought to check size */
static uint32_t marker_to_position(const struct MARK_ID_POS *m, uint16_t n, int marksize)
{
    for (int i = 0; i < marksize; i++)
        if (m[i].markerID == n)
            return m[i].position;
    return 0;
}

static int aiff_read_header(SF_PRIVATE *psf, struct COMM_CHUNK *comm_fmt)
{
    struct SSND_CHUNK ssnd_fmt;
    BUF_UNION ubuf;
    uint32_t chunk_size = 0, FORMsize, SSNDsize, bytesread, mark_count = 0;
    int found_chunk = 0, done = 0, error = 0;
    char *cptr;
    int instr_found = 0, mark_found = 0;

    if (psf->m_filelength > INT64_C(0xffffffff))
        psf->log_printf("Warning : filelength > 0xffffffff. This is bad!!!!\n");

    struct AIFF_PRIVATE *paiff = (struct AIFF_PRIVATE *)psf->m_container_data;
    if (!paiff)
        return SFE_INTERNAL;

    paiff->comm_offset = 0;
    paiff->ssnd_offset = 0;

    /* Set position to start of file to begin reading header. */
    psf->binheader_seekf(0, SF_SEEK_SET);

    memset(comm_fmt, 0, sizeof(struct COMM_CHUNK));

    /* Until recently AIF* file were all BIG endian. */
    psf->m_endian = SF_ENDIAN_BIG;

    /*
     * AIFF files can apparently have their chunks in any order. However, they
     * must have a FORM chunk. Approach here is to read all the chunks one by
     * one and then check for the mandatory chunks at the end.
     */
    while (!done)
    {
        size_t jump = chunk_size & 1;
        unsigned marker = chunk_size = 0;

		psf->binheader_seekf((sf_count_t)jump, SF_SEEK_CUR);
        psf->binheader_readf("Em4", &marker, &chunk_size);
        if (marker == 0)
        {
            sf_count_t pos = psf->ftell();
            psf->log_printf("Have 0 marker at position %D (0x%x).\n", pos, pos);
            break;
        };

        if (psf->m_mode == SFM_RDWR && (found_chunk & HAVE_SSND))
            return SFE_AIFF_RW_SSND_NOT_LAST;

        psf_store_read_chunk_u32(&psf->m_rchunks, marker, psf->ftell(), chunk_size);

        switch (marker)
        {
        case FORM_MARKER:
            if (found_chunk)
                return SFE_AIFF_NO_FORM;

            FORMsize = chunk_size;

            found_chunk |= HAVE_FORM;
            psf->binheader_readf("m", &marker);
            switch (marker)
            {
            case AIFC_MARKER:
            case AIFF_MARKER:
                found_chunk |= (marker == AIFC_MARKER) ? (HAVE_AIFC | HAVE_AIFF) : HAVE_AIFF;
                break;
            default:
                break;
            };

            if (FORMsize != psf->m_filelength - 2 * SIGNED_SIZEOF(chunk_size))
            {
                chunk_size = (uint32_t)(psf->m_filelength - 2 * sizeof(chunk_size));
                psf->log_printf("FORM : %u (should be %u)\n %M\n", FORMsize, chunk_size,
                               marker);
                FORMsize = chunk_size;
            }
            else
            {
                psf->log_printf("FORM : %u\n %M\n", FORMsize, marker);
            }
            /* Set this to 0, so we don't jump a byte when parsing the next marker. */
            chunk_size = 0;
            break;

        case COMM_MARKER:
            paiff->comm_offset = psf->ftell() - 8;
            chunk_size += chunk_size & 1;
            comm_fmt->size = chunk_size;
            if ((error = aiff_read_comm_chunk(psf, comm_fmt)) != 0)
                return error;

            found_chunk |= HAVE_COMM;
            break;

        case PEAK_MARKER:
            /* Must have COMM chunk before PEAK chunk. */
            if ((found_chunk & (HAVE_FORM | HAVE_AIFF | HAVE_COMM)) !=
                (HAVE_FORM | HAVE_AIFF | HAVE_COMM))
                return SFE_AIFF_PEAK_B4_COMM;

            psf->log_printf("%M : %d\n", marker, chunk_size);
            if (chunk_size != AIFF_PEAK_CHUNK_SIZE(psf->sf.channels))
            {
				psf->binheader_seekf((sf_count_t)chunk_size, SF_SEEK_CUR);
                psf->log_printf("*** File PEAK chunk too big.\n");
                return SFE_WAV_BAD_PEAK;
            };

            psf->m_peak_info = std::make_unique<PEAK_INFO>(psf->sf.channels);

            /* read in rest of PEAK chunk. */
            psf->binheader_readf("E44", &(psf->m_peak_info->version),
                                &(psf->m_peak_info->timestamp));

            if (psf->m_peak_info->version != 1)
                psf->log_printf("  version    : %d *** (should be version 1)\n",
                               psf->m_peak_info->version);
            else
                psf->log_printf("  version    : %d\n", psf->m_peak_info->version);

            psf->log_printf("  time stamp : %d\n", psf->m_peak_info->timestamp);
            psf->log_printf("    Ch   Position       Value\n");

            cptr = ubuf.cbuf;
            for (int k = 0; k < psf->sf.channels; k++)
            {
                float value;
                uint32_t position;

                psf->binheader_readf("Ef4", &value, &position);
                psf->m_peak_info->peaks[k].value = value;
                psf->m_peak_info->peaks[k].position = position;

                snprintf(cptr, sizeof(ubuf.scbuf), "    %2d   %-12" PRId64 "   %g\n", k,
                         psf->m_peak_info->peaks[k].position, psf->m_peak_info->peaks[k].value);
                cptr[sizeof(ubuf.scbuf) - 1] = 0;
                psf->log_printf("%s", cptr);
            };

            psf->m_peak_info->peak_loc =
                ((found_chunk & HAVE_SSND) == 0) ? SF_PEAK_START : SF_PEAK_END;
            break;

        case SSND_MARKER:
            if ((found_chunk & HAVE_AIFC) && (found_chunk & HAVE_FVER) == 0)
                psf->log_printf("*** Valid AIFC files should have an FVER chunk.\n");

            paiff->ssnd_offset = psf->ftell() - 8;
            SSNDsize = chunk_size;
            psf->binheader_readf("E44", &(ssnd_fmt.offset), &(ssnd_fmt.blocksize));

            psf->m_datalength = SSNDsize - sizeof(ssnd_fmt);
            psf->m_dataoffset = psf->ftell();

            if (psf->m_datalength > psf->m_filelength - psf->m_dataoffset || psf->m_datalength < 0)
            {
                psf->log_printf(" SSND : %u (should be %D)\n", SSNDsize,
                               psf->m_filelength - psf->m_dataoffset + sizeof(struct SSND_CHUNK));
                psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
            }
            else
                psf->log_printf(" SSND : %u\n", SSNDsize);

            if (ssnd_fmt.offset == 0 || psf->m_dataoffset + ssnd_fmt.offset == ssnd_fmt.blocksize)
            {
                psf->log_printf("  Offset     : %u\n", ssnd_fmt.offset);
                psf->log_printf("  Block Size : %u\n", ssnd_fmt.blocksize);

                psf->m_dataoffset += ssnd_fmt.offset;
                psf->m_datalength -= ssnd_fmt.offset;
            }
            else
            {
                psf->log_printf("  Offset     : %u\n", ssnd_fmt.offset);
                psf->log_printf("  Block Size : %u ???\n", ssnd_fmt.blocksize);
                psf->m_dataoffset += ssnd_fmt.offset;
                psf->m_datalength -= ssnd_fmt.offset;
            };

            /* Only set dataend if there really is data at the end. */
            if (psf->m_datalength + psf->m_dataoffset < psf->m_filelength)
                psf->m_dataend = psf->m_datalength + psf->m_dataoffset;

            found_chunk |= HAVE_SSND;

            if (!psf->sf.seekable)
                break;

            /* Seek to end of SSND chunk. */
            psf->fseek(psf->m_dataoffset + psf->m_datalength, SEEK_SET);
            break;

        case c_MARKER:
            if (chunk_size == 0)
                break;
            if (chunk_size >= SIGNED_SIZEOF(ubuf.scbuf))
            {
                psf->log_printf(" %M : %d (too big)\n", marker, chunk_size);
                return SFE_INTERNAL;
            };

            cptr = ubuf.cbuf;
            psf->binheader_readf("b", cptr, chunk_size + (chunk_size & 1));
            cptr[chunk_size] = 0;

            psf_sanitize_string(cptr, chunk_size);

            psf->log_printf(" %M : %s\n", marker, cptr);
            psf_store_string(psf, SF_STR_COPYRIGHT, cptr);
            chunk_size += chunk_size & 1;
            break;

        case AUTH_MARKER:
            if (chunk_size == 0)
                break;
            if (chunk_size >= SIGNED_SIZEOF(ubuf.scbuf) - 1)
            {
                psf->log_printf(" %M : %d (too big)\n", marker, chunk_size);
                return SFE_INTERNAL;
            };

            cptr = ubuf.cbuf;
            psf->binheader_readf("b", cptr, chunk_size + (chunk_size & 1));
            cptr[chunk_size] = 0;
            psf->log_printf(" %M : %s\n", marker, cptr);
            psf_store_string(psf, SF_STR_ARTIST, cptr);
            chunk_size += chunk_size & 1;
            break;

        case COMT_MARKER:
        {
            uint16_t count, id, len;
            uint32_t timestamp, bytes;

            if (chunk_size == 0)
                break;
            bytes = chunk_size;
            bytes -= psf->binheader_readf("E2", &count);
            psf->log_printf(" %M : %d\n  count  : %d\n", marker, chunk_size, count);

            for (uint16_t k = 0; k < count; k++)
            {
                bytes -= psf->binheader_readf("E422", &timestamp, &id, &len);
                psf->log_printf("   time   : 0x%x\n   marker : %x\n   length : %d\n", timestamp,
                               id, len);

                if (len + 1 > SIGNED_SIZEOF(ubuf.scbuf))
                {
                    psf->log_printf("\nError : string length (%d) too big.\n", len);
                    return SFE_INTERNAL;
                };

                cptr = ubuf.cbuf;
                bytes -= psf->binheader_readf("b", cptr, len);
                cptr[len] = 0;
                psf->log_printf("   string : %s\n", cptr);
            };

			if (bytes > 0)
				psf->binheader_seekf((sf_count_t)bytes, SF_SEEK_CUR);
        };
        break;

        case APPL_MARKER:
        {
            unsigned appl_marker;

            if (chunk_size == 0)
                break;
            if (chunk_size >= SIGNED_SIZEOF(ubuf.scbuf) - 1)
            {
                psf->log_printf(" %M : %u (too big, skipping)\n", marker, chunk_size);
				psf->binheader_seekf((sf_count_t)(chunk_size + (chunk_size & 1)), SF_SEEK_CUR);
                break;
            };

            if (chunk_size < 4)
            {
                psf->log_printf(" %M : %d (too small, skipping)\n", marker, chunk_size);
				psf->binheader_seekf((sf_count_t)(chunk_size + (chunk_size & 1)), SF_SEEK_CUR);
                break;
            };

            cptr = ubuf.cbuf;
            psf->binheader_readf("mb", &appl_marker, cptr, chunk_size + (chunk_size & 1) - 4);
            cptr[chunk_size] = 0;

            for (uint32_t k = 0; k < chunk_size; k++)
                if (!psf_isprint(cptr[k]))
                {
                    cptr[k] = 0;
                    break;
                };

            psf->log_printf(" %M : %d\n  AppSig : %M\n  Name   : %s\n", marker, chunk_size,
                           appl_marker, cptr);
            psf_store_string(psf, SF_STR_SOFTWARE, cptr);
            chunk_size += chunk_size & 1;
        };
        break;

        case NAME_MARKER:
            if (chunk_size == 0)
                break;
            if (chunk_size >= SIGNED_SIZEOF(ubuf.scbuf) - 2)
            {
                psf->log_printf(" %M : %d (too big)\n", marker, chunk_size);
                return SFE_INTERNAL;
            };

            cptr = ubuf.cbuf;
            psf->binheader_readf("b", cptr, chunk_size + (chunk_size & 1));
            cptr[chunk_size] = 0;
            psf->log_printf(" %M : %s\n", marker, cptr);
            psf_store_string(psf, SF_STR_TITLE, cptr);
            chunk_size += chunk_size & 1;
            break;

        case ANNO_MARKER:
            if (chunk_size == 0)
                break;
            if (chunk_size >= SIGNED_SIZEOF(ubuf.scbuf) - 2)
            {
                psf->log_printf(" %M : %d (too big)\n", marker, chunk_size);
                return SFE_INTERNAL;
            };

            cptr = ubuf.cbuf;
            psf->binheader_readf("b", cptr, chunk_size + (chunk_size & 1));
            cptr[chunk_size] = 0;
            psf->log_printf(" %M : %s\n", marker, cptr);
            psf_store_string(psf, SF_STR_COMMENT, cptr);
            chunk_size += chunk_size & 1;
            break;

        case INST_MARKER:
            if (chunk_size != SIZEOF_INST_CHUNK)
            {
                psf->log_printf(" %M : %d (should be %d)\n", marker, chunk_size,
                               SIZEOF_INST_CHUNK);
				psf->binheader_seekf((sf_count_t)chunk_size, SF_SEEK_CUR);
                break;
            };
            psf->log_printf(" %M : %d\n", marker, chunk_size);
            {
                uint8_t bytes[6];
                int16_t gain;

                if (psf->m_instrument == NULL && (psf->m_instrument = psf_instrument_alloc()) == NULL)
                    return SFE_MALLOC_FAILED;

                psf->binheader_readf("b", bytes, 6);
                psf->log_printf(
                               "  Base Note : %u\n  Detune    : %u\n"
                               "  Low  Note : %u\n  High Note : %u\n"
                               "  Low  Vel. : %u\n  High Vel. : %u\n",
                               bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5]);
                psf->m_instrument->basenote = bytes[0];
                psf->m_instrument->detune = bytes[1];
                psf->m_instrument->key_lo = bytes[2];
                psf->m_instrument->key_hi = bytes[3];
                psf->m_instrument->velocity_lo = bytes[4];
                psf->m_instrument->velocity_hi = bytes[5];
                psf->binheader_readf("E2", &gain);
                psf->m_instrument->gain = gain;
                psf->log_printf("  Gain (dB) : %d\n", gain);
            };
            {
                /* 0 - no loop, 1 - forward looping, 2 - backward looping */
                int16_t mode;
                const char *loop_mode;
                uint16_t begin, end;

                psf->binheader_readf("E222", &mode, &begin, &end);
                loop_mode = get_loop_mode_str(mode);
                mode = get_loop_mode(mode);
                if (mode == SF_LOOP_NONE)
                {
                    psf->m_instrument->loop_count = 0;
                    psf->m_instrument->loops[0].mode = SF_LOOP_NONE;
                }
                else
                {
                    psf->m_instrument->loop_count = 1;
                    psf->m_instrument->loops[0].mode = SF_LOOP_FORWARD;
                    psf->m_instrument->loops[0].start = begin;
                    psf->m_instrument->loops[0].end = end;
                    psf->m_instrument->loops[0].count = 0;
                };
                psf->log_printf(
                               "  Sustain\n   mode  : %d => %s\n   begin "
                               ": %u\n   end   : %u\n",
                               mode, loop_mode, begin, end);
                psf->binheader_readf("E222", &mode, &begin, &end);
                loop_mode = get_loop_mode_str(mode);
                mode = get_loop_mode(mode);
                if (mode == SF_LOOP_NONE)
                {
                    psf->m_instrument->loops[1].mode = SF_LOOP_NONE;
                }
                else
                {
                    psf->m_instrument->loop_count += 1;
                    psf->m_instrument->loops[1].mode = SF_LOOP_FORWARD;
                    psf->m_instrument->loops[1].start = begin;
                    psf->m_instrument->loops[1].end = end;
                    psf->m_instrument->loops[1].count = 0;
                };
                psf->log_printf(
                               "  Release\n   mode  : %d => %s\n   begin "
                               ": %u\n   end   : %u\n",
                               mode, loop_mode, begin, end);
            };
            instr_found++;
            break;

        case basc_MARKER:
            psf->log_printf(" basc : %u\n", chunk_size);

            if ((error = aiff_read_basc_chunk(psf, chunk_size)))
                return error;
            break;

        case MARK_MARKER:
            psf->log_printf(" %M : %d\n", marker, chunk_size);
            {
                uint16_t n = 0;

                bytesread = psf->binheader_readf("E2", &n);
                mark_count = n;
                psf->log_printf("  Count : %u\n", mark_count);
                if (paiff->markstr != NULL)
                {
                    psf->log_printf("*** Second MARK chunk found. Throwing "
                                        "away the first.\n");
                    free(paiff->markstr);
                };
                paiff->markstr = (struct MARK_ID_POS *)calloc(mark_count, sizeof(struct MARK_ID_POS));
                if (paiff->markstr == NULL)
                    return SFE_MALLOC_FAILED;

                if (mark_count > 1000)
                {
                    psf->log_printf("  More than 1000 markers, skipping!\n");
					psf->binheader_seekf(chunk_size - bytesread, SF_SEEK_CUR);
                    break;
                };

                psf->m_cues.resize(mark_count);

                for (n = 0; n < mark_count && bytesread < chunk_size; n++)
                {
                    uint8_t ch;
                    uint16_t mark_id;
                    uint32_t position;

                    bytesread += psf->binheader_readf("E241", &mark_id, &position, &ch);
                    psf->log_printf("   Mark ID  : %u\n   Position : %u\n", mark_id, position);

                    psf->m_cues[n].indx = mark_id;
					psf->m_cues[n].position = 0;
					psf->m_cues[n].fcc_chunk = MAKE_MARKER('d', 'a', 't', 'a'); /* always data */
					psf->m_cues[n].chunk_start = 0;
					psf->m_cues[n].block_start = 0;
					psf->m_cues[n].sample_offset = position;

                    uint32_t pstr_len = (ch & 1) ? ch : ch + 1;

                    if (pstr_len < sizeof(ubuf.scbuf) - 1)
                    {
                        bytesread += psf->binheader_readf("b", ubuf.scbuf, pstr_len);
                        ubuf.scbuf[pstr_len] = 0;
                    }
                    else
                    {
                        uint32_t read_len = pstr_len - (sizeof(ubuf.scbuf) - 1);
                        bytesread += psf->binheader_readf("b", ubuf.scbuf, read_len);
						psf->binheader_seekf(pstr_len - read_len, SF_SEEK_CUR);
						bytesread += pstr_len - read_len;
                        ubuf.scbuf[sizeof(ubuf.scbuf) - 1] = 0;
                    }

                    psf->log_printf("   Name     : %s\n", ubuf.scbuf);

                    psf_strlcpy(psf->m_cues[n].name,
                                sizeof(psf->m_cues[n].name), ubuf.cbuf);

                    paiff->markstr[n].markerID = mark_id;
                    paiff->markstr[n].position = position;
                    /*
                     * TODO if ubuf.scbuf is equal to
                     * either Beg_loop, Beg loop or beg loop and spam
                     * if (psf->instrument == NULL && 
                     *    (psf->instrument = psf_instrument_alloc ()) == NULL)
                     *      return SFE_MALLOC_FAILED ;
                     */
                };
            };
            mark_found++;
			psf->binheader_seekf(chunk_size - bytesread, SF_SEEK_CUR);
            break;

        case FVER_MARKER:
            found_chunk |= HAVE_FVER;
            /* Falls through. */

        case SFX_MARKER:
            psf->log_printf(" %M : %d\n", marker, chunk_size);
			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        case NONE_MARKER:
            /* Fix for broken AIFC files with incorrect COMM chunk length. */
            chunk_size = (chunk_size >> 24) - 3;
            psf->log_printf(" %M : %d\n", marker, chunk_size);
			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        case CHAN_MARKER:
            if (chunk_size < 12)
            {
                psf->log_printf(" %M : %d (should be >= 12)\n", marker, chunk_size);
				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
                break;
            }

            psf->log_printf(" %M : %d\n", marker, chunk_size);

            if ((error = aiff_read_chanmap(psf, chunk_size)))
                return error;
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
                psf->log_printf(" %M : %u (unknown marker)\n", marker, chunk_size);

				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
                break;
            };

            if (psf->ftell() & 0x03)
            {
                psf->log_printf("  Unknown chunk marker at position %D. Resynching.\n",
                               psf->ftell() - 8);
				psf->binheader_seekf(-3, SF_SEEK_CUR);
                break;
            };
            psf->log_printf("*** Unknown chunk marker %X at position %D. Exiting parser.\n",
                           marker, psf->ftell());
            done = SF_TRUE;
            break;
        }; /* switch (marker) */

        if (chunk_size >= psf->m_filelength)
        {
            psf->log_printf("*** Chunk size %u > file length %D. Exiting parser.\n", chunk_size,
                           psf->m_filelength);
            break;
        };

        if ((!psf->sf.seekable) && (found_chunk & HAVE_SSND))
            break;

        if (psf->ftell() >= psf->m_filelength - (2 * SIGNED_SIZEOF(int32_t)))
            break;
    };

    if (instr_found && mark_found)
    {
        /* Next loop will convert markers to loop positions for internal handling */
        for (int ji = 0; ji < psf->m_instrument->loop_count; ji++)
        {
            if (ji < ARRAY_LEN(psf->m_instrument->loops))
            {
                psf->m_instrument->loops[ji].start = marker_to_position(
                    paiff->markstr, psf->m_instrument->loops[ji].start, mark_count);
                psf->m_instrument->loops[ji].end =
                    marker_to_position(paiff->markstr, psf->m_instrument->loops[ji].end, mark_count);
                psf->m_instrument->loops[ji].mode = SF_LOOP_FORWARD;
            };
        };

        /* The markers that correspond to loop positions can now be removed from cues struct */
        if (psf->m_cues.size() > (uint32_t)(psf->m_instrument->loop_count * 2))
        {
            for (uint32_t j = 0; j < psf->m_cues.size() - (uint32_t)(psf->m_instrument->loop_count * 2); j++)
            {
                /* This simply copies the information in cues above loop positions and writes it at current count instead */
				psf->m_cues[j].indx = psf->m_cues[j + psf->m_instrument->loop_count * 2].indx;
				psf->m_cues[j].position = psf->m_cues[j + psf->m_instrument->loop_count * 2].position;
				psf->m_cues[j].fcc_chunk = psf->m_cues[j + psf->m_instrument->loop_count * 2].fcc_chunk;
				psf->m_cues[j].chunk_start = psf->m_cues[j + psf->m_instrument->loop_count * 2].chunk_start;
				psf->m_cues[j].block_start = psf->m_cues[j + psf->m_instrument->loop_count * 2].block_start;
				psf->m_cues[j].sample_offset = psf->m_cues[j + psf->m_instrument->loop_count * 2].sample_offset;
                for (int str_index = 0; str_index < 256; str_index++)
					psf->m_cues[j].name[str_index] = psf->m_cues[j + psf->m_instrument->loop_count * 2].name[str_index];
            };
			size_t new_cues_size = psf->m_cues.size() - psf->m_instrument->loop_count * 2;
			psf->m_cues.resize(new_cues_size);
        }
        else
        {
            /* All the cues were in fact loop positions so we can actually remove the cues altogether */
			psf->m_cues.clear();
        }
    };

    if (psf->sf.channels < 1)
        return SFE_CHANNEL_COUNT_ZERO;

    if (psf->sf.channels > SF_MAX_CHANNELS)
        return SFE_CHANNEL_COUNT;

    if (!(found_chunk & HAVE_FORM))
        return SFE_AIFF_NO_FORM;

    if (!(found_chunk & HAVE_AIFF))
        return SFE_AIFF_COMM_NO_FORM;

    if (!(found_chunk & HAVE_COMM))
        return SFE_AIFF_SSND_NO_COMM;

    if (!psf->m_dataoffset)
        return SFE_AIFF_NO_DATA;

    return 0;
}

static int aiff_close(SF_PRIVATE *psf)
{
    struct AIFF_PRIVATE *paiff = (struct AIFF_PRIVATE *)psf->m_container_data;

    if (paiff != NULL && paiff->markstr != NULL)
    {
        free(paiff->markstr);
        paiff->markstr = NULL;
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        aiff_write_tailer(psf);
        aiff_write_header(psf, SF_TRUE);
    };

    return 0;
}

static int aiff_read_comm_chunk(SF_PRIVATE *psf, struct COMM_CHUNK *comm_fmt)
{
    BUF_UNION ubuf;
    ubuf.scbuf[0] = 0;

    /*
     * The COMM chunk has an int aligned to an odd word boundary. Some
     * procesors are not able to deal with this (ie bus fault) so we have
     * to take special care.
     */

    psf->binheader_readf("E242b", &(comm_fmt->numChannels), &(comm_fmt->numSampleFrames),
                        &(comm_fmt->sampleSize), &(comm_fmt->sampleRate),
                        SIGNED_SIZEOF(comm_fmt->sampleRate));

    if (comm_fmt->size > 0x10000 && (comm_fmt->size & 0xffff) == 0)
    {
        psf->log_printf(" COMM : %d (0x%x) *** should be ", comm_fmt->size, comm_fmt->size);
        comm_fmt->size = ENDSWAP_32(comm_fmt->size);
        psf->log_printf("%d (0x%x)\n", comm_fmt->size, comm_fmt->size);
    }
    else
    {
        psf->log_printf(" COMM : %d\n", comm_fmt->size);
    }

    if (comm_fmt->size == SIZEOF_AIFF_COMM)
    {
        comm_fmt->encoding = NONE_MARKER;
    }
    else if (comm_fmt->size == SIZEOF_AIFC_COMM_MIN)
    {
        psf->binheader_readf("Em", &(comm_fmt->encoding));
    }
    else if (comm_fmt->size >= SIZEOF_AIFC_COMM)
    {
        uint8_t encoding_len;
        unsigned read_len;

        psf->binheader_readf("Em1", &(comm_fmt->encoding), &encoding_len);

        if ((uint32_t)sizeof(ubuf.scbuf) < comm_fmt->size)
            comm_fmt->size = (uint32_t)sizeof(ubuf.scbuf);
        memset(ubuf.scbuf, 0, comm_fmt->size);
        read_len = comm_fmt->size - SIZEOF_AIFC_COMM + 1;
        psf->binheader_readf("b", ubuf.scbuf, read_len);
        ubuf.scbuf[read_len + 1] = 0;
    };

    int samplerate = tenbytefloat2int(comm_fmt->sampleRate);

    psf->log_printf("  Sample Rate : %d\n", samplerate);
    psf->log_printf("  Frames      : %u%s\n", comm_fmt->numSampleFrames,
                   (comm_fmt->numSampleFrames == 0 && psf->m_filelength > 104) ? " (Should not be 0)"
                                                                             : "");

    if (comm_fmt->numChannels < 1 || comm_fmt->numChannels > SF_MAX_CHANNELS)
    {
        psf->log_printf("  Channels    : %d (should be >= 1 and < %d)\n", comm_fmt->numChannels,
                       SF_MAX_CHANNELS);
        return SFE_CHANNEL_COUNT_BAD;
    };

    psf->log_printf("  Channels    : %d\n", comm_fmt->numChannels);

    /* Found some broken 'fl32' files with comm.samplesize == 16. Fix it here. */
    if ((comm_fmt->encoding == fl32_MARKER || comm_fmt->encoding == FL32_MARKER) &&
        comm_fmt->sampleSize != 32)
    {
        psf->log_printf("  Sample Size : %d (should be 32)\n", comm_fmt->sampleSize);
        comm_fmt->sampleSize = 32;
    }
    else if ((comm_fmt->encoding == fl64_MARKER || comm_fmt->encoding == FL64_MARKER) &&
             comm_fmt->sampleSize != 64)
    {
        psf->log_printf("  Sample Size : %d (should be 64)\n", comm_fmt->sampleSize);
        comm_fmt->sampleSize = 64;
    }
    else
    {
        psf->log_printf("  Sample Size : %d\n", comm_fmt->sampleSize);
    }

    int subformat = s_bitwidth_to_subformat(comm_fmt->sampleSize);

    psf->sf.samplerate = samplerate;
    psf->sf.frames = comm_fmt->numSampleFrames;
    psf->sf.channels = comm_fmt->numChannels;
    psf->m_bytewidth = BITWIDTH2BYTES(comm_fmt->sampleSize);

    psf->m_endian = SF_ENDIAN_BIG;

    switch (comm_fmt->encoding)
    {
    case NONE_MARKER:
        psf->sf.format = (SF_FORMAT_AIFF | subformat);
        break;

    case twos_MARKER:
    case in24_MARKER:
    case in32_MARKER:
        psf->sf.format = (SF_ENDIAN_BIG | SF_FORMAT_AIFF | subformat);
        break;

    case sowt_MARKER:
    case ni24_MARKER:
    case ni32_MARKER:
        psf->m_endian = SF_ENDIAN_LITTLE;
        psf->sf.format = (SF_ENDIAN_LITTLE | SF_FORMAT_AIFF | subformat);
        break;

    case fl32_MARKER:
    case FL32_MARKER:
        psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_FLOAT);
        break;

    case ulaw_MARKER:
    case ULAW_MARKER:
        psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_ULAW);
        break;

    case alaw_MARKER:
    case ALAW_MARKER:
        psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_ALAW);
        break;

    case fl64_MARKER:
    case FL64_MARKER:
        psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_DOUBLE);
        break;

    case raw_MARKER:
        psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_PCM_U8);
        break;

    case DWVW_MARKER:
        psf->sf.format = SF_FORMAT_AIFF;
        switch (comm_fmt->sampleSize)
        {
        case 12:
            psf->sf.format |= SF_FORMAT_DWVW_12;
            break;
        case 16:
            psf->sf.format |= SF_FORMAT_DWVW_16;
            break;
        case 24:
            psf->sf.format |= SF_FORMAT_DWVW_24;
            break;

        default:
            psf->sf.format |= SF_FORMAT_DWVW_N;
            break;
        };
        break;

    case GSM_MARKER:
        psf->sf.format = SF_FORMAT_AIFF;
        psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_GSM610);
        break;

    case ima4_MARKER:
        psf->m_endian = SF_ENDIAN_BIG;
        psf->sf.format = (SF_FORMAT_AIFF | SF_FORMAT_IMA_ADPCM);
        break;

    default:
        psf->log_printf("AIFC : Unimplemented format : %M\n", comm_fmt->encoding);
        return SFE_UNIMPLEMENTED;
    };

    if (!ubuf.scbuf[0])
        psf->log_printf("  Encoding    : %M\n", comm_fmt->encoding);
    else
        psf->log_printf("  Encoding    : %M => %s\n", comm_fmt->encoding, ubuf.scbuf);

    return 0;
}

static void aiff_rewrite_header(SF_PRIVATE *psf)
{
    /*
     * Assuming here that the header has already been written and just
     * needs to be corrected for new data length. That means that we
     * only change the length fields of the FORM and SSND chunks ;
     * everything else can be skipped over.
     */
    int k;

    psf->fseek(0, SEEK_SET);
    psf->fread(psf->m_header.ptr, psf->m_dataoffset, 1);

    psf->m_header.indx = 0;

    /* FORM chunk. */
    psf->binheader_writef("Etm8", BHWm(FORM_MARKER), BHW8(psf->m_filelength - 8));

    /* COMM chunk. */
    if ((k = psf_find_read_chunk_m32(&psf->m_rchunks, COMM_MARKER)) >= 0)
    {
        psf->m_header.indx = psf->m_rchunks.chunks[k].offset - 8;
        int comm_frames = (int)psf->sf.frames;
        int comm_size = psf->m_rchunks.chunks[k].len;
        psf->binheader_writef("Em42t4", BHWm(COMM_MARKER), BHW4(comm_size),
                             BHW2(psf->sf.channels), BHW4(comm_frames));
    };

    /* PEAK chunk. */
    if ((k = psf_find_read_chunk_m32(&psf->m_rchunks, PEAK_MARKER)) >= 0)
    {
        psf->m_header.indx = psf->m_rchunks.chunks[k].offset - 8;
        psf->binheader_writef("Em4", BHWm(PEAK_MARKER),
                             BHW4(AIFF_PEAK_CHUNK_SIZE(psf->sf.channels)));
        psf->binheader_writef("E44", BHW4(1), BHW4(time(NULL)));
        for (int ch = 0; ch < psf->sf.channels; ch++)
            psf->binheader_writef("Eft8", BHWf((float)psf->m_peak_info->peaks[ch].value),
                                 BHW8(psf->m_peak_info->peaks[ch].position));
    };

    /* SSND chunk. */
    if ((k = psf_find_read_chunk_m32(&psf->m_rchunks, SSND_MARKER)) >= 0)
    {
        psf->m_header.indx = psf->m_rchunks.chunks[k].offset - 8;
        psf->binheader_writef("Etm8", BHWm(SSND_MARKER),
                             BHW8(psf->m_datalength + SIZEOF_SSND_CHUNK));
    };

    /* Header mangling complete so write it out. */
    psf->fseek(0, SEEK_SET);
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    return;
}

static int aiff_write_header(SF_PRIVATE *psf, int calc_length)
{
    struct AIFF_PRIVATE *paiff = (struct AIFF_PRIVATE *)psf->m_container_data;
    if (!paiff)
        return SFE_INTERNAL;

    sf_count_t current = psf->ftell();

    int has_data = (current > psf->m_dataoffset) ? SF_TRUE : SF_FALSE;

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
        if (psf->m_dataend)
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;

        if (psf->m_bytewidth > 0)
            psf->sf.frames = psf->m_datalength / (psf->m_bytewidth * psf->sf.channels);
    };

    if (psf->m_mode == SFM_RDWR && psf->m_dataoffset > 0 && psf->m_rchunks.count > 0)
    {
        aiff_rewrite_header(psf);
        if (current > 0)
            psf->fseek(current, SEEK_SET);
        return 0;
    };

    int endian = SF_ENDIAN(psf->sf.format);
    if (CPU_IS_LITTLE_ENDIAN && endian == SF_ENDIAN_CPU)
        endian = SF_ENDIAN_LITTLE;

    /* Standard value here. */
    int16_t bit_width = psf->m_bytewidth * 8;
    uint32_t comm_frames = (psf->sf.frames > 0xFFFFFFFF) ? 0xFFFFFFFF : (uint32_t)psf->sf.frames;
    uint32_t comm_type, comm_size, comm_encoding;

    switch (SF_CODEC(psf->sf.format) | endian)
    {
    case SF_FORMAT_PCM_S8 | SF_ENDIAN_BIG:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = twos_MARKER;
        break;

    case SF_FORMAT_PCM_S8 | SF_ENDIAN_LITTLE:
        psf->m_endian = SF_ENDIAN_LITTLE;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = sowt_MARKER;
        break;

    case SF_FORMAT_PCM_16 | SF_ENDIAN_BIG:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = twos_MARKER;
        break;

    case SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE:
        psf->m_endian = SF_ENDIAN_LITTLE;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = sowt_MARKER;
        break;

    case SF_FORMAT_PCM_24 | SF_ENDIAN_BIG:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = in24_MARKER;
        break;

    case SF_FORMAT_PCM_24 | SF_ENDIAN_LITTLE:
        psf->m_endian = SF_ENDIAN_LITTLE;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = ni24_MARKER;
        break;

    case SF_FORMAT_PCM_32 | SF_ENDIAN_BIG:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = in32_MARKER;
        break;

    case SF_FORMAT_PCM_32 | SF_ENDIAN_LITTLE:
        psf->m_endian = SF_ENDIAN_LITTLE;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = ni32_MARKER;
        break;

    case SF_FORMAT_PCM_S8: /* SF_ENDIAN_FILE */
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFF_MARKER;
        comm_size = SIZEOF_AIFF_COMM;
        comm_encoding = 0;
        break;

    case SF_FORMAT_FLOAT: /* Big endian floating point. */
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = FL32_MARKER; /* Use 'FL32' because its easier to read. */
        break;

    case SF_FORMAT_DOUBLE: /* Big endian double precision floating point. */
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = FL64_MARKER; /* Use 'FL64' because its easier to read. */
        break;

    case SF_FORMAT_ULAW:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = ulaw_MARKER;
        break;

    case SF_FORMAT_ALAW:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = alaw_MARKER;
        break;

    case SF_FORMAT_PCM_U8:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = raw_MARKER;
        break;

    case SF_FORMAT_DWVW_12:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = DWVW_MARKER;

        /* Override standard value here.*/
        bit_width = 12;
        break;

    case SF_FORMAT_DWVW_16:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = DWVW_MARKER;

        /* Override standard value here.*/
        bit_width = 16;
        break;

    case SF_FORMAT_DWVW_24:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = DWVW_MARKER;

        /* Override standard value here.*/
        bit_width = 24;
        break;

    case SF_FORMAT_GSM610:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = GSM_MARKER;

        /* Override standard value here.*/
        bit_width = 16;
        break;

    case SF_FORMAT_IMA_ADPCM:
        psf->m_endian = SF_ENDIAN_BIG;
        comm_type = AIFC_MARKER;
        comm_size = SIZEOF_AIFC_COMM;
        comm_encoding = ima4_MARKER;

        /* Override standard value here.*/
        bit_width = 16;
        comm_frames = (uint32_t)(psf->sf.frames / AIFC_IMA4_SAMPLES_PER_BLOCK);
        break;

    default:
        return SFE_BAD_OPEN_FORMAT;
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    psf->binheader_writef("Etm8", BHWm(FORM_MARKER), BHW8(psf->m_filelength - 8));

    /* Write AIFF/AIFC marker and COM chunk. */
    if (comm_type == AIFC_MARKER)
        /* AIFC must have an FVER chunk. */
        psf->binheader_writef("Emm44", BHWm(comm_type), BHWm(FVER_MARKER), BHW4(4),
                             BHW4(0xA2805140));
    else
        psf->binheader_writef("Em", BHWm(comm_type));

    paiff->comm_offset = psf->m_header.indx - 8;

    uint8_t comm_sample_rate[10];
    memset(comm_sample_rate, 0, sizeof(comm_sample_rate));
    uint2tenbytefloat(psf->sf.samplerate, comm_sample_rate);

    psf->binheader_writef("Em42t42", BHWm(COMM_MARKER), BHW4(comm_size), BHW2(psf->sf.channels),
                         BHW4(comm_frames), BHW2(bit_width));
    psf->binheader_writef("b", BHWv(comm_sample_rate), BHWz(sizeof(comm_sample_rate)));

    /* AIFC chunks have some extra data. */
    uint8_t comm_zero_bytes[2] = { 0, 0 };
    if (comm_type == AIFC_MARKER)
        psf->binheader_writef("mb", BHWm(comm_encoding), BHWv(comm_zero_bytes),
                             BHWz(sizeof(comm_zero_bytes)));

    if (!psf->m_channel_map.empty() && paiff->chanmap_tag)
        psf->binheader_writef("Em4444", BHWm(CHAN_MARKER), BHW4(12), BHW4(paiff->chanmap_tag),
                             BHW4(0), BHW4(0));

    /* Check if there's a INST chunk to write */
    if (psf->m_instrument != NULL && !psf->m_cues.empty())
    {
        /*
         * Huge chunk of code removed here because it had egregious errors that 
         * were not detected by either the compiler or the tests. It was found
         * when updating the way psf_binheader_writef works.
         */
    }
    else if (psf->m_instrument == NULL && !psf->m_cues.empty())
    {
        /* There are cues but no loops */
        size_t totalStringLength = 0;

        /* Here we count how many bytes will the pascal strings need */
        for (auto &cue: psf->m_cues) {
            /* We'll count the first byte also of every pascal string */
            size_t stringLength = strlen(cue.name) + 1;
            totalStringLength += stringLength + (stringLength % 2 == 0 ? 0 : 1);
        };

        psf->binheader_writef("Em42", BHWm(MARK_MARKER),
                             BHW4(2 + psf->m_cues.size() * (2 + 4) + totalStringLength),
                             BHW2(psf->m_cues.size()));

		for (auto &cue : psf->m_cues) {
			psf->binheader_writef("E24p", BHW2(cue.indx), BHW4(cue.sample_offset), BHWp(cue.name));
		}
    };

    if (psf->m_strings.flags & SF_STR_LOCATE_START)
        aiff_write_strings(psf, SF_STR_LOCATE_START);

    if (psf->m_peak_info != NULL && psf->m_peak_info->peak_loc == SF_PEAK_START)
    {
        psf->binheader_writef("Em4", BHWm(PEAK_MARKER),
                             BHW4(AIFF_PEAK_CHUNK_SIZE(psf->sf.channels)));
        psf->binheader_writef("E44", BHW4(1), BHW4(time(NULL)));
        for (int k = 0; k < psf->sf.channels; k++)
        {
            psf->binheader_writef("Eft8", BHWf((float)psf->m_peak_info->peaks[k].value),
                                 BHW8(psf->m_peak_info->peaks[k].position));
        }
    };

    /* Write custom headers. */
    for (uint32_t uk = 0; uk < psf->m_wchunks.used; uk++)
    {
        psf->binheader_writef("Em4b", BHWm(psf->m_wchunks.chunks[uk].mark32),
                             BHW4(psf->m_wchunks.chunks[uk].len), BHWv(psf->m_wchunks.chunks[uk].data),
                             BHWz(psf->m_wchunks.chunks[uk].len));
    }

    /* Write SSND chunk. */
    paiff->ssnd_offset = psf->m_header.indx;
    psf->binheader_writef("Etm844", BHWm(SSND_MARKER),
                         BHW8(psf->m_datalength + SIZEOF_SSND_CHUNK), BHW4(0), BHW4(0));

    /* Header construction complete so write it out. */
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    if (has_data && psf->m_dataoffset != psf->m_header.indx)
        return psf->m_error = SFE_INTERNAL;

    psf->m_dataoffset = psf->m_header.indx;

    if (!has_data)
        psf->fseek(psf->m_dataoffset, SEEK_SET);
    else if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int aiff_write_tailer(SF_PRIVATE *psf)
{
    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;

    psf->m_dataend = psf->fseek(0, SEEK_END);

    /* Make sure tailer data starts at even byte offset. Pad if necessary. */
    if (psf->m_dataend % 2 == 1)
    {
        psf->fwrite(psf->m_header.ptr, 1, 1);
        psf->m_dataend++;
    };

    if (psf->m_peak_info != NULL && psf->m_peak_info->peak_loc == SF_PEAK_END)
    {
        psf->binheader_writef("Em4", BHWm(PEAK_MARKER),
                             BHW4(AIFF_PEAK_CHUNK_SIZE(psf->sf.channels)));
        psf->binheader_writef("E44", BHW4(1), BHW4(time(NULL)));
        for (int k = 0; k < psf->sf.channels; k++)
            psf->binheader_writef("Eft8", BHWf((float)psf->m_peak_info->peaks[k].value),
                                 BHW8(psf->m_peak_info->peaks[k].position));
    };

    if (psf->m_strings.flags & SF_STR_LOCATE_END)
        aiff_write_strings(psf, SF_STR_LOCATE_END);

    /* Write the tailer. */
    if (psf->m_header.indx > 0)
        psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    return 0;
}

static void aiff_write_strings(SF_PRIVATE *psf, int location)
{
    for (int k = 0; k < SF_MAX_STRINGS; k++)
    {
        if (psf->m_strings.data[k].type == 0)
            break;

        if (psf->m_strings.data[k].flags != location)
            continue;

        switch (psf->m_strings.data[k].type)
        {
        case SF_STR_SOFTWARE:
        {
            size_t slen = strlen(psf->m_strings.storage + psf->m_strings.data[k].offset);
            psf->binheader_writef("Em4mb", BHWm(APPL_MARKER), BHW4(slen + 4), BHWm(m3ga_MARKER),
                                 BHWv(psf->m_strings.storage + psf->m_strings.data[k].offset),
                                 BHWz(slen + (slen & 1)));
        }
            break;

        case SF_STR_TITLE:
            psf->binheader_writef("EmS", BHWm(NAME_MARKER),
                                 BHWS(psf->m_strings.storage + psf->m_strings.data[k].offset));
            break;

        case SF_STR_COPYRIGHT:
            psf->binheader_writef("EmS", BHWm(c_MARKER),
                                 BHWS(psf->m_strings.storage + psf->m_strings.data[k].offset));
            break;

        case SF_STR_ARTIST:
            psf->binheader_writef("EmS", BHWm(AUTH_MARKER),
                                 BHWS(psf->m_strings.storage + psf->m_strings.data[k].offset));
            break;

        case SF_STR_COMMENT:
            psf->binheader_writef("EmS", BHWm(ANNO_MARKER),
                                 BHWS(psf->m_strings.storage + psf->m_strings.data[k].offset));
            break;
        };
    };

    return;
}

static size_t aiff_command(SF_PRIVATE *psf, int command, void *UNUSED(data), size_t UNUSED(datasize))
{
    struct AIFF_PRIVATE *paiff = (struct AIFF_PRIVATE *)psf->m_container_data;

    if (!paiff)
        return SFE_INTERNAL;

    switch (command)
    {
    case SFC_SET_CHANNEL_MAP_INFO:
        paiff->chanmap_tag = aiff_caf_find_channel_layout_tag(psf->m_channel_map.data(), psf->sf.channels);
        return (paiff->chanmap_tag != 0);

    default:
        break;
    };

    return 0;
}

static const char *get_loop_mode_str(int16_t mode)
{
    switch (mode)
    {
    case 0:
        return "none";
    case 1:
        return "forward";
    case 2:
        return "backward";
    };

    return "*** unknown";
}

static int16_t get_loop_mode(int16_t mode)
{
    switch (mode)
    {
    case 0:
        return SF_LOOP_NONE;
    case 1:
        return SF_LOOP_FORWARD;
    case 2:
        return SF_LOOP_BACKWARD;
    };

    return SF_LOOP_NONE;
}

/*
 * Rough hack at converting from 80 bit IEEE float in AIFF header to an int and
 * back again. It assumes that all sample rates are between 1 and 800MHz, which
 * should be OK as other sound file formats use a 32 bit integer to store sample
 * rate.
 * There is another (probably better) version in the source code to the SoX but it
 * has a copyright which probably prevents it from being allowable as GPL/LGPL.
 */

static int tenbytefloat2int(uint8_t *bytes)
{
    if (bytes[0] & 0x80) /* Negative number. */
        return 0;

    if (bytes[0] <= 0x3F) /* Less than 1. */
        return 1;

    if (bytes[0] > 0x40) /* Way too big. */
        return 0x4000000;

    if (bytes[0] == 0x40 && bytes[1] > 0x1C) /* Too big. */
        return 800000000;

    /* Ok, can handle it. */

    int val = (bytes[2] << 23) | (bytes[3] << 15) | (bytes[4] << 7) | (bytes[5] >> 1);

    val >>= (29 - bytes[1]);

    return val;
}

static void uint2tenbytefloat(uint32_t num, uint8_t *bytes)
{
    if (num <= 1)
    {
        bytes[0] = 0x3F;
        bytes[1] = 0xFF;
        bytes[2] = 0x80;
        return;
    };

    bytes[0] = 0x40;
    uint32_t mask = 0x40000000;

    if (num >= mask)
    {
        bytes[1] = 0x1D;
        return;
    };

    int count;

    for (count = 0; count < 32; count++)
    {
        if (num & mask)
            break;
        mask >>= 1;
    };

    num = count < 31 ? num << (count + 1) : 0;
    bytes[1] = 29 - count;
    bytes[2] = (num >> 24) & 0xFF;
    bytes[3] = (num >> 16) & 0xFF;
    bytes[4] = (num >> 8) & 0xFF;
    bytes[5] = num & 0xFF;
}

static int aiff_read_basc_chunk(SF_PRIVATE *psf, int datasize)
{
    const char *type_str;
    struct basc_CHUNK bc;
    int count;

    count = psf->binheader_readf("E442", &bc.version, &bc.numBeats, &bc.rootNote);
    count += psf->binheader_readf("E222", &bc.scaleType, &bc.sigNumerator, &bc.sigDenominator);
    count += psf->binheader_readf("E2", &bc.loopType, datasize - sizeof(bc));
	psf->binheader_seekf(datasize - sizeof(bc), SF_SEEK_CUR);
	count += datasize - sizeof(bc);

    psf->log_printf("  Version ? : %u\n  Num Beats : %u\n  Root Note : 0x%x\n", bc.version,
                   bc.numBeats, bc.rootNote);

    switch (bc.scaleType)
    {
    case basc_SCALE_MINOR:
        type_str = "MINOR";
        break;
    case basc_SCALE_MAJOR:
        type_str = "MAJOR";
        break;
    case basc_SCALE_NEITHER:
        type_str = "NEITHER";
        break;
    case basc_SCALE_BOTH:
        type_str = "BOTH";
        break;
    default:
        type_str = "!!WRONG!!";
        break;
    };

    psf->log_printf("  ScaleType : 0x%x (%s)\n", bc.scaleType, type_str);
    psf->log_printf("  Time Sig  : %d/%d\n", bc.sigNumerator, bc.sigDenominator);

    switch (bc.loopType)
    {
    case basc_TYPE_ONE_SHOT:
        type_str = "One Shot";
        break;
    case basc_TYPE_LOOP:
        type_str = "Loop";
        break;
    default:
        type_str = "!!WRONG!!";
        break;
    };

    psf->log_printf("  Loop Type : 0x%x (%s)\n", bc.loopType, type_str);

    if ((psf->m_loop_info = (SF_LOOP_INFO *)calloc(1, sizeof(SF_LOOP_INFO))) == NULL)
        return SFE_MALLOC_FAILED;

    psf->m_loop_info->time_sig_num = bc.sigNumerator;
    psf->m_loop_info->time_sig_den = bc.sigDenominator;
    psf->m_loop_info->loop_mode = (bc.loopType == basc_TYPE_ONE_SHOT) ? SF_LOOP_NONE : SF_LOOP_FORWARD;
    psf->m_loop_info->num_beats = bc.numBeats;

    /* Can always be recalculated from other known fields. */
    psf->m_loop_info->bpm = (float)((1.0 / psf->sf.frames) * psf->sf.samplerate *
                                  ((bc.numBeats * 4.0) / bc.sigDenominator) * 60.0);
    psf->m_loop_info->root_key = bc.rootNote;

	if (count < datasize)
		psf->binheader_seekf(datasize - count, SF_SEEK_CUR);

    return 0;
}

static int aiff_read_chanmap(SF_PRIVATE *psf, unsigned dword)
{
    unsigned channel_bitmap, channel_decriptions, bytesread;
    int layout_tag;

    bytesread = psf->binheader_readf("444", &layout_tag, &channel_bitmap, &channel_decriptions);

    const AIFF_CAF_CHANNEL_MAP *map_info = aiff_caf_of_channel_layout_tag(layout_tag);
    if (!map_info)
        return 0;

    psf->log_printf("  Tag    : %x\n", layout_tag);
    if (map_info)
        psf->log_printf("  Layout : %s\n", map_info->name);

	if (bytesread < dword)
		psf->binheader_seekf(dword - bytesread, SF_SEEK_CUR);

    if (map_info->channel_map != NULL)
    {
        size_t chanmap_size = std::min(psf->sf.channels, layout_tag & 0xffff) * sizeof(psf->m_channel_map[0]);

        psf->m_channel_map.resize(chanmap_size);
        memcpy(psf->m_channel_map.data(), map_info->channel_map, sizeof(int) * chanmap_size);
    };

    return 0;
}

static int aiff_set_chunk(SF_PRIVATE *psf, const SF_CHUNK_INFO *chunk_info)
{
    return psf_save_write_chunk(&psf->m_wchunks, chunk_info);
}

static SF_CHUNK_ITERATOR *aiff_next_chunk_iterator(SF_PRIVATE *psf, SF_CHUNK_ITERATOR *iterator)
{
    return psf_next_chunk_iterator(&psf->m_rchunks, iterator);
}

static int aiff_get_chunk_size(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator, SF_CHUNK_INFO *chunk_info)
{
    int indx = psf_find_read_chunk_iterator(&psf->m_rchunks, iterator);
    if (indx < 0)
        return SFE_UNKNOWN_CHUNK;

    chunk_info->datalen = psf->m_rchunks.chunks[indx].len;

    return SFE_NO_ERROR;
}

static int aiff_get_chunk_data(SF_PRIVATE *psf, const SF_CHUNK_ITERATOR *iterator, SF_CHUNK_INFO *chunk_info)
{
    sf_count_t pos;
    int indx = psf_find_read_chunk_iterator(&psf->m_rchunks, iterator);
    if (indx < 0)
        return SFE_UNKNOWN_CHUNK;

    if (!chunk_info->data)
        return SFE_BAD_CHUNK_DATA_PTR;

    chunk_info->id_size = psf->m_rchunks.chunks[indx].id_size;
    memcpy(chunk_info->id, psf->m_rchunks.chunks[indx].id, sizeof(chunk_info->id) / sizeof(*chunk_info->id));

    pos = psf->ftell();
    psf->fseek(psf->m_rchunks.chunks[indx].offset, SEEK_SET);
    psf->fread(chunk_info->data, std::min(chunk_info->datalen, psf->m_rchunks.chunks[indx].len), 1);
    psf->fseek(pos, SEEK_SET);

    return SFE_NO_ERROR;
}
