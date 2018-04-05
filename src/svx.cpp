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
#include <string.h>
#include <ctype.h>
#include <stdarg.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

/*------------------------------------------------------------------------------
 * Macros to handle big/little endian issues.
*/

#define FORM_MARKER (MAKE_MARKER('F', 'O', 'R', 'M'))
#define SVX8_MARKER (MAKE_MARKER('8', 'S', 'V', 'X'))
#define SV16_MARKER (MAKE_MARKER('1', '6', 'S', 'V'))
#define VHDR_MARKER (MAKE_MARKER('V', 'H', 'D', 'R'))
#define BODY_MARKER (MAKE_MARKER('B', 'O', 'D', 'Y'))

#define ATAK_MARKER (MAKE_MARKER('A', 'T', 'A', 'K'))
#define RLSE_MARKER (MAKE_MARKER('R', 'L', 'S', 'E'))

#define c_MARKER (MAKE_MARKER('(', 'c', ')', ' '))
#define NAME_MARKER (MAKE_MARKER('N', 'A', 'M', 'E'))
#define AUTH_MARKER (MAKE_MARKER('A', 'U', 'T', 'H'))
#define ANNO_MARKER (MAKE_MARKER('A', 'N', 'N', 'O'))
#define CHAN_MARKER (MAKE_MARKER('C', 'H', 'A', 'N'))

/*------------------------------------------------------------------------------
 * Typedefs for file chunks.
*/

typedef struct
{
    unsigned int oneShotHiSamples, repeatHiSamples, samplesPerHiCycle;
    unsigned short samplesPerSec;
    unsigned char octave, compression;
    unsigned int volume;
} VHDR_CHUNK;

enum
{
    HAVE_FORM = 0x01,

    HAVE_SVX = 0x02,
    HAVE_VHDR = 0x04,
    HAVE_BODY = 0x08
};

static int svx_close(SF_PRIVATE *psf);
static int svx_write_header(SF_PRIVATE *psf, int calc_length);
static int svx_read_header(SF_PRIVATE *psf);

int svx_open(SF_PRIVATE *psf)
{
    int error;

    if (psf->file.mode == SFM_READ || (psf->file.mode == SFM_RDWR && psf->filelength > 0))
    {
        if ((error = svx_read_header(psf)))
            return error;

        psf->endian = SF_ENDIAN_BIG; /* All SVX files are big endian. */

        psf->blockwidth = psf->sf.channels * psf->bytewidth;
        if (psf->blockwidth)
            psf->sf.frames = psf->datalength / psf->blockwidth;

        psf->fseek(psf->dataoffset, SEEK_SET);
    };

    if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
    {
        if (psf->file.is_pipe)
            return SFE_NO_PIPE_WRITE;

        if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_SVX)
            return SFE_BAD_OPEN_FORMAT;

        psf->endian = SF_ENDIAN(psf->sf.format);

        if (psf->endian == SF_ENDIAN_LITTLE ||
            (CPU_IS_LITTLE_ENDIAN && psf->endian == SF_ENDIAN_CPU))
            return SFE_BAD_ENDIAN;

        psf->endian = SF_ENDIAN_BIG; /* All SVX files are big endian. */

        error = svx_write_header(psf, SF_FALSE);
        if (error)
            return error;

        psf->write_header = svx_write_header;
    };

    psf->container_close = svx_close;

    if ((error = pcm_init(psf)))
        return error;

    return 0;
}

