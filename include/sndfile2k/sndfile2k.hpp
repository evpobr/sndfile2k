/*
** Copyright (C) 2005-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**
**     * Redistributions of source code must retain the above copyright
**       notice, this list of conditions and the following disclaimer.
**     * Redistributions in binary form must reproduce the above copyright
**       notice, this list of conditions and the following disclaimer in
**       the documentation and/or other materials provided with the
**       distribution.
**     * Neither the author nor the names of any contributors may be used
**       to endorse or promote products derived from this software without
**       specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/*
** The above modified BSD style license (GPL and LGPL compatible) applies to
** this file. It does not apply to libsndfile itself which is released under
** the GNU LGPL or the libsndfile test suite which is released under the GNU
** GPL.
** This means that this header file can be used under this modified BSD style
** license, but the LGPL still holds for the libsndfile library itself.
*/

/** A lightweight C++ wrapper for the libsndfile API.
 *
 * \file sndfile2k.hpp
 *
 * All the methods are inlines and all functionality is contained in this
 * file. There is no separate implementation file.
 */

#pragma once

#include "sndfile2k.h"

#include <string>
#include <new> // for std::nothrow

namespace sf {

/** SndFile class.
 *
 */
class SndfileHandle
{
private:
	struct SNDFILE_ref
	{
		SNDFILE_ref(void);
		~SNDFILE_ref(void);

		SNDFILE *sf;
		SF_INFO sfinfo;
		int ref;
	};

	SNDFILE_ref *p;

public:
	/** SndFileHandle default constructor.
	 *
	 */
	SndfileHandle(void) : p(NULL) {};

	/** Opens the specified file using path
	 *
	 * @param[in] path Path to the file
	 * @param[in] mode File open mode
	 * @param[in] format Format
	 * @param[in] channels Number of channels
	 * @param[in] samplerate Samplerate
	 *
	 * The @p path is byte encoded, but may be utf-8 on Linux, while on
	 * Mac OS X it will use the filesystem character set. On Windows, there is
	 * also a Windows specific SndfileHandle(const wchar_t *, int, int, int)
	 * that takes a @c UTF16_BE encoded filename.
	 *
	 * The @p mode specifies the kind of access that is requested for the
	 * file, one of ::SF_FILEMODE values.
	 *
	 * When opening a file for read, the @p format field should be set to
	 * zero before passing to constructor. The only exception to this is the
	 * case of @c RAW files where the caller has to set the @p samplerate,
	 * @p channels and @p format parameters to valid values.
	 *
	 * When opening a file for write, the caller must fill @p samplerate,
	 * @p channels, @p format.
	 *
	 * @sa sf_wchar_open(), sf_open_fd(), sf_open_virtual()
	 * @sa sf_close()
	 */
	SndfileHandle(const char *path, SF_FILEMODE mode = SFM_READ, int format = 0, int channels = 0,
				  int samplerate = 0);

	/** Opens the specified file using path
	 *
	 * @param[in] path Path to the file
	 * @param[in] mode File open mode
	 * @param[in] format Format
	 * @param[in] channels Number of channels
	 * @param[in] samplerate Samplerate
	 *
	 * This constructor is similar to SndfileHandle(const char *, int, int, int),
	 * but takes @c std::string as path.
	 *
	 * @sa sf_wchar_open(), sf_open_fd(), sf_open_virtual()
	 * @sa sf_close()
	 */
	SndfileHandle(std::string const &path, SF_FILEMODE mode = SFM_READ, int format = 0, int channels = 0,
				  int samplerate = 0);

