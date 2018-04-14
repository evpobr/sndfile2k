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

extern int sf_errno;

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <windows.h>



SNDFILE *sf_wchar_open(const wchar_t *path, SF_FILEMODE mode, SF_INFO *sfinfo)
{
    //SF_PRIVATE *psf;

    //if ((psf = psf_allocate()) == NULL)
    //{
    //    sf_errno = SFE_MALLOC_FAILED;
    //    return NULL;
    //};

    //wcstombs(psf->_path, path, FILENAME_MAX);

    //psf->log_printf("File : %s\n", path);

    //psf->file_mode = mode;
    //psf->error = psf->fopen(path, mode, sfinfo);
    //if (psf->error != SFE_NO_ERROR)
    //{
    //    sf_errno = psf->error;
    //    return nullptr;
    //}

    //return psf->open_file(sfinfo);
    return NULL;
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
