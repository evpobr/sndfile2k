/*
** Copyright (C) 2002-2014 Erik de Castro Lopo <erikd@mega-nerd.com>
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

/*
**	This is the OKI / Dialogic ADPCM encoder/decoder. It converts from
**	12 bit linear sample data to a 4 bit ADPCM.
*/

/*
 * Note: some early Dialogic hardware does not always reset the ADPCM encoder
 * at the start of each vox file. This can result in clipping and/or DC offset
 * problems when it comes to decoding the audio. Whilst little can be done
 * about the clipping, a DC offset can be removed by passing the decoded audio
 * through a high-pass filter at e.g. 10Hz.
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
#include "ima_oki_adpcm.h"

static size_t vox_read_s(SndFile *psf, short *ptr, size_t len);
static size_t vox_read_i(SndFile *psf, int *ptr, size_t len);
static size_t vox_read_f(SndFile *psf, float *ptr, size_t len);
static size_t vox_read_d(SndFile *psf, double *ptr, size_t len);

static size_t vox_write_s(SndFile *psf, const short *ptr, size_t len);
static size_t vox_write_i(SndFile *psf, const int *ptr, size_t len);
static size_t vox_write_f(SndFile *psf, const float *ptr, size_t len);
static size_t vox_write_d(SndFile *psf, const double *ptr, size_t len);

static size_t vox_read_block(SndFile *psf, IMA_OKI_ADPCM *pvox, short *ptr, size_t len);

static int codec_close(SndFile *psf)
{
    IMA_OKI_ADPCM *p = (IMA_OKI_ADPCM *)psf->m_codec_data;

    if (p->errors)
        psf->log_printf("*** Warning : ADPCM state errors: %d\n", p->errors);
    return p->errors;
}

int vox_adpcm_init(SndFile *psf)
{
    IMA_OKI_ADPCM *pvox = NULL;

    if (psf->m_mode == SFM_RDWR)
        return SFE_BAD_MODE_RW;

    if (psf->m_mode == SFM_WRITE && psf->sf.channels != 1)
        return SFE_CHANNEL_COUNT;

    if ((pvox = (IMA_OKI_ADPCM *)malloc(sizeof(IMA_OKI_ADPCM))) == NULL)
        return SFE_MALLOC_FAILED;

    psf->m_codec_data = (void *)pvox;
    memset(pvox, 0, sizeof(IMA_OKI_ADPCM));

    if (psf->m_mode == SFM_WRITE)
    {
        psf->write_short = vox_write_s;
        psf->write_int = vox_write_i;
        psf->write_float = vox_write_f;
        psf->write_double = vox_write_d;
    }
    else
    {
        psf->log_printf("Header-less OKI Dialogic ADPCM encoded file.\n");
        psf->log_printf("Setting up for 8kHz, mono, Vox ADPCM.\n");

        psf->read_short = vox_read_s;
        psf->read_int = vox_read_i;
        psf->read_float = vox_read_f;
        psf->read_double = vox_read_d;
    };

    /* Standard sample rate chennels etc. */
    if (psf->sf.samplerate < 1)
        psf->sf.samplerate = 8000;
    psf->sf.channels = 1;

    psf->sf.frames = psf->m_filelength * 2;

    psf->sf.seekable = SF_FALSE;
    psf->codec_close = codec_close;

    /* Seek back to start of data. */
    if (psf->fseek(0, SEEK_SET) == -1)
        return SFE_BAD_SEEK;

    ima_oki_adpcm_init(pvox, IMA_OKI_ADPCM_TYPE_OKI);

    return 0;
}

static size_t vox_read_block(SndFile *psf, IMA_OKI_ADPCM *pvox, short *ptr, size_t len)
{
    size_t indx = 0, k;

    while (indx < len)
    {
        pvox->code_count =
            (len - indx > IMA_OKI_ADPCM_PCM_LEN) ? IMA_OKI_ADPCM_CODE_LEN : (len - indx + 1) / 2;

        if ((k = psf->fread(pvox->codes, 1, pvox->code_count)) != pvox->code_count)
        {
            if (psf->ftell() != psf->m_filelength)
                psf->log_printf("*** Warning : short read (%d != %d).\n", k, pvox->code_count);
            if (k == 0)
                break;
        };

        pvox->code_count = k;

        ima_oki_adpcm_decode_block(pvox);

        memcpy(&(ptr[indx]), pvox->pcm, pvox->pcm_count * sizeof(short));
        indx += pvox->pcm_count;
    };

    return indx;
}

static size_t vox_read_s(SndFile *psf, short *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    size_t readcount, count;
    size_t total = 0;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    while (len > 0)
    {
        readcount = (len > 0x10000000) ? 0x10000000 : len;

        count = vox_read_block(psf, pvox, ptr, readcount);

        total += count;
        len -= count;
        if (count != readcount)
            break;
    };

    return total;
}

