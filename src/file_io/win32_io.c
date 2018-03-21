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

/* Win32 file i/o functions implemented using native Win32 API */

#include "config.h"

#if (USE_WINDOWS_API == 1)

#include "common.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>

static int psf_close_handle(HANDLE handle);
static HANDLE psf_open_handle(PSF_FILE *pfile);
static sf_count_t psf_get_filelen_handle(HANDLE handle);

int psf_fopen(SF_PRIVATE *psf)
{
	psf->error = 0;
	psf->file.handle = psf_open_handle(&psf->file);

	if (psf->file.handle == NULL)
		psf_log_syserr(psf, GetLastError());

	return psf->error;
} /* psf_fopen */

int psf_fclose(SF_PRIVATE *psf)
{
	int retval;

	if (psf->virtual_io)
		return 0;

	if (psf->file.do_not_close_descriptor)
	{
		psf->file.handle = NULL;
		return 0;
	};

	if ((retval = psf_close_handle(psf->file.handle)) == -1)
		psf_log_syserr(psf, GetLastError());

	psf->file.handle = NULL;

	return retval;
}

int psf_open_rsrc(SF_PRIVATE *psf)
{
	if (psf->rsrc.handle != NULL)
		return 0;

	/* Test for MacOSX style resource fork on HPFS or HPFS+ filesystems. */
	snprintf(psf->rsrc.path.c, sizeof(psf->rsrc.path.c), "%s/rsrc", psf->file.path.c);
	psf->error = SFE_NO_ERROR;
	if ((psf->rsrc.handle = psf_open_handle(&psf->rsrc)) != NULL)
	{
		psf->rsrclength = psf_get_filelen_handle(psf->rsrc.handle);
		return SFE_NO_ERROR;
	};

	/*
	* Now try for a resource fork stored as a separate file in the same
	* directory, but preceded with a dot underscore.
	*/
	snprintf(psf->rsrc.path.c, sizeof(psf->rsrc.path.c), "%s._%s", psf->file.dir.c,
			 psf->file.name.c);
	psf->error = SFE_NO_ERROR;
	if ((psf->rsrc.handle = psf_open_handle(&psf->rsrc)) != NULL)
	{
		psf->rsrclength = psf_get_filelen_handle(psf->rsrc.handle);
		return SFE_NO_ERROR;
	};

	/*
	* Now try for a resource fork stored in a separate file in the
	* .AppleDouble/ directory.
	*/
	snprintf(psf->rsrc.path.c, sizeof(psf->rsrc.path.c), "%s.AppleDouble/%s", psf->file.dir.c,
			 psf->file.name.c);
	psf->error = SFE_NO_ERROR;
	if ((psf->rsrc.handle = psf_open_handle(&psf->rsrc)) != NULL)
	{
		psf->rsrclength = psf_get_filelen_handle(psf->rsrc.handle);
		return SFE_NO_ERROR;
	};

	/* No resource file found. */
	if (psf->rsrc.handle == NULL)
		psf_log_syserr(psf, GetLastError());

	psf->rsrc.handle = NULL;

	return psf->error;
}

