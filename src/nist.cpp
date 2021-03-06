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

/*
**	Some of the information used to read NIST files was gleaned from
**	reading the code of Bill Schottstaedt's sndlib library
**		ftp://ccrma-ftp.stanford.edu/pub/Lisp/sndlib.tar.gz
**	However, no code from that package was used.
*/

#include "config.h"

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

#define NIST_HEADER_LENGTH (1024)

static int nist_close(SndFile *psf);
static int nist_write_header(SndFile *psf, int calc_length);
static int nist_read_header(SndFile *psf);

int nist_open(SndFile *psf)
{
    int error;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = nist_read_header(psf)))
            return error;
    };

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_NIST)
            return SFE_BAD_OPEN_FORMAT;

        psf->m_endian = SF_ENDIAN(psf->sf.format);
        if (psf->m_endian == 0 || psf->m_endian == SF_ENDIAN_CPU)
            psf->m_endian = (CPU_IS_BIG_ENDIAN) ? SF_ENDIAN_BIG : SF_ENDIAN_LITTLE;

        psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;
        psf->sf.frames = 0;

        if ((error = nist_write_header(psf, SF_FALSE)))
            return error;

        psf->write_header = nist_write_header;
    };

    psf->container_close = nist_close;

    switch (SF_CODEC(psf->sf.format))
    {
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

    default:
        error = SFE_UNIMPLEMENTED;
        break;
    };

    return error;
}

static char bad_header[] = {'N', 'I', 'S', 'T', '_', '1', 'A',  0x0d, 0x0a, ' ',
                            ' ', ' ', '1', '0', '2', '4', 0x0d, 0x0a, 0};

static int nist_read_header(SndFile *psf)
{
    char psf_header[NIST_HEADER_LENGTH + 2];
    int bitwidth = 0, count, encoding;
    unsigned bytes = 0;
    char str[64], *cptr;
    long samples;

    /* Go to start of file and read in the whole header. */
	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("b", psf_header, NIST_HEADER_LENGTH);

    /* Header is a string, so make sure it is null terminated. */
    psf_header[NIST_HEADER_LENGTH] = 0;

    /* Now trim the header after the end marker. */
    if ((cptr = strstr(psf_header, "end_head")))
    {
        cptr += strlen("end_head") + 1;
        cptr[0] = 0;
    };

    if (strstr(psf_header, bad_header) == psf_header)
        return SFE_NIST_CRLF_CONVERISON;

    /* Make sure its a NIST file. */
    if (strstr(psf_header, "NIST_1A\n") != psf_header)
    {
        psf->log_printf("Not a NIST file.\n");
        return SFE_NIST_BAD_HEADER;
    };

    if (sscanf(psf_header, "NIST_1A\n%d\n", &count) == 1)
        psf->m_dataoffset = count;
    else
    {
        psf->log_printf("*** Suspicious header length.\n");
        psf->m_dataoffset = NIST_HEADER_LENGTH;
    };

    /* Determine sample encoding, start by assuming PCM. */
    encoding = SF_FORMAT_PCM_U8;
    if ((cptr = strstr(psf_header, "sample_coding -s")))
    {
        sscanf(cptr, "sample_coding -s%d %63s", &count, str);

        if (strcmp(str, "pcm") == 0)
        {
            /* Correct this later when we find out the bitwidth. */
            encoding = SF_FORMAT_PCM_U8;
        }
        else if (strcmp(str, "alaw") == 0)
        {
            encoding = SF_FORMAT_ALAW;
        }
        else if ((strcmp(str, "ulaw") == 0) || (strcmp(str, "mu-law") == 0))
        {
            encoding = SF_FORMAT_ULAW;
        }
        else
        {
            psf->log_printf("*** Unknown encoding : %s\n", str);
            encoding = 0;
        };
    };

    if ((cptr = strstr(psf_header, "channel_count -i ")) != NULL)
        sscanf(cptr, "channel_count -i %d", &(psf->sf.channels));

    if ((cptr = strstr(psf_header, "sample_rate -i ")) != NULL)
        sscanf(cptr, "sample_rate -i %d", &(psf->sf.samplerate));

    if ((cptr = strstr(psf_header, "sample_count -i ")) != NULL)
    {
        sscanf(cptr, "sample_count -i %ld", &samples);
        psf->sf.frames = samples;
    };

    if ((cptr = strstr(psf_header, "sample_n_bytes -i ")) != NULL)
        sscanf(cptr, "sample_n_bytes -i %d", &(psf->m_bytewidth));

    /* Default endian-ness (for 8 bit, u-law, A-law. */
    psf->m_endian = (CPU_IS_BIG_ENDIAN) ? SF_ENDIAN_BIG : SF_ENDIAN_LITTLE;

    /* This is where we figure out endian-ness. */
    if ((cptr = strstr(psf_header, "sample_byte_format -s")) &&
        sscanf(cptr, "sample_byte_format -s%u %8s", &bytes, str) == 2)
    {
        if (bytes != strlen(str))
            psf->log_printf("Weird sample_byte_format : strlen '%s' != %d\n", str, bytes);

        if (bytes > 1)
        {
            if (psf->m_bytewidth == 0)
                psf->m_bytewidth = bytes;
            else if (psf->m_bytewidth - bytes != 0)
            {
                psf->log_printf("psf->bytewidth (%d) != bytes (%d)\n", psf->m_bytewidth, bytes);
                return SFE_NIST_BAD_ENCODING;
            };

            if (strcmp(str, "01") == 0)
                psf->m_endian = SF_ENDIAN_LITTLE;
            else if (strcmp(str, "10") == 0)
                psf->m_endian = SF_ENDIAN_BIG;
            else
            {
                psf->log_printf("Weird endian-ness : %s\n", str);
                return SFE_NIST_BAD_ENCODING;
            };
        };

        psf->sf.format |= psf->m_endian;
    };

    if ((cptr = strstr(psf_header, "sample_sig_bits -i ")))
        sscanf(cptr, "sample_sig_bits -i %d", &bitwidth);

    if (strstr(psf_header, "channels_interleaved -s5 FALSE"))
    {
        psf->log_printf("Non-interleaved data unsupported.\n", str);
        return SFE_NIST_BAD_ENCODING;
    };

    psf->m_blockwidth = psf->sf.channels * psf->m_bytewidth;
    psf->m_datalength = psf->m_filelength - psf->m_dataoffset;

    psf->fseek(psf->m_dataoffset, SEEK_SET);

    if (encoding == SF_FORMAT_PCM_U8)
    {
        switch (psf->m_bytewidth)
        {
        case 1:
            psf->sf.format |= SF_FORMAT_PCM_S8;
            break;

        case 2:
            psf->sf.format |= SF_FORMAT_PCM_16;
            break;

        case 3:
            psf->sf.format |= SF_FORMAT_PCM_24;
            break;

        case 4:
            psf->sf.format |= SF_FORMAT_PCM_32;
            break;

        default:
            break;
        };
    }
    else if (encoding != 0)
        psf->sf.format |= encoding;
    else
        return SFE_UNIMPLEMENTED;

    /* Sanitize psf->sf.format. */
    switch (SF_CODEC(psf->sf.format))
    {
    case SF_FORMAT_ULAW:
    case SF_FORMAT_ALAW:
    case SF_FORMAT_PCM_U8:
        /* Blank out endian bits. */
        psf->sf.format = SF_FORMAT_NIST | SF_CODEC(psf->sf.format);
        break;

    default:
        break;
    };

    return 0;
}