static int svx_read_header(SF_PRIVATE *psf)
{
    VHDR_CHUNK vhdr;
    uint32_t chunk_size, marker;
    int filetype = 0, parsestage = 0, done = 0;
    int bytecount = 0, channels;

    if (psf->filelength > INT64_C(0xffffffff))
        psf->log_printf("Warning : filelength > 0xffffffff. This is bad!!!!\n");

    memset(&vhdr, 0, sizeof(vhdr));
	psf->binheader_seekf(0, SF_SEEK_SET);

    /* Set default number of channels. Modify later if necessary */
    psf->sf.channels = 1;

    psf->sf.format = SF_FORMAT_SVX;

    while (!done)
    {
        psf->binheader_readf("Em4", &marker, &chunk_size);

        switch (marker)
        {
        case FORM_MARKER:
            if (parsestage)
                return SFE_SVX_NO_FORM;

            if (chunk_size != psf->filelength - 2 * sizeof(chunk_size))
                psf->log_printf("FORM : %u (should be %u)\n", chunk_size,
                               (uint32_t)psf->filelength - 2 * sizeof(chunk_size));
            else
                psf->log_printf("FORM : %u\n", chunk_size);
            parsestage |= HAVE_FORM;

            psf->binheader_readf("m", &marker);

            filetype = marker;
            psf->log_printf(" %M\n", marker);
            parsestage |= HAVE_SVX;
            break;

        case VHDR_MARKER:
            if (!(parsestage & (HAVE_FORM | HAVE_SVX)))
                return SFE_SVX_NO_FORM;

            psf->log_printf(" VHDR : %d\n", chunk_size);

            psf->binheader_readf("E4442114", &(vhdr.oneShotHiSamples), &(vhdr.repeatHiSamples),
                                &(vhdr.samplesPerHiCycle), &(vhdr.samplesPerSec), &(vhdr.octave),
                                &(vhdr.compression), &(vhdr.volume));

            psf->log_printf("  OneShotHiSamples  : %d\n", vhdr.oneShotHiSamples);
            psf->log_printf("  RepeatHiSamples   : %d\n", vhdr.repeatHiSamples);
            psf->log_printf("  samplesPerHiCycle : %d\n", vhdr.samplesPerHiCycle);
            psf->log_printf("  Sample Rate       : %d\n", vhdr.samplesPerSec);
            psf->log_printf("  Octave            : %d\n", vhdr.octave);

            psf->log_printf("  Compression       : %d => ", vhdr.compression);

            switch (vhdr.compression)
            {
            case 0:
                psf->log_printf("None.\n");
                break;
            case 1:
                psf->log_printf("Fibonacci delta\n");
                break;
            case 2:
                psf->log_printf("Exponential delta\n");
                break;
            };

            psf->log_printf("  Volume            : %d\n", vhdr.volume);

            psf->sf.samplerate = vhdr.samplesPerSec;

            if (filetype == SVX8_MARKER)
            {
                psf->sf.format |= SF_FORMAT_PCM_S8;
                psf->bytewidth = 1;
            }
            else if (filetype == SV16_MARKER)
            {
                psf->sf.format |= SF_FORMAT_PCM_16;
                psf->bytewidth = 2;
            };

            parsestage |= HAVE_VHDR;
            break;

        case BODY_MARKER:
            if (!(parsestage & HAVE_VHDR))
                return SFE_SVX_NO_BODY;

            psf->datalength = chunk_size;

            psf->dataoffset = psf->ftell();
            if (psf->dataoffset < 0)
                return SFE_SVX_NO_BODY;

            if (psf->datalength > psf->filelength - psf->dataoffset)
            {
                psf->log_printf(" BODY : %D (should be %D)\n", psf->datalength,
                               psf->filelength - psf->dataoffset);
                psf->datalength = psf->filelength - psf->dataoffset;
            }
            else
                psf->log_printf(" BODY : %D\n", psf->datalength);

            parsestage |= HAVE_BODY;

            if (!psf->sf.seekable)
                break;

            psf->fseek(psf->datalength, SEEK_CUR);
            break;

        case NAME_MARKER:
            if (!(parsestage & HAVE_SVX))
                return SFE_SVX_NO_FORM;

            psf->log_printf(" %M : %u\n", marker, chunk_size);

            if (strlen(psf->file.name.c) != chunk_size)
            {
                if (chunk_size > sizeof(psf->file.name.c) - 1)
                    return SFE_SVX_BAD_NAME_LENGTH;

                psf->binheader_readf("b", psf->file.name.c, chunk_size);
                psf->file.name.c[chunk_size] = 0;
            }
            else
				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        case ANNO_MARKER:
            if (!(parsestage & HAVE_SVX))
                return SFE_SVX_NO_FORM;

            psf->log_printf(" %M : %u\n", marker, chunk_size);

			psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
            break;

        case CHAN_MARKER:
            if (!(parsestage & HAVE_SVX))
                return SFE_SVX_NO_FORM;

            psf->log_printf(" %M : %u\n", marker, chunk_size);

            bytecount += psf->binheader_readf("E4", &channels);

            if (channels == 2 || channels == 4)
                psf->log_printf("  Channels : %d => mono\n", channels);
            else if (channels == 6)
            {
                psf->sf.channels = 2;
                psf->log_printf("  Channels : %d => stereo\n", channels);
            }
            else
                psf->log_printf("  Channels : %d *** assuming mono\n", channels);

			psf->binheader_seekf(chunk_size - bytecount, SF_SEEK_CUR);
            break;

        case AUTH_MARKER:
        case c_MARKER:
            if (!(parsestage & HAVE_SVX))
                return SFE_SVX_NO_FORM;

            psf->log_printf(" %M : %u\n", marker, chunk_size);

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
                psf->log_printf("%M : %u (unknown marker)\n", marker, chunk_size);
				psf->binheader_seekf(chunk_size, SF_SEEK_CUR);
                break;
            };
            if ((chunk_size = psf->ftell()) & 0x03)
            {
                psf->log_printf("  Unknown chunk marker at position %d. Resynching.\n",
                               chunk_size - 4);

				psf->binheader_seekf(-3, SF_SEEK_CUR);
                break;
            };
            psf->log_printf(
                           "*** Unknown chunk marker (%X) at position %D. "
                           "Exiting parser.\n",
                           marker, psf->ftell() - 8);
            done = SF_TRUE;
        };

        if (!psf->sf.seekable && (parsestage & HAVE_BODY))
            break;

        if (psf->ftell() >= psf->filelength - SIGNED_SIZEOF(chunk_size))
            break;
    }; /* while (1) */

    if (vhdr.compression)
        return SFE_SVX_BAD_COMP;

    if (psf->dataoffset <= 0)
        return SFE_SVX_NO_DATA;

    return 0;
}

