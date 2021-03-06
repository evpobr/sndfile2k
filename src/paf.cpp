/*
** Copyright (C) 1999-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

#define FAP_MARKER (MAKE_MARKER('f', 'a', 'p', ' '))
#define PAF_MARKER (MAKE_MARKER(' ', 'p', 'a', 'f'))

#define PAF_HEADER_LENGTH (2048)

#define PAF24_SAMPLES_PER_BLOCK (10)
#define PAF24_BLOCK_SIZE (32)

typedef struct
{
    int version;
    int endianness;
    int samplerate;
    int format;
    int channels;
    int source;
} PAF_FMT;

typedef struct
{
    int max_blocks, channels, blocksize;
    int read_block, write_block, read_count, write_count;
    sf_count_t sample_count;
    int *samples;
    int *block;
    int data[];
} PAF24_PRIVATE;

static int paf24_init(SndFile *psf);

static int paf_read_header(SndFile *psf);
static int paf_write_header(SndFile *psf, int calc_length);

static size_t paf24_read_s(SndFile *psf, short *ptr, size_t len);
static size_t paf24_read_i(SndFile *psf, int *ptr, size_t len);
static size_t paf24_read_f(SndFile *psf, float *ptr, size_t len);
static size_t paf24_read_d(SndFile *psf, double *ptr, size_t len);

static size_t paf24_write_s(SndFile *psf, const short *ptr, size_t len);
static size_t paf24_write_i(SndFile *psf, const int *ptr, size_t len);
static size_t paf24_write_f(SndFile *psf, const float *ptr, size_t len);
static size_t paf24_write_d(SndFile *psf, const double *ptr, size_t len);

static sf_count_t paf24_seek(SndFile *psf, int mode, sf_count_t offset);

enum
{
    PAF_PCM_16 = 0,
    PAF_PCM_24 = 1,
    PAF_PCM_S8 = 2
};

int paf_open(SndFile *psf)
{
    int subformat, error, endian;

    psf->m_dataoffset = PAF_HEADER_LENGTH;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = paf_read_header(psf)))
            return error;
    };

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_PAF)
            return SFE_BAD_OPEN_FORMAT;

        endian = SF_ENDIAN(psf->sf.format);

        /* PAF is by default big endian. */
        psf->m_endian = SF_ENDIAN_BIG;

        if (endian == SF_ENDIAN_LITTLE || (CPU_IS_LITTLE_ENDIAN && (endian == SF_ENDIAN_CPU)))
            psf->m_endian = SF_ENDIAN_LITTLE;

        if ((error = paf_write_header(psf, SF_FALSE)))
            return error;

        psf->write_header = paf_write_header;
    };

    switch (subformat)
    {
    case SF_FORMAT_PCM_S8:
        psf->m_bytewidth = 1;
        error = pcm_init(psf);
        break;

    case SF_FORMAT_PCM_16:
        psf->m_bytewidth = 2;
        error = pcm_init(psf);
        break;

    case SF_FORMAT_PCM_24:
        /* No bytewidth because of whacky 24 bit encoding. */
        error = paf24_init(psf);
        break;

    default:
        return SFE_PAF_UNKNOWN_FORMAT;
    };

    return error;
}

