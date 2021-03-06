/*
** Copyright (C) 1999-2018 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#pragma once

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <inttypes.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "sndfile2k/sndfile2k.hpp"
#include "ref_ptr.h"

#include <vector>
#include <memory>

/*
** Inspiration : http://sourcefrog.net/weblog/software/languages/C/unused.html
*/
#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

#ifdef __GNUC__
#define WARN_UNUSED __attribute__((warn_unused_result))
#else
#define WARN_UNUSED
#endif

#define SNDFILE_MAGICK (0x1234C0DE)
#define SF_BUFFER_LEN (8192)
#define SF_FILENAME_LEN (1024)
#define SF_SYSERR_LEN (256)
#define SF_MAX_STRINGS (32)
#define SF_PARSELOG_LEN (2048)

#define PSF_SEEK_ERROR ((sf_count_t)-1)

#define BITWIDTH2BYTES(x) (((x) + 7) / 8)

/*	For some reason sizeof returns an unsigned  value which causes
**	a warning when that value is added or subtracted from a signed
**	value. Use SIGNED_SIZEOF instead.
*/
#define SIGNED_SIZEOF(x) ((int)sizeof(x))

#define ARRAY_LEN(x) ((sizeof(x) / sizeof((x)[0])))

#define COMPILE_TIME_ASSERT(e) (sizeof(struct { int : -!!(e); }))

#define SF_MAX_CHANNELS (1024)

/*
*	Macros for spliting the format file of SF_INFO into container type,
**	codec type and endian-ness.
*/
#define SF_CONTAINER(x) ((x)&SF_FORMAT_TYPEMASK)
#define SF_CODEC(x) ((x)&SF_FORMAT_SUBMASK)
#define SF_ENDIAN(x) ((x)&SF_FORMAT_ENDMASK)

/*
**	Binheader cast macros.
*/

#define BHW1(x) ((uint8_t)(x))
#define BHW2(x) ((uint16_t)(x))
#define BHW3(x) ((uint32_t)(x))
#define BHW4(x) ((uint32_t)(x))
#define BHW8(x) ((uint64_t)(x))

#define BHWm(x) ((uint32_t)(x))
#define BHWS(x) ((char *)(x))

#define BHWf(x) ((double)(x))
#define BHWd(x) ((double)(x))

#define BHWh(x) ((void *)(x))
#define BHWj(x) ((size_t)(x))
#define BHWp(x) ((char *)(x))
#define BHWo(x) ((size_t)(x))
#define BHWs(x) ((char *)(x))
#define BHWv(x) ((const void *)(x))
#define BHWz(x) ((size_t)(x))

/*------------------------------------------------------------------------------
*/

/* PEAK chunk location. */
enum SF_PEAK_POSITION
{
	SF_PEAK_START = 42,
	SF_PEAK_END = 43
};

enum SF_SCALE_VALUE
{
	SF_SCALE_MAX = 52,
	SF_SCALE_MIN = 53,
};

/* str_flags values. */
enum SF_STR_FLAGS
{
	SF_STR_ALLOW_START = 0x0100,
	SF_STR_ALLOW_END = 0x0200,
    SF_STR_LOCATE_START = 0x0400,
    SF_STR_LOCATE_END = 0x0800,
};

#define SFM_MASK (SFM_READ | SFM_WRITE | SFM_RDWR)
#define SFM_UNMASK (~SFM_MASK)

/*---------------------------------------------------------------------------------------
** Formats that may be supported at some time in the future.
** When support is finalised, these values move to src/sndfile.h.
*/

enum _SF_FORMAT
{
    /* Work in progress. */
    SF_FORMAT_SPEEX = 0x5000000,
    SF_FORMAT_OGGFLAC = 0x5000001,

    /* Formats supported read only. */
    SF_FORMAT_TXW = 0x4030000, /* Yamaha TX16 sampler file */
    SF_FORMAT_DWD = 0x4040000, /* DiamondWare Digirized */

    /* Following are detected but not supported. */
    SF_FORMAT_REX = 0x40A0000, /* Propellorheads Rex/Rcy */
    SF_FORMAT_REX2 = 0x40D0000, /* Propellorheads Rex2 */
    SF_FORMAT_KRZ = 0x40E0000, /* Kurzweil sampler file */
    SF_FORMAT_WMA = 0x4100000, /* Windows Media Audio. */
    SF_FORMAT_SHN = 0x4110000, /* Shorten. */

    /* Unsupported encodings. */
    SF_FORMAT_SVX_FIB = 0x1020, /* SVX Fibonacci Delta encoding. */
    SF_FORMAT_SVX_EXP = 0x1021, /* SVX Exponential Delta encoding. */

    SF_FORMAT_PCM_N = 0x1030
};

/*---------------------------------------------------------------------------------------
*/

struct ALAC_DECODER_INFO
{
    unsigned kuki_offset;
    unsigned pakt_offset;

