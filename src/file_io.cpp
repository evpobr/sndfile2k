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

static sf_count_t vfget_filelen(void *user_data);
static int vfset_filelen(void *user_data, sf_count_t len);
static sf_count_t vfseek(sf_count_t offset, int whence, void *user_data);
static sf_count_t vfread(void *ptr, sf_count_t count, void *user_data);
static sf_count_t vfwrite(const void *ptr, sf_count_t count, void *user_data);
static sf_count_t vftell(void *user_data);
static void vfflush(void *user_data);
static unsigned long vfref(void *user_data);
static void vfunref(void *user_data);

static int psf_close_fd(int fd);
static int psf_open_fd(PSF_FILE *pfile);
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

int SF_PRIVATE::fopen()
{
	error = 0;
	file.filedes = psf_open_fd(&file);

	if (file.filedes == -SFE_BAD_OPEN_MODE)
	{
		error = SFE_BAD_OPEN_MODE;
		file.filedes = -1;
		return error;
	};

	if (file.filedes == -1)
	{
		psf_log_syserr(this, errno);
	}
	else
	{
		file.virtual_io = SF_TRUE;
		file.vio = *psf_get_vio();
		file.vio_user_data = this;
		file.use_new_vio = SF_TRUE;
		file.vio.ref(this);
	}

	return error;
}

int SF_PRIVATE::fclose()
{
	int retval = 0;

	if (file.virtual_io)
	{
		if (file.use_new_vio && file.vio.ref && file.vio.unref)
		{
			file.vio.unref(this);
			retval = error;
		}
	}
	else
	{
		if (file.do_not_close_descriptor)
		{
			file.filedes = -1;
			return 0;
		};

		if ((retval = psf_close_fd(file.filedes)) == -1)
			psf_log_syserr(this, errno);

		file.filedes = -1;
	}

	return retval;
}

sf_count_t SF_PRIVATE::get_filelen()
{
	sf_count_t filelen;

	if (file.virtual_io)
	{
		if (file.use_new_vio)
		{
			filelen = file.vio.get_filelen(file.vio_user_data);
		}
		else
		{
			return file.vio.get_filelen(file.vio_user_data);
		}
	}

	if (filelen == -1)
	{
		psf_log_syserr(this, errno);
		return (sf_count_t)-1;
	};

	if (filelen == -SFE_BAD_STAT_SIZE)
	{
		error = SFE_BAD_STAT_SIZE;
		return (sf_count_t)-1;
	};

	return filelen;
}

void SF_PRIVATE::set_file(int fd)
{
	file.filedes = fd;
}

int SF_PRIVATE::file_valid()
{
	return (file.filedes >= 0) ? SF_TRUE : SF_FALSE;
}

sf_count_t SF_PRIVATE::fseek(sf_count_t offset, int whence)
{
	sf_count_t absolute_position;

	if (file.virtual_io && !file.use_new_vio)
		return file.vio.seek(offset, whence, file.vio_user_data);

	absolute_position = file.vio.seek(offset, whence, file.vio_user_data);

	if (absolute_position < 0)
		psf_log_syserr(this, errno);

	return absolute_position;
}

size_t SF_PRIVATE::fread(void *ptr, size_t bytes, size_t items)
{
	sf_count_t total = 0;
	ssize_t count;

	if (file.virtual_io && !file.use_new_vio)
		return file.vio.read(ptr, bytes * items, file.vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the read down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : (ssize_t)items;

		count = file.vio.read(((char *)ptr) + total, (size_t)count, file.vio_user_data);

		if (count == -1)
		{
			if (errno == EINTR)
				continue;

			psf_log_syserr(this, errno);
			break;
		};

		if (count == 0)
			break;

		total += count;
		items -= count;
	};

	return total / bytes;
}

size_t SF_PRIVATE::fwrite(const void *ptr, size_t bytes, size_t items)
{
	sf_count_t total = 0;
	ssize_t count;

	if (bytes == 0 || items == 0)
		return 0;

	if (file.virtual_io && !file.use_new_vio)
		return file.vio.write(ptr, bytes * items, file.vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items;

		count = file.vio.write(((const char *)ptr) + total, count, file.vio_user_data);

		if (count == -1)
		{
			if (errno == EINTR)
				continue;

			psf_log_syserr(this, errno);
			break;
		};

		if (count == 0)
			break;

		total += count;
		items -= count;
	};

	return total / bytes;
}

sf_count_t SF_PRIVATE::ftell()
{
	sf_count_t pos;

	if (file.virtual_io && !file.use_new_vio)
		return file.vio.tell(file.vio_user_data);

	pos = file.vio.tell(file.vio_user_data);

	if (pos == ((sf_count_t)-1))
	{
		psf_log_syserr(this, errno);
		return -1;
	};

	return pos;
}

unsigned long vfref(void * user_data)
{
	unsigned long ref_counter = 0;

	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
	if (psf && psf->file.virtual_io && psf->file.use_new_vio && psf->file.vio.ref && psf->file.vio.unref)
	{
		ref_counter = ++psf->file.vio_ref_counter;
	}
	return ref_counter;
}

