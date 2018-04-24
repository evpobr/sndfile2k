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

struct ISndFile: public SF_OBJECT
{
	/** Gets number of frames
	 */
	virtual sf_count_t getFrames(void) const = 0;

	/** Gets format
	 *
	 * @sa ::SF_FORMAT
	 */
	virtual int getFormat(void) const = 0;

	/** Gets number of channels
	 */
	virtual int getChannels(void) const = 0;

	/** Gets samplerate
	 */
	virtual int getSamplerate(void) const = 0;

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
	virtual int getError(void) const = 0;

	/** Returns textual description of  the current error code
	 *
	 * @return Pointer to the NULL-terminated error description string.
	 *
	 * @sa error()
	 */
	virtual const char *getErrorString(void) const = 0;

	/** Gets or sets parameters of library or sound file object.
	 *
	 * @param cmd Command ID see ::SF_COMMAND for details
	 * @param data Command specific, could be @c NULL
	 * @param datasize Size of @c data, could be @c 0
	 *
	 * @return Command-specific, see particular command description.
	 */
	virtual int command(int cmd, void *data, int datasize) = 0;

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
	virtual sf_count_t seek(sf_count_t frames, int whence) = 0;

	/** Forces writing of data to disk
	 *
	 * If the file is opened #SFM_WRITE or #SFM_RDWR, call fsync() on the file
	 * to force the writing of data to disk. If the file is opened ::SFM_READ no
	 * action is taken.
	 */
	virtual void writeSync(void) = 0;

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
	virtual int setString(int str_type, const char *str) = 0;

	/** Gets string field
	 *
	 * @param[in] str_type Type of field to retrieve, one of ::SF_STR_FIELD
	 * values.
	 *
	 * @return Zero-terminated string in utf-8 encoding on success or @c NULL
	 * otherwise.
	 */
	virtual const char *getString(int str_type) const = 0;

	/** Reads short (16-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	virtual sf_count_t readShortSamples(short *ptr, sf_count_t items) = 0;

	/** Reads integer (32-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	virtual sf_count_t readIntSamples(int *ptr, sf_count_t items) = 0;

	/** Reads float (32-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	virtual sf_count_t readFloatSamples(float *ptr, sf_count_t items) = 0;

	/** Reads double (64-bit) samples from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to read
	 *
	 * @return Number of samples actually read.
	 */
	virtual sf_count_t readDoubleSamples(double *ptr, sf_count_t items) = 0;

	/** Writes short (16-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	virtual sf_count_t writeShortSamples(const short *ptr, sf_count_t items) = 0;

	/** Writes integer (32-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	virtual sf_count_t writeIntSamples(const int *ptr, sf_count_t items) = 0;

	/** Writes float (32-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	virtual sf_count_t writeFroatSamples(const float *ptr, sf_count_t items) = 0;

	/** Writes double (64-bit) samples to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] items Count of samples to write
	 *
	 * @return Number of samples actually written.
	 */
	virtual sf_count_t writeDoubleSamples(const double *ptr, sf_count_t items) = 0;

	/** Reads short (16-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	virtual sf_count_t readShortFrames(short *ptr, sf_count_t frames) = 0;

	/** Reads integer (32-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	virtual sf_count_t readIntFrames(int *ptr, sf_count_t frames) = 0;

	/** Reads float (32-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	virtual sf_count_t readFloatFrames(float *ptr, sf_count_t frames) = 0;

	/** Reads double (64-bit) frames from file
	 *
	 * @param[out] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to read
	 *
	 * @return Number of frames actually read.
	 */
	virtual sf_count_t readDoubleFrames(double *ptr, sf_count_t frames) = 0;

	/** Writes short (16-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	virtual sf_count_t writeShortFrames(const short *ptr, sf_count_t frames) = 0;

	/** Writes integer (32-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	virtual sf_count_t writeIntFrames(const int *ptr, sf_count_t frames) = 0;

	/** Writes float (32-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	virtual sf_count_t writeFloatFrames(const float *ptr, sf_count_t frames) = 0;

	/** Writes double (64-bit) frames to file
	 *
	 * @param[in] ptr Pointer to an allocated block of memory.
	 * @param[in] frames Count of frames to write
	 *
	 * @return Number of frames actually written.
	 */
	virtual sf_count_t writeDoubleFrames(const double *ptr, sf_count_t frames) = 0;

