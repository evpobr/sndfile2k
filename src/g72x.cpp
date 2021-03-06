/*
** Copyright (C) 1999-2014 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <math.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"
#include "shift.h"
#ifdef __cplusplus
extern "C" {
#endif
#include "G72x/g72x.h"
#ifdef __cplusplus
}
#endif

/* This struct is private to the G72x code. */
struct g72x_state;
typedef struct g72x_state G72x_STATE;

typedef struct
{
    /* Private data. Don't mess with it. */
    struct g72x_state *priv;

    /* Public data. Read only. */
    int blocksize, samplesperblock, bytesperblock;

    /* Public data. Read and write. */
    int blocks_total, block_curr, sample_curr;
    unsigned char block[G72x_BLOCK_SIZE];
    short samples[G72x_BLOCK_SIZE];
} G72x_PRIVATE;

static int psf_g72x_decode_block(SndFile *psf, G72x_PRIVATE *pg72x);
static int psf_g72x_encode_block(SndFile *psf, G72x_PRIVATE *pg72x);

static size_t g72x_read_s(SndFile *psf, short *ptr, size_t len);
static size_t g72x_read_i(SndFile *psf, int *ptr, size_t len);
static size_t g72x_read_f(SndFile *psf, float *ptr, size_t len);
static size_t g72x_read_d(SndFile *psf, double *ptr, size_t len);

static size_t g72x_write_s(SndFile *psf, const short *ptr, size_t len);
static size_t g72x_write_i(SndFile *psf, const int *ptr, size_t len);
static size_t g72x_write_f(SndFile *psf, const float *ptr, size_t len);
static size_t g72x_write_d(SndFile *psf, const double *ptr, size_t len);

static sf_count_t g72x_seek(SndFile *psf, int mode, sf_count_t offset);

static int g72x_close(SndFile *psf);

/*
 * WAV G721 Reader initialisation function.
 */

