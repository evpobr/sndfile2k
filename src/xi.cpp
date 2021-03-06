/*
** Copyright (C) 2003-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include "shift.h"

#define MAX_XI_SAMPLES (16)

typedef struct
{
    /* Warning, this filename is NOT nul terminated. */
    char filename[22];
    char software[20];
    char sample_name[22];

    int loop_begin, loop_end;
    int sample_flags;

    /* Data for encoder and decoder. */
    short last_16;
} XI_PRIVATE;

static int xi_close(SndFile *psf);
static int xi_write_header(SndFile *psf, int calc_length);
static int xi_read_header(SndFile *psf);
static int dpcm_init(SndFile *psf);

static sf_count_t dpcm_seek(SndFile *psf, int mode, sf_count_t offset);

int xi_open(SndFile *psf)
{
    XI_PRIVATE *pxi;
    int subformat, error = 0;

    if (psf->m_codec_data)
        pxi = (XI_PRIVATE *)psf->m_codec_data;
    else if ((pxi = (XI_PRIVATE *)calloc(1, sizeof(XI_PRIVATE))) == NULL)
        return SFE_MALLOC_FAILED;

    psf->m_codec_data = pxi;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = xi_read_header(psf)))
            return error;
    };

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_XI)
            return SFE_BAD_OPEN_FORMAT;

        psf->m_endian = SF_ENDIAN_LITTLE;
        psf->sf.channels = 1; /* Always mono */
        psf->sf.samplerate = 44100; /* Always */

        /* Set up default instrument and software name. */
        memcpy(pxi->filename, "Default Name            ", sizeof(pxi->filename));
        memcpy(pxi->software, PACKAGE_NAME "-" PACKAGE_VERSION "               ",
               sizeof(pxi->software));

        memset(pxi->sample_name, 0, sizeof(pxi->sample_name));
        snprintf(pxi->sample_name, sizeof(pxi->sample_name), "%s", "Sample #1");

        pxi->sample_flags = (subformat == SF_FORMAT_DPCM_16) ? 16 : 0;

        if (xi_write_header(psf, SF_FALSE))
            return psf->m_error;

        psf->write_header = xi_write_header;
    };

    psf->container_close = xi_close;
    psf->seek_from_start = dpcm_seek;

    psf->sf.seekable = SF_FALSE;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    switch (subformat)
    {
    case SF_FORMAT_DPCM_8: /* 8-bit differential PCM. */
    case SF_FORMAT_DPCM_16: /* 16-bit differential PCM. */
        error = dpcm_init(psf);
        break;

    default:
        break;
    };

    return error;
}

/*------------------------------------------------------------------------------
*/

static int xi_close(SndFile *UNUSED(psf))
{
    return 0;
}

/*==============================================================================
*/

static size_t dpcm_read_dsc2s(SndFile *psf, short *ptr, size_t len);
static size_t dpcm_read_dsc2i(SndFile *psf, int *ptr, size_t len);
static size_t dpcm_read_dsc2f(SndFile *psf, float *ptr, size_t len);
static size_t dpcm_read_dsc2d(SndFile *psf, double *ptr, size_t len);

static size_t dpcm_write_s2dsc(SndFile *psf, const short *ptr, size_t len);
static size_t dpcm_write_i2dsc(SndFile *psf, const int *ptr, size_t len);
static size_t dpcm_write_f2dsc(SndFile *psf, const float *ptr, size_t len);
static size_t dpcm_write_d2dsc(SndFile *psf, const double *ptr, size_t len);

static size_t dpcm_read_dles2s(SndFile *psf, short *ptr, size_t len);
static size_t dpcm_read_dles2i(SndFile *psf, int *ptr, size_t len);
static size_t dpcm_read_dles2f(SndFile *psf, float *ptr, size_t len);
static size_t dpcm_read_dles2d(SndFile *psf, double *ptr, size_t len);

