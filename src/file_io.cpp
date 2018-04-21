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
#include "sndfile_error.h"
#include "ref_ptr.h"

#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sf_unistd.h>

using namespace std;

static sf_count_t psf_get_filelen_fd(int fd);

class SF_FILE_STREAM: public SF_STREAM
{
    unsigned long m_ref = 0;
    int m_filedes = -1;

    void close()
    {
        if (m_filedes >= 0)
            ::close(m_filedes);
        m_filedes = -1;
    }

public:
    SF_FILE_STREAM(const char * filename, SF_FILEMODE mode)
    {
        int open_flag, share_flag;

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
            throw sf::sndfile_error(-SFE_BAD_OPEN_MODE);
            break;
        };

        try
        {
            m_filedes = open(filename, open_flag, share_flag);
            if (m_filedes == -SFE_BAD_OPEN_MODE)
            {
                throw sf::sndfile_error(-SFE_BAD_OPEN_MODE);
            }
            else if (m_filedes == -1)
            {
                throw sf::sndfile_error(-SFE_BAD_FILE_PTR);
            }
        }
        catch (...)
        {
            close();
            throw;
        }
    }

#ifdef _WIN32

    SF_FILE_STREAM(const wchar_t *filename, SF_FILEMODE mode)
    {
        int open_flag, share_flag;

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
            throw sf::sndfile_error(-SFE_BAD_OPEN_MODE);
            break;
        };

        try
        {
            m_filedes = _wopen(filename, open_flag, share_flag);
            if (m_filedes == -SFE_BAD_OPEN_MODE)
            {
                throw sf::sndfile_error(-SFE_BAD_OPEN_MODE);
            }
            else if (m_filedes == -1)
            {
                throw sf::sndfile_error(-SFE_BAD_FILE_PTR);
            }
        }
        catch (...)
        {
            close();
        }
    }

#endif

    ~SF_FILE_STREAM()
    {
        close();
    }

    // Inherited via SF_STREAM
    unsigned long ref() override
    {
        unsigned long ref_counter = 0;

        ref_counter = ++m_ref;

        return ref_counter;
    }

    void unref() override
    {
        m_ref--;
        if (m_ref == 0)
            delete this;
    }

    sf_count_t get_filelen() override
    {
#ifdef _WIN32
        struct _stat64 statbuf;

        if (_fstat64(m_filedes, &statbuf) == -1)
            return (sf_count_t)-1;

        return statbuf.st_size;
#else
        struct stat statbuf;

        if (fstat(m_filedes, &statbuf) == -1)
            return (sf_count_t)-1;

        return statbuf.st_size;
#endif
    }

    sf_count_t seek(sf_count_t offset, int whence) override
    {
#ifdef _WIN32
        return _lseeki64(m_filedes, offset, whence);
#else
        return lseek(m_filedes, offset, whence);
#endif
    }

    sf_count_t read(void * ptr, sf_count_t count) override
    {
        return ::read(m_filedes, ptr, count);
    }

    sf_count_t write(const void * ptr, sf_count_t count) override
    {
        return ::write(m_filedes, ptr, count);
    }

    sf_count_t tell() override
    {
#if _WIN32
        return _lseeki64(m_filedes, 0, SEEK_CUR);
#else
        return lseek(m_filedes, 0, SEEK_CUR);
#endif
    }

    void flush() override
    {
#ifdef HAVE_FSYNC
        ::fsync(m_filedes);
#elif _WIN32
        _commit(m_filedes);
#endif
    }

    int set_filelen(sf_count_t len) override
    {
        if (len < 0)
            return -1;

        if ((sizeof(off_t) < sizeof(sf_count_t)) && len > 0x7FFFFFFF)
            return -1;

#ifdef _WIN32
        int retval = _chsize_s(m_filedes, len);
#else
        int retval = ::ftruncate(m_filedes, len);
#endif

        return retval;
    }
};

int psf_open_file_stream(const char * filename, SF_FILEMODE mode, SF_STREAM **stream)
{
    if (!stream)
        return SFE_BAD_VIRTUAL_IO;

    *stream = nullptr;

    SF_FILE_STREAM *s = nullptr;
    try
    {
        s = new SF_FILE_STREAM(filename, mode);

        *stream = static_cast<SF_STREAM *>(s);
        s->ref();

        return SFE_NO_ERROR;
    }
    catch (const sf::sndfile_error &e)
    {
        delete s;
        return e.error();
    }
}

#ifdef _WIN32

int psf_open_file_stream(const wchar_t * filename, SF_FILEMODE mode, SF_STREAM **stream)
{
    if (!stream)
        return SFE_BAD_VIRTUAL_IO;

    *stream = nullptr;

    SF_FILE_STREAM *s = nullptr;
    try
    {
        s = new SF_FILE_STREAM(filename, mode);
    }
    catch (const sf::sndfile_error &e)
    {
        delete s;
        return e.error();
    }

    *stream = static_cast<SF_STREAM *>(s);
    s->ref();

    return SFE_NO_ERROR;
}

#endif

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