    unsigned bits_per_sample;
    unsigned frames_per_packet;

    int64_t packets;
    int64_t valid_frames;
    int32_t priming_frames;
    int32_t remainder_frames;
};

/*---------------------------------------------------------------------------------------
**	PEAK_CHUNK - This chunk type is common to both AIFF and WAVE files although their
**	endian encodings are different.
*/

struct PEAK_POS
{
    double value; /* signed value of peak */
    sf_count_t position; /* the sample frame for the peak */
};

struct PEAK_INFO
{
    PEAK_INFO(int channels)
        : peaks(channels)
    {
    }

    /* libsndfile internal : write a PEAK chunk at the start or end of the file? */
    enum SF_PEAK_POSITION peak_loc = SF_PEAK_START;

    /* WAV/AIFF */
    unsigned int version = 0; /* version of the PEAK chunk */
    unsigned int timestamp = 0; /* secs since 1/1/1970  */

    /* CAF */
    unsigned int edit_number = 0;

    /* the per channel peak info */
    std::vector<PEAK_POS> peaks;

private:
    PEAK_INFO() = delete;
};

struct STR_DATA
{
    int type;
    int flags;
    size_t offset;
};

struct READ_CHUNK
{
    uint64_t hash;
    char id[64];
    unsigned id_size;
    uint32_t mark32;
    sf_count_t offset;
    uint32_t len;
};

struct WRITE_CHUNK
{
    uint64_t hash;
    uint32_t mark32;
    uint32_t len;
    void *data;
};

struct READ_CHUNKS
{
    uint32_t count;
    uint32_t used;
    struct READ_CHUNK *chunks;
};
struct WRITE_CHUNKS
{
    uint32_t count;
    uint32_t used;
    struct WRITE_CHUNK *chunks;
};

struct SF_CHUNK_ITERATOR
{
    uint32_t current;
    int64_t hash;
    char id[64];
    unsigned id_size;
    SNDFILE *sndfile;
};

static inline size_t make_size_t(int x)
{
    return (size_t)x;
} /* make_size_t */

#if SIZEOF_WCHAR_T == 2
typedef wchar_t sfwchar_t;
#else
typedef int16_t sfwchar_t;
#endif

static inline void *psf_memdup(const void *src, sf_count_t n)
{
    void *mem = calloc(1, n & 3 ? n + 4 - (n & 3) : n);
    return memcpy(mem, src, n);
} /* psf_memdup */

/*
**	This version of isprint specifically ignores any locale info. Its used for
**	determining which characters can be printed in things like hexdumps.
*/
static inline int psf_isprint(int ch)
{
    return (ch >= ' ' && ch <= '~');
} /* psf_isprint */

typedef union
{
    double dbuf[SF_BUFFER_LEN / sizeof(double)];
#if (defined(SIZEOF_INT64_T) && (SIZEOF_INT64_T == 8))
    int64_t lbuf[SF_BUFFER_LEN / sizeof(int64_t)];
#else
    long lbuf[SF_BUFFER_LEN / sizeof(double)];
#endif
    float fbuf[SF_BUFFER_LEN / sizeof(float)];
    int ibuf[SF_BUFFER_LEN / sizeof(int)];
    short sbuf[SF_BUFFER_LEN / sizeof(short)];
    char cbuf[SF_BUFFER_LEN / sizeof(char)];
    signed char scbuf[SF_BUFFER_LEN / sizeof(signed char)];
    unsigned char ucbuf[SF_BUFFER_LEN / sizeof(signed char)];
} BUF_UNION;

struct DITHER_DATA;
struct INTERLEAVE_DATA;

class SndFile: public ISndFile
{
public:

    SndFile();

    unsigned long ref() final;
    void unref() final;

	sf_count_t getFrames(void) const override;
	int getFormat(void) const override;
	int getChannels(void) const override;
	int getSamplerate(void) const override;

	int getError(void) const override;
	const char *getErrorString(void) const override;

	int command(int cmd, void *data, int datasize) override;

	sf_count_t seek(sf_count_t frames, int whence) override;

	void writeSync(void) override;

	int setString(int str_type, const char *str) override;
	const char *getString(int str_type) const override;

	sf_count_t readShortSamples(short *ptr, sf_count_t items) override;
	sf_count_t readIntSamples(int *ptr, sf_count_t items) override;
	sf_count_t readFloatSamples(float *ptr, sf_count_t items) override;
	sf_count_t readDoubleSamples(double *ptr, sf_count_t items) override;
	sf_count_t writeShortSamples(const short *ptr, sf_count_t items) override;
	sf_count_t writeIntSamples(const int *ptr, sf_count_t items) override;
	sf_count_t writeFroatSamples(const float *ptr, sf_count_t items) override;
	sf_count_t writeDoubleSamples(const double *ptr, sf_count_t items) override;
	sf_count_t readShortFrames(short *ptr, sf_count_t frames) override;
	sf_count_t readIntFrames(int *ptr, sf_count_t frames) override;
	sf_count_t readFloatFrames(float *ptr, sf_count_t frames) override;
	sf_count_t readDoubleFrames(double *ptr, sf_count_t frames) override;
	sf_count_t writeShortFrames(const short *ptr, sf_count_t frames) override;
	sf_count_t writeIntFrames(const int *ptr, sf_count_t frames) override;
	sf_count_t writeFloatFrames(const float *ptr, sf_count_t frames) override;
	sf_count_t writeDoubleFrames(const double *ptr, sf_count_t frames) override;