static int paf_read_header(SndFile *psf)
{
    PAF_FMT paf_fmt;
    int marker;

    if (psf->m_filelength < PAF_HEADER_LENGTH)
        return SFE_PAF_SHORT_HEADER;

    memset(&paf_fmt, 0, sizeof(paf_fmt));
	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("m", &marker);

    psf->log_printf("Signature   : '%M'\n", marker);

    if (marker == PAF_MARKER)
    {
        psf->binheader_readf("E444444", &(paf_fmt.version), &(paf_fmt.endianness),
                            &(paf_fmt.samplerate), &(paf_fmt.format), &(paf_fmt.channels),
                            &(paf_fmt.source));
    }
    else if (marker == FAP_MARKER)
    {
        psf->binheader_readf("e444444", &(paf_fmt.version), &(paf_fmt.endianness),
                            &(paf_fmt.samplerate), &(paf_fmt.format), &(paf_fmt.channels),
                            &(paf_fmt.source));
    }
    else
        return SFE_PAF_NO_MARKER;

    psf->log_printf("Version     : %d\n", paf_fmt.version);

    if (paf_fmt.version != 0)
    {
        psf->log_printf("*** Bad version number. should be zero.\n");
        return SFE_PAF_VERSION;
    };

    psf->log_printf("Sample Rate : %d\n", paf_fmt.samplerate);
    psf->log_printf("Channels    : %d\n", paf_fmt.channels);

    psf->log_printf("Endianness  : %d => ", paf_fmt.endianness);
    if (paf_fmt.endianness)
    {
        psf->log_printf("Little\n", paf_fmt.endianness);
        psf->m_endian = SF_ENDIAN_LITTLE;
    }
    else
    {
        psf->log_printf("Big\n", paf_fmt.endianness);
        psf->m_endian = SF_ENDIAN_BIG;
    };

    if (paf_fmt.channels < 1 || paf_fmt.channels > SF_MAX_CHANNELS)
        return SFE_PAF_BAD_CHANNELS;

    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

	psf->binheader_seekf(psf->m_dataoffset, SF_SEEK_SET);

    psf->sf.samplerate = paf_fmt.samplerate;
    psf->sf.channels = paf_fmt.channels;

    /* Only fill in type major. */
    psf->sf.format = SF_FORMAT_PAF;

    psf->log_printf("Format      : %d => ", paf_fmt.format);

    /* PAF is by default big endian. */
    psf->sf.format |= paf_fmt.endianness ? SF_ENDIAN_LITTLE : SF_ENDIAN_BIG;

    switch (paf_fmt.format)
    {
    case PAF_PCM_S8:
        psf->log_printf("8 bit linear PCM\n");
        psf->m_bytewidth = 1;

        psf->sf.format |= SF_FORMAT_PCM_S8;

        psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;
        psf->sf.frames = psf->m_datalength / psf->m_blockwidth;
        break;

    case PAF_PCM_16:
        psf->log_printf("16 bit linear PCM\n");
        psf->m_bytewidth = 2;

        psf->sf.format |= SF_FORMAT_PCM_16;

        psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;
        psf->sf.frames = psf->m_datalength / psf->m_blockwidth;
        break;

    case PAF_PCM_24:
        psf->log_printf("24 bit linear PCM\n");
        psf->m_bytewidth = 3;

        psf->sf.format |= SF_FORMAT_PCM_24;

        psf->m_blockwidth = 0;
        psf->sf.frames =
            PAF24_SAMPLES_PER_BLOCK * psf->m_datalength / (PAF24_BLOCK_SIZE * psf->sf.channels);
        break;

    default:
        psf->log_printf("Unknown\n");
        return SFE_PAF_UNKNOWN_FORMAT;
        break;
    };

    psf->log_printf("Source      : %d => ", paf_fmt.source);

    switch (paf_fmt.source)
    {
    case 1:
        psf->log_printf("Analog Recording\n");
        break;
    case 2:
        psf->log_printf("Digital Transfer\n");
        break;
    case 3:
        psf->log_printf("Multi-track Mixdown\n");
        break;
    case 5:
        psf->log_printf("Audio Resulting From DSP Processing\n");
        break;
    default:
        psf->log_printf("Unknown\n");
        break;
    };

    return 0;
}