static size_t dpcm_write_s2dles(SndFile *psf, const short *ptr, size_t len);
static size_t dpcm_write_i2dles(SndFile *psf, const int *ptr, size_t len);
static size_t dpcm_write_f2dles(SndFile *psf, const float *ptr, size_t len);
static size_t dpcm_write_d2dles(SndFile *psf, const double *ptr, size_t len);

static int dpcm_init(SndFile *psf)
{
    if (psf->m_bytewidth == 0 || psf->sf.channels == 0)
        return SFE_INTERNAL;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    if (psf->m_mode == SFM_READ || psf->m_mode == SFM_RDWR)
    {
        switch (psf->m_bytewidth)
        {
        case 1:
            psf->read_short = dpcm_read_dsc2s;
            psf->read_int = dpcm_read_dsc2i;
            psf->read_float = dpcm_read_dsc2f;
            psf->read_double = dpcm_read_dsc2d;
            break;
        case 2:
            psf->read_short = dpcm_read_dles2s;
            psf->read_int = dpcm_read_dles2i;
            psf->read_float = dpcm_read_dles2f;
            psf->read_double = dpcm_read_dles2d;
            break;
        default:
            psf->log_printf("dpcm_init() returning SFE_UNIMPLEMENTED\n");
            return SFE_UNIMPLEMENTED;
        };
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        switch (psf->m_bytewidth)
        {
        case 1:
            psf->write_short = dpcm_write_s2dsc;
            psf->write_int = dpcm_write_i2dsc;
            psf->write_float = dpcm_write_f2dsc;
            psf->write_double = dpcm_write_d2dsc;
            break;
        case 2:
            psf->write_short = dpcm_write_s2dles;
            psf->write_int = dpcm_write_i2dles;
            psf->write_float = dpcm_write_f2dles;
            psf->write_double = dpcm_write_d2dles;
            break;
        default:
            psf->log_printf("dpcm_init() returning SFE_UNIMPLEMENTED\n");
            return SFE_UNIMPLEMENTED;
        };
    };

    psf->m_filelength = psf->get_filelen();
    psf->m_datalength =
        (psf->m_dataend) ? psf->m_dataend - psf->m_dataoffset : psf->m_filelength - psf->m_dataoffset;
    psf->sf.frames = psf->m_datalength / psf->m_blockwidth;

    return 0;
}

static sf_count_t dpcm_seek(SndFile *psf, int mode, sf_count_t offset)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t total, bufferlen, len;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return SFE_INTERNAL;

    if (psf->m_datalength < 0 || psf->m_dataoffset < 0)
    {
        psf->m_error = SFE_BAD_SEEK;
        return PSF_SEEK_ERROR;
    };

    if (offset == 0)
    {
        psf->fseek(psf->m_dataoffset, SEEK_SET);
        pxi->last_16 = 0;
        return 0;
    };

    if (offset < 0 || offset > psf->sf.frames)
    {
        psf->m_error = SFE_BAD_SEEK;
        return PSF_SEEK_ERROR;
    };

    if (mode != SFM_READ)
    {
        /* What to do about write??? */
        psf->m_error = SFE_BAD_SEEK;
        return PSF_SEEK_ERROR;
    };

    psf->fseek(psf->m_dataoffset, SEEK_SET);

    if ((SF_CODEC(psf->sf.format)) == SF_FORMAT_DPCM_16)
    {
        total = offset;
        bufferlen = ARRAY_LEN(ubuf.sbuf);
        while (total > 0)
        {
            len = (total > bufferlen) ? bufferlen : total;
            total -= dpcm_read_dles2s(psf, ubuf.sbuf, len);
        };
    }
    else
    {
        total = offset;
        bufferlen = ARRAY_LEN(ubuf.sbuf);
        while (total > 0)
        {
            len = (total > bufferlen) ? bufferlen : total;
            total -= dpcm_read_dsc2s(psf, ubuf.sbuf, len);
        };
    };

    return offset;
}

