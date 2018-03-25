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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include <sf_unistd.h>
#endif

static sf_count_t vfget_filelen(void *user_data);
static int vfset_filelen(void *user_data, sf_count_t len);
static sf_count_t vfseek(sf_count_t offset, int whence, void *user_data);
static sf_count_t vfread(void *ptr, sf_count_t count, void *user_data);
static sf_count_t vfwrite(const void *ptr, sf_count_t count, void *user_data);
static sf_count_t vftell(void *user_data);
static void vfflush(void *user_data);
static SF_BOOL vfis_pipe(void *user_data);
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
	vio.is_pipe = vfis_pipe;

	return &vio;
}

int psf_fopen(SF_PRIVATE *psf)
{
	psf->error = 0;
	psf->file.filedes = psf_open_fd(&psf->file);

	if (psf->file.filedes == -SFE_BAD_OPEN_MODE)
	{
		psf->error = SFE_BAD_OPEN_MODE;
		psf->file.filedes = -1;
		return psf->error;
	};

	if (psf->file.filedes == -1)
	{
		psf_log_syserr(psf, errno);
	}
	else
	{
		psf->file.virtual_io = SF_TRUE;
		psf->file.vio = *psf_get_vio();
		psf->file.vio_user_data = psf;
		psf->file.use_new_vio = SF_TRUE;
		psf->file.vio.ref(psf);
	}

	return psf->error;
}

int psf_fclose(SF_PRIVATE *psf)
{
	int retval = 0;

	if (psf->file.virtual_io)
	{
		if (psf->file.use_new_vio && psf->file.vio.ref && psf->file.vio.unref)
		{
			psf->file.vio.unref(psf);
			retval = psf->error;
		}
	}
	else
	{
		if (psf->file.do_not_close_descriptor)
		{
			psf->file.filedes = -1;
			return 0;
		};

		if ((retval = psf_close_fd(psf->file.filedes)) == -1)
			psf_log_syserr(psf, errno);

		psf->file.filedes = -1;
	}

	return retval;
}

int psf_open_rsrc(SF_PRIVATE *psf)
{
	if (psf->rsrc.filedes > 0)
		return 0;

	/* Test for MacOSX style resource fork on HPFS or HPFS+ filesystems. */
	snprintf(psf->rsrc.path.c, sizeof(psf->rsrc.path.c), "%s/..namedfork/rsrc", psf->file.path.c);
	psf->error = SFE_NO_ERROR;
	if ((psf->rsrc.filedes = psf_open_fd(&psf->rsrc)) >= 0)
	{
		psf->rsrclength = psf_get_filelen_fd(psf->rsrc.filedes);
		if (psf->rsrclength > 0 || (psf->rsrc.mode & SFM_WRITE))
			return SFE_NO_ERROR;
		psf_close_fd(psf->rsrc.filedes);
		psf->rsrc.filedes = -1;
	};

	if (psf->rsrc.filedes == -SFE_BAD_OPEN_MODE)
	{
		psf->error = SFE_BAD_OPEN_MODE;
		return psf->error;
	};

	/*
	** Now try for a resource fork stored as a separate file in the same
	** directory, but preceded with a dot underscore.
	*/
	snprintf(psf->rsrc.path.c, sizeof(psf->rsrc.path.c), "%s._%s", psf->file.dir.c,
			 psf->file.name.c);
	psf->error = SFE_NO_ERROR;
	if ((psf->rsrc.filedes = psf_open_fd(&psf->rsrc)) >= 0)
	{
		psf->rsrclength = psf_get_filelen_fd(psf->rsrc.filedes);
		return SFE_NO_ERROR;
	};

	/*
	** Now try for a resource fork stored in a separate file in the
	** .AppleDouble/ directory.
	*/
	snprintf(psf->rsrc.path.c, sizeof(psf->rsrc.path.c), "%s.AppleDouble/%s", psf->file.dir.c,
			 psf->file.name.c);
	psf->error = SFE_NO_ERROR;
	if ((psf->rsrc.filedes = psf_open_fd(&psf->rsrc)) >= 0)
	{
		psf->rsrclength = psf_get_filelen_fd(psf->rsrc.filedes);
		return SFE_NO_ERROR;
	};

	/* No resource file found. */
	if (psf->rsrc.filedes == -1)
		psf_log_syserr(psf, errno);

	psf->rsrc.filedes = -1;

	return psf->error;
}