static int paf_write_header(SndFile *psf, int UNUSED(calc_length))
{
    int paf_format;

    /* PAF header already written so no need to re-write. */
    if (psf->ftell() >= PAF_HEADER_LENGTH)
        return 0;

    psf->m_dataoffset = PAF_HEADER_LENGTH;

    switch (SF_CODEC(psf->sf.format))
    {
    case SF_FORMAT_PCM_S8:
        paf_format = PAF_PCM_S8;
        break;

    case SF_FORMAT_PCM_16:
        paf_format = PAF_PCM_16;
        break;

    case SF_FORMAT_PCM_24:
        paf_format = PAF_PCM_24;
        break;

    default:
        return SFE_PAF_UNKNOWN_FORMAT;
    };

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;

    if (psf->m_endian == SF_ENDIAN_BIG)
    {
        /* Marker, version, endianness, samplerate */
        psf->binheader_writef("Em444", BHWm(PAF_MARKER), BHW4(0), BHW4(0),
                             BHW4(psf->sf.samplerate));
        /* format, channels, source */
        psf->binheader_writef("E444", BHW4(paf_format), BHW4(psf->sf.channels), BHW4(0));
    }
    else if (psf->m_endian == SF_ENDIAN_LITTLE)
    {
        /* Marker, version, endianness, samplerate */
        psf->binheader_writef("em444", BHWm(FAP_MARKER), BHW4(0), BHW4(1),
                             BHW4(psf->sf.samplerate));
        /* format, channels, source */
        psf->binheader_writef("e444", BHW4(paf_format), BHW4(psf->sf.channels), BHW4(0));
    };

    /* Zero fill to dataoffset. */
    psf->binheader_writef("z", BHWz((size_t)(psf->m_dataoffset - psf->m_header.indx)));

    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    return psf->m_error;
}

/*
 * 24 bit PAF files have a really weird encoding.
 * For a mono file, 10 samples (each being 3 bytes) are packed into a 32 byte
 * block. The 8 ints in this 32 byte block are then endian swapped (as ints)
 * if necessary before being written to disk.
 * For a stereo file, blocks of 10 samples from the same channel are encoded
 * into 32 bytes as for the mono case. The 32 byte blocks are then interleaved
 * on disk.
 * Reading has to reverse the above process :-).
 * Weird!!!
 *
 * The code below attempts to gain efficiency while maintaining readability.
 */

static int paf24_read_block(SndFile *psf, PAF24_PRIVATE *ppaf24);
static int paf24_write_block(SndFile *psf, PAF24_PRIVATE *ppaf24);
static int paf24_close(SndFile *psf);

static int paf24_init(SndFile *psf)
{
    PAF24_PRIVATE *ppaf24;
    int paf24size;

    paf24size = sizeof(PAF24_PRIVATE) +
                psf->sf.channels * (PAF24_BLOCK_SIZE + PAF24_SAMPLES_PER_BLOCK * sizeof(int));

    /*
	 * Not exatly sure why this needs to be here but the tests
	 * fail without it.
	 */
    psf->m_last_op = 0;

    if (!(psf->m_codec_data = calloc(1, paf24size)))
        return SFE_MALLOC_FAILED;

    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    ppaf24->channels = psf->sf.channels;
    ppaf24->samples = ppaf24->data;
    ppaf24->block = ppaf24->data + PAF24_SAMPLES_PER_BLOCK * ppaf24->channels;

    ppaf24->blocksize = PAF24_BLOCK_SIZE * ppaf24->channels;

    if (psf->m_mode == SFM_READ || psf->m_mode == SFM_RDWR)
    {
        paf24_read_block(psf, ppaf24); /* Read first block. */

        psf->read_short = paf24_read_s;
        psf->read_int = paf24_read_i;
        psf->read_float = paf24_read_f;
        psf->read_double = paf24_read_d;
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        psf->write_short = paf24_write_s;
        psf->write_int = paf24_write_i;
        psf->write_float = paf24_write_f;
        psf->write_double = paf24_write_d;
    };

    psf->seek_from_start = paf24_seek;
    psf->container_close = paf24_close;

    psf->m_filelength = psf->get_filelen();
    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

    if (psf->m_datalength % PAF24_BLOCK_SIZE)
    {
        if (psf->m_mode == SFM_READ)
            psf->log_printf("*** Warning : file seems to be truncated.\n");
        ppaf24->max_blocks = psf->m_datalength / ppaf24->blocksize + 1;
    }
    else
        ppaf24->max_blocks = psf->m_datalength / ppaf24->blocksize;

    ppaf24->read_block = 0;
    if (psf->m_mode == SFM_RDWR)
        ppaf24->write_block = ppaf24->max_blocks;
    else
        ppaf24->write_block = 0;

    psf->sf.frames = PAF24_SAMPLES_PER_BLOCK * ppaf24->max_blocks;
    ppaf24->sample_count = psf->sf.frames;

    return 0;
}

