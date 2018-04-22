/*
** Copyright (C) 2002-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"

/*
 * Information on how to decode and encode this file was obtained in a PDF
 * file which I found on http://www.wotsit.org/.
 * Also did a lot of testing with GNU Octave but do not have access to
 * Matlab (tm) and so could not test it there.
 */

/*
 * Macros to handle big/little endian issues.
 */

#define MAT4_BE_DOUBLE (MAKE_MARKER(0, 0, 0x03, 0xE8))
#define MAT4_LE_DOUBLE (MAKE_MARKER(0, 0, 0, 0))

#define MAT4_BE_FLOAT (MAKE_MARKER(0, 0, 0x03, 0xF2))
#define MAT4_LE_FLOAT (MAKE_MARKER(0x0A, 0, 0, 0))

#define MAT4_BE_PCM_32 (MAKE_MARKER(0, 0, 0x03, 0xFC))
#define MAT4_LE_PCM_32 (MAKE_MARKER(0x14, 0, 0, 0))

#define MAT4_BE_PCM_16 (MAKE_MARKER(0, 0, 0x04, 0x06))
#define MAT4_LE_PCM_16 (MAKE_MARKER(0x1E, 0, 0, 0))

/* Can't see any reason to ever implement this. */
#define MAT4_BE_PCM_U8 (MAKE_MARKER(0, 0, 0x04, 0x1A))
#define MAT4_LE_PCM_U8 (MAKE_MARKER(0x32, 0, 0, 0))

static int mat4_close(SF_PRIVATE *psf);

static int mat4_format_to_encoding(int format, int endian);

static int mat4_write_header(SF_PRIVATE *psf, int calc_length);
static int mat4_read_header(SF_PRIVATE *psf);

static const char *mat4_marker_to_str(uint32_t marker);

int mat4_open(SF_PRIVATE *psf)
{
    int subformat, error = 0;

    if (psf->m_mode == SFM_READ || (psf->m_mode == SFM_RDWR && psf->m_filelength > 0))
    {
        if ((error = mat4_read_header(psf)))
            return error;
    };

    if ((SF_CONTAINER(psf->sf.format)) != SF_FORMAT_MAT4)
        return SFE_BAD_OPEN_FORMAT;

    subformat = SF_CODEC(psf->sf.format);

    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
    {
        psf->m_endian = SF_ENDIAN(psf->sf.format);
        if (CPU_IS_LITTLE_ENDIAN && (psf->m_endian == SF_ENDIAN_CPU || psf->m_endian == 0))
            psf->m_endian = SF_ENDIAN_LITTLE;
        else if (CPU_IS_BIG_ENDIAN && (psf->m_endian == SF_ENDIAN_CPU || psf->m_endian == 0))
            psf->m_endian = SF_ENDIAN_BIG;

        if ((error = mat4_write_header(psf, SF_FALSE)))
            return error;

        psf->write_header = mat4_write_header;
    };

    psf->container_close = mat4_close;

    psf->m_blockwidth = psf->m_bytewidth * psf->sf.channels;

    switch (subformat)
    {
    case SF_FORMAT_PCM_16:
    case SF_FORMAT_PCM_32:
        error = pcm_init(psf);
        break;

    case SF_FORMAT_FLOAT:
        error = float32_init(psf);
        break;

    case SF_FORMAT_DOUBLE:
        error = double64_init(psf);
        break;

    default:
        break;
    };

    return error;
}

static int mat4_close(SF_PRIVATE *psf)
{
    if (psf->m_mode == SFM_WRITE || psf->m_mode == SFM_RDWR)
        mat4_write_header(psf, SF_TRUE);

    return 0;
}