	/** Opens file using @c POSIX file descriptor
	 *
	 * @param[in] fd File descriptor
	 * @param[in] close_desc File descriptor close mode
	 * @param[in] mode File open mode
	 * @param[in] format Format
	 * @param[in] channels Number of channels
	 * @param[in] samplerate Samplerate
	 *
	 * This constructor is similar to SndfileHandle(const char *, int, int, int),
	 * but takes @c POSIX file descriptor of already opened file instead of path.
	 *
	 * Care should be taken to ensure that the mode of the file represented by the
	 * descriptor matches the @c mode argument.
	 *
	 * This function is useful in the following circumstances:
	 * - Opening temporary files securely (ie use the @c tmpfile() to return a
	 * @c FILE* pointer and then using @c fileno() to retrieve the file descriptor
	 * which is then passed to sndfile2k.
	 * - Opening files with file names using OS specific character encodings and then
	 * passing the file descriptor to sf_open_fd().
	 * - Opening \ref md_docs_embedded_files "sound files embedded within larger files".
	 *
	 * When destructor is called, the file descriptor is only closed if the
	 * @p close_desc parameter is set ::SF_TRUE.
	 *
	 * @note On Microsoft Windows, this function does not work if the application
	 * and the sndfile2k DLL are linked to different versions of the Microsoft C
	 * runtime DLL.
	 *
	 * @sa sf_open(), sf_wchar_open(), sf_open_virtual()
	 * @sa sf_close()
	 */
	SndfileHandle(int fd, bool close_desc, SF_FILEMODE mode = SFM_READ, int format = 0, int channels = 0,
				  int samplerate = 0);

	/** Opens sound file using Virtual I/O context
	 *
	 * @param[in] sfvirtual Virtual I/O context
	 * @param[in] user_data User data
	 * @param[in] mode File open mode
	 * @param[in] format Format
	 * @param[in] channels Number of channels
	 * @param[in] samplerate Samplerate
	 *
	 * This constructor is similar to SndfileHandle(const char *, int, int, int),
	 * but takes Virtual I/O context of already opened file instead of path.
	 *
	 * Care should be taken to ensure that the mode of the file represented by
	 * the descriptor matches the @p mode argument.
	 *
	 * @sa sf_open(), sf_wchar_open(), sf_open_fd()
	 * @sa sf_close()
	 */
	SndfileHandle(SF_VIRTUAL_IO &sfvirtual, void *user_data, SF_FILEMODE mode = SFM_READ, int format = 0,
				  int channels = 0, int samplerate = 0);

#if (defined(_WIN32) || defined(__CYGWIN__))

	/** Opens file using unicode filename
	 *
	 * @param[in] wpath Path to the file
	 * @param[in] mode File open mode
	 * @param[in] format Format
	 * @param[in] channels Number of channels
	 * @param[in] samplerate Samplerate
	 *
	 * This constructor is similar to SndfileHandle(const char *, int, int, int),
	 * but takes @c wchar_t string as path.
	 *
	 * @note This function is Windows-specific.
	 *
	 * @sa sf_open(), sf_open_fd(), sf_open_virtual()
	 * @sa sf_close()
	 */
	SndfileHandle(const wchar_t *wpath, SF_FILEMODE mode = SFM_READ, int format = 0, int channels = 0,
				  int samplerate = 0);
#endif

	/** SndfileHandle destructor
	 */
	~SndfileHandle(void);

	/** SndfileHandle copy constructor
	 */
	SndfileHandle(const SndfileHandle &orig);

	/** Assignment operator
	 */
	SndfileHandle &operator=(const SndfileHandle &rhs);

	/** Gets number of references to this sound file
	 */
	int refCount(void) const
	{
		return (p == NULL) ? 0 : p->ref;
	}

	/** Compares sound file with NULL
	 */
	operator bool() const
	{
		return (p != NULL);
	}

	/** Compares one sound file object with other
	 */
	bool operator==(const SndfileHandle &rhs) const
	{
		return (p == rhs.p);
	}

	/** Gets number of frames
	 */
	sf_count_t frames(void) const
	{
		return p ? p->sfinfo.frames : 0;
	}

	/** Gets format
	 *
	 * @sa ::SF_FORMAT
	 */
	int format(void) const
	{
		return p ? p->sfinfo.format : 0;
	}