static sf_count_t paf24_seek(SndFile *psf, int mode, sf_count_t offset)
{
    PAF24_PRIVATE *ppaf24;
    int newblock, newsample;

    if (psf->m_codec_data == NULL)
    {
        psf->m_error = SFE_INTERNAL;
        return PSF_SEEK_ERROR;
    };

    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    if (mode == SFM_READ && ppaf24->write_count > 0)
        paf24_write_block(psf, ppaf24);

    newblock = offset / PAF24_SAMPLES_PER_BLOCK;
    newsample = offset % PAF24_SAMPLES_PER_BLOCK;

    switch (mode)
    {
    case SFM_READ:
        if (psf->m_last_op == SFM_WRITE && ppaf24->write_count)
            paf24_write_block(psf, ppaf24);

        psf->fseek(psf->m_dataoffset + newblock * ppaf24->blocksize, SEEK_SET);
        ppaf24->read_block = newblock;
        paf24_read_block(psf, ppaf24);
        ppaf24->read_count = newsample;
        break;

    case SFM_WRITE:
        if (offset > ppaf24->sample_count)
        {
            psf->m_error = SFE_BAD_SEEK;
            return PSF_SEEK_ERROR;
        };

        if (psf->m_last_op == SFM_WRITE && ppaf24->write_count)
            paf24_write_block(psf, ppaf24);

        psf->fseek(psf->m_dataoffset + newblock * ppaf24->blocksize, SEEK_SET);
        ppaf24->write_block = newblock;
        paf24_read_block(psf, ppaf24);
        ppaf24->write_count = newsample;
        break;

    default:
        psf->m_error = SFE_BAD_SEEK;
        return PSF_SEEK_ERROR;
    };

    return newblock * PAF24_SAMPLES_PER_BLOCK + newsample;
}

static int paf24_close(SndFile *psf)
{
    PAF24_PRIVATE *ppaf24;

    if (psf->m_codec_data == NULL)
        return 0;

    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if (ppaf24->write_count > 0)
            paf24_write_block(psf, ppaf24);
    };

    return 0;
}

static int paf24_read_block(SndFile *psf, PAF24_PRIVATE *ppaf24)
{
    int k, channel;
    unsigned char *cptr;

    ppaf24->read_block++;
    ppaf24->read_count = 0;

    if (ppaf24->read_block * PAF24_SAMPLES_PER_BLOCK > ppaf24->sample_count)
    {
        memset(ppaf24->samples, 0, PAF24_SAMPLES_PER_BLOCK * ppaf24->channels);
        return 1;
    };

    /* Read the block. */
    if ((k = psf->fread(ppaf24->block, 1, ppaf24->blocksize)) != ppaf24->blocksize)
        psf->log_printf("*** Warning : short read (%d != %d).\n", k, ppaf24->blocksize);

    /* Do endian swapping if necessary. */
    if ((CPU_IS_BIG_ENDIAN && psf->m_endian == SF_ENDIAN_LITTLE) ||
        (CPU_IS_LITTLE_ENDIAN && psf->m_endian == SF_ENDIAN_BIG))
        endswap_int_array(ppaf24->block, 8 * ppaf24->channels);

    /* Unpack block. */
    for (k = 0; k < PAF24_SAMPLES_PER_BLOCK * ppaf24->channels; k++)
    {
        channel = k % ppaf24->channels;
        cptr = ((unsigned char *)ppaf24->block) + PAF24_BLOCK_SIZE * channel +
               3 * (k / ppaf24->channels);
        ppaf24->samples[k] = (cptr[0] << 8) | (cptr[1] << 16) | (((unsigned)cptr[2]) << 24);
    };

    return 1;
}