static int xi_write_header(SndFile *psf, int UNUSED(calc_length))
{
    XI_PRIVATE *pxi;
    sf_count_t current;
    const char *string;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return SFE_INTERNAL;

    current = psf->ftell();

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    string = "Extended Instrument: ";
    psf->binheader_writef("b", BHWv(string), BHWz(strlen(string)));
    psf->binheader_writef("b1", BHWv(pxi->filename), BHWz(sizeof(pxi->filename)), BHW1(0x1A));

    /* Write software version and two byte XI version. */
    psf->binheader_writef("eb2", BHWv(pxi->software), BHWz(sizeof(pxi->software)),
                         BHW2((1 << 8) + 2));

    /*
	 * Jump note numbers (96), volume envelope (48), pan envelope (48),
	 * volume points (1), pan points (1)
	 */
    psf->binheader_writef("z", BHWz((size_t)(96 + 48 + 48 + 1 + 1)));

    /*
	 * Jump volume loop (3 bytes), pan loop (3), envelope flags (3), vibrato (3)
	 * fade out (2), 22 unknown bytes, and then write sample_count (2 bytes).
	 */
    psf->binheader_writef("ez2z2", BHWz((size_t)(4 * 3)), BHW2(0x1234), BHWz(22), BHW2(1));

    pxi->loop_begin = 0;
    pxi->loop_end = 0;

    psf->binheader_writef("et844", BHW8(psf->sf.frames), BHW4(pxi->loop_begin),
                         BHW4(pxi->loop_end));

    /* volume, fine tune, flags, pan, note, namelen */
    psf->binheader_writef("111111", BHW1(128), BHW1(0), BHW1(pxi->sample_flags), BHW1(128),
                         BHW1(0), BHW1(strlen(pxi->sample_name)));

    psf->binheader_writef("b", BHWv(pxi->sample_name), BHWz(sizeof(pxi->sample_name)));

    /* Header construction complete so write it out. */
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int xi_read_header(SndFile *psf)
{
    char buffer[64], name[32];
    short version, fade_out, sample_count;
    int k, loop_begin, loop_end;
    int sample_sizes[MAX_XI_SAMPLES];

	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("b", buffer, 21);

    memset(sample_sizes, 0, sizeof(sample_sizes));

    buffer[20] = 0;
    if (strcmp(buffer, "Extended Instrument:") != 0)
        return SFE_XI_BAD_HEADER;

    memset(buffer, 0, sizeof(buffer));
    psf->binheader_readf("b", buffer, 23);

    if (buffer[22] != 0x1A)
        return SFE_XI_BAD_HEADER;

    buffer[22] = 0;
    for (k = 21; k >= 0 && buffer[k] == ' '; k--)
        buffer[k] = 0;

    psf->log_printf("Extended Instrument : %s\n", buffer);
    psf->store_string(SF_STR_TITLE, buffer);

    psf->binheader_readf("be2", buffer, 20, &version);
    buffer[19] = 0;
    for (k = 18; k >= 0 && buffer[k] == ' '; k--)
        buffer[k] = 0;

    psf->log_printf("Software : %s\nVersion  : %d.%02d\n", buffer, version / 256,
                   version % 256);
    psf->store_string(SF_STR_SOFTWARE, buffer);

    /*
	 * Jump note numbers (96), volume envelope (48), pan envelope (48),
	 * volume points (1), pan points (1)
	 */
	psf->binheader_seekf(96 + 48 + 48 + 1 + 1, SF_SEEK_CUR);

    psf->binheader_readf("b", buffer, 12);
    psf->log_printf("Volume Loop\n  sustain : %u\n  begin   : %u\n  end     : %u\n", buffer[0],
                   buffer[1], buffer[2]);
    psf->log_printf("Pan Loop\n  sustain : %u\n  begin   : %u\n  end     : %u\n", buffer[3],
                   buffer[4], buffer[5]);
    psf->log_printf("Envelope Flags\n  volume  : 0x%X\n  pan     : 0x%X\n", buffer[6] & 0xFF,
                   buffer[7] & 0xFF);

    psf->log_printf(
                   "Vibrato\n  type    : %u\n  sweep   : %u\n  depth   : "
                   "%u\n  rate    : %u\n",
                   buffer[8], buffer[9], buffer[10], buffer[11]);

    /*
	 * Read fade_out then jump reserved (2 bytes) and ???? (20 bytes) and
	 * sample_count.
	 */
    psf->binheader_readf("e2", &fade_out);
	psf->binheader_seekf(2 + 20, SF_SEEK_CUR);
	psf->binheader_readf("e2", &sample_count);
    psf->log_printf("Fade out  : %d\n", fade_out);

    /* XI file can contain up to 16 samples. */
    if (sample_count > MAX_XI_SAMPLES)
        return SFE_XI_EXCESS_SAMPLES;

    if (psf->m_instrument == NULL && (psf->m_instrument = psf_instrument_alloc()) == NULL)
        return SFE_MALLOC_FAILED;

    psf->m_instrument->basenote = 0;
    /* Log all data for each sample. */
    for (k = 0; k < sample_count; k++)
    {
        psf->binheader_readf("e444", &(sample_sizes[k]), &loop_begin, &loop_end);

        /* Read 5 know bytes, 1 unknown byte and 22 name bytes. */
        psf->binheader_readf("bb", buffer, 6, name, 22);
        name[21] = 0;

        psf->log_printf("Sample #%d\n  name    : %s\n", k + 1, name);

        psf->log_printf("  size    : %d\n", sample_sizes[k]);

        psf->log_printf("  loop\n    begin : %d\n    end   : %d\n", loop_begin, loop_end);

        psf->log_printf("  volume  : %u\n  f. tune : %d\n  flags   : 0x%02X ", buffer[0] & 0xFF,
                       buffer[1] & 0xFF, buffer[2] & 0xFF);

        psf->log_printf(" (");
        if (buffer[2] & 1)
            psf->log_printf(" Loop");
        if (buffer[2] & 2)
            psf->log_printf(" PingPong");
        psf->log_printf((buffer[2] & 16) ? " 16bit" : " 8bit");
        psf->log_printf(" )\n");

        psf->log_printf("  pan     : %u\n  note    : %d\n  namelen : %d\n", buffer[3] & 0xFF,
                       buffer[4], buffer[5]);

        psf->m_instrument->basenote = buffer[4];
        if (buffer[2] & 1)
        {
            psf->m_instrument->loop_count = 1;
            psf->m_instrument->loops[0].mode =
                (buffer[2] & 2) ? SF_LOOP_ALTERNATING : SF_LOOP_FORWARD;
            psf->m_instrument->loops[0].start = loop_begin;
            psf->m_instrument->loops[0].end = loop_end;
        };

        if (k != 0)
            continue;

        if (buffer[2] & 16)
        {
            psf->sf.format = SF_FORMAT_XI | SF_FORMAT_DPCM_16;
            psf->m_bytewidth = 2;
        }
        else
        {
            psf->sf.format = SF_FORMAT_XI | SF_FORMAT_DPCM_8;
            psf->m_bytewidth = 1;
        };
    };

    while (sample_count > 1 && sample_sizes[sample_count - 1] == 0)
        sample_count--;

    /* Currently, we can only handle 1 sample per file. */

    if (sample_count > 2)
    {
        psf->log_printf("*** Sample count is less than 16 but more than 1.\n");
        psf->log_printf("  sample count : %d    sample_sizes [%d] : %d\n", sample_count,
                       sample_count - 1, sample_sizes[sample_count - 1]);
        return SFE_XI_EXCESS_SAMPLES;
    };

    psf->m_datalength = sample_sizes[0];

    psf->m_dataoffset = psf->ftell();
    if (psf->m_dataoffset < 0)
    {
        psf->log_printf("*** Bad Data Offset : %D\n", psf->m_dataoffset);
        return SFE_BAD_OFFSET;
    };
    psf->log_printf("Data Offset : %D\n", psf->m_dataoffset);

    if (psf->m_dataoffset + psf->m_datalength > psf->m_filelength)
    {
        psf->log_printf(
                       "*** File seems to be truncated. Should be at "
                       "least %D bytes long.\n",
                       psf->m_dataoffset + sample_sizes[0]);
        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
    };

    if (psf->fseek(psf->m_dataoffset, SEEK_SET) != psf->m_dataoffset)
        return SFE_BAD_SEEK;

    psf->m_endian = SF_ENDIAN_LITTLE;
    psf->sf.channels = 1; /* Always mono */
    psf->sf.samplerate = 44100; /* Always */

    psf->m_blockwidth = psf->sf.channels * psf->m_bytewidth;

    if (!psf->sf.frames && psf->m_blockwidth)
        psf->sf.frames = (psf->m_filelength - psf->m_dataoffset) / psf->m_blockwidth;

    psf->m_instrument->gain = 1;
    psf->m_instrument->velocity_lo = psf->m_instrument->key_lo = 0;
    psf->m_instrument->velocity_hi = psf->m_instrument->key_hi = 127;

    return 0;
}

/*==============================================================================
*/

static void dsc2s_array(XI_PRIVATE *pxi, signed char *src, size_t count, short *dest);
static void dsc2i_array(XI_PRIVATE *pxi, signed char *src, size_t count, int *dest);
static void dsc2f_array(XI_PRIVATE *pxi, signed char *src, size_t count, float *dest,
                        float normfact);
static void dsc2d_array(XI_PRIVATE *pxi, signed char *src, size_t count, double *dest,
                        double normfact);

static void dles2s_array(XI_PRIVATE *pxi, short *src, size_t count, short *dest);
static void dles2i_array(XI_PRIVATE *pxi, short *src, size_t count, int *dest);
static void dles2f_array(XI_PRIVATE *pxi, short *src, size_t count, float *dest, float normfact);
static void dles2d_array(XI_PRIVATE *pxi, short *src, size_t count, double *dest, double normfact);

static size_t dpcm_read_dsc2s(SndFile *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        dsc2s_array(pxi, ubuf.scbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t dpcm_read_dsc2i(SndFile *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        dsc2i_array(pxi, ubuf.scbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t dpcm_read_dsc2f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? 1.0 / ((float)0x80) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        dsc2f_array(pxi, ubuf.scbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t dpcm_read_dsc2d(SndFile *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (psf->m_norm_double == SF_TRUE) ? 1.0 / ((double)0x80) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.scbuf, sizeof(signed char), bufferlen);
        dsc2d_array(pxi, ubuf.scbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t dpcm_read_dles2s(SndFile *psf, short *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        dles2s_array(pxi, ubuf.sbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t dpcm_read_dles2i(SndFile *psf, int *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        dles2i_array(pxi, ubuf.sbuf, readcount, ptr + total);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t dpcm_read_dles2f(SndFile *psf, float *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;
    float normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? 1.0 / ((float)0x8000) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        dles2f_array(pxi, ubuf.sbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static size_t dpcm_read_dles2d(SndFile *psf, double *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, readcount;
    size_t total = 0;
    double normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (psf->m_norm_double == SF_TRUE) ? 1.0 / ((double)0x8000) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        readcount = psf->fread(ubuf.sbuf, sizeof(short), bufferlen);
        dles2d_array(pxi, ubuf.sbuf, readcount, ptr + total, normfact);
        total += readcount;
        if (readcount < bufferlen)
            break;
        len -= readcount;
    };

    return total;
}

static void s2dsc_array(XI_PRIVATE *pxi, const short *src, signed char *dest, size_t count);
static void i2dsc_array(XI_PRIVATE *pxi, const int *src, signed char *dest, size_t count);
static void f2dsc_array(XI_PRIVATE *pxi, const float *src, signed char *dest, size_t count,
                        float normfact);
static void d2dsc_array(XI_PRIVATE *pxi, const double *src, signed char *dest, size_t count,
                        double normfact);

static void s2dles_array(XI_PRIVATE *pxi, const short *src, short *dest, size_t count);
static void i2dles_array(XI_PRIVATE *pxi, const int *src, short *dest, size_t count);
static void f2dles_array(XI_PRIVATE *pxi, const float *src, short *dest, size_t count,
                         float normfact);
static void d2dles_array(XI_PRIVATE *pxi, const double *src, short *dest, size_t count,
                         double normfact);

static size_t dpcm_write_s2dsc(SndFile *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = (int)len;
        s2dsc_array(pxi, ptr + total, ubuf.scbuf, bufferlen);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t dpcm_write_i2dsc(SndFile *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2dsc_array(pxi, ptr + total, ubuf.scbuf, bufferlen);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t dpcm_write_f2dsc(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;
    float normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? (1.0 * 0x7F) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        f2dsc_array(pxi, ptr + total, ubuf.scbuf, bufferlen, normfact);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t dpcm_write_d2dsc(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;
    double normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (psf->m_norm_double == SF_TRUE) ? (1.0 * 0x7F) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.ucbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        d2dsc_array(pxi, ptr + total, ubuf.scbuf, bufferlen, normfact);
        writecount = psf->fwrite(ubuf.scbuf, sizeof(signed char), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t dpcm_write_s2dles(SndFile *psf, const short *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        s2dles_array(pxi, ptr + total, ubuf.sbuf, bufferlen);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t dpcm_write_i2dles(SndFile *psf, const int *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        i2dles_array(pxi, ptr + total, ubuf.sbuf, bufferlen);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t dpcm_write_f2dles(SndFile *psf, const float *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;
    float normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (float)((psf->m_norm_float == SF_TRUE) ? (1.0 * 0x7FFF) : 1.0);

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        f2dles_array(pxi, ptr + total, ubuf.sbuf, bufferlen, normfact);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static size_t dpcm_write_d2dles(SndFile *psf, const double *ptr, size_t len)
{
    BUF_UNION ubuf;
    XI_PRIVATE *pxi;
    size_t bufferlen, writecount;
    size_t total = 0;
    double normfact;

    if ((pxi = (XI_PRIVATE *)psf->m_codec_data) == NULL)
        return 0;

    normfact = (psf->m_norm_double == SF_TRUE) ? (1.0 * 0x7FFF) : 1.0;

    bufferlen = ARRAY_LEN(ubuf.sbuf);

    while (len > 0)
    {
        if (len < bufferlen)
            bufferlen = len;
        d2dles_array(pxi, ptr + total, ubuf.sbuf, bufferlen, normfact);
        writecount = psf->fwrite(ubuf.sbuf, sizeof(short), bufferlen);
        total += writecount;
        if (writecount < bufferlen)
            break;
        len -= writecount;
    };

    return total;
}

static void dsc2s_array(XI_PRIVATE *pxi, signed char *src, size_t count, short *dest)
{
    signed char last_val;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        last_val += src[k];
        dest[k] = arith_shift_left(last_val, 8);
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void dsc2i_array(XI_PRIVATE *pxi, signed char *src, size_t count, int *dest)
{
    signed char last_val;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        last_val += src[k];
        dest[k] = arith_shift_left(last_val, 24);
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void dsc2f_array(XI_PRIVATE *pxi, signed char *src, size_t count, float *dest,
                        float normfact)
{
    signed char last_val;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        last_val += src[k];
        dest[k] = last_val * normfact;
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void dsc2d_array(XI_PRIVATE *pxi, signed char *src, size_t count, double *dest,
                        double normfact)
{
    signed char last_val;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        last_val += src[k];
        dest[k] = last_val * normfact;
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void s2dsc_array(XI_PRIVATE *pxi, const short *src, signed char *dest, size_t count)
{
    signed char last_val, current;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        current = src[k] >> 8;
        dest[k] = current - last_val;
        last_val = current;
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void i2dsc_array(XI_PRIVATE *pxi, const int *src, signed char *dest, size_t count)
{
    signed char last_val, current;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        current = src[k] >> 24;
        dest[k] = current - last_val;
        last_val = current;
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void f2dsc_array(XI_PRIVATE *pxi, const float *src, signed char *dest, size_t count,
                        float normfact)
{
    signed char last_val, current;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        current = (signed char)lrintf(src[k] * normfact);
        dest[k] = current - last_val;
        last_val = current;
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void d2dsc_array(XI_PRIVATE *pxi, const double *src, signed char *dest, size_t count,
                        double normfact)
{
    signed char last_val, current;
    size_t k;

    last_val = pxi->last_16 >> 8;

    for (k = 0; k < count; k++)
    {
        current = (signed char)lrint(src[k] * normfact);
        dest[k] = current - last_val;
        last_val = current;
    };

    pxi->last_16 = arith_shift_left(last_val, 8);
}

static void dles2s_array(XI_PRIVATE *pxi, short *src, size_t count, short *dest)
{
    short last_val;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        last_val += LE2H_16(src[k]);
        dest[k] = last_val;
    };

    pxi->last_16 = last_val;
}

static void dles2i_array(XI_PRIVATE *pxi, short *src, size_t count, int *dest)
{
    short last_val;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        last_val += LE2H_16(src[k]);
        dest[k] = arith_shift_left(last_val, 16);
    };

    pxi->last_16 = last_val;
}

static void dles2f_array(XI_PRIVATE *pxi, short *src, size_t count, float *dest, float normfact)
{
    short last_val;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        last_val += LE2H_16(src[k]);
        dest[k] = last_val * normfact;
    };

    pxi->last_16 = last_val;
}

static void dles2d_array(XI_PRIVATE *pxi, short *src, size_t count, double *dest, double normfact)
{
    short last_val;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        last_val += LE2H_16(src[k]);
        dest[k] = last_val * normfact;
    };

    pxi->last_16 = last_val;
}

static void s2dles_array(XI_PRIVATE *pxi, const short *src, short *dest, size_t count)
{
    short diff, last_val;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        diff = src[k] - last_val;
        dest[k] = LE2H_16(diff);
        last_val = src[k];
    };

    pxi->last_16 = last_val;
}

static void i2dles_array(XI_PRIVATE *pxi, const int *src, short *dest, size_t count)
{
    short diff, last_val;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        diff = (src[k] >> 16) - last_val;
        dest[k] = LE2H_16(diff);
        last_val = src[k] >> 16;
    };

    pxi->last_16 = last_val;
}

static void f2dles_array(XI_PRIVATE *pxi, const float *src, short *dest, size_t count,
                         float normfact)
{
    short diff, last_val, current;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        current = (short)lrintf(src[k] * normfact);
        diff = current - last_val;
        dest[k] = LE2H_16(diff);
        last_val = current;
    };

    pxi->last_16 = last_val;
}

static void d2dles_array(XI_PRIVATE *pxi, const double *src, short *dest, size_t count,
                         double normfact)
{
    short diff, last_val, current;
    size_t k;

    last_val = pxi->last_16;

    for (k = 0; k < count; k++)
    {
        current = (short)lrint(src[k] * normfact);
        diff = current - last_val;
        dest[k] = LE2H_16(diff);
        last_val = current;
    };

    pxi->last_16 = last_val;
}
