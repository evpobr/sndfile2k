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

#if (USE_WINDOWS_API == 0)

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

/*
* Win32 stuff at the bottom of the file. Unix and other sensible OSes here.
*/

static int psf_close_fd(int fd);
static int psf_open_fd(PSF_FILE *pfile);
static sf_count_t psf_get_filelen_fd(int fd);

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
		psf_log_syserr(psf, errno);

	return psf->error;
}

int psf_fclose(SF_PRIVATE *psf)
{
	int retval;

	if (psf->virtual_io)
		return 0;

	if (psf->file.do_not_close_descriptor)
	{
		psf->file.filedes = -1;
		return 0;
	};

	if ((retval = psf_close_fd(psf->file.filedes)) == -1)
		psf_log_syserr(psf, errno);

	psf->file.filedes = -1;

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

	if (psf->virtual_io)
		return psf->vio.get_filelen(psf->vio_user_data);

	filelen = psf_get_filelen_fd(psf->file.filedes);

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

	if (psf->virtual_io)
		return psf->vio.seek(offset, whence, psf->vio_user_data);

	/* When decoding from pipes sometimes see seeks to the pipeoffset, which
	appears to mean do nothing. */
	if (psf->is_pipe)
	{
		if (whence != SEEK_SET || offset != psf->pipeoffset)
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

	absolute_position = lseek(psf->file.filedes, offset, whence);

	if (absolute_position < 0)
		psf_log_syserr(psf, errno);

	return absolute_position - psf->fileoffset;
}

size_t psf_fread(void *ptr, size_t bytes, size_t items, SF_PRIVATE *psf)
{
	sf_count_t total = 0;
	ssize_t count;

	if (psf->virtual_io)
		return psf->vio.read(ptr, bytes * items, psf->vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the read down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : (ssize_t)items;

		count = read(psf->file.filedes, ((char *)ptr) + total, (size_t)count);

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

	if (psf->is_pipe)
		psf->pipeoffset += total;

	return total / bytes;
}

size_t psf_fwrite(const void *ptr, size_t bytes, size_t items, SF_PRIVATE *psf)
{
	sf_count_t total = 0;
	ssize_t count;

	if (bytes == 0 || items == 0)
		return 0;

	if (psf->virtual_io)
		return psf->vio.write(ptr, bytes * items, psf->vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items;

		count = write(psf->file.filedes, ((const char *)ptr) + total, count);

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

	if (psf->is_pipe)
		psf->pipeoffset += total;

	return total / bytes;
}

sf_count_t psf_ftell(SF_PRIVATE *psf)
{
	sf_count_t pos;

	if (psf->virtual_io)
		return psf->vio.tell(psf->vio_user_data);

	if (psf->is_pipe)
		return psf->pipeoffset;

#if _WIN32
	pos = _lseeki64(psf->file.filedes, 0, SEEK_CUR);
#else
	pos = lseek(psf->file.filedes, 0, SEEK_CUR);
#endif

	if (pos == ((sf_count_t)-1))
	{
		psf_log_syserr(psf, errno);
		return -1;
	};

	return pos - psf->fileoffset;
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

	if (psf->virtual_io)
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
	struct stat64 statbuf;

	if (fstat64(fd, &statbuf) == -1)
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
		mode = 0;
		break;

	case SFM_WRITE:
		oflag = O_WRONLY | O_CREAT | O_TRUNC;
		mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
		break;

	case SFM_RDWR:
		oflag = O_RDWR | O_CREAT;
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
#ifdef HAVE_FSYNC
	if (psf->file.mode == SFM_WRITE || psf->file.mode == SFM_RDWR)
		fsync(psf->file.filedes);
#else
	psf = NULL;
#endif
}

#endif