static size_t paf24_read(SndFile *psf, PAF24_PRIVATE *ppaf24, int *ptr, size_t len)
{
    size_t count, total = 0;

    while (total < len)
    {
        if (ppaf24->read_block * PAF24_SAMPLES_PER_BLOCK >= ppaf24->sample_count)
        {
            memset(&(ptr[total]), 0, (len - total) * sizeof(int));
            return total;
        };

        if (ppaf24->read_count >= PAF24_SAMPLES_PER_BLOCK)
            paf24_read_block(psf, ppaf24);

        count = (PAF24_SAMPLES_PER_BLOCK - ppaf24->read_count) * ppaf24->channels;
        count = (len - total > count) ? count : len - total;

        memcpy(&(ptr[total]), &(ppaf24->samples[ppaf24->read_count * ppaf24->channels]),
               count * sizeof(int));
        total += count;
        ppaf24->read_count += count / ppaf24->channels;
    };

    return total;
}

static size_t paf24_read_s(SndFile *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    PAF24_PRIVATE *ppaf24;
    int *iptr;
    size_t k, bufferlen, readcount, count;
    size_t total = 0;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    iptr = ubuf.ibuf;
    bufferlen = ARRAY_LEN(ubuf.ibuf);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = paf24_read(psf, ppaf24, iptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = iptr[k] >> 16;
        total += count;
        len -= readcount;
    };
    return total;
}

static size_t paf24_read_i(SndFile *psf, int *ptr, size_t len)
{
    PAF24_PRIVATE *ppaf24;
    size_t total;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    total = paf24_read(psf, ppaf24, ptr, len);

    return total;
}

static size_t paf24_read_f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    PAF24_PRIVATE *ppaf24;
    int *iptr;
    size_t k, bufferlen, readcount, count;
    size_t total = 0;
    float normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? (1.0 / 0x80000000) : (1.0 / 0x100));

    iptr = ubuf.ibuf;
    bufferlen = ARRAY_LEN(ubuf.ibuf);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = paf24_read(psf, ppaf24, iptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = normfact * iptr[k];
        total += count;
        len -= readcount;
    };
    return total;
}

static size_t paf24_read_d(SndFile *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    PAF24_PRIVATE *ppaf24;
    int *iptr;
    size_t k, bufferlen, readcount, count;
    size_t total = 0;
    double normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    normfact = (psf->m_norm_double == SF_TRUE) ? (1.0 / 0x80000000) : (1.0 / 0x100);

    iptr = ubuf.ibuf;
    bufferlen = ARRAY_LEN(ubuf.ibuf);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = paf24_read(psf, ppaf24, iptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = normfact * iptr[k];
        total += count;
        len -= readcount;
    };
    return total;
}

static int paf24_write_block(SndFile *psf, PAF24_PRIVATE *ppaf24)
{
    int k, nextsample, channel;
    unsigned char *cptr;

    /* First pack block. */

    if (CPU_IS_LITTLE_ENDIAN)
    {
        for (k = 0; k < PAF24_SAMPLES_PER_BLOCK * ppaf24->channels; k++)
        {
            channel = k % ppaf24->channels;
            cptr = ((unsigned char *)ppaf24->block) + PAF24_BLOCK_SIZE * channel +
                   3 * (k / ppaf24->channels);
            nextsample = ppaf24->samples[k] >> 8;
            cptr[0] = nextsample;
            cptr[1] = nextsample >> 8;
            cptr[2] = nextsample >> 16;
        };

        /* Do endian swapping if necessary. */
        if (psf->m_endian == SF_ENDIAN_BIG)
            endswap_int_array(ppaf24->block, 8 * ppaf24->channels);
    }
    else if (CPU_IS_BIG_ENDIAN)
    {
        /* This is correct. */
        for (k = 0; k < PAF24_SAMPLES_PER_BLOCK * ppaf24->channels; k++)
        {
            channel = k % ppaf24->channels;
            cptr = ((unsigned char *)ppaf24->block) + PAF24_BLOCK_SIZE * channel +
                   3 * (k / ppaf24->channels);
            nextsample = ppaf24->samples[k] >> 8;
            cptr[0] = nextsample;
            cptr[1] = nextsample >> 8;
            cptr[2] = nextsample >> 16;
        };
        if (psf->m_endian == SF_ENDIAN_LITTLE)
            endswap_int_array(ppaf24->block, 8 * ppaf24->channels);
    };

    /* Write block to disk. */
    if ((k = psf->fwrite(ppaf24->block, 1, ppaf24->blocksize)) != ppaf24->blocksize)
        psf->log_printf("*** Warning : short write (%d != %d).\n", k, ppaf24->blocksize);

    if (ppaf24->sample_count < ppaf24->write_block * PAF24_SAMPLES_PER_BLOCK + ppaf24->write_count)
        ppaf24->sample_count = ppaf24->write_block * PAF24_SAMPLES_PER_BLOCK + ppaf24->write_count;

    if (ppaf24->write_count == PAF24_SAMPLES_PER_BLOCK)
    {
        ppaf24->write_block++;
        ppaf24->write_count = 0;
    };

    return 1;
}