    int getCurrentByterate() const override;

	sf_count_t readRaw(void *ptr, sf_count_t bytes) override;
	sf_count_t writeRaw(const void *ptr, sf_count_t bytes) override;

    int setChunk(const SF_CHUNK_INFO *chunk_info) override;
    SF_CHUNK_ITERATOR *getChunkIterator(const SF_CHUNK_INFO *chunk_info) override;
    SF_CHUNK_ITERATOR *getNextChunkIterator(SF_CHUNK_ITERATOR *iterator) override;
    int getChunkSize(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info) override;
    int getChunkData(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info) override;

    int open(const char *filename, SF_FILEMODE mode, SF_INFO *sfinfo);
    int open(sf::ref_ptr<SF_STREAM> &stream, SF_FILEMODE mode, SF_INFO *sfinfo);
    bool is_open() const;
    void close();

    ~SndFile();

    char m_path[FILENAME_MAX] = {0};
    SF_FILEMODE m_mode = SFM_READ;
    sf::ref_ptr<SF_STREAM> m_stream;

    char m_syserr[SF_SYSERR_LEN] = {0};

    /* parselog and indx should only be changed within the logging functions
	** of common.c
	*/
    struct parselog_buffer
    {
        char buf[SF_PARSELOG_LEN];
        int indx;
    } m_parselog = {};

    struct header_storage
    {
        unsigned char *ptr;
        sf_count_t indx, end, len;
    } m_header = {};

    int m_rwf_endian = SF_ENDIAN_LITTLE; /* Header endian-ness flag. */

    /* Storage and housekeeping data for adding/reading strings from
	** sound files.
	*/
    struct strings_storage
    {
        struct STR_DATA data[SF_MAX_STRINGS];
        char *storage;
        size_t storage_len;
        size_t storage_used;
        uint32_t flags;
    } m_strings = {};

    /* Guard value. If this changes the buffers above have overflowed. */
    int m_Magick = SNDFILE_MAGICK;

    unsigned m_unique_id = 0;

    int m_error = 0;

    int m_endian = 0; /* File endianness : SF_ENDIAN_LITTLE or SF_ENDIAN_BIG. */
    int m_data_endswap = 0; /* Need to endswap data? */

    /*
	** Maximum float value for calculating the multiplier for
	** float/double to short/int conversions.
	*/
    int m_float_int_mult = 0;
    float m_float_max = -1.0;

    int m_scale_int_float = 0;

    /* True if clipping must be performed on float->int conversions. */
    bool m_add_clipping = false;

    SF_INFO sf = {};

    bool m_have_written = false; /* Has a single write been done to the file? */
    std::unique_ptr<PEAK_INFO> m_peak_info = nullptr;

    /* Cue Marker Info */
    std::vector<SF_CUE_POINT> m_cues;

    /* Loop Info */
    SF_LOOP_INFO *m_loop_info = nullptr;
    SF_INSTRUMENT *m_instrument = nullptr;

    /* Channel map data (if present) : an array of ints. */
    std::vector<int> m_channel_map;

    sf_count_t m_filelength = 0; /* Overall length of file. */

    sf_count_t m_dataoffset = 0; /* Offset in number of bytes from beginning of file. */
    sf_count_t m_datalength = 0; /* Length in bytes of the audio data. */
    sf_count_t m_dataend = 0; /* Offset to file tailer. */

    int m_blockwidth = 0; /* Size in bytes of one set of interleaved samples. */
    int m_bytewidth = 0; /* Size in bytes of one sample (one channel). */

    DITHER_DATA *m_dither = nullptr;
    INTERLEAVE_DATA *m_interleave = nullptr;

    int m_last_op = SFM_READ; /* Last operation; either SFM_READ or SFM_WRITE */
    sf_count_t m_read_current = 0;
    sf_count_t m_write_current = 0;

    /* This is a pointer to dynamically allocated file
	** container format specific data.
	*/
    void *m_container_data = nullptr;

    /* This is a pointer to dynamically allocated file
	** codec format specific data.
	*/
    void *m_codec_data = nullptr;

    SF_DITHER_INFO m_write_dither = {};
    SF_DITHER_INFO m_read_dither = {};

    int m_norm_double = SF_TRUE;
    int m_norm_float = SF_TRUE;