sf_count_t psf_get_filelen(SF_PRIVATE *psf)
{
	sf_count_t filelen;

	if (psf->file.virtual_io)
		if (psf->file.use_new_vio)
			filelen = psf->file.vio.get_filelen(psf->file.vio_user_data);
		else
			return psf->file.vio.get_filelen(psf->file.vio_user_data);

	if (filelen == -1)
	{
		psf_log_syserr(psf, errno);
		return (sf_count_t)-1;
	};

	if (filelen == -SFE_BAD_STAT_SIZE)
	{
		psf->error = SFE_BAD_STAT_SIZE;
		return (sf_count_t)-1;
	};

	switch (psf->file.mode)
	{
	case SFM_WRITE:
		filelen = filelen - psf->fileoffset;
		break;

	case SFM_READ:
		if (psf->fileoffset > 0 && psf->filelength > 0)
			filelen = psf->filelength;
		break;

	case SFM_RDWR:
		/*
		** Cannot open embedded files SFM_RDWR so we don't need to
		** subtract psf->fileoffset. We already have the answer we
		** need.
		*/
		break;

	default:
		/* Shouldn't be here, so return error. */
		filelen = -1;
	};

	return filelen;
}

int psf_close_rsrc(SF_PRIVATE *psf)
{
	psf_close_fd(psf->rsrc.filedes);
	psf->rsrc.filedes = -1;
	return 0;
}

int psf_set_stdio(SF_PRIVATE *psf)
{
	int error = 0;

	switch (psf->file.mode)
	{
	case SFM_RDWR:
		error = SFE_OPEN_PIPE_RDWR;
		break;

	case SFM_READ:
		psf->file.filedes = 0;
		break;

	case SFM_WRITE:
		psf->file.filedes = 1;
		break;

	default:
		error = SFE_BAD_OPEN_MODE;
		break;
	};
	psf->filelength = 0;

	return error;
}

void psf_set_file(SF_PRIVATE *psf, int fd)
{
	psf->file.filedes = fd;
}

int psf_file_valid(SF_PRIVATE *psf)
{
	return (psf->file.filedes >= 0) ? SF_TRUE : SF_FALSE;
}

sf_count_t psf_fseek(SF_PRIVATE *psf, sf_count_t offset, int whence)
{
	sf_count_t absolute_position;

	if (psf->file.virtual_io && !psf->file.use_new_vio)
		return psf->file.vio.seek(offset, whence, psf->file.vio_user_data);

	/* When decoding from pipes sometimes see seeks to the pipeoffset, which
	appears to mean do nothing. */
	if (psf->file.is_pipe)
	{
		if (whence != SEEK_SET || offset != psf->file.pipeoffset)
			psf_log_printf(psf, "psf_fseek : pipe seek to value other than pipeoffset\n");
		return offset;
	}

	switch (whence)
	{
	case SEEK_SET:
		offset += psf->fileoffset;
		break;

	case SEEK_END:
		break;

	case SEEK_CUR:
		break;

	default:
		/* We really should not be here. */
		psf_log_printf(psf, "psf_fseek : whence is %d *****.\n", whence);
		return 0;
	};

	absolute_position = psf->file.vio.seek(offset, whence, psf->file.vio_user_data);

	if (absolute_position < 0)
		psf_log_syserr(psf, errno);

	return absolute_position - psf->fileoffset;
}