	/** Gets number of channels
	 */
	int channels(void) const
	{
		return p ? p->sfinfo.channels : 0;
	}

	/** Gets samplerate
	 */
	int samplerate(void) const
	{
		return p ? p->sfinfo.samplerate : 0;
	}

	/** Gets the current error code of sound file object
	 *
	 * The error code may be one of ::SF_ERR values, or any one of many other
	 * internal error values.
	 *
	 * Applications should only test the return value against ::SF_ERR values as
	 * the internal error values are subject to change at any time.
	 *
	 * For errors not in the above list, the strError() method can be used to
	 * convert it to an error string.
	 *
	 * @return Error number.
	 *
	 * @sa strError()
	 */
	int error(void) const;

	/** Returns textual description of  the current error code
	 *
	 * @return Pointer to the NULL-terminated error description string.
	 *
	 * @sa error()
	 */
	const char *strError(void) const;

	/** Gets or sets parameters of library or sound file object.
	 *
	 * @param cmd Command ID see ::SF_COMMAND for details
	 * @param data Command specific, could be @c NULL
	 * @param datasize Size of @c data, could be @c 0
	 *
	 * @return Command-specific, see particular command description.
	 */
	int command(int cmd, void *data, int datasize);

	/** Changes position of sound file
	 *
	 * @param[in] frames Count of frames
	 * @param[in] whence Seek origin
	 *
	 * The file seek functions work much like lseek in @c unistd.h with the
	 * exception that the non-audio data is ignored and the seek only moves
	 * within the audio data section of the file. In addition, seeks are defined
	 * in number of (multichannel) frames. Therefore, a seek in a stereo file
	 * from the current position forward with an offset of 1 would skip forward
	 * by one sample of both channels.
	 *
	 * Internally, library keeps track of the read and write locations using
	 * separate read and write pointers. If a file has been opened with a mode
	 * of ::SFM_RDWR, bitwise OR-ing the standard whence values above with
	 * either ::SFM_READ or ::SFM_WRITE allows the read and write pointers to be
	 * modified separately. If the @c SEEK_* values are used on their own, the
	 * read and write pointers are both modified.
	 *
	 * @note The frames offset can be negative and in fact should be when
	 * ::SF_SEEK_END is used for the @c whence parameter.
	 *
	 * @return Offset in (multichannel) frames from the start of the audio data
	 * or @c -1 if an error occured (ie an attempt is made to seek beyond the
	 * start or end of the file).
	 */
	sf_count_t seek(sf_count_t frames, int whence);

	/** Forces writing of data to disk
	 *
	 * If the file is opened #SFM_WRITE or #SFM_RDWR, call fsync() on the file
	 * to force the writing of data to disk. If the file is opened ::SFM_READ no
	 * action is taken.
	 */
	void writeSync(void);

	/** Sets string field
	 *
	 * @param[in] str_type Type of field to set, one of ::SF_STR_FIELD values
	 * @param[in] str Pointer to a null-terminated string with data to set
	 *
	 * @note Strings passed to this function is assumed to be utf-8. However, while
	 * formats like Ogg/Vorbis and FLAC fully support utf-8, others like WAV and
	 * AIFF officially only support ASCII. Current policy is to write such string
	 * as it is without conversion. Such tags can be read back with SndFile2K
	 * without problems, but other programs may fail to decode and display data
	 * correctly. If you want to write strings in ASCII, perform reencoding before
	 * passing data to sf_set_string().
	 *
	 * @return Zero on success and non-zero value otherwise.
	 */
	int setString(int str_type, const char *str);

	/** Gets string field
	 *
	 * @param[in] str_type Type of field to retrieve, one of ::SF_STR_FIELD
	 * values.
	 *
	 * @return Zero-terminated string in utf-8 encoding on success or @c NULL
	 * otherwise.
	 */
	const char *getString(int str_type) const;

