/*
** Copyright (C) 2002-2013 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#include "sfendian.h"

#include <stdlib.h>

#include "sndfile2k/sndfile2k.h"
#include "common.h"

#define INTERLEAVE_CHANNELS (6)

static size_t interleave_read_short(SndFile *psf, short *ptr, size_t len);
static size_t interleave_read_int(SndFile *psf, int *ptr, size_t len);
static size_t interleave_read_float(SndFile *psf, float *ptr, size_t len);
static size_t interleave_read_double(SndFile *psf, double *ptr, size_t len);

static sf_count_t interleave_seek(SndFile *, int mode, sf_count_t samples_from_start);

int interleave_init(SndFile *psf)
{
    INTERLEAVE_DATA *pdata;

    if (psf->m_mode != SFM_READ)
        return SFE_INTERLEAVE_MODE;

    if (psf->m_interleave)
    {
        psf->log_printf("*** Weird, already have interleave.\n");
        return 666;
    };

    /* Free this in sf_close() function. */
    if (!(pdata = (INTERLEAVE_DATA *)malloc(sizeof(INTERLEAVE_DATA))))
        return SFE_MALLOC_FAILED;

    puts("interleave_init");

    psf->m_interleave = pdata;

    /* Save the existing methods. */
    pdata->read_short = psf->read_short;
    pdata->read_int = psf->read_int;
    pdata->read_float = psf->read_float;
    pdata->read_double = psf->read_double;

    pdata->channel_len = psf->sf.frames * psf->m_bytewidth;

    /* Insert our new methods. */
    psf->read_short = interleave_read_short;
    psf->read_int = interleave_read_int;
    psf->read_float = interleave_read_float;
    psf->read_double = interleave_read_double;

    psf->seek_from_start = interleave_seek;

    return 0;
}

static size_t interleave_read_short(SndFile *psf, short *ptr, size_t len)
{
    INTERLEAVE_DATA *pdata;
    sf_count_t offset, templen;
    int chan;
    size_t count, k;
    short *inptr, *outptr;

    if (!(pdata = (INTERLEAVE_DATA *)psf->m_interleave))
        return 0;

    inptr = (short *)pdata->buffer;

    for (chan = 0; chan < psf->sf.channels; chan++)
    {
        outptr = ptr + chan;

        offset = psf->m_dataoffset + chan * psf->m_bytewidth * psf->m_read_current;

        if (psf->fseek(offset, SEEK_SET) != offset)
        {
            psf->m_error = SFE_INTERLEAVE_SEEK;
            return 0;
        };

        templen = len / psf->sf.channels;

        while (templen > 0)
        {
            if (templen > SIGNED_SIZEOF(pdata->buffer) / SIGNED_SIZEOF(short))
                count = SIGNED_SIZEOF(pdata->buffer) / SIGNED_SIZEOF(short);
            else
                count = (int)templen;

            if (pdata->read_short(psf, inptr, count) != count)
            {
                psf->m_error = SFE_INTERLEAVE_READ;
                return 0;
            };

            for (k = 0; k < count; k++)
            {
                *outptr = inptr[k];
                outptr += psf->sf.channels;
            };

            templen -= count;
        };
    };

    return len;
}

static size_t interleave_read_int(SndFile *psf, int *ptr, size_t len)
{
    INTERLEAVE_DATA *pdata;
    sf_count_t offset, templen;
    int chan;
    size_t count, k;
    int *inptr, *outptr;

    if (!(pdata = (INTERLEAVE_DATA *)psf->m_interleave))
        return 0;

    inptr = (int *)pdata->buffer;

    for (chan = 0; chan < psf->sf.channels; chan++)
    {
        outptr = ptr + chan;

        offset = psf->m_dataoffset + chan * psf->m_bytewidth * psf->m_read_current;

        if (psf->fseek(offset, SEEK_SET) != offset)
        {
            psf->m_error = SFE_INTERLEAVE_SEEK;
            return 0;
        };

        templen = len / psf->sf.channels;

        while (templen > 0)
        {
            if (templen > sizeof(pdata->buffer) / sizeof(int))
                count = sizeof(pdata->buffer) / sizeof(int);
            else
                count = (int)templen;

            if (pdata->read_int(psf, inptr, count) != count)
            {
                psf->m_error = SFE_INTERLEAVE_READ;
                return 0;
            };

            for (k = 0; k < count; k++)
            {
                *outptr = inptr[k];
                outptr += psf->sf.channels;
            };

            templen -= count;
        };
    };

    return len;
}