size_t psf_fread(void *ptr, size_t bytes, size_t items, SF_PRIVATE *psf)
{
	sf_count_t total = 0;
	ssize_t count;

	if (psf->file.virtual_io && !psf->file.use_new_vio)
		return psf->file.vio.read(ptr, bytes * items, psf->file.vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the read down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : (ssize_t)items;

		count = psf->file.vio.read(((char *)ptr) + total, (size_t)count, psf->file.vio_user_data);

		if (count == -1)
		{
			if (errno == EINTR)
				continue;

			psf_log_syserr(psf, errno);
			break;
		};

		if (count == 0)
			break;

		total += count;
		items -= count;
	};

	if (psf->file.is_pipe)
		psf->file.pipeoffset += total;

	return total / bytes;
}

size_t psf_fwrite(const void *ptr, size_t bytes, size_t items, SF_PRIVATE *psf)
{
	sf_count_t total = 0;
	ssize_t count;

	if (bytes == 0 || items == 0)
		return 0;

	if (psf->file.virtual_io && !psf->file.use_new_vio)
		return psf->file.vio.write(ptr, bytes * items, psf->file.vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items;

		count = psf->file.vio.write(((const char *)ptr) + total, count, psf->file.vio_user_data);

		if (count == -1)
		{
			if (errno == EINTR)
				continue;

			psf_log_syserr(psf, errno);
			break;
		};

		if (count == 0)
			break;

		total += count;
		items -= count;
	};

	if (psf->file.is_pipe)
		psf->file.pipeoffset += total;

	return total / bytes;
}

sf_count_t psf_ftell(SF_PRIVATE *psf)
{
	sf_count_t pos;

	if (psf->file.virtual_io && !psf->file.use_new_vio)
		return psf->file.vio.tell(psf->file.vio_user_data);

	if (psf->file.is_pipe)
		return psf->file.pipeoffset;

	pos = psf->file.vio.tell(psf->file.vio_user_data);

	if (pos == ((sf_count_t)-1))
	{
		psf_log_syserr(psf, errno);
		return -1;
	};

	return pos - psf->fileoffset;
}

SF_BOOL vfis_pipe(void * user_data)
{
	SF_PRIVATE *psf = user_data;
	struct stat statbuf;

	if (psf->file.virtual_io)
	{
		if (psf->file.use_new_vio && psf->file.vio.is_pipe)
		{
			return psf->file.vio.is_pipe(psf);
		}
		else
		{
			return SF_FALSE;
		}
	}
	else
	{
		if (fstat(psf->file.filedes, &statbuf) == -1)
		{
			psf_log_syserr(psf, errno);
			/* Default to maximum safety. */
			return SF_TRUE;
		};

		if (S_ISFIFO(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode))
			return SF_TRUE;
	}

	return SF_FALSE;
}

unsigned long vfref(void * user_data)
{
	unsigned long ref_counter = 0;

	SF_PRIVATE *psf = user_data;
	if (psf && psf->file.virtual_io && psf->file.use_new_vio && psf->file.vio.ref && psf->file.vio.unref)
	{
		ref_counter = ++psf->file.vio_ref_counter;
	}
	return ref_counter;
}

