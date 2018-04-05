/*
** Copyright (C) 2002-2012 Erik de Castro Lopo <erikd@mega-nerd.com>
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

/*===========================================================================
** Yamaha TX16 Sampler Files.
**
** This header parser was written using information from the SoX source code
** and trial and error experimentation. The code here however is all original.
*/

#include "config.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

#if (ENABLE_EXPERIMENTAL_CODE == 0)

int txw_open(SF_PRIVATE *psf)
{
    if (psf)
        return SFE_UNIMPLEMENTED;
    return 0;
}

#else

/*
 * Markers.
 */

#define TXW_DATA_OFFSET (32)

#define TXW_LOOPED (0x49)
#define TXW_NO_LOOP (0xC9)

static int txw_read_header(SF_PRIVATE *psf);

static size_t txw_read_s(SF_PRIVATE *psf, short *ptr, size_t len);
static size_t txw_read_i(SF_PRIVATE *psf, int *ptr, size_t len);
static size_t txw_read_f(SF_PRIVATE *psf, float *ptr, size_t len);
static size_t txw_read_d(SF_PRIVATE *psf, double *ptr, size_t len);

static sf_count_t txw_seek(SF_PRIVATE *psf, int mode, sf_count_t offset);

/*
 * ftp://ftp.t0.or.at/pub/sound/tx16w/samples.yamaha
 * ftp://ftp.t0.or.at/pub/sound/tx16w/faq/tx16w.tec
 * http://www.t0.or.at/~mpakesch/tx16w/
 *
 * from tx16w.c sox 12.15: (7-Oct-98) (Mark Lakata and Leigh Smith)
 *  char filetype[6] "LM8953"
 *  nulls[10],
 *  dummy_aeg[6]
 *  format 0x49 = looped, 0xC9 = non-looped
 *  sample_rate 1 = 33 kHz, 2 = 50 kHz, 3 = 16 kHz
 *  atc_length[3] if sample rate 0, [2]&0xfe = 6: 33kHz, 0x10:50, 0xf6: 16,
 *                  depending on [5] but to heck with it
 *  rpt_length[3] (these are for looped samples, attack and loop lengths)
 *  unused[2]
 */

typedef struct
{
    unsigned char format, srate, sr2, sr3;
    unsigned short srhash;
    unsigned int attacklen, repeatlen;
} TXW_HEADER;

#define ERROR_666 (666)

int txw_open(SF_PRIVATE *psf)
{
    int error;

    if (psf->file.mode != SFM_READ)
        return SFE_UNIMPLEMENTED;

    if ((error = txw_read_header(psf)))
        return error;

    if (psf->fseek(psf->dataoffset, SEEK_SET) != psf->dataoffset)
        return SFE_BAD_SEEK;

    psf->read_short = txw_read_s;
    psf->read_int = txw_read_i;
    psf->read_float = txw_read_f;
    psf->read_double = txw_read_d;

    psf->seek = txw_seek;

    return 0;
}

static int txw_read_header(SF_PRIVATE *psf)
{
    BUF_UNION ubuf;
    TXW_HEADER txwh;
    const char *strptr;

    memset(&txwh, 0, sizeof(txwh));
    memset(ubuf.cbuf, 0, sizeof(ubuf.cbuf));
    psf->binheader_readf("pb", 0, ubuf.cbuf, 16);

    if (memcmp(ubuf.cbuf, "LM8953\0\0\0\0\0\0\0\0\0\0", 16) != 0)
        return ERROR_666;

    psf->log_printf("Read only : Yamaha TX-16 Sampler (.txw)\nLM8953\n");

    /* Jump 6 bytes (dummp_aeg), read format, read sample rate. */
    psf->binheader_readf("j11", 6, &txwh.format, &txwh.srate);

    /* 8 bytes (atc_length[3], rpt_length[3], unused[2]). */
    psf->binheader_readf("e33j", &txwh.attacklen, &txwh.repeatlen, 2);
    txwh.sr2 = (txwh.attacklen >> 16) & 0xFE;
    txwh.sr3 = (txwh.repeatlen >> 16) & 0xFE;
    txwh.attacklen &= 0x1FFFF;
    txwh.repeatlen &= 0x1FFFF;

    switch (txwh.format)
    {
    case TXW_LOOPED:
        strptr = "looped";
        break;

    case TXW_NO_LOOP:
        strptr = "non-looped";
        break;

    default:
        psf->log_printf(" Format      : 0x%02x => ?????\n", txwh.format);
        return ERROR_666;
    };

    psf->log_printf(" Format      : 0x%02X => %s\n", txwh.format, strptr);

    strptr = NULL;

    switch (txwh.srate)
    {
    case 1:
        psf->sf.samplerate = 33333;
        break;

    case 2:
        psf->sf.samplerate = 50000;
        break;

    case 3:
        psf->sf.samplerate = 16667;
        break;

    default:
        /* This is ugly and braindead. */
        txwh.srhash = ((txwh.sr2 & 0xFE) << 8) | (txwh.sr3 & 0xFE);
        switch (txwh.srhash)
        {
        case ((0x6 << 8) | 0x52):
            psf->sf.samplerate = 33333;
            break;

        case ((0x10 << 8) | 0x52):
            psf->sf.samplerate = 50000;
            break;

        case ((0xF6 << 8) | 0x52):
            psf->sf.samplerate = 166667;
            break;

        default:
            strptr = " Sample Rate : Unknown : forcing to 33333\n";
            psf->sf.samplerate = 33333;
            break;
        };
    };

    if (strptr)
        psf->log_printf(strptr);
    else if (txwh.srhash)
        psf->log_printf(" Sample Rate : %d (0x%X) => %d\n", txwh.srate, txwh.srhash,
                       psf->sf.samplerate);
    else
        psf->log_printf(" Sample Rate : %d => %d\n", txwh.srate, psf->sf.samplerate);

    if (txwh.format == TXW_LOOPED)
    {
        psf->log_printf(" Attack Len  : %d\n", txwh.attacklen);
        psf->log_printf(" Repeat Len  : %d\n", txwh.repeatlen);
    };

    psf->dataoffset = TXW_DATA_OFFSET;
    psf->datalength = psf->filelength - TXW_DATA_OFFSET;
    psf->sf.frames = 2 * psf->datalength / 3;

    if (psf->datalength % 3 == 1)
        psf->log_printf("*** File seems to be truncated, %d extra bytes.\n",
                       (int)(psf->datalength % 3));

    if (txwh.attacklen + txwh.repeatlen > psf->sf.frames)
        psf->log_printf("*** File has been truncated.\n");

    psf->sf.format = SF_FORMAT_TXW | SF_FORMAT_PCM_16;
    psf->sf.channels = 1;
    psf->sf.sections = 1;
    psf->sf.seekable = SF_TRUE;

    return 0;
}