int g72x_init(SndFile *psf)
{
    G72x_PRIVATE *pg72x;
    int bitspersample, bytesperblock, codec;

    if (psf->m_codec_data != NULL)
    {
        psf->log_printf("*** psf->codec_data is not NULL.\n");
        return SFE_INTERNAL;
    };

    psf->sf.seekable = SF_FALSE;

    if (psf->sf.channels != 1)
        return SFE_G72X_NOT_MONO;

    if ((pg72x = (G72x_PRIVATE *)calloc(1, sizeof(G72x_PRIVATE))) == NULL)
        return SFE_MALLOC_FAILED;

    psf->m_codec_data = (void *)pg72x;

    pg72x->block_curr = 0;
    pg72x->sample_curr = 0;

    switch (SF_CODEC(psf->sf.format))
    {
    case SF_FORMAT_G721_32:
        codec = G721_32_BITS_PER_SAMPLE;
        bytesperblock = G721_32_BYTES_PER_BLOCK;
        bitspersample = G721_32_BITS_PER_SAMPLE;
        break;

    case SF_FORMAT_G723_24:
        codec = G723_24_BITS_PER_SAMPLE;
        bytesperblock = G723_24_BYTES_PER_BLOCK;
        bitspersample = G723_24_BITS_PER_SAMPLE;
        break;

    case SF_FORMAT_G723_40:
        codec = G723_40_BITS_PER_SAMPLE;
        bytesperblock = G723_40_BYTES_PER_BLOCK;
        bitspersample = G723_40_BITS_PER_SAMPLE;
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    psf->m_filelength = psf->get_filelen();
    if (psf->m_filelength < psf->m_dataoffset)
        psf->m_filelength = psf->m_dataoffset;

    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
    if (psf->m_dataend > 0)
        psf->m_datalength -= psf->m_filelength - psf->m_dataend;

    if (psf->m_mode == SFM_READ)
    {
        pg72x->priv = g72x_reader_init(codec, &(pg72x->blocksize), &(pg72x->samplesperblock));
        if (pg72x->priv == NULL)
            return SFE_MALLOC_FAILED;

        pg72x->bytesperblock = bytesperblock;

        psf->read_short = g72x_read_s;
        psf->read_int = g72x_read_i;
        psf->read_float = g72x_read_f;
        psf->read_double = g72x_read_d;

        psf->seek_from_start = g72x_seek;

        if (psf->m_datalength % pg72x->blocksize)
        {
            psf->log_printf("*** Odd psf->datalength (%D) should be a multiple of %d\n",
                           psf->m_datalength, pg72x->blocksize);
            pg72x->blocks_total = (psf->m_datalength / pg72x->blocksize) + 1;
        }
        else
        {
            pg72x->blocks_total = psf->m_datalength / pg72x->blocksize;
        }

        psf->sf.frames = pg72x->blocks_total * pg72x->samplesperblock;

        psf_g72x_decode_block(psf, pg72x);
    }
    else if (psf->m_mode == SFM_WRITE)
    {
        pg72x->priv = g72x_writer_init(codec, &(pg72x->blocksize), &(pg72x->samplesperblock));
        if (pg72x->priv == NULL)
            return SFE_MALLOC_FAILED;

        pg72x->bytesperblock = bytesperblock;

        psf->write_short = g72x_write_s;
        psf->write_int = g72x_write_i;
        psf->write_float = g72x_write_f;
        psf->write_double = g72x_write_d;

        if (psf->m_datalength % pg72x->blocksize)
            pg72x->blocks_total = (psf->m_datalength / pg72x->blocksize) + 1;
        else
            pg72x->blocks_total = psf->m_datalength / pg72x->blocksize;

        if (psf->m_datalength > 0)
            psf->sf.frames = (8 * psf->m_datalength) / bitspersample;

        if ((psf->sf.frames * bitspersample) / 8 != psf->m_datalength)
            psf->log_printf("*** Warning : weird psf->datalength.\n");
    };

    psf->codec_close = g72x_close;

    return 0;
}

static int psf_g72x_decode_block(SndFile *psf, G72x_PRIVATE *pg72x)
{
    size_t k;

    pg72x->block_curr++;
    pg72x->sample_curr = 0;

    if (pg72x->block_curr > pg72x->blocks_total)
    {
        memset(pg72x->samples, 0, G72x_BLOCK_SIZE * sizeof(short));
        return 1;
    };

    if ((k = psf->fread(pg72x->block, 1, pg72x->bytesperblock)) != pg72x->bytesperblock)
        psf->log_printf("*** Warning : short read (%z != %z).\n", k, pg72x->bytesperblock);

    pg72x->blocksize = k;
    g72x_decode_block(pg72x->priv, pg72x->block, pg72x->samples);

    return 1;
}

static size_t g72x_read_block(SndFile *psf, G72x_PRIVATE *pg72x, short *ptr, size_t len)
{
    size_t count, total = 0, indx = 0;

    while (indx < len)
    {
        if (pg72x->block_curr > pg72x->blocks_total)
        {
            memset(&(ptr[indx]), 0, (len - indx) * sizeof(short));
            return total;
        };

        if (pg72x->sample_curr >= pg72x->samplesperblock)
            psf_g72x_decode_block(psf, pg72x);

        count = pg72x->samplesperblock - pg72x->sample_curr;
        count = (len - indx > count) ? count : len - indx;

        memcpy(&(ptr[indx]), &(pg72x->samples[pg72x->sample_curr]), count * sizeof(short));
        indx += count;
        pg72x->sample_curr += count;
        total = indx;
    };

    return total;
}

static size_t g72x_read_s(SndFile *psf, short *ptr, size_t len)
{
    G72x_PRIVATE *pg72x;
    size_t readcount, count;
    size_t total = 0;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    while (len > 0)
    {
        readcount = (len > 0x10000000) ? 0x10000000 : len;

        count = g72x_read_block(psf, pg72x, ptr, readcount);

        total += count;
        len -= count;

        if (count != readcount)
            break;
    };

    return total;
}

static size_t g72x_read_i(SndFile *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    G72x_PRIVATE *pg72x;
    short *sptr;
    size_t k, bufferlen, readcount = 0, count;
    size_t total = 0;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    sptr = ubuf.sbuf;
    bufferlen = SF_BUFFER_LEN / sizeof(short);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = g72x_read_block(psf, pg72x, sptr, readcount);

        for (k = 0; k < readcount; k++)
            ptr[total + k] = arith_shift_left(sptr[k], 16);

        total += count;
        len -= readcount;
        if (count != readcount)
            break;
    };

    return total;
}

static size_t g72x_read_f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    G72x_PRIVATE *pg72x;
    short *sptr;
    size_t k, bufferlen, readcount = 0, count;
    size_t total = 0;
    float normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? 1.0 / ((float)0x8000) : 1.0);

    sptr = ubuf.sbuf;
    bufferlen = SF_BUFFER_LEN / sizeof(short);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = g72x_read_block(psf, pg72x, sptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = normfact * sptr[k];

        total += count;
        len -= readcount;
        if (count != readcount)
            break;
    };

    return total;
}

static size_t g72x_read_d(SndFile *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    G72x_PRIVATE *pg72x;
    short *sptr;
    size_t k, bufferlen, readcount = 0, count;
    size_t total = 0;
    double normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    normfact = (psf->m_norm_double == SF_TRUE) ? 1.0 / ((double)0x8000) : 1.0;

    sptr = ubuf.sbuf;
    bufferlen = SF_BUFFER_LEN / sizeof(short);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = g72x_read_block(psf, pg72x, sptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = normfact * (double)(sptr[k]);

        total += count;
        len -= readcount;
        if (count != readcount)
            break;
    };

    return total;
}