static int svx_close(SF_PRIVATE *psf)
{
    if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
        svx_write_header(psf, SF_TRUE);

    return 0;
}

static int svx_write_header(SF_PRIVATE *psf, int calc_length)
{
    static char annotation[] = "libsndfile by Erik de Castro Lopo\0\0\0";
    sf_count_t current;

    current = psf->ftell();

    if (calc_length)
    {
        psf->filelength = psf->get_filelen();

        psf->datalength = psf->filelength - psf->dataoffset;

        if (psf->dataend)
            psf->datalength -= psf->filelength - psf->dataend;

        psf->sf.frames = psf->datalength / (psf->bytewidth * psf->sf.channels);
    };

    psf->header.ptr[0] = 0;
    psf->header.indx = 0;
    psf->fseek(0, SEEK_SET);

    /* FORM marker and FORM size. */
    psf->binheader_writef("Etm8", BHWm(FORM_MARKER),
                         BHW8((psf->filelength < 8) ? psf->filelength * 0 : psf->filelength - 8));

    psf->binheader_writef("m", BHWm((psf->bytewidth == 1) ? SVX8_MARKER : SV16_MARKER));

    /* VHDR chunk. */
    psf->binheader_writef("Em4", BHWm(VHDR_MARKER), BHW4(sizeof(VHDR_CHUNK)));
    /* VHDR : oneShotHiSamples, repeatHiSamples, samplesPerHiCycle */
    psf->binheader_writef("E444", BHW4(psf->sf.frames), BHW4(0), BHW4(0));
    /* VHDR : samplesPerSec, octave, compression */
    psf->binheader_writef("E211", BHW2(psf->sf.samplerate), BHW1(1), BHW1(0));
    /* VHDR : volume */
    psf->binheader_writef("E4", BHW4((psf->bytewidth == 1) ? 0xFF : 0xFFFF));

    if (psf->sf.channels == 2)
        psf->binheader_writef("Em44", BHWm(CHAN_MARKER), BHW4(4), BHW4(6));

    /* Filename and annotation strings. */
    psf->binheader_writef("Emsms", BHWm(NAME_MARKER), BHWs(psf->file.name.c), BHWm(ANNO_MARKER),
                         BHWs(annotation));

    /* BODY marker and size. */
    psf->binheader_writef("Etm8", BHWm(BODY_MARKER),
                         BHW8((psf->datalength < 0) ? psf->datalength * 0 : psf->datalength));

    psf->fwrite(psf->header.ptr, psf->header.indx, 1);

    if (psf->error)
        return psf->error;

    psf->dataoffset = psf->header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->error;
}
