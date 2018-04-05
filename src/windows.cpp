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

static void copy_filename(SF_PRIVATE *psf, LPCWSTR wpath);

SNDFILE *sf_wchar_open(const wchar_t *wpath, SF_FILEMODE mode, SF_INFO *sfinfo)
{
    SF_PRIVATE *psf;
    char utf8name[512];

    if ((psf = psf_allocate()) == NULL)
    {
        sf_errno = SFE_MALLOC_FAILED;
        return NULL;
    };

    psf->init_files();

    if (WideCharToMultiByte(CP_UTF8, 0, wpath, -1, utf8name, sizeof(utf8name), NULL, NULL) == 0)
        psf->file.path.wc[0] = 0;

    psf->log_printf("File : '%s' (utf-8 converted from ucs-2)\n", utf8name);
    copy_filename(psf, wpath);
    psf->file.use_wchar = SF_TRUE;
    psf->file.mode = mode;

    psf->error = psf->fopen();

    return psf->open_file(sfinfo);
}

static void copy_filename(SF_PRIVATE *psf, LPCWSTR wpath)
{
    const wchar_t *cwcptr;
    wchar_t *wcptr;

    wcsncpy(psf->file.path.wc, wpath, ARRAY_LEN(psf->file.path.wc));
    psf->file.path.wc[ARRAY_LEN(psf->file.path.wc) - 1] = 0;
    if ((cwcptr = wcsrchr(wpath, '/')) || (cwcptr = wcsrchr(wpath, '\\')))
        cwcptr++;
    else
        cwcptr = wpath;

    wcsncpy(psf->file.name.wc, cwcptr, ARRAY_LEN(psf->file.name.wc));
    psf->file.name.wc[ARRAY_LEN(psf->file.name.wc) - 1] = 0;

    /* Now grab the directory. */
    wcsncpy(psf->file.dir.wc, wpath, ARRAY_LEN(psf->file.dir.wc));
    psf->file.dir.wc[ARRAY_LEN(psf->file.dir.wc) - 1] = 0;

    if ((wcptr = wcsrchr(psf->file.dir.wc, '/')) || (wcptr = wcsrchr(psf->file.dir.wc, '\\')))
        wcptr[1] = 0;
    else
        psf->file.dir.wc[0] = 0;

    return;
}

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