void vfunref(void * user_data)
{
	SF_PRIVATE *psf = user_data;
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

sf_count_t psf_fgets(char *buffer, size_t bufsize, SF_PRIVATE *psf)
{
	sf_count_t k = 0;
	sf_count_t count;

	while (k < bufsize - 1)
	{
		count = read(psf->file.filedes, &(buffer[k]), 1);

		if (count == -1)
		{
			if (errno == EINTR)
				continue;

			psf_log_syserr(psf, errno);
			break;
		};

		if (count == 0 || buffer[k++] == '\n')
			break;
	};

	buffer[k] = 0;

	return k;
}

int psf_is_pipe(SF_PRIVATE *psf)
{
	struct stat statbuf;

	if (psf->file.virtual_io && !psf->file.use_new_vio)
		return SF_FALSE;

	if (fstat(psf->file.filedes, &statbuf) == -1)
	{
		psf_log_syserr(psf, errno);
		/* Default to maximum safety. */
		return SF_TRUE;
	};

	if (S_ISFIFO(statbuf.st_mode) || S_ISSOCK(statbuf.st_mode))
		return SF_TRUE;

	return SF_FALSE;
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

int psf_ftruncate(SF_PRIVATE *psf, sf_count_t len)
{
	if (psf->file.virtual_io && psf->file.use_new_vio && psf->file.vio.set_filelen)
		return psf->file.vio.set_filelen(psf, len);

	int retval;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return -1;

	if ((sizeof(off_t) < sizeof(sf_count_t)) && len > 0x7FFFFFFF)
		return -1;

#ifdef _WIN32
	retval = _chsize_s(psf->file.filedes, len);
#else
	retval = ftruncate(psf->file.filedes, len);
#endif

	if (retval != 0)
		psf_log_syserr(psf, errno);

	return retval;
}

void psf_init_files(SF_PRIVATE *psf)
{
	psf->file.filedes = -1;
	psf->rsrc.filedes = -1;
	psf->file.savedes = -1;
}

void psf_use_rsrc(SF_PRIVATE *psf, int on_off)
{
	if (on_off)
	{
		if (psf->file.filedes != psf->rsrc.filedes)
		{
			psf->file.savedes = psf->file.filedes;
			psf->file.filedes = psf->rsrc.filedes;
		};
	}
	else if (psf->file.filedes == psf->rsrc.filedes)
	{
		psf->file.filedes = psf->file.savedes;
	}

	return;
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
		oflag = O_RDONLY;
#ifdef O_BINARY
		oflag |= O_BINARY;
#endif
		mode = 0;
		break;

	case SFM_WRITE:
		oflag = O_WRONLY | O_CREAT | O_TRUNC;
#ifdef O_BINARY
		oflag |= O_BINARY;
#endif
		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
		break;

	case SFM_RDWR:
		oflag = O_RDWR | O_CREAT;
#ifdef O_BINARY
		oflag |= O_BINARY;
#endif
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

void psf_fsync(SF_PRIVATE *psf)
{
	if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
#ifdef HAVE_FSYNC
		fsync(psf->file.filedes);
#elif _WIN32
		_commit(psf->file.filedes);
#else
	psf = NULL;
#endif
}

static sf_count_t vfget_filelen(void *user_data)
{
	SF_PRIVATE *psf = user_data;
	return psf_get_filelen_fd(psf->file.filedes);
}

static sf_count_t vfseek(sf_count_t offset, int whence, void *user_data)
{
	SF_PRIVATE *psf = user_data;
#ifdef _WIN32
	return _lseeki64(psf->file.filedes, offset, whence);
#else
	return lseek(psf->file.filedes, offset, whence);
#endif
}

static sf_count_t vfread(void *ptr, sf_count_t count, void *user_data)
{
	SF_PRIVATE *psf = user_data;
	return read(psf->file.filedes, ptr, count);
}

static sf_count_t vfwrite(const void *ptr, sf_count_t count, void *user_data)
{
	SF_PRIVATE *psf = user_data;
	return write(psf->file.filedes, ptr, count);
}

static sf_count_t vftell(void *user_data)
{
	SF_PRIVATE *psf = user_data;
#if _WIN32
	return _lseeki64(psf->file.filedes, 0, SEEK_CUR);
#else
	return lseek(psf->file.filedes, 0, SEEK_CUR);
#endif
}

static void vfflush(void *user_data)
{
	SF_PRIVATE *psf = user_data;
	if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
#ifdef HAVE_FSYNC
		fsync(psf->file.filedes);
#elif _WIN32
		_commit(psf->file.filedes);
#endif
}


int vfset_filelen(void * user_data, sf_count_t len)
{
	SF_PRIVATE *psf = user_data;
	int retval;

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return -1;

	if ((sizeof(off_t) < sizeof(sf_count_t)) && len > 0x7FFFFFFF)
		return -1;

#ifdef _WIN32
	retval = _chsize_s(psf->file.filedes, len);
#else
	retval = ftruncate(psf->file.filedes, len);
#endif

	if (retval != 0)
		psf_log_syserr(psf, errno);

	return retval;
}
