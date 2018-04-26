/*
** Copyright (C) 2009-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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
**	This needs to be a separate file so that we don't have to include
**	<windows.h> elsewhere (too many symbol clashes).
*/

#include "config.h"

#if defined(_WIN32) || defined(__CYGWIN__)

#include "sndfile2k/sndfile2k.h"
#include "common.h"
#include "ref_ptr.h"
#include "sndfile_error.h"

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <windows.h>

bool guess_file_type(sf::ref_ptr<SF_STREAM> &stream, SF_INFO *sfinfo);
bool format_from_extension(const char *path, SF_INFO *sfinfo);

int validate_sfinfo(SF_INFO *sfinfo);
int validate_psf(SndFile *psf);
void save_header_info(SndFile *psf);

int sf_wchar_open(const wchar_t *path, SF_FILEMODE mode, SF_INFO *sfinfo, SNDFILE **sndfile)
{
    // Input parameters check

    if (mode != SFM_READ && mode != SFM_WRITE && mode != SFM_RDWR)
        return SFE_BAD_OPEN_MODE;

    if (!sfinfo)
        return SFE_BAD_SF_INFO_PTR;

    if (!sndfile)
        return SFE_BAD_FILE_PTR;

    *sndfile = nullptr;

    // Handle open modes

    // Read mode
    if (mode == SFM_READ)
    {
        // For RAW files sfinfo parameter must be properly set
        if ((SF_CONTAINER(sfinfo->format)) == SF_FORMAT_RAW)
        {
            if (sf_format_check(sfinfo) == 0)
            {
                return SFE_RAW_BAD_FORMAT;
            };
        }
        // For any other format in read mode sfinfo parameter is
        // ignored, set its fields to zero.
        else
        {
            memset(sfinfo, 0, sizeof(SF_INFO));
        }
};

    SndFile *psf = nullptr;
    try
    {
        psf = new SndFile();

        sf::ref_ptr<SF_STREAM> stream;
        int error = psf_open_file_stream(path, mode, stream.get_address_of());
        if (error != SFE_NO_ERROR)
            throw sf::sndfile_error(error);

        char ansi_path[FILENAME_MAX];
        wcstombs(ansi_path, path, FILENAME_MAX);

        // Need this to detect if we create new file (filelength = 0).
        sf_count_t filelength = stream->get_filelen();

        if (mode == SFM_WRITE || (mode == SFM_RDWR && filelength == 0))
        {
            // If the file is being opened for write or RDWR and the file is currently
            // empty, then the SF_INFO struct must contain valid data.
            if ((SF_CONTAINER(sfinfo->format)) == 0)
                throw sf::sndfile_error(SFE_ZERO_MAJOR_FORMAT);

            if ((SF_CODEC(sfinfo->format)) == 0)
                throw sf::sndfile_error(SFE_ZERO_MINOR_FORMAT);

            if (sf_format_check(sfinfo) == 0)
                throw sf::sndfile_error(SFE_BAD_OPEN_FORMAT);
        }
        else if ((SF_CONTAINER(sfinfo->format)) != SF_FORMAT_RAW)
        {
            // If type RAW has not been specified then need to figure out file type.
            if (!guess_file_type(stream, sfinfo))
            {
                if (!format_from_extension(ansi_path, sfinfo))
                {
                    throw sf::sndfile_error(SFE_BAD_OPEN_FORMAT);
                }
            }
        };

        // Here we have format detected

        psf->log_printf("File : %s\n", ansi_path);
        strcpy(psf->m_path, ansi_path);

        psf->open(stream, mode, sfinfo);
        if (!psf->is_open())
            throw sf::sndfile_error(psf->m_error);

        /* Call the initialisation function for the relevant file type. */
        switch (SF_CONTAINER(psf->sf.format))
        {
        case SF_FORMAT_WAV:
        case SF_FORMAT_WAVEX:
            error = wav_open(psf);
            break;

        case SF_FORMAT_AIFF:
            error = aiff_open(psf);
            break;

        case SF_FORMAT_AU:
            error = au_open(psf);
            break;

        case SF_FORMAT_RAW:
            error = raw_open(psf);
            break;

        case SF_FORMAT_W64:
            error = w64_open(psf);
            break;

        case SF_FORMAT_RF64:
            error = rf64_open(psf);
            break;

        case SF_FORMAT_PAF:
            error = paf_open(psf);
            break;

        case SF_FORMAT_SVX:
            error = svx_open(psf);
            break;

        case SF_FORMAT_NIST:
            error = nist_open(psf);
            break;

        case SF_FORMAT_IRCAM:
            error = ircam_open(psf);
            break;

        case SF_FORMAT_VOC:
            error = voc_open(psf);
            break;

        case SF_FORMAT_SDS:
            error = sds_open(psf);
            break;

        case SF_FORMAT_OGG:
            error = ogg_open(psf);
            break;

        case SF_FORMAT_TXW:
            error = txw_open(psf);
            break;

        case SF_FORMAT_WVE:
            error = wve_open(psf);
            break;

        case SF_FORMAT_DWD:
            error = dwd_open(psf);
            break;

        case SF_FORMAT_MAT4:
            error = mat4_open(psf);
            break;

        case SF_FORMAT_MAT5:
            error = mat5_open(psf);
            break;

        case SF_FORMAT_PVF:
            error = pvf_open(psf);
            break;

        case SF_FORMAT_XI:
            error = xi_open(psf);
            break;

        case SF_FORMAT_HTK:
            error = htk_open(psf);
            break;

        case SF_FORMAT_REX2:
            error = rx2_open(psf);
            break;

        case SF_FORMAT_AVR:
            error = avr_open(psf);
            break;

        case SF_FORMAT_FLAC:
            error = flac_open(psf);
            break;

        case SF_FORMAT_CAF:
            error = caf_open(psf);
            break;

        case SF_FORMAT_MPC2K:
            error = mpc2k_open(psf);
            break;

        default:
            error = SF_ERR_UNRECOGNISED_FORMAT;
        };

        if (error != SFE_NO_ERROR)
            throw sf::sndfile_error(error);

        if (mode == SFM_RDWR && sf_format_check(&psf->sf) == 0)
            throw sf::sndfile_error(SFE_BAD_MODE_RW);

        if (validate_sfinfo(&psf->sf) == 0)
        {
            psf->log_SF_INFO();
            save_header_info(psf);
            throw sf::sndfile_error(SFE_BAD_SF_INFO);
        };

        if (validate_psf(psf) == 0)
        {
            save_header_info(psf);
            throw sf::sndfile_error(SFE_INTERNAL);
        };

        psf->m_read_current = 0;
        psf->m_write_current = 0;
        if (mode == SFM_RDWR)
        {
            psf->m_write_current = psf->sf.frames;
            psf->m_have_written = psf->sf.frames > 0 ? true : false;
        };

        memcpy(sfinfo, &psf->sf, sizeof(SF_INFO));

        if (mode == SFM_WRITE)
        {
            /* Zero out these fields. */
            sfinfo->frames = 0;
            sfinfo->sections = 0;
            sfinfo->seekable = 0;
        };

        *sndfile = static_cast<SNDFILE *>(psf);
        psf->ref();

        return SF_ERR_NO_ERROR;
    }
    catch (sf::sndfile_error &e)
    {
        delete psf;

        return e.error();
    };
} /* sf_open */

// CYGWIN
#else

SNDFILE *sf_wchar_open(const wchar_t *wpath, int mode, SF_INFO *sfinfo)
{
    char utf8name[512];

    wcstombs(utf8name, wpath, sizeof(utf8name));
    return sf_open(utf8name, mode, sfinfo);
};

#endif

#endif