    int m_auto_header = SF_FALSE;

    int m_ieee_replace = SF_FALSE;

    /* A set of file specific function pointers */
    size_t (*read_short)(SndFile *, short *ptr, size_t len) = nullptr;
    size_t (*read_int)(SndFile *, int *ptr, size_t len) = nullptr;
    size_t (*read_float)(SndFile *, float *ptr, size_t len) = nullptr;
    size_t (*read_double)(SndFile *, double *ptr, size_t len) = nullptr;

    size_t (*write_short)(SndFile *, const short *ptr, size_t len) = nullptr;
    size_t (*write_int)(SndFile *, const int *ptr, size_t len) = nullptr;
    size_t (*write_float)(SndFile *, const float *ptr, size_t len) = nullptr;
    size_t (*write_double)(SndFile *, const double *ptr, size_t len) = nullptr;

    sf_count_t (*seek_from_start)(SndFile *, int mode, sf_count_t samples_from_start) = nullptr;
    int (*write_header)(SndFile *, int calc_length) = nullptr;
    size_t (*on_command)(SndFile *, int command, void *data, size_t datasize) = nullptr;
    int (*byterate)(SndFile *) = nullptr;

    /*
	**	Separate close functions for the codec and the container.
	**	The codec close function is always called first.
	*/
    int (*codec_close)(SndFile *) = nullptr;
    int (*container_close)(SndFile *) = nullptr;

    char *m_format_desc = nullptr;

    /* Chunk get/set. */
    SF_CHUNK_ITERATOR *m_iterator = nullptr;

    READ_CHUNKS m_rchunks = {};
	WRITE_CHUNKS m_wchunks = {};

    int (*set_chunk)(SndFile *, const SF_CHUNK_INFO *chunk_info) = nullptr;
    SF_CHUNK_ITERATOR *(*next_chunk_iterator)(SndFile *, SF_CHUNK_ITERATOR *iterator) = nullptr;
    int (*get_chunk_size)(SndFile *, const SF_CHUNK_ITERATOR *iterator, SF_CHUNK_INFO *chunk_info) = nullptr;
    int (*get_chunk_data)(SndFile *, const SF_CHUNK_ITERATOR *iterator, SF_CHUNK_INFO *chunk_info) = nullptr;

    /* Functions for writing to the internal logging buffer. */

    void log_putchar(char ch);
    void log_printf(const char *format, ...);
    void log_SF_INFO();

    /* Functions used when writing file headers. */

    int binheader_writef(const char *format, ...);
    void asciiheader_printf(const char *format, ...);

    /* Functions used when reading file headers. */

    int bump_header_allocation(sf_count_t needed);
    void binheader_seekf(sf_count_t position, SF_SEEK_MODE whence);
    int binheader_readf(char const *format, ...);

    size_t header_read(void *ptr, size_t bytes);
    int header_gets(char *ptr, int bufsize);
    void header_put_byte(char x);
    void header_put_marker(int x);
    void header_put_be_short(int x);
    void header_put_le_short(int x);
    void header_put_be_3byte(int x);
    void header_put_le_3byte(int x);
    void header_put_be_int(int x);
    void header_put_le_int(int x);
    void header_put_be_8byte(sf_count_t x);
    void header_put_le_8byte(sf_count_t x);

    int file_valid();

    //SNDFILE *open_file(SF_INFO *sfinfo);

    sf_count_t fseek(sf_count_t offset, int whence);
    size_t fread(void *ptr, size_t bytes, size_t count);
    size_t fwrite(const void *ptr, size_t bytes, size_t count);
    sf_count_t ftell();
    sf_count_t get_filelen();

    void fsync();

    int ftruncate(sf_count_t len);

    // Functions in strings.cpp

    const char *get_string(int str_type) const;
    int set_string(int str_type, const char *str);
    int store_string(int str_type, const char *str);
    int location_string_count(int location);

private:
    bool m_is_open = false;
    unsigned long m_ref = 0;
};

enum
{
    SFE_NO_ERROR = SF_ERR_NO_ERROR,
    SFE_BAD_OPEN_FORMAT = SF_ERR_UNRECOGNISED_FORMAT,
    SFE_SYSTEM = SF_ERR_SYSTEM,
    SFE_MALFORMED_FILE = SF_ERR_MALFORMED_FILE,
    SFE_UNSUPPORTED_ENCODING = SF_ERR_UNSUPPORTED_ENCODING,