static size_t interleave_read_float(SndFile *psf, float *ptr, size_t len)
{
    INTERLEAVE_DATA *pdata;
    sf_count_t offset, templen;
    int chan;
    size_t count, k;
    float *inptr, *outptr;

    if (!(pdata = (INTERLEAVE_DATA *)psf->m_interleave))
        return 0;

    inptr = (float *)pdata->buffer;

    for (chan = 0; chan < psf->sf.channels; chan++)
    {
        outptr = ptr + chan;

        offset = psf->m_dataoffset + pdata->channel_len * chan + psf->m_read_current * psf->m_bytewidth;

        /*-printf ("chan : %d     read_current : %6lld    offset : %6lld\n", chan, psf->read_current, offset) ;-*/

        if (psf->fseek(offset, SEEK_SET) != offset)
        {
            psf->m_error = SFE_INTERLEAVE_SEEK;
            /*-puts ("interleave_seek error") ; exit (1) ;-*/
            return 0;
        };

        templen = len / psf->sf.channels;

        while (templen > 0)
        {
            if (templen > SIGNED_SIZEOF(pdata->buffer) / SIGNED_SIZEOF(float))
                count = SIGNED_SIZEOF(pdata->buffer) / SIGNED_SIZEOF(float);
            else
                count = (int)templen;

            if (pdata->read_float(psf, inptr, count) != count)
            {
                psf->m_error = SFE_INTERLEAVE_READ;
                /*-puts ("interleave_read error") ; exit (1) ;-*/
                return 0;
            };

            for (k = 0; k < count; k++)
            {
                *outptr = inptr[k];
                outptr += psf->sf.channels;
            };

            templen -= count;
        };
    };

    return len;
}

static size_t interleave_read_double(SndFile *psf, double *ptr, size_t len)
{
    INTERLEAVE_DATA *pdata;
    sf_count_t offset, templen;
    int chan;
    size_t count, k;
    double *inptr, *outptr;

    if (!(pdata = (INTERLEAVE_DATA *)psf->m_interleave))
        return 0;

    inptr = (double *)pdata->buffer;

    for (chan = 0; chan < psf->sf.channels; chan++)
    {
        outptr = ptr + chan;

        offset = psf->m_dataoffset + chan * psf->m_bytewidth * psf->m_read_current;

        if (psf->fseek(offset, SEEK_SET) != offset)
        {
            psf->m_error = SFE_INTERLEAVE_SEEK;
            return 0;
        };

        templen = len / psf->sf.channels;

        while (templen > 0)
        {
            if (templen > SIGNED_SIZEOF(pdata->buffer) / SIGNED_SIZEOF(double))
                count = SIGNED_SIZEOF(pdata->buffer) / SIGNED_SIZEOF(double);
            else
                count = (int)templen;

            if (pdata->read_double(psf, inptr, count) != count)
            {
                psf->m_error = SFE_INTERLEAVE_READ;
                return 0;
            };

            for (k = 0; k < count; k++)
            {
                *outptr = inptr[k];
                outptr += psf->sf.channels;
            };

            templen -= count;
        };
    };

    return len;
}

static sf_count_t interleave_seek(SndFile *UNUSED(psf), int UNUSED(mode),
                                  sf_count_t samples_from_start)
{
    /*
	 * Do nothing here. This is a place holder to prevent the default
	 * seek function from being called.
	 */

    return samples_from_start;
}