static int mat4_write_header(SF_PRIVATE *psf, int calc_length)
{
    sf_count_t current;
    int encoding;
    double samplerate;

    current = psf->ftell();

    if (calc_length)
    {
        psf->m_filelength = psf->get_filelen();

        psf->m_datalength = psf->m_filelength - psf->m_dataoffset;
        if (psf->m_dataend)
            psf->m_datalength -= psf->m_filelength - psf->m_dataend;

        psf->sf.frames = psf->m_datalength / (psf->m_bytewidth * psf->sf.channels);
    };

    encoding = mat4_format_to_encoding(SF_CODEC(psf->sf.format), psf->m_endian);

    if (encoding == -1)
        return SFE_BAD_OPEN_FORMAT;

    /* Reset the current header length to zero. */
    psf->m_header.ptr[0] = 0;
    psf->m_header.indx = 0;
    psf->fseek(0, SEEK_SET);

    /* Need sample rate as a double for writing to the header. */
    samplerate = psf->sf.samplerate;

    if (psf->m_endian == SF_ENDIAN_BIG)
    {
        psf->binheader_writef("Em444", BHWm(MAT4_BE_DOUBLE), BHW4(1), BHW4(1), BHW4(0));
        psf->binheader_writef("E4bd", BHW4(11), BHWv("samplerate"), BHWz(11), BHWd(samplerate));
        psf->binheader_writef("tEm484", BHWm(encoding), BHW4(psf->sf.channels),
                             BHW8(psf->sf.frames), BHW4(0));
        psf->binheader_writef("E4b", BHW4(9), BHWv("wavedata"), BHWz(9));
    }
    else if (psf->m_endian == SF_ENDIAN_LITTLE)
    {
        psf->binheader_writef("em444", BHWm(MAT4_LE_DOUBLE), BHW4(1), BHW4(1), BHW4(0));
        psf->binheader_writef("e4bd", BHW4(11), BHWv("samplerate"), BHWz(11), BHWd(samplerate));
        psf->binheader_writef("tem484", BHWm(encoding), BHW4(psf->sf.channels),
                             BHW8(psf->sf.frames), BHW4(0));
        psf->binheader_writef("e4b", BHW4(9), BHWv("wavedata"), BHWz(9));
    }
    else
        return SFE_BAD_OPEN_FORMAT;

    /* Header construction complete so write it out. */
    psf->fwrite(psf->m_header.ptr, psf->m_header.indx, 1);

    if (psf->m_error)
        return psf->m_error;

    psf->m_dataoffset = psf->m_header.indx;

    if (current > 0)
        psf->fseek(current, SEEK_SET);

    return psf->m_error;
}

static int mat4_read_header(SF_PRIVATE *psf)
{
    char buffer[256];
    uint32_t marker, namesize;
    int rows, cols, imag;
    double value;
    const char *marker_str;
    char name[64];

	psf->binheader_seekf(0, SF_SEEK_SET);
    psf->binheader_readf("m", &marker);

    /* MAT4 file must start with a double for the samplerate. */
    if (marker == MAT4_BE_DOUBLE)
    {
        psf->m_endian = psf->m_rwf_endian = SF_ENDIAN_BIG;
        marker_str = "big endian double";
    }
    else if (marker == MAT4_LE_DOUBLE)
    {
        psf->m_endian = psf->m_rwf_endian = SF_ENDIAN_LITTLE;
        marker_str = "little endian double";
    }
    else
    {
        return SFE_UNIMPLEMENTED;
    }

    psf->log_printf("GNU Octave 2.0 / MATLAB v4.2 format\nMarker : %s\n", marker_str);

    psf->binheader_readf("444", &rows, &cols, &imag);

    psf->log_printf(" Rows  : %d\n Cols  : %d\n Imag  : %s\n", rows, cols,
                   imag ? "True" : "False");

    psf->binheader_readf("4", &namesize);

    if (namesize >= SIGNED_SIZEOF(name))
        return SFE_MAT4_BAD_NAME;

    psf->binheader_readf("b", name, namesize);
    name[namesize] = 0;

    psf->log_printf(" Name  : %s\n", name);

    psf->binheader_readf("d", &value);

    snprintf(buffer, sizeof(buffer), " Value : %f\n", value);
    psf->log_printf(buffer);

    if ((rows != 1) || (cols != 1))
        return SFE_MAT4_NO_SAMPLERATE;

    psf->sf.samplerate = lrint(value);

    /* Now write out the audio data. */

    psf->binheader_readf("m", &marker);

    psf->log_printf("Marker : %s\n", mat4_marker_to_str(marker));

    psf->binheader_readf("444", &rows, &cols, &imag);

    psf->log_printf(" Rows  : %d\n Cols  : %d\n Imag  : %s\n", rows, cols,
                   imag ? "True" : "False");

    psf->binheader_readf("4", &namesize);

    if (namesize >= SIGNED_SIZEOF(name))
        return SFE_MAT4_BAD_NAME;

    psf->binheader_readf("b", name, namesize);
    name[namesize] = 0;

    psf->log_printf(" Name  : %s\n", name);

    psf->m_dataoffset = psf->ftell();

    if (rows == 0)
    {
        psf->log_printf("*** Error : zero channel count.\n");
        return SFE_CHANNEL_COUNT_ZERO;
    }
    else if (rows > SF_MAX_CHANNELS)
    {
        psf->log_printf("*** Error : channel count %d > SF_MAX_CHANNELS.\n", rows);
        return SFE_CHANNEL_COUNT;
    };

    psf->sf.channels = rows;
    psf->sf.frames = cols;

    psf->sf.format = psf->m_endian | SF_FORMAT_MAT4;
    switch (marker)
    {
    case MAT4_BE_DOUBLE:
    case MAT4_LE_DOUBLE:
        psf->sf.format |= SF_FORMAT_DOUBLE;
        psf->m_bytewidth = 8;
        break;

    case MAT4_BE_FLOAT:
    case MAT4_LE_FLOAT:
        psf->sf.format |= SF_FORMAT_FLOAT;
        psf->m_bytewidth = 4;
        break;

    case MAT4_BE_PCM_32:
    case MAT4_LE_PCM_32:
        psf->sf.format |= SF_FORMAT_PCM_32;
        psf->m_bytewidth = 4;
        break;

    case MAT4_BE_PCM_16:
    case MAT4_LE_PCM_16:
        psf->sf.format |= SF_FORMAT_PCM_16;
        psf->m_bytewidth = 2;
        break;

    default:
        psf->log_printf("*** Error : Bad marker %08X\n", marker);
        return SFE_UNIMPLEMENTED;
    };

    if ((psf->m_filelength - psf->m_dataoffset) < psf->sf.channels * psf->sf.frames * psf->m_bytewidth)
    {
        psf->log_printf("*** File seems to be truncated. %D <--> %D\n",
                       psf->m_filelength - psf->m_dataoffset,
                       psf->sf.channels * psf->sf.frames * psf->m_bytewidth);
    }
    else if ((psf->m_filelength - psf->m_dataoffset) >
             psf->sf.channels * psf->sf.frames * psf->m_bytewidth)
    {
        psf->m_dataend = psf->m_dataoffset + rows * cols * psf->m_bytewidth;
    }

    psf->m_datalength = psf->m_filelength - psf->m_dataoffset - psf->m_dataend;

    psf->sf.sections = 1;

    return 0;
}