void vfunref(void * user_data)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
	if (psf && psf->file.virtual_io && psf->file.use_new_vio && psf->file.vio.ref && psf->file.vio.unref)
	{
		psf->file.vio_ref_counter--;
		if (psf->file.vio_ref_counter == 0)
		{
			psf->error = close(psf->file.filedes);			
			psf->file.filedes = -1;
		}
	}
}

static int psf_close_fd(int fd)
{
	int retval;

	if (fd < 0)
		return 0;

	while ((retval = close(fd)) == -1 && errno == EINTR)
		/* Do nothing. */;

	return retval;
}

sf_count_t SF_PRIVATE::fgets(char *buffer, size_t bufsize)
{
	sf_count_t k = 0;
	sf_count_t count;

	while (k < bufsize - 1)
	{
		count = read(file.filedes, &(buffer[k]), 1);

		if (count == -1)
		{
			if (errno == EINTR)
				continue;

			psf_log_syserr(this, errno);
			break;
		};

		if (count == 0 || buffer[k++] == '\n')
			break;
	};

	buffer[k] = 0;

	return k;
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

int SF_PRIVATE::ftruncate(sf_count_t len)
{
	if (file.virtual_io && file.use_new_vio && file.vio.set_filelen)
		return file.vio.set_filelen(this, len);

	int retval;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return -1;

	if ((sizeof(off_t) < sizeof(sf_count_t)) && len > 0x7FFFFFFF)
		return -1;

#ifdef _WIN32
	retval = _chsize_s(file.filedes, len);
#else
	retval = ::ftruncate(file.filedes, len);
#endif

	if (retval != 0)
		psf_log_syserr(this, errno);

	return retval;
}

void SF_PRIVATE::init_files()
{
	file.filedes = -1;
	file.savedes = -1;
}

static int psf_open_fd(PSF_FILE *pfile)
{
	int fd, oflag, mode;

	/*
	* Sanity check. If everything is OK, this test and the printfs will
	* be optimised out. This is meant to catch the problems caused by
	* "sfconfig.h" being included after <stdio.h>.
	*/
	if (sizeof(sf_count_t) != 8)
	{
		puts("\n\n*** Fatal error : sizeof (sf_count_t) != 8");
		puts("*** This means that libsndfile was not configured correctly.\n");
		exit(1);
	};

	switch (pfile->mode)
	{
	case SFM_READ:
		oflag = O_BINARY | O_RDONLY;
		mode = 0;
		break;

	case SFM_WRITE:
		oflag = O_BINARY | O_WRONLY | O_CREAT | O_TRUNC;
		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
		break;

	case SFM_RDWR:
		oflag = O_BINARY | O_RDWR | O_CREAT;
		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
		break;

	default:
		return -SFE_BAD_OPEN_MODE;
		break;
	};

	if (mode == 0)
	{
#if defined(_WIN32)
		if (pfile->use_wchar)
			fd = _wopen(pfile->path.wc, oflag);
		else
			fd = open(pfile->path.c, oflag);
#else
		fd = open(pfile->path.c, oflag);
#endif
	}
	else
	{
#if defined(_WIN32)
		if (pfile->use_wchar)
			fd = _wopen(pfile->path.wc, oflag, mode);
		else
			fd = open(pfile->path.c, oflag, mode);
#else
		fd = open(pfile->path.c, oflag, mode);
#endif
	}

	return fd;
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

void SF_PRIVATE::fsync()
{
	if (file.mode == SFM_WRITE || file.mode == SFM_RDWR)
#ifdef HAVE_FSYNC
		::fsync(file.filedes);
#elif _WIN32
		_commit(file.filedes);
#else
	psf = NULL;
#endif
}

static sf_count_t vfget_filelen(void *user_data)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
	return psf_get_filelen_fd(psf->file.filedes);
}

static sf_count_t vfseek(sf_count_t offset, int whence, void *user_data)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
#ifdef _WIN32
	return _lseeki64(psf->file.filedes, offset, whence);
#else
	return lseek(psf->file.filedes, offset, whence);
#endif
}

static sf_count_t vfread(void *ptr, sf_count_t count, void *user_data)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
	return read(psf->file.filedes, ptr, count);
}

static sf_count_t vfwrite(const void *ptr, sf_count_t count, void *user_data)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
	return write(psf->file.filedes, ptr, count);
}

static sf_count_t vftell(void *user_data)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
#if _WIN32
	return _lseeki64(psf->file.filedes, 0, SEEK_CUR);
#else
	return lseek(psf->file.filedes, 0, SEEK_CUR);
#endif
}

static void vfflush(void *user_data)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
	if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
#ifdef HAVE_FSYNC
		::fsync(psf->file.filedes);
#elif _WIN32
		_commit(psf->file.filedes);
#endif
}


int vfset_filelen(void * user_data, sf_count_t len)
{
	SF_PRIVATE *psf = (SF_PRIVATE *)user_data;
	int retval;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return -1;

	if ((sizeof(off_t) < sizeof(sf_count_t)) && len > 0x7FFFFFFF)
		return -1;

#ifdef _WIN32
	retval = _chsize_s(psf->file.filedes, len);
#else
	retval = ::ftruncate(psf->file.filedes, len);
#endif

	if (retval != 0)
		psf_log_syserr(psf, errno);

	return retval;
}