	/** Checks correctness of format parameters combination
	 *
	 * To open sound file in write mode you need to set up @p format,
	 * @p channels and @p samplerate parameters. To be sure that combination is
	 * correct and will be accepted you can use this method.
	 *
	 * @return ::SF_TRUE if format is valid, ::SF_FALSE otherwise.
	 *
	 * @sa sf_format_check()
	 */
	static int formatCheck(int format, int channels, int samplerate);

	/** Reads short (16-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	sf_count_t read(short *ptr, sf_count_t items);

	/** Reads integer (32-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	sf_count_t read(int *ptr, sf_count_t items);

	/** Reads float (32-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	sf_count_t read(float *ptr, sf_count_t items);

	/** Reads double (64-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	sf_count_t read(double *ptr, sf_count_t items);

	/** Writes short (16-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	sf_count_t write(const short *ptr, sf_count_t items);

	/** Writes integer (32-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	sf_count_t write(const int *ptr, sf_count_t items);

	/** Writes float (32-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	sf_count_t write(const float *ptr, sf_count_t items);

	/** Writes double (64-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	sf_count_t write(const double *ptr, sf_count_t items);

	/** Reads short (16-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	sf_count_t readf(short *ptr, sf_count_t frames);

	/** Reads integer (32-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	sf_count_t readf(int *ptr, sf_count_t frames);

	/** Reads float (32-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	sf_count_t readf(float *ptr, sf_count_t frames);

	/** Reads double (64-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	sf_count_t readf(double *ptr, sf_count_t frames);

	/** Writes short (16-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	sf_count_t writef(const short *ptr, sf_count_t frames);

	/** Writes integer (32-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	sf_count_t writef(const int *ptr, sf_count_t frames);

	/** Writes float (32-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	sf_count_t writef(const float *ptr, sf_count_t frames);

	/** Writes double (64-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	sf_count_t writef(const double *ptr, sf_count_t frames);

	/** Read raw bytes from sound file
	 *
	 * @param[out] ptr Pointer to an allocated block memory.
	 * @param[in] bytes Count of bytes to read
	 *
	 * @note This function is not for general use. No type conversion or
	 * byte-swapping performed, data is returned as it is.
	 *
	 * @return Number of bytes actually read.
	 */
	sf_count_t readRaw(void *ptr, sf_count_t bytes);

	/** Writes raw bytes to sound file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] bytes Count of bytes to write
	 *
	 * @note This function is not for general use. No type conversion or
	 * byte-swapping performed, data is written as it is.
	 *
	 * @return Number of bytes actually written.
	 */
	sf_count_t writeRaw(const void *ptr, sf_count_t bytes);

	/** Gets ccess to the raw sound file handle
	 */
	SNDFILE *rawHandle(void);