static int nist_close(SndFile *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        nist_write_header(psf, SF_TRUE);

    return 0;
}

static int nist_write_header(SndFile *psf, int calc_length)
{
    const char *end_str;
    long samples;
    sf_count_t current;

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

    if (psf->m_endian == SF_ENDIAN_BIG)
        end_str = "10";
    else if (psf->m_endian == SF_ENDIAN_LITTLE)
        end_str = "01";
    else
        end_str = "error";

    /* Clear the whole header. */
    memset(psf->m_header.ptr, 0, psf->m_header.len);
    psf->m_header.indx = 0;

    psf->fseek(0, SEEK_SET);

    psf->asciiheader_printf("NIST_1A\n   1024\n");
    psf->asciiheader_printf("channel_count -i %d\n", psf->sf.channels);
    psf->asciiheader_printf("sample_rate -i %d\n", psf->sf.samplerate);

    switch (SF_CODEC(psf->sf.format))
    {
    case SF_FORMAT_PCM_S8:
        psf->asciiheader_printf("sample_coding -s3 pcm\n");
        psf->asciiheader_printf("sample_n_bytes -i 1\n"
                                    "sample_sig_bits -i 8\n");
        break;

    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_24:
    case SF_FORMAT_PCM_32:
        psf->asciiheader_printf("sample_n_bytes -i %d\n", psf->m_bytewidth);
        psf->asciiheader_printf("sample_sig_bits -i %d\n", psf->m_bytewidth * 8);
        psf->asciiheader_printf("sample_coding -s3 pcm\n"
                                "sample_byte_format -s%d %s\n",
                                psf->m_bytewidth, end_str);
        break;

    case SF_FORMAT_ALAW:
        psf->asciiheader_printf("sample_coding -s4 alaw\n");
        psf->asciiheader_printf("sample_n_bytes -s1 1\n");
        break;

    case SF_FORMAT_ULAW:
        psf->asciiheader_printf("sample_coding -s4 ulaw\n");
        psf->asciiheader_printf("sample_n_bytes -s1 1\n");
        break;

    default:
        return SFE_UNIMPLEMENTED;
    };

    psf->m_dataoffset = NIST_HEADER_LENGTH;

    /* Fix this */
    samples = psf->sf.frames;
    psf->asciiheader_printf("sample_count -i %ld\n", samples);
    psf->asciiheader_printf("end_head\n");

    /* Zero fill to dataoffset. */
    psf->binheader_writef("z", BHWz((size_t)(NIST_HEADER_LENGTH - psf->m_header.indx)));

    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}