static int mat4_format_to_encoding(int format, int endian)
{
    switch (format | endian)
    {
    case (SF_FORMAT_PCM_16 | SF_ENDIAN_BIG):
        return MAT4_BE_PCM_16;

    case (SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE):
        return MAT4_LE_PCM_16;

    case (SF_FORMAT_PCM_32 | SF_ENDIAN_BIG):
        return MAT4_BE_PCM_32;

    case (SF_FORMAT_PCM_32 | SF_ENDIAN_LITTLE):
        return MAT4_LE_PCM_32;

    case (SF_FORMAT_FLOAT | SF_ENDIAN_BIG):
        return MAT4_BE_FLOAT;

    case (SF_FORMAT_FLOAT | SF_ENDIAN_LITTLE):
        return MAT4_LE_FLOAT;

    case (SF_FORMAT_DOUBLE | SF_ENDIAN_BIG):
        return MAT4_BE_DOUBLE;

    case (SF_FORMAT_DOUBLE | SF_ENDIAN_LITTLE):
        return MAT4_LE_DOUBLE;

    default:
        break;
    };

    return -1;
}

static const char *mat4_marker_to_str(uint32_t marker)
{
    static char str[32];

    switch (marker)
    {
    case MAT4_BE_PCM_16:
        return "big endian 16 bit PCM";
    case MAT4_LE_PCM_16:
        return "little endian 16 bit PCM";

    case MAT4_BE_PCM_32:
        return "big endian 32 bit PCM";
    case MAT4_LE_PCM_32:
        return "little endian 32 bit PCM";

    case MAT4_BE_FLOAT:
        return "big endian float";
    case MAT4_LE_FLOAT:
        return "big endian float";

    case MAT4_BE_DOUBLE:
        return "big endian double";
    case MAT4_LE_DOUBLE:
        return "little endian double";
    };

    /* This is a little unsafe but is really only for debugging. */
    str[sizeof(str) - 1] = 0;
    snprintf(str, sizeof(str) - 1, "%08X", marker);
    return str;
}