sf_count_t psf_get_filelen(SF_PRIVATE *psf)
{
	sf_count_t filelen;

	if (psf->virtual_io)
		return psf->vio.get_filelen(psf->vio_user_data);

	filelen = psf_get_filelen_handle(psf->file.handle);

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

void psf_init_files(SF_PRIVATE *psf)
{
	psf->file.handle = NULL;
	psf->rsrc.handle = NULL;
	psf->file.hsaved = NULL;
}

void psf_use_rsrc(SF_PRIVATE *psf, int on_off)
{
	if (on_off)
	{
		if (psf->file.handle != psf->rsrc.handle)
		{
			psf->file.hsaved = psf->file.handle;
			psf->file.handle = psf->rsrc.handle;
		};
	}
	else if (psf->file.handle == psf->rsrc.handle)
	{
		psf->file.handle = psf->file.hsaved;
	}

	return;
}

static HANDLE psf_open_handle(PSF_FILE *pfile)
{
	DWORD dwDesiredAccess;
	DWORD dwShareMode;
	DWORD dwCreationDistribution;
	HANDLE handle;

	switch (pfile->mode)
	{
	case SFM_READ:
		dwDesiredAccess = GENERIC_READ;
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		dwCreationDistribution = OPEN_EXISTING;
		break;

	case SFM_WRITE:
		dwDesiredAccess = GENERIC_WRITE;
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		dwCreationDistribution = CREATE_ALWAYS;
		break;

	case SFM_RDWR:
		dwDesiredAccess = GENERIC_READ | GENERIC_WRITE;
		dwShareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;
		dwCreationDistribution = OPEN_ALWAYS;
		break;

	default:
		return NULL;
	};

	if (pfile->use_wchar)
		handle = CreateFileW(
			pfile->path.wc, /* pointer to name of the file */
			dwDesiredAccess, /* access (read-write) mode */
			dwShareMode, /* share mode */
			0, /* pointer to security attributes */
			dwCreationDistribution, /* how to create */
			FILE_ATTRIBUTE_NORMAL, /* file attributes (could use FILE_FLAG_SEQUENTIAL_SCAN) */
			NULL /* handle to file with attributes to copy */
		);
	else
		handle = CreateFile(
			pfile->path.c, /* pointer to name of the file */
			dwDesiredAccess, /* access (read-write) mode */
			dwShareMode, /* share mode */
			0, /* pointer to security attributes */
			dwCreationDistribution, /* how to create */
			FILE_ATTRIBUTE_NORMAL, /* file attributes (could use FILE_FLAG_SEQUENTIAL_SCAN) */
			NULL /* handle to file with attributes to copy */
		);

	if (handle == INVALID_HANDLE_VALUE)
		return NULL;

	return handle;
}

static void psf_log_syserr(SF_PRIVATE *psf, int error)
{
	LPVOID lpMsgBuf;

	/* Only log an error if no error has been set yet. */
	if (psf->error == 0)
	{
		psf->error = SFE_SYSTEM;

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error,
					  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		snprintf(psf->syserr, sizeof(psf->syserr), "System error : %s", (char *)lpMsgBuf);
		LocalFree(lpMsgBuf);
	};

	return;
}

int psf_close_rsrc(SF_PRIVATE *psf)
{
	psf_close_handle(psf->rsrc.handle);
	psf->rsrc.handle = NULL;
	return 0;
}

int psf_set_stdio(SF_PRIVATE *psf)
{
	HANDLE handle = NULL;
	int error = 0;

	switch (psf->file.mode)
	{
	case SFM_RDWR:
		error = SFE_OPEN_PIPE_RDWR;
		break;

	case SFM_READ:
		handle = GetStdHandle(STD_INPUT_HANDLE);
		psf->file.do_not_close_descriptor = 1;
		break;

	case SFM_WRITE:
		handle = GetStdHandle(STD_OUTPUT_HANDLE);
		psf->file.do_not_close_descriptor = 1;
		break;

	default:
		error = SFE_BAD_OPEN_MODE;
		break;
	};

	psf->file.handle = handle;
	psf->filelength = 0;

	return error;
}

void psf_set_file(SF_PRIVATE *psf, int fd)
{
	HANDLE handle;
	intptr_t osfhandle;

	osfhandle = _get_osfhandle(fd);
	handle = (HANDLE)osfhandle;

	psf->file.handle = handle;
}

int psf_file_valid(SF_PRIVATE *psf)
{
	if (psf->file.handle == NULL)
		return SF_FALSE;
	if (psf->file.handle == INVALID_HANDLE_VALUE)
		return SF_FALSE;
	return SF_TRUE;
}

sf_count_t psf_fseek(SF_PRIVATE *psf, sf_count_t offset, int whence)
{
	sf_count_t new_position;
	LONG lDistanceToMove, lDistanceToMoveHigh;
	DWORD dwMoveMethod;
	DWORD dwResult, dwError;

	if (psf->virtual_io)
		return psf->vio.seek(offset, whence, psf->vio_user_data);

	switch (whence)
	{
	case SEEK_SET:
		offset += psf->fileoffset;
		dwMoveMethod = FILE_BEGIN;
		break;

	case SEEK_END:
		dwMoveMethod = FILE_END;
		break;

	default:
		dwMoveMethod = FILE_CURRENT;
		break;
	};

	lDistanceToMove = (DWORD)(offset & 0xFFFFFFFF);
	lDistanceToMoveHigh = (DWORD)((offset >> 32) & 0xFFFFFFFF);

	dwResult =
		SetFilePointer(psf->file.handle, lDistanceToMove, &lDistanceToMoveHigh, dwMoveMethod);

	if (dwResult == 0xFFFFFFFF)
		dwError = GetLastError();
	else
		dwError = NO_ERROR;

	if (dwError != NO_ERROR)
	{
		psf_log_syserr(psf, dwError);
		return -1;
	};

	new_position = (dwResult + ((__int64)lDistanceToMoveHigh << 32)) - psf->fileoffset;

	return new_position;
}

size_t psf_fread(void *ptr, size_t bytes, size_t items, SF_PRIVATE *psf)
{
	size_t total = 0;
	size_t count;
	DWORD dwNumberOfBytesRead;

	if (psf->virtual_io)
		return psf->vio.read(ptr, bytes * items, psf->vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : items;

		if (ReadFile(psf->file.handle, ((char *)ptr) + total, count, &dwNumberOfBytesRead, 0) == 0)
		{
			psf_log_syserr(psf, GetLastError());
			break;
		}
		else
		{
			count = dwNumberOfBytesRead;
		}

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
	size_t total = 0;
	size_t count;
	DWORD dwNumberOfBytesWritten;

	if (psf->virtual_io)
		return psf->vio.write(ptr, bytes * items, psf->vio_user_data) / bytes;

	items *= bytes;

	/* Do this check after the multiplication above. */
	if (items <= 0)
		return 0;

	while (items > 0)
	{
		/* Break the writes down to a sensible size. */
		count = (items > SENSIBLE_SIZE) ? SENSIBLE_SIZE : (ssize_t)items;

		if (WriteFile(psf->file.handle, ((const char *)ptr) + total, count, &dwNumberOfBytesWritten,
					  0) == 0)
		{
			psf_log_syserr(psf, GetLastError());
			break;
		}
		else
		{
			count = dwNumberOfBytesWritten;
		}

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
	LONG lDistanceToMoveLow, lDistanceToMoveHigh;
	DWORD dwResult, dwError;

	if (psf->virtual_io)
		return psf->vio.tell(psf->vio_user_data);

	if (psf->is_pipe)
		return psf->pipeoffset;

	lDistanceToMoveLow = 0;
	lDistanceToMoveHigh = 0;

	dwResult =
		SetFilePointer(psf->file.handle, lDistanceToMoveLow, &lDistanceToMoveHigh, FILE_CURRENT);

	if (dwResult == 0xFFFFFFFF)
		dwError = GetLastError();
	else
		dwError = NO_ERROR;

	if (dwError != NO_ERROR)
	{
		psf_log_syserr(psf, dwError);
		return -1;
	};

	pos = (dwResult + ((__int64)lDistanceToMoveHigh << 32));

	return pos - psf->fileoffset;
}

static int psf_close_handle(HANDLE handle)
{
	if (handle == NULL)
		return 0;

	if (CloseHandle(handle) == 0)
		return -1;

	return 0;
}

sf_count_t psf_fgets(char *buffer, size_t bufsize, SF_PRIVATE *psf)
{
	sf_count_t k = 0;
	sf_count_t count;
	DWORD dwNumberOfBytesRead;

	while (k < bufsize - 1)
	{
		if (ReadFile(psf->file.handle, &(buffer[k]), 1, &dwNumberOfBytesRead, 0) == 0)
		{
			psf_log_syserr(psf, GetLastError());
			break;
		}
		else
		{
			count = dwNumberOfBytesRead;
			/* note that we only check for '\n' not other line endings such as CRLF */
			if (count == 0 || buffer[k++] == '\n')
				break;
		};
	};

	buffer[k] = 0;

	return k;
}

int psf_is_pipe(SF_PRIVATE *psf)
{
	if (psf->virtual_io)
		return SF_FALSE;

	if (GetFileType(psf->file.handle) == FILE_TYPE_DISK)
		return SF_FALSE;

	/* Default to maximum safety. */
	return SF_TRUE;
}

sf_count_t psf_get_filelen_handle(HANDLE handle)
{
	sf_count_t filelen;
	DWORD dwFileSizeLow, dwFileSizeHigh, dwError = NO_ERROR;

	dwFileSizeLow = GetFileSize(handle, &dwFileSizeHigh);

	if (dwFileSizeLow == 0xFFFFFFFF)
		dwError = GetLastError();

	if (dwError != NO_ERROR)
		return (sf_count_t)-1;

	filelen = dwFileSizeLow + ((__int64)dwFileSizeHigh << 32);

	return filelen;
} /* psf_get_filelen_handle */

void psf_fsync(SF_PRIVATE *psf)
{
	FlushFileBuffers(psf->file.handle);
}

int psf_ftruncate(SF_PRIVATE *psf, sf_count_t len)
{
	int retval = 0;
	LONG lDistanceToMoveLow, lDistanceToMoveHigh;
	DWORD dwResult, dwError = NO_ERROR;

	/* This implementation trashes the current file position.
	** should it save and restore it? what if the current position is past
	** the new end of file?
	*/

	/* Returns 0 on success, non-zero on failure. */
	if (len < 0)
		return 1;

	lDistanceToMoveLow = (DWORD)(len & 0xFFFFFFFF);
	lDistanceToMoveHigh = (DWORD)((len >> 32) & 0xFFFFFFFF);

	dwResult =
		SetFilePointer(psf->file.handle, lDistanceToMoveLow, &lDistanceToMoveHigh, FILE_BEGIN);

	if (dwResult == 0xFFFFFFFF)
		dwError = GetLastError();

	if (dwError != NO_ERROR)
	{
		retval = -1;
		psf_log_syserr(psf, dwError);
	}
	else
	{
		/* Note: when SetEndOfFile is used to extend a file, the contents of the
		** new portion of the file is undefined. This is unlike chsize(),
		** which guarantees that the new portion of the file will be zeroed.
		** Not sure if this is important or not.
		*/
		if (SetEndOfFile(psf->file.handle) == 0)
		{
			retval = -1;
			psf_log_syserr(psf, GetLastError());
		};
	};

	return retval;
}

#endif