static size_t paf24_write(SndFile *psf, PAF24_PRIVATE *ppaf24, const int *ptr, size_t len)
{
    size_t count, total = 0;

    while (total < len)
    {
        count = (PAF24_SAMPLES_PER_BLOCK - ppaf24->write_count) * ppaf24->channels;

        if (count > len - total)
            count = len - total;

        memcpy(&(ppaf24->samples[ppaf24->write_count * ppaf24->channels]), &(ptr[total]),
               count * sizeof(int));
        total += count;
        ppaf24->write_count += count / ppaf24->channels;

        if (ppaf24->write_count >= PAF24_SAMPLES_PER_BLOCK)
            paf24_write_block(psf, ppaf24);
    };

    return total;
}

static size_t paf24_write_s(SndFile *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    PAF24_PRIVATE *ppaf24;
    int *iptr;
    size_t k, bufferlen, writecount = 0, count;
    size_t total = 0;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    iptr = ubuf.ibuf;
    bufferlen = ARRAY_LEN(ubuf.ibuf);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            iptr[k] = ptr[total + k] << 16;
        count = paf24_write(psf, ppaf24, iptr, writecount);
        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };
    return total;
}

static size_t paf24_write_i(SndFile *psf, const int *ptr, size_t len)
{
    PAF24_PRIVATE *ppaf24;
    size_t writecount, count;
    size_t total = 0;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    while (len > 0)
    {
        writecount = (len > 0x10000000) ? 0x10000000 : len;

        count = paf24_write(psf, ppaf24, ptr, writecount);

        total += count;
        len -= count;
        if (count != writecount)
            break;
    };

    return total;
}

static size_t paf24_write_f(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    PAF24_PRIVATE *ppaf24;
    int *iptr;
    size_t k, bufferlen, writecount = 0, count;
    size_t total = 0;
    float normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? (1.0 * 0x7FFFFFFF) : (1.0 / 0x100));

    iptr = ubuf.ibuf;
    bufferlen = ARRAY_LEN(ubuf.ibuf);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            iptr[k] = lrintf(normfact * ptr[total + k]);
        count = paf24_write(psf, ppaf24, iptr, writecount);
        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };

    return total;
}

static size_t paf24_write_d(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    PAF24_PRIVATE *ppaf24;
    int *iptr;
    size_t k, bufferlen, writecount = 0, count;
    size_t total = 0;
    double normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    ppaf24 = (PAF24_PRIVATE *)psf->m_codec_data;

    normfact = (psf->m_norm_double == SF_TRUE) ? (1.0 * 0x7FFFFFFF) : (1.0 / 0x100);

    iptr = ubuf.ibuf;
    bufferlen = ARRAY_LEN(ubuf.ibuf);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            iptr[k] = lrint(normfact * ptr[total + k]);
        count = paf24_write(psf, ppaf24, iptr, writecount);
        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };

    return total;
}