    SFE_ZERO_MAJOR_FORMAT,
    SFE_ZERO_MINOR_FORMAT,
    SFE_BAD_FILE,
    SFE_BAD_FILE_READ,
    SFE_OPEN_FAILED,
    SFE_BAD_SNDFILE_PTR,
    SFE_BAD_SF_INFO_PTR,
    SFE_BAD_SF_INCOMPLETE,
    SFE_BAD_FILE_PTR,
    SFE_BAD_INT_PTR,
    SFE_BAD_STAT_SIZE,
    SFE_NO_TEMP_DIR,
    SFE_MALLOC_FAILED,
    SFE_UNIMPLEMENTED,
    SFE_BAD_READ_ALIGN,
    SFE_BAD_WRITE_ALIGN,
    SFE_NOT_READMODE,
    SFE_NOT_WRITEMODE,
    SFE_BAD_MODE_RW,
    SFE_BAD_SF_INFO,
    SFE_BAD_OFFSET,

    SFE_INTERNAL,
    SFE_BAD_COMMAND_PARAM,
    SFE_BAD_ENDIAN,
    SFE_CHANNEL_COUNT_ZERO,
    SFE_CHANNEL_COUNT,
    SFE_CHANNEL_COUNT_BAD,

    SFE_BAD_VIRTUAL_IO,

    SFE_INTERLEAVE_MODE,
    SFE_INTERLEAVE_SEEK,
    SFE_INTERLEAVE_READ,

    SFE_BAD_SEEK,
    SFE_NOT_SEEKABLE,
    SFE_AMBIGUOUS_SEEK,
    SFE_WRONG_SEEK,
    SFE_SEEK_FAILED,

    SFE_BAD_OPEN_MODE,
    SFE_RDWR_POSITION,
    SFE_RDWR_BAD_HEADER,
    SFE_CMD_HAS_DATA,
    SFE_BAD_BROADCAST_INFO_SIZE,
    SFE_BAD_BROADCAST_INFO_TOO_BIG,
    SFE_BAD_CART_INFO_SIZE,
    SFE_BAD_CART_INFO_TOO_BIG,

    SFE_STR_NO_SUPPORT,
    SFE_STR_NOT_WRITE,
    SFE_STR_MAX_DATA,
    SFE_STR_MAX_COUNT,
    SFE_STR_BAD_TYPE,
    SFE_STR_NO_ADD_END,
    SFE_STR_BAD_STRING,
    SFE_STR_WEIRD,

    SFE_WAV_NO_RIFF,
    SFE_WAV_NO_WAVE,
    SFE_WAV_NO_FMT,
    SFE_WAV_BAD_FMT,
    SFE_WAV_FMT_SHORT,
    SFE_WAV_BAD_FACT,
    SFE_WAV_BAD_PEAK,
    SFE_WAV_PEAK_B4_FMT,
    SFE_WAV_BAD_FORMAT,
    SFE_WAV_BAD_BLOCKALIGN,
    SFE_WAV_NO_DATA,
    SFE_WAV_BAD_LIST,
    SFE_WAV_ADPCM_NOT4BIT,
    SFE_WAV_ADPCM_CHANNELS,
    SFE_WAV_ADPCM_SAMPLES,
    SFE_WAV_GSM610_FORMAT,
    SFE_WAV_UNKNOWN_CHUNK,
    SFE_WAV_WVPK_DATA,
    SFE_WAV_NMS_FORMAT,

    SFE_AIFF_NO_FORM,
    SFE_AIFF_AIFF_NO_FORM,
    SFE_AIFF_COMM_NO_FORM,
    SFE_AIFF_SSND_NO_COMM,
    SFE_AIFF_UNKNOWN_CHUNK,
    SFE_AIFF_COMM_CHUNK_SIZE,
    SFE_AIFF_BAD_COMM_CHUNK,
    SFE_AIFF_PEAK_B4_COMM,
    SFE_AIFF_BAD_PEAK,
    SFE_AIFF_NO_SSND,
    SFE_AIFF_NO_DATA,
    SFE_AIFF_RW_SSND_NOT_LAST,

    SFE_AU_UNKNOWN_FORMAT,
    SFE_AU_NO_DOTSND,

    SFE_RAW_READ_BAD_SPEC,
    SFE_RAW_BAD_BITWIDTH,
    SFE_RAW_BAD_FORMAT,

    SFE_PAF_NO_MARKER,
    SFE_PAF_VERSION,
    SFE_PAF_UNKNOWN_FORMAT,
    SFE_PAF_SHORT_HEADER,
    SFE_PAF_BAD_CHANNELS,

    SFE_SVX_NO_FORM,
    SFE_SVX_NO_BODY,
    SFE_SVX_NO_DATA,
    SFE_SVX_BAD_COMP,
    SFE_SVX_BAD_NAME_LENGTH,

    SFE_NIST_BAD_HEADER,
    SFE_NIST_CRLF_CONVERISON,
    SFE_NIST_BAD_ENCODING,

    SFE_VOC_NO_CREATIVE,
    SFE_VOC_BAD_FORMAT,
    SFE_VOC_BAD_VERSION,
    SFE_VOC_BAD_MARKER,
    SFE_VOC_BAD_SECTIONS,
    SFE_VOC_MULTI_SAMPLERATE,
    SFE_VOC_MULTI_SECTION,
    SFE_VOC_MULTI_PARAM,
    SFE_VOC_SECTION_COUNT,