    virtual int getCurrentByterate() const = 0;

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
	virtual sf_count_t readRaw(void *ptr, sf_count_t bytes) = 0;

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
	virtual sf_count_t writeRaw(const void *ptr, sf_count_t bytes) = 0;

    /** Sets the specified chunk info
     *
     * @param[in] chunk_info Pointer to a SF_CHUNK_INFO struct
     *
     * Chunk info must be done before any audio data is written to the file.
     *
     * This will fail for format specific reserved chunks.
     *
     * The @c chunk_info->data pointer must be valid until the file is closed.
     *
     * @return Returns ::SF_ERR_NO_ERROR on success or non-zero on failure.
     */
    virtual int setChunk(const SF_CHUNK_INFO *chunk_info) = 0;

    /** Gets an iterator for all chunks matching chunk_info.
     *
     * @param[in] chunk_info Pointer to a ::SF_CHUNK_INFO struct
     *
     * The iterator will point to the first chunk matching chunk_info.
     *
     * Chunks are matching, if (chunk_info->id) matches the first
     *     (chunk_info->id_size) bytes of a chunk found in the SNDFILE* handle.
     *
     * If chunk_info is NULL, an iterator to all chunks in the SNDFILE* handle
     *     is returned.
     *
     * The values of chunk_info->datalen and chunk_info->data are ignored.
     *
     * If no matching chunks are found in the sndfile, NULL is returned.
     *
     * The returned iterator will stay valid until one of the following occurs:
     *
     * 1. The sndfile is closed.
     * 2. A new chunk is added using sf_set_chunk().
     * 3. Another chunk iterator function is called on the same SNDFILE* handle
     *    that causes the iterator to be modified.
     *
     * The memory for the iterator belongs to the SNDFILE* handle and is freed when
     * sf_close() is called.
     *
     * @return Chunk iterator on success, @c NULL otherwise.
     */
    virtual SF_CHUNK_ITERATOR *getChunkIterator(const SF_CHUNK_INFO *chunk_info) = 0;

    /** Iterates through chunks by incrementing the iterator.
     *
     * @param[in] iterator Current chunk iterator
     *
     * Increments the iterator and returns a handle to the new one.
     *
     * After this call, iterator will no longer be valid, and you must use the
     * newly returned handle from now on.
     *
     * The returned handle can be used to access the next chunk matching
     * the criteria as defined in sf_get_chunk_iterator().
     *
     * If iterator points to the last chunk, this will free all resources
     * associated with iterator and return @c NULL.
     *
     * The returned iterator will stay valid until sf_get_chunk_iterator_next
     * is called again, the sndfile is closed or a new chunk us added.
     *
     * @return Next chunk interator on success, @c NULL otherwise.
     */
    virtual SF_CHUNK_ITERATOR *getNextChunkIterator(SF_CHUNK_ITERATOR *iterator) = 0;

    /** Gets the size of the specified chunk
     *
     * @param[in] it Chunk iterafor
     * @param[out] chunk_info Pointer to a ::SF_CHUNK_INFO struct
     *
     * If the specified chunk exists, the size will be returned in the
     * ::SF_CHUNK_INFO::datalen field of the ::SF_CHUNK_INFO struct.
     *
     * Additionally, the id of the chunk will be copied to the @c id
     * field of the SF_CHUNK_INFO struct and it's @c id_size field will
     * be updated accordingly.
     *
     * If the chunk doesn't exist @c chunk_info->datalen will be zero, and the
     * @c id and @c id_size fields will be undefined.
     *
     * @return ::SF_ERR_NO_ERROR on success, negative error code otherwise.
     */
    virtual int getChunkSize(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info) = 0;

    /** Gets the specified chunk data
     *
     * @param[in] it Chunk iterafor
     * @param[out] chunk_info Pointer to a ::SF_CHUNK_INFO struct
     *
     * If the specified chunk exists, up to @c chunk_info->datalen bytes of
     * the chunk data will be copied into the @c chunk_info->data buffer
     * (allocated by the caller) and the @c chunk_info->datalen field
     * updated to reflect the size of the data. The @c id and @c id_size
     * field will be updated according to the retrieved chunk.
     *
     * If the chunk doesn't exist @c chunk_info->datalen will be zero, and the
     * @c id and @c id_size fields will be undefined.
     *
     * @return ::SF_ERR_NO_ERROR on success, negative error code otherwise.
     */
    virtual int getChunkData(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info) = 0;
};