static size_t txw_read_s(SF_PRIVATE *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    unsigned char *ucptr;
    short sample;
    size_t k, bufferlen, readcount, count, total = 0;

    bufferlen = sizeof(ubuf.cbuf) / 3;
    bufferlen -= (bufferlen & 1);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = psf->fread(ubuf.cbuf, 3, readcount);

        ucptr = ubuf.ucbuf;
        for (k = 0; k < readcount; k += 2)
        {
            sample = (ucptr[0] << 8) | (ucptr[1] & 0xF0);
            ptr[total + k] = sample;
            sample = (ucptr[2] << 8) | ((ucptr[1] & 0xF) << 4);
            ptr[total + k + 1] = sample;
            ucptr += 3;
        };

        total += count;
        len -= readcount;
    };

    return total;
}

static size_t txw_read_i(SF_PRIVATE *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    unsigned char *ucptr;
    short sample;
    size_t k, bufferlen, readcount, count, total = 0;

    bufferlen = sizeof(ubuf.cbuf) / 3;
    bufferlen -= (bufferlen & 1);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = psf->fread(ubuf.cbuf, 3, readcount);

        ucptr = ubuf.ucbuf;
        for (k = 0; k < readcount; k += 2)
        {
            sample = (ucptr[0] << 8) | (ucptr[1] & 0xF0);
            ptr[total + k] = sample << 16;
            sample = (ucptr[2] << 8) | ((ucptr[1] & 0xF) << 4);
            ptr[total + k + 1] = sample << 16;
            ucptr += 3;
        };

        total += count;
        len -= readcount;
    };

    return total;
}

static size_t txw_read_f(SF_PRIVATE *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    unsigned char *ucptr;
    short sample;
    size_t k, bufferlen, readcount, count, total = 0;
    float normfact;

    if (psf->norm_float == SF_TRUE)
        normfact = 1.0 / 0x8000;
    else
        normfact = 1.0 / 0x10;

    bufferlen = sizeof(ubuf.cbuf) / 3;
    bufferlen -= (bufferlen & 1);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = psf->fread(ubuf.cbuf, 3, readcount);

        ucptr = ubuf.ucbuf;
        for (k = 0; k < readcount; k += 2)
        {
            sample = (ucptr[0] << 8) | (ucptr[1] & 0xF0);
            ptr[total + k] = normfact * sample;
            sample = (ucptr[2] << 8) | ((ucptr[1] & 0xF) << 4);
            ptr[total + k + 1] = normfact * sample;
            ucptr += 3;
        };

        total += count;
        len -= readcount;
    };

    return total;
}

static size_t txw_read_d(SF_PRIVATE *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    unsigned char *ucptr;
    short sample;
    size_t k, bufferlen, readcount, count, total = 0;
    double normfact;

    if (psf->norm_double == SF_TRUE)
        normfact = 1.0 / 0x8000;
    else
        normfact = 1.0 / 0x10;

    bufferlen = sizeof(ubuf.cbuf) / 3;
    bufferlen -= (bufferlen & 1);
    while (len > 0)
    {
        readcount = (len >= bufferlen) ? bufferlen : len;
        count = psf->fread(ubuf.cbuf, 3, readcount);

        ucptr = ubuf.ucbuf;
        for (k = 0; k < readcount; k += 2)
        {
            sample = (ucptr[0] << 8) | (ucptr[1] & 0xF0);
            ptr[total + k] = normfact * sample;
            sample = (ucptr[2] << 8) | ((ucptr[1] & 0xF) << 4);
            ptr[total + k + 1] = normfact * sample;
            ucptr += 3;
        };

        total += count;
        len -= readcount;
    };

    return total;
}

static sf_count_t txw_seek(SF_PRIVATE *psf, int mode, sf_count_t offset)
{
    if (psf && mode)
        return offset;

    return 0;
}

#endif