    SFE_IRCAM_NO_MARKER,
    SFE_IRCAM_BAD_CHANNELS,
    SFE_IRCAM_UNKNOWN_FORMAT,

    SFE_W64_64_BIT,
    SFE_W64_NO_RIFF,
    SFE_W64_NO_WAVE,
    SFE_W64_NO_DATA,
    SFE_W64_ADPCM_NOT4BIT,
    SFE_W64_ADPCM_CHANNELS,
    SFE_W64_GSM610_FORMAT,

    SFE_MAT4_BAD_NAME,
    SFE_MAT4_NO_SAMPLERATE,

    SFE_MAT5_BAD_ENDIAN,
    SFE_MAT5_NO_BLOCK,
    SFE_MAT5_SAMPLE_RATE,

    SFE_PVF_NO_PVF1,
    SFE_PVF_BAD_HEADER,
    SFE_PVF_BAD_BITWIDTH,

    SFE_DWVW_BAD_BITWIDTH,
    SFE_G72X_NOT_MONO,
    SFE_NMS_ADPCM_NOT_MONO,

    SFE_XI_BAD_HEADER,
    SFE_XI_EXCESS_SAMPLES,

    SFE_SDS_NOT_SDS,
    SFE_SDS_BAD_BIT_WIDTH,

    SFE_FLAC_BAD_HEADER,
    SFE_FLAC_NEW_DECODER,
    SFE_FLAC_INIT_DECODER,
    SFE_FLAC_LOST_SYNC,
    SFE_FLAC_BAD_SAMPLE_RATE,
    SFE_FLAC_CHANNEL_COUNT_CHANGED,
    SFE_FLAC_UNKOWN_ERROR,

    SFE_WVE_NOT_WVE,

    SFE_VORBIS_ENCODER_BUG,

    SFE_RF64_NOT_RF64,
    SFE_RF64_PEAK_B4_FMT,
    SFE_RF64_NO_DATA,

    SFE_BAD_CHUNK_PTR,
    SFE_UNKNOWN_CHUNK,
    SFE_BAD_CHUNK_FORMAT,
    SFE_BAD_CHUNK_MARKER,
    SFE_BAD_CHUNK_DATA_PTR,
    SFE_ALAC_FAIL_TMPFILE,
    SFE_FILENAME_TOO_LONG,
    SFE_NEGATIVE_RW_LEN,

    SFE_ALREADY_INITIALIZED,

    SFE_MAX_ERROR /* This must be last in list. */
};

int subformat_to_bytewidth(int format);
int s_bitwidth_to_subformat(int bits);
int u_bitwidth_to_subformat(int bits);

/*  Functions for reading and writing floats and doubles on processors
**	with non-IEEE floats/doubles.
*/
float float32_be_read(const unsigned char *cptr);
float float32_le_read(const unsigned char *cptr);
void float32_be_write(float in, unsigned char *out);
void float32_le_write(float in, unsigned char *out);

double double64_be_read(const unsigned char *cptr);
double double64_le_read(const unsigned char *cptr);
void double64_be_write(double in, unsigned char *out);
void double64_le_write(double in, unsigned char *out);

int32_t psf_rand_int32(void);

void append_snprintf(char *dest, size_t maxlen, const char *fmt, ...);
void psf_strlcpy_crlf(char *dest, const char *src, size_t destmax, size_t srcmax);

sf_count_t psf_decode_frame_count(SndFile *psf);

/* Functions used in the write function for updating the peak chunk. */

void peak_update_short(SndFile *psf, short *ptr, size_t items);
void peak_update_int(SndFile *psf, int *ptr, size_t items);
void peak_update_double(SndFile *psf, double *ptr, size_t items);

/* Functions defined in command.c. */

int psf_get_format_simple_count(void);
int psf_get_format_simple(SF_FORMAT_INFO *data);

int psf_get_format_info(SF_FORMAT_INFO *data);

int psf_get_format_major_count(void);
int psf_get_format_major(SF_FORMAT_INFO *data);

int psf_get_format_subtype_count(void);
int psf_get_format_subtype(SF_FORMAT_INFO *data);

void psf_generate_format_desc(SndFile *psf);

double psf_calc_signal_max(SndFile *psf, int normalize);
int psf_calc_max_all_channels(SndFile *psf, double *peaks, int normalize);

int psf_get_signal_max(SndFile *psf, double *peak);
int psf_get_max_all_channels(SndFile *psf, double *peaks);

/* Default seek function. Use for PCM and float encoded data. */
sf_count_t psf_default_seek(SndFile *psf, int mode, sf_count_t samples_from_start);

int macos_guess_file_type(SndFile *psf, const char *filename);

/*------------------------------------------------------------------------------------
**	File I/O functions which will allow access to large files (> 2 Gig) on
**	some 32 bit OSes. Implementation in file_io.c.
*/

