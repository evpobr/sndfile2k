/*
** Copyright (C) 2002-2014 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2003 Ross Bencina <rbencina@iprimus.com.au>
** Copyright (C) 2018 evpobr <evpobr@gmail.com>
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

#include "common.h"

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sf_unistd.h>

using namespace std;

/*=======================================================================================
**	SF_PRIVATE stuct - a pointer to this struct is passed back to the caller of the
**	sf_open_XXXX functions. The caller however has no knowledge of the struct's
**	contents.
*/

struct PSF_FILE
{
    SF_VIRTUAL_IO vio;
    void *vio_user_data;
    unsigned long vio_ref_counter;
    int filedes;
};

static sf_count_t vfget_filelen(void *user_data);
static int vfset_filelen(void *user_data, sf_count_t len);
static sf_count_t vfseek(sf_count_t offset, int whence, void *user_data);
static sf_count_t vfread(void *ptr, sf_count_t count, void *user_data);
static sf_count_t vfwrite(const void *ptr, sf_count_t count, void *user_data);
static sf_count_t vftell(void *user_data);
static void vfflush(void *user_data);
static unsigned long vfref(void *user_data);
static void vfunref(void *user_data);

static sf_count_t psf_get_filelen_fd(int fd);

static SF_VIRTUAL_IO vio;

SF_VIRTUAL_IO *psf_get_vio()
{
	vio.ref = vfref;
	vio.unref = vfunref;
	vio.get_filelen = vfget_filelen;
	vio.set_filelen = vfset_filelen;
	vio.seek = vfseek;
	vio.read = vfread;
	vio.write = vfwrite;
	vio.flush = vfflush;
	vio.tell = vftell;
	vio.flush = vfflush;

	return &vio;
}

int SF_PRIVATE::fopen(const char *path, SF_FILEMODE mode, SF_INFO *sfinfo)
{
    PSF_FILE *file = new(nothrow) PSF_FILE();
    if (!file)
        return SFE_MALLOC_FAILED;

    int open_flag, share_flag;

    /*
    * Sanity check. If everything is OK, this test and the printfs will
    * be optimised out. This is meant to catch the problems caused by
    * "sfconfig.h" being included after <stdio.h>.
    */
    switch (mode)
    {
    case SFM_READ:
        open_flag = O_BINARY | O_RDONLY;
        share_flag = 0;
        break;

    case SFM_WRITE:
        open_flag = O_BINARY | O_WRONLY | O_CREAT | O_TRUNC;
        share_flag = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        break;

    case SFM_RDWR:
        open_flag = O_BINARY | O_RDWR | O_CREAT;
        share_flag = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        break;

    default:
        delete file;
        return -SFE_BAD_OPEN_MODE;
        break;
    };

    file->filedes = open(path, open_flag, share_flag);
	if (file->filedes == -SFE_BAD_OPEN_MODE)
	{
		error = SFE_BAD_OPEN_MODE;
		file->filedes = -1;
        delete file;
		return error;
	}
    else if (file->filedes == -1)
	{
		psf_log_syserr(this, errno);
        delete file;
        return error;
	}
	else
	{
		file->vio = *psf_get_vio();
        file->vio_user_data = file;
        file->vio_ref_counter = 0;
        vio = &file->vio;
		vio_user_data = file;
		vio->ref(vio_user_data);
        file_mode = mode;
	}

	return error;
}

#ifdef _WIN32