static sf_count_t g72x_seek(SndFile *psf, int UNUSED(mode), sf_count_t UNUSED(offset))
{
    psf->log_printf("seek unsupported\n");

    /* No simple solution. To do properly, would need to seek
	 * to start of file and decode everything up to seek position.
	 * Maybe implement SEEK_SET to 0 only?
	 */
    return 0;
}

static int psf_g72x_encode_block(SndFile *psf, G72x_PRIVATE *pg72x)
{
    int k;

    /* Encode the samples. */
    g72x_encode_block(pg72x->priv, pg72x->samples, pg72x->block);

    /* Write the block to disk. */
    if ((k = psf->fwrite(pg72x->block, 1, pg72x->blocksize)) != pg72x->blocksize)
        psf->log_printf("*** Warning : short write (%d != %d).\n", k, pg72x->blocksize);

    pg72x->sample_curr = 0;
    pg72x->block_curr++;

    /* Set samples to zero for next block. */
    memset(pg72x->samples, 0, G72x_BLOCK_SIZE * sizeof(short));

    return 1;
}

static int g72x_write_block(SndFile *psf, G72x_PRIVATE *pg72x, const short *ptr, int len)
{
    int count, total = 0, indx = 0;

    while (indx < len)
    {
        count = pg72x->samplesperblock - pg72x->sample_curr;

        if (count > len - indx)
            count = len - indx;

        memcpy(&(pg72x->samples[pg72x->sample_curr]), &(ptr[indx]), count * sizeof(short));
        indx += count;
        pg72x->sample_curr += count;
        total = indx;

        if (pg72x->sample_curr >= pg72x->samplesperblock)
            psf_g72x_encode_block(psf, pg72x);
    };

    return total;
}

static size_t g72x_write_s(SndFile *psf, const short *ptr, size_t len)
{
    G72x_PRIVATE *pg72x;
    size_t writecount, count;
    size_t total = 0;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    while (len > 0)
    {
        writecount = (len > 0x10000000) ? 0x10000000 : len;

        count = g72x_write_block(psf, pg72x, ptr, writecount);

        total += count;
        len -= count;
        if (count != writecount)
            break;
    };

    return total;
}

static size_t g72x_write_i(SndFile *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    G72x_PRIVATE *pg72x;
    short *sptr;
    size_t k, bufferlen, writecount = 0, count;
    size_t total = 0;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    sptr = ubuf.sbuf;
    bufferlen = SF_BUFFER_LEN / sizeof(short);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            sptr[k] = ptr[total + k] >> 16;
        count = g72x_write_block(psf, pg72x, sptr, writecount);

        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };
    return total;
}

static size_t g72x_write_f(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    G72x_PRIVATE *pg72x;
    short *sptr;
    size_t k, bufferlen, writecount = 0, count;
    size_t total = 0;
    float normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? (1.0 * 0x8000) : 1.0);

    sptr = ubuf.sbuf;
    bufferlen = SF_BUFFER_LEN / sizeof(short);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            sptr[k] = (short)lrintf(normfact * ptr[total + k]);
        count = g72x_write_block(psf, pg72x, sptr, writecount);

        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };

    return total;
}

static size_t g72x_write_d(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    G72x_PRIVATE *pg72x;
    short *sptr;
    size_t k, bufferlen, writecount = 0, count;
    size_t total = 0;
    double normfact;

    if (psf->m_codec_data == NULL)
        return 0;
    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    normfact = (psf->m_norm_double == SF_TRUE) ? (1.0 * 0x8000) : 1.0;

    sptr = ubuf.sbuf;
    bufferlen = SF_BUFFER_LEN / sizeof(short);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            sptr[k] = (short)lrint(normfact * ptr[total + k]);
        count = g72x_write_block(psf, pg72x, sptr, writecount);

        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };

    return total;
}

static int g72x_close(SndFile *psf)
{
    G72x_PRIVATE *pg72x;

    pg72x = (G72x_PRIVATE *)psf->m_codec_data;

    if (psf->m_mode == SFM_WRITE)
    {
        /*	If a block has been partially assembled, write it out
		**	as the final block.
		*/

        if (pg72x->sample_curr && pg72x->sample_curr < G72x_BLOCK_SIZE)
            psf_g72x_encode_block(psf, pg72x);

        if (psf->write_header)
            psf->write_header(psf, SF_FALSE);
    };

    /* Only free the pointer allocated by g72x_(reader|writer)_init. */
    free(pg72x->priv);

    return 0;
}