#define SENSIBLE_SIZE (0x40000000)

static void psf_log_syserr(SndFile *psf, int error);

int psf_open_file_stream(const char *filename, SF_FILEMODE mode, SF_STREAM **stream);
#ifdef _WIN32
int psf_open_file_stream(const wchar_t *filename, SF_FILEMODE mode, SF_STREAM **stream);
#endif

/*
void psf_fclearerr (SndFile *psf) ;
int psf_ferror (SndFile *psf) ;
*/

/*------------------------------------------------------------------------------------
** Functions for reading and writing different file formats.
*/

int aiff_open(SndFile *psf);
int au_open(SndFile *psf);
int avr_open(SndFile *psf);
int htk_open(SndFile *psf);
int ircam_open(SndFile *psf);
int mat4_open(SndFile *psf);
int mat5_open(SndFile *psf);
int nist_open(SndFile *psf);
int paf_open(SndFile *psf);
int pvf_open(SndFile *psf);
int raw_open(SndFile *psf);
int sds_open(SndFile *psf);
int svx_open(SndFile *psf);
int voc_open(SndFile *psf);
int w64_open(SndFile *psf);
int wav_open(SndFile *psf);
int xi_open(SndFile *psf);
int flac_open(SndFile *psf);
int caf_open(SndFile *psf);
int mpc2k_open(SndFile *psf);
int rf64_open(SndFile *psf);

int ogg_vorbis_open(SndFile *psf);
int ogg_speex_open(SndFile *psf);
int ogg_pcm_open(SndFile *psf);
int ogg_opus_open(SndFile *psf);
int ogg_open(SndFile *psf);

/* In progress. Do not currently work. */

int mpeg_open(SndFile *psf);
int rx2_open(SndFile *psf);
int txw_open(SndFile *psf);
int wve_open(SndFile *psf);
int dwd_open(SndFile *psf);

/*------------------------------------------------------------------------------------
**	Init functions for a number of common data encodings.
*/

int pcm_init(SndFile *psf);
int ulaw_init(SndFile *psf);
int alaw_init(SndFile *psf);
int float32_init(SndFile *psf);
int double64_init(SndFile *psf);
int dwvw_init(SndFile *psf, int bitwidth);
int gsm610_init(SndFile *psf);
int nms_adpcm_init(SndFile *psf);
int vox_adpcm_init(SndFile *psf);
int flac_init(SndFile *psf);
int g72x_init(SndFile *psf);
int alac_init(SndFile *psf, const struct ALAC_DECODER_INFO *info);

struct DITHER_DATA
{
    int read_short_dither_bits, read_int_dither_bits;
    int write_short_dither_bits, write_int_dither_bits;
    double read_float_dither_scale, read_double_dither_bits;
    double write_float_dither_scale, write_double_dither_bits;

    size_t (*read_short)(SndFile *psf, short *ptr, size_t len);
    size_t (*read_int)(SndFile *psf, int *ptr, size_t len);
    size_t (*read_float)(SndFile *psf, float *ptr, size_t len);
    size_t (*read_double)(SndFile *psf, double *ptr, size_t len);

    size_t (*write_short)(SndFile *psf, const short *ptr, size_t len);
    size_t (*write_int)(SndFile *psf, const int *ptr, size_t len);
    size_t (*write_float)(SndFile *psf, const float *ptr, size_t len);
    size_t (*write_double)(SndFile *psf, const double *ptr, size_t len);

    double buffer[SF_BUFFER_LEN / sizeof(double)];
};

int dither_init(SndFile *psf, int mode);

int wavlike_ima_init(SndFile *psf, int blockalign, int samplesperblock);
int wavlike_msadpcm_init(SndFile *psf, int blockalign, int samplesperblock);

int aiff_ima_init(SndFile *psf, int blockalign, int samplesperblock);

struct INTERLEAVE_DATA
{
    double buffer[SF_BUFFER_LEN / sizeof(double)];

    sf_count_t channel_len;

    size_t (*read_short)(SndFile *, short *ptr, size_t len);
    size_t (*read_int)(SndFile *, int *ptr, size_t len);
    size_t (*read_float)(SndFile *, float *ptr, size_t len);
    size_t (*read_double)(SndFile *, double *ptr, size_t len);
};

int interleave_init(SndFile *psf);

/*------------------------------------------------------------------------------------
** Chunk logging functions.
*/

SF_CHUNK_ITERATOR *psf_get_chunk_iterator(SndFile *psf, const char *marker_str);
SF_CHUNK_ITERATOR *psf_next_chunk_iterator(const struct READ_CHUNKS *pchk, SF_CHUNK_ITERATOR *iterator);
int psf_store_read_chunk_u32(struct READ_CHUNKS *pchk, uint32_t marker, sf_count_t offset, uint32_t len);
int psf_store_read_chunk_str(struct READ_CHUNKS *pchk, const char *marker, sf_count_t offset,
                             uint32_t len);