static size_t vox_read_i(SndFile *psf, int *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    BUF_UNION ubuf;
    short *sptr;
    size_t k, bufferlen, readcount, count;
    size_t total = 0;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    sptr = ubuf.sbuf;
    bufferlen = ARRAY_LEN(ubuf.sbuf);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = vox_read_block(psf, pvox, sptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = arith_shift_left(sptr[k], 16);
        total += count;
        len -= readcount;
        if (count != readcount)
            break;
    };

    return total;
}

static size_t vox_read_f(SndFile *psf, float *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    BUF_UNION ubuf;
    short *sptr;
    size_t k, bufferlen, readcount, count;
    size_t total = 0;
    float normfact;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? 1.0 / ((float)0x8000) : 1.0);

    sptr = ubuf.sbuf;
    bufferlen = ARRAY_LEN(ubuf.sbuf);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = vox_read_block(psf, pvox, sptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = normfact * (float)(sptr[k]);
        total += count;
        len -= readcount;
        if (count != readcount)
            break;
    };

    return total;
}

static size_t vox_read_d(SndFile *psf, double *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    BUF_UNION ubuf;
    short *sptr;
    size_t k, bufferlen, readcount, count;
    size_t total = 0;
    double normfact;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    normfact = (psf->m_norm_double == SF_TRUE) ? 1.0 / ((double)0x8000) : 1.0;

    sptr = ubuf.sbuf;
    bufferlen = ARRAY_LEN(ubuf.sbuf);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = vox_read_block(psf, pvox, sptr, readcount);
        for (k = 0; k < readcount; k++)
            ptr[total + k] = normfact * (double)(sptr[k]);
        total += count;
        len -= readcount;
        if (count != readcount)
            break;
    };

    return total;
}

static size_t vox_write_block(SndFile *psf, IMA_OKI_ADPCM *pvox, const short *ptr, size_t len)
{
    size_t indx = 0, k;

    while (indx < len)
    {
        pvox->pcm_count = (len - indx > IMA_OKI_ADPCM_PCM_LEN) ? IMA_OKI_ADPCM_PCM_LEN : len - indx;

        memcpy(pvox->pcm, &(ptr[indx]), pvox->pcm_count * sizeof(short));

        ima_oki_adpcm_encode_block(pvox);

        if ((k = psf->fwrite(pvox->codes, 1, pvox->code_count)) != pvox->code_count)
            psf->log_printf("*** Warning : short write (%d != %d).\n", k, pvox->code_count);

        indx += pvox->pcm_count;
    };

    return indx;
}

static size_t vox_write_s(SndFile *psf, const short *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    size_t writecount, count;
    size_t total = 0;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    while (len)
    {
        writecount = (len > 0x10000000) ? 0x10000000 : len;

        count = vox_write_block(psf, pvox, ptr, writecount);

        total += count;
        len -= count;
        if (count != writecount)
            break;
    };

    return total;
}

static size_t vox_write_i(SndFile *psf, const int *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    BUF_UNION ubuf;
    short *sptr;
    size_t k, bufferlen, writecount, count;
    size_t total = 0;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    sptr = ubuf.sbuf;
    bufferlen = ARRAY_LEN(ubuf.sbuf);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            sptr[k] = ptr[total + k] >> 16;
        count = vox_write_block(psf, pvox, sptr, writecount);
        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };

    return total;
}

static size_t vox_write_f(SndFile *psf, const float *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    BUF_UNION ubuf;
    short *sptr;
    size_t k, bufferlen, writecount, count;
    size_t total = 0;
    float normfact;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? (1.0 * 0x7FFF) : 1.0);

    sptr = ubuf.sbuf;
    bufferlen = ARRAY_LEN(ubuf.sbuf);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : (int)len;
        for (k = 0; k < writecount; k++)
            sptr[k] = (short)lrintf(normfact * ptr[total + k]);
        count = vox_write_block(psf, pvox, sptr, writecount);
        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };

    return total;
}

static size_t vox_write_d(SndFile *psf, const double *ptr, size_t len)
{
    IMA_OKI_ADPCM *pvox;
    BUF_UNION ubuf;
    short *sptr;
    size_t k, bufferlen, writecount, count;
    size_t total = 0;
    double normfact;

    if (!psf->m_codec_data)
        return 0;
    pvox = (IMA_OKI_ADPCM *)psf->m_codec_data;

    normfact = (psf->m_norm_double == SF_TRUE) ? (1.0 * 0x7FFF) : 1.0;

    sptr = ubuf.sbuf;
    bufferlen = ARRAY_LEN(ubuf.sbuf);
    while (len > 0)
    {
        writecount = (len >= bufferlen) ? bufferlen : len;
        for (k = 0; k < writecount; k++)
            sptr[k] = (short)lrint(normfact * ptr[total + k]);
        count = vox_write_block(psf, pvox, sptr, writecount);
        total += count;
        len -= writecount;
        if (count != writecount)
            break;
    };

    return total;
}