	/** Takes ownership of handle, if reference count is 1
	 */
	SNDFILE *takeOwnership(void);
};

/*==============================================================================
**	Nothing but implementation below.
*/

inline SndfileHandle::SNDFILE_ref::SNDFILE_ref(void) : sf(NULL), sfinfo(), ref(1)
{
}

inline SndfileHandle::SNDFILE_ref::~SNDFILE_ref(void)
{
	if (sf != NULL)
		sf_close(sf);
}

inline SndfileHandle::SndfileHandle(const char *path, SF_FILEMODE mode, int fmt, int chans, int srate)
	: p(NULL)
{
	p = new (std::nothrow) SNDFILE_ref();

	if (p != NULL)
	{
		p->ref = 1;

		p->sfinfo.frames = 0;
		p->sfinfo.channels = chans;
		p->sfinfo.format = fmt;
		p->sfinfo.samplerate = srate;
		p->sfinfo.sections = 0;
		p->sfinfo.seekable = 0;

		p->sf = sf_open(path, mode, &p->sfinfo);
	};

	return;
} /* SndfileHandle const char * constructor */

inline SndfileHandle::SndfileHandle(std::string const &path, SF_FILEMODE mode, int fmt, int chans,
									int srate)
	: p(NULL)
{
	p = new (std::nothrow) SNDFILE_ref();

	if (p != NULL)
	{
		p->ref = 1;

		p->sfinfo.frames = 0;
		p->sfinfo.channels = chans;
		p->sfinfo.format = fmt;
		p->sfinfo.samplerate = srate;
		p->sfinfo.sections = 0;
		p->sfinfo.seekable = 0;

		p->sf = sf_open(path.c_str(), mode, &p->sfinfo);
	};

	return;
} /* SndfileHandle std::string constructor */

inline SndfileHandle::SndfileHandle(int fd, bool close_desc, SF_FILEMODE mode, int fmt, int chans,
									int srate)
	: p(NULL)
{
	if (fd < 0)
		return;

	p = new (std::nothrow) SNDFILE_ref();

	if (p != NULL)
	{
		p->ref = 1;

		p->sfinfo.frames = 0;
		p->sfinfo.channels = chans;
		p->sfinfo.format = fmt;
		p->sfinfo.samplerate = srate;
		p->sfinfo.sections = 0;
		p->sfinfo.seekable = 0;

		p->sf = sf_open_fd(fd, mode, &p->sfinfo, close_desc);
	};

	return;
} /* SndfileHandle fd constructor */

inline SndfileHandle::SndfileHandle(SF_VIRTUAL_IO &sfvirtual, void *user_data, SF_FILEMODE mode, int fmt,
									int chans, int srate)
	: p(NULL)
{
	p = new (std::nothrow) SNDFILE_ref();

	if (p != NULL)
	{
		p->ref = 1;

		p->sfinfo.frames = 0;
		p->sfinfo.channels = chans;
		p->sfinfo.format = fmt;
		p->sfinfo.samplerate = srate;
		p->sfinfo.sections = 0;
		p->sfinfo.seekable = 0;

		p->sf = sf_open_virtual(&sfvirtual, mode, &p->sfinfo, user_data);
	};

	return;
} /* SndfileHandle std::string constructor */

inline SndfileHandle::~SndfileHandle(void)
{
	if (p != NULL && --p->ref == 0)
		delete p;
} /* SndfileHandle destructor */

inline SndfileHandle::SndfileHandle(const SndfileHandle &orig) : p(orig.p)
{
	if (p != NULL)
		++p->ref;
} /* SndfileHandle copy constructor */

inline SndfileHandle &SndfileHandle::operator=(const SndfileHandle &rhs)
{
	if (&rhs == this)
		return *this;
	if (p != NULL && --p->ref == 0)
		delete p;

	p = rhs.p;
	if (p != NULL)
		++p->ref;

	return *this;
} /* SndfileHandle assignment operator */

inline int SndfileHandle::error(void) const
{
	return sf_error(p->sf);
}

inline const char *SndfileHandle::strError(void) const
{
	return sf_strerror(p->sf);
}

inline int SndfileHandle::command(int cmd, void *data, int datasize)
{
	return sf_command(p->sf, cmd, data, datasize);
}

inline sf_count_t SndfileHandle::seek(sf_count_t frame_count, int whence)
{
	return sf_seek(p->sf, frame_count, whence);
}

inline void SndfileHandle::writeSync(void)
{
	sf_write_sync(p->sf);
}

/** Sets string field
 *
 * @param[in] str_type Type of field to set, one of ::SF_STR_FIELD values
 * @param[in] str Pointer to a null-terminated string with data to set
 *
 * @note Strings passed to this function is assumed to be utf-8. However, while
 * formats like Ogg/Vorbis and FLAC fully support utf-8, others like WAV and
 * AIFF officially only support ASCII. Current policy is to write such string
 * as it is without conversion. Such tags can be read back with SndFile2K
 * without problems, but other programs may fail to decode and display data
 * correctly. If you want to write strings in ASCII, perform reencoding before
 * passing data to setString().
 *
 * @return Zero on success and non-zero value otherwise.
 */
inline int SndfileHandle::setString(int str_type, const char *str)
{
	return sf_set_string(p->sf, str_type, str);
}

/** Gets string field
 *
 * @param[in] str_type Type of field to retrieve, one of ::SF_STR_FIELD values
 *
 * @return Zero-terminated string in utf-8 encoding on success or @c NULL
 * otherwise.
 */
inline const char *SndfileHandle::getString(int str_type) const
{
	return sf_get_string(p->sf, str_type);
}

inline int SndfileHandle::formatCheck(int fmt, int chans, int srate)
{
	SF_INFO sfinfo;

	sfinfo.frames = 0;
	sfinfo.channels = chans;
	sfinfo.format = fmt;
	sfinfo.samplerate = srate;
	sfinfo.sections = 0;
	sfinfo.seekable = 0;

	return sf_format_check(&sfinfo);
}

/*---------------------------------------------------------------------*/

inline sf_count_t SndfileHandle::read(short *ptr, sf_count_t items)
{
	return sf_read_short(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::read(int *ptr, sf_count_t items)
{
	return sf_read_int(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::read(float *ptr, sf_count_t items)
{
	return sf_read_float(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::read(double *ptr, sf_count_t items)
{
	return sf_read_double(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::write(const short *ptr, sf_count_t items)
{
	return sf_write_short(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::write(const int *ptr, sf_count_t items)
{
	return sf_write_int(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::write(const float *ptr, sf_count_t items)
{
	return sf_write_float(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::write(const double *ptr, sf_count_t items)
{
	return sf_write_double(p->sf, ptr, items);
}

inline sf_count_t SndfileHandle::readf(short *ptr, sf_count_t frame_count)
{
	return sf_readf_short(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::readf(int *ptr, sf_count_t frame_count)
{
	return sf_readf_int(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::readf(float *ptr, sf_count_t frame_count)
{
	return sf_readf_float(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::readf(double *ptr, sf_count_t frame_count)
{
	return sf_readf_double(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::writef(const short *ptr, sf_count_t frame_count)
{
	return sf_writef_short(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::writef(const int *ptr, sf_count_t frame_count)
{
	return sf_writef_int(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::writef(const float *ptr, sf_count_t frame_count)
{
	return sf_writef_float(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::writef(const double *ptr, sf_count_t frame_count)
{
	return sf_writef_double(p->sf, ptr, frame_count);
}

inline sf_count_t SndfileHandle::readRaw(void *ptr, sf_count_t bytes)
{
	return sf_read_raw(p->sf, ptr, bytes);
}

inline sf_count_t SndfileHandle::writeRaw(const void *ptr, sf_count_t bytes)
{
	return sf_write_raw(p->sf, ptr, bytes);
}

inline SNDFILE *SndfileHandle::rawHandle(void)
{
	return (p ? p->sf : NULL);
}

inline SNDFILE *SndfileHandle::takeOwnership(void)
{
	if (p == NULL || (p->ref != 1))
		return NULL;

	SNDFILE *sf = p->sf;
	p->sf = NULL;
	delete p;
	p = NULL;
	return sf;
}

#if (defined(_WIN32) || defined(__CYGWIN__))

inline SndfileHandle::SndfileHandle(const wchar_t *wpath, SF_FILEMODE mode, int fmt, int chans, int srate)
	: p(NULL)
{
	p = new (std::nothrow) SNDFILE_ref();

	if (p != NULL)
	{
		p->ref = 1;

		p->sfinfo.frames = 0;
		p->sfinfo.channels = chans;
		p->sfinfo.format = fmt;
		p->sfinfo.samplerate = srate;
		p->sfinfo.sections = 0;
		p->sfinfo.seekable = 0;

		p->sf = sf_wchar_open(wpath, mode, &p->sfinfo);
	};

	return;
} /* SndfileHandle const wchar_t * constructor */

#endif

}