int psf_save_write_chunk(struct WRITE_CHUNKS *pchk, const SF_CHUNK_INFO *chunk_info);
int psf_find_read_chunk_str(const struct READ_CHUNKS *pchk, const char *marker);
int psf_find_read_chunk_m32(const struct READ_CHUNKS *pchk, uint32_t marker);
int psf_find_read_chunk_iterator(const struct READ_CHUNKS *pchk, const SF_CHUNK_ITERATOR *marker);

int psf_find_write_chunk(struct WRITE_CHUNKS *pchk, const char *marker);

static inline int fourcc_to_marker(const SF_CHUNK_INFO *chunk_info)
{
    const unsigned char *cptr;

    if (chunk_info->id_size != 4)
        return 0;

    cptr = (const unsigned char *)chunk_info->id;
    return (cptr[3] << 24) + (cptr[2] << 16) + (cptr[1] << 8) + cptr[0];
} /* fourcc_to_marker */

/*------------------------------------------------------------------------------------
** Functions that work like OpenBSD's strlcpy/strlcat to replace strncpy/strncat.
**
** See : http://www.gratisoft.us/todd/papers/strlcpy.html
**
** These functions are available on *BSD, but are not avaialble everywhere so we
** implement them here.
**
** The argument order has been changed to that of strncpy/strncat to cause
** compiler errors if code is carelessly converted from one to the other.
*/

static inline void psf_strlcat(char *dest, size_t n, const char *src)
{
    strncat(dest, src, n - strlen(dest) - 1);
    dest[n - 1] = 0;
} /* psf_strlcat */

static inline void psf_strlcpy(char *dest, size_t n, const char *src)
{
    strncpy(dest, src, n - 1);
    dest[n - 1] = 0;
} /* psf_strlcpy */

/*------------------------------------------------------------------------------------
** Other helper functions.
*/

void *psf_memset(void *s, int c, sf_count_t n);

SF_CUE_POINT *psf_cues_dup(const SF_CUE_POINT *cues, uint32_t cue_count);
SF_CUE_POINT *psf_cues_alloc(uint32_t cue_count);
void psf_get_cues(SndFile *psf, void *data, size_t datasize);

SF_INSTRUMENT *psf_instrument_alloc(void);

void psf_sanitize_string(char *cptr, int len);

/* Generate the current date as a string. */
void psf_get_date_str(char *str, int maxlen);

struct AUDIO_DETECT
{
    int channels;
    int endianness;
};

int audio_detect(SndFile *psf, struct AUDIO_DETECT *ad, const unsigned char *data, int datalen);
int id3_skip(SndFile *psf);

void alac_get_desc_chunk_items(int subformat, uint32_t *fmt_flags, uint32_t *frames_per_packet);

FILE *psf_open_tmpfile(char *fname, size_t fnamelen);

/*------------------------------------------------------------------------------------
** Helper/debug functions.
*/

void psf_hexdump(const void *ptr, int len);

const char *str_of_major_format(int format);
const char *str_of_minor_format(int format);
const char *str_of_open_mode(int mode);
const char *str_of_endianness(int end);

/*------------------------------------------------------------------------------------
** Extra commands for sf_command(). Not for public use yet.
*/

enum _SFC_COMMAND
{
    SFC_TEST_AIFF_ADD_INST_CHUNK = 0x2000,
    SFC_TEST_WAV_ADD_INFO_CHUNK = 0x2010
};

/*
** Maybe, one day, make these functions or something like them, public.
**
** Buffer to buffer dithering. Pointer in and out are allowed to point
** to the same buffer for in-place dithering.
*/

#if 0
int sf_dither_short(const SF_DITHER_INFO *dither, const short *in, short *out,
                    size_t count);
int sf_dither_int(const SF_DITHER_INFO *dither, const int *in, int *out,
                  size_t count);
int sf_dither_float(const SF_DITHER_INFO *dither, const float *in, float *out,
                    size_t count);
int sf_dither_double(const SF_DITHER_INFO *dither, const double *in,
                     double *out, size_t count);
#endif

/*------------------------------------------------------------------------------------
** Data conversion functions.
*/

void psf_f2s_array(const float *src, short *dest, size_t count, int normalize);
void psf_f2s_clip_array(const float *src, short *dest, size_t count, int normalize);

void psf_d2s_array(const double *src, short *dest, size_t count, int normalize);
void psf_d2s_clip_array(const double *src, short *dest, size_t count, int normalize);

void psf_f2i_array(const float *src, int *dest, size_t count, int normalize);
void psf_f2i_clip_array(const float *src, int *dest, size_t count, int normalize);

void psf_d2i_array(const double *src, int *dest, size_t count, int normalize);
void psf_d2i_clip_array(const double *src, int *dest, size_t count, int normalize);