int SF_PRIVATE::fopen(const wchar_t *path, SF_FILEMODE mode, SF_INFO *sfinfo)
{
    error = 0;

    PSF_FILE *file = new(nothrow) PSF_FILE();
    if (!file)
        return SFE_MALLOC_FAILED;

    int open_flag, share_flag;

    /*
    * Sanity check. If everything is OK, this test and the printfs will
    * be optimised out. This is meant to catch the problems caused by
    * "sfconfig.h" being included after <stdio.h>.
    */
    switch (mode)
    {
    case SFM_READ:
        open_flag = O_BINARY | O_RDONLY;
        share_flag = 0;
        break;

    case SFM_WRITE:
        open_flag = O_BINARY | O_WRONLY | O_CREAT | O_TRUNC;
        share_flag = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        break;

    case SFM_RDWR:
        open_flag = O_BINARY | O_RDWR | O_CREAT;
        share_flag = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
        break;

    default:
        delete file;
        return -SFE_BAD_OPEN_MODE;
        break;
    };

    file->filedes = _wopen(path, open_flag, share_flag);
    if (file->filedes == -SFE_BAD_OPEN_MODE)
    {
        error = SFE_BAD_OPEN_MODE;
        file->filedes = -1;
        delete file;
        return error;
    }
    else if (file->filedes == -1)
    {
        psf_log_syserr(this, errno);
        delete file;
        return error;
    }
    else
    {
        file->vio = *psf_get_vio();
        file->vio_user_data = file;
        file->vio_ref_counter = 0;
        vio = &file->vio;
        vio_user_data = file;
        vio->ref(this);
        file_mode = mode;
    }

    return error;
}

#endif

unsigned long vfref(void * user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);

	unsigned long ref_counter = 0;

	if (file && file->vio.ref && file->vio.unref)
	{
		ref_counter = ++file->vio_ref_counter;
	}
	return ref_counter;
}

void vfunref(void * user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);

	if (file && file->vio.unref)
	{
		file->vio_ref_counter--;
		if (file->vio_ref_counter == 0)
		{
            close(file->filedes);
			file->filedes = -1;
		}
	}
}

static sf_count_t psf_get_filelen_fd(int fd)
{
#ifdef _WIN32
	struct _stat64 statbuf;

	if (_fstat64(fd, &statbuf) == -1)
		return (sf_count_t)-1;

	return statbuf.st_size;
#else
	struct stat statbuf;

	if (fstat(fd, &statbuf) == -1)
		return (sf_count_t)-1;

	return statbuf.st_size;
#endif
}

static void psf_log_syserr(SF_PRIVATE *psf, int error)
{
	/* Only log an error if no error has been set yet. */
	if (psf->error == 0)
	{
		psf->error = SFE_SYSTEM;
		snprintf(psf->syserr, sizeof(psf->syserr), "System error : %s.", strerror(error));
	};

	return;
}

static sf_count_t vfget_filelen(void *user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);

    if (!file)
        return -1;

#ifdef _WIN32
    struct _stat64 statbuf;

    if (_fstat64(file->filedes, &statbuf) == -1)
        return (sf_count_t)-1;

    return statbuf.st_size;
#else
    struct stat statbuf;

    if (fstat(file->filedes, &statbuf) == -1)
        return (sf_count_t)-1;

    return statbuf.st_size;
#endif
}

static sf_count_t vfseek(sf_count_t offset, int whence, void *user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);
    if (!file)
        return -1;

#ifdef _WIN32
	return _lseeki64(file->filedes, offset, whence);
#else
	return lseek(file->filedes, offset, whence);
#endif
}

static sf_count_t vfread(void *ptr, sf_count_t count, void *user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);
    if (!file)
        return 0;

	return read(file->filedes, ptr, count);
}

static sf_count_t vfwrite(const void *ptr, sf_count_t count, void *user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);
    if (!file)
        return 0;

	return write(file->filedes, ptr, count);
}

static sf_count_t vftell(void *user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);
    if (!file)
        return -1;

#if _WIN32
	return _lseeki64(file->filedes, 0, SEEK_CUR);
#else
	return lseek(file->filedes, 0, SEEK_CUR);
#endif
}

static void vfflush(void *user_data)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);
    if (!file)
        return;

#ifdef HAVE_FSYNC
    ::fsync(file->filedes);
#elif _WIN32
    _commit(file->filedes);
#endif
}


int vfset_filelen(void * user_data, sf_count_t len)
{
    PSF_FILE *file = reinterpret_cast<PSF_FILE *>(user_data);
    if (!file)
        return -1;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return -1;

	if ((sizeof(off_t) < sizeof(sf_count_t)) && len > 0x7FFFFFFF)
		return -1;

#ifdef _WIN32
	int retval = _chsize_s(file->filedes, len);
#else
	int retval = ::ftruncate(file->filedes, len);
#endif

	return retval;
}
