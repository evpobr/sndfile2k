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
#include <string.h>

#include <inttypes.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef SNDFILE2_H
#include "sndfile2k/sndfile2k.h"
#endif

#include <vector>

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
    /* libsndfile internal : write a PEAK chunk at the start or end of the file? */
    enum SF_PEAK_POSITION peak_loc;

    /* WAV/AIFF */
    unsigned int version; /* version of the PEAK chunk */
    unsigned int timestamp; /* secs since 1/1/1970  */

    /* CAF */
    unsigned int edit_number;

    /* the per channel peak info */
    struct PEAK_POS peaks[];
};

static inline struct PEAK_INFO *peak_info_calloc(int channels)
{
    return (struct PEAK_INFO *)calloc(1, sizeof(struct PEAK_INFO) + channels * sizeof(struct PEAK_POS));
} /* peak_info_calloc */

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

/*=======================================================================================
**	SF_PRIVATE stuct - a pointer to this struct is passed back to the caller of the
**	sf_open_XXXX functions. The caller however has no knowledge of the struct's
**	contents.
*/

typedef struct PSF_FILE
{
    /* Virtual I/O functions. */
    SF_VIRTUAL_IO vio;
    int virtual_io;
    void *vio_user_data;
    unsigned long vio_ref_counter;
    SF_BOOL use_new_vio;

    union
    {
        char c[SF_FILENAME_LEN];
        sfwchar_t wc[SF_FILENAME_LEN];
    } path;

    union
    {
        char c[SF_FILENAME_LEN];
        sfwchar_t wc[SF_FILENAME_LEN];
    } dir;

    union
    {
        char c[SF_FILENAME_LEN / 4];
        sfwchar_t wc[SF_FILENAME_LEN / 4];
    } name;

    /*
	**	These fields can only be used in src/file_io.c.
	**	They are basically the same as a windows file HANDLE.
	*/
    void *handle, *hsaved;
    /* These fields can only be used in src/file_io.c. */
    int filedes, savedes;

#if (defined(_WIN32) || defined(__CYGWIN__))
    SF_BOOL use_wchar;
#endif

    SF_BOOL do_not_close_descriptor;
    SF_FILEMODE mode; /* Open mode : SFM_READ, SFM_WRITE or SFM_RDWR. */

    /* Vairables for handling pipes. */
    SF_BOOL is_pipe; /* True if file is a pipe. */
    sf_count_t pipeoffset; /* Number of bytes read from a pipe. */

} PSF_FILE;

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

struct SF_PRIVATE
{
    /* Canary in a coal mine. */
    union canary
    {
        /* Place a double here to encourage double alignment. */
        double d[2];
        char c[16];
    } _canary;

    PSF_FILE file;

    char syserr[SF_SYSERR_LEN];

    /* parselog and indx should only be changed within the logging functions
	** of common.c
	*/
    struct parselog_buffer
    {
        char buf[SF_PARSELOG_LEN];
        int indx;
    } parselog;

    struct header_storage
    {
        unsigned char *ptr;
        sf_count_t indx, end, len;
    } header;

    int rwf_endian; /* Header endian-ness flag. */

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
    } strings;

    /* Guard value. If this changes the buffers above have overflowed. */
    int Magick;

    unsigned unique_id;

    int error;

    int endian; /* File endianness : SF_ENDIAN_LITTLE or SF_ENDIAN_BIG. */
    int data_endswap; /* Need to endswap data? */

    /*
	** Maximum float value for calculating the multiplier for
	** float/double to short/int conversions.
	*/
    int float_int_mult;
    float float_max;

    int scale_int_float;

    /* True if clipping must be performed on float->int conversions. */
    int add_clipping;

    SF_INFO sf;

    int have_written; /* Has a single write been done to the file? */
    struct PEAK_INFO *peak_info;

    /* Cue Marker Info */
    std::vector<SF_CUE_POINT> cues;

    /* Loop Info */
    SF_LOOP_INFO *loop_info;
    SF_INSTRUMENT *instrument;

    /* Channel map data (if present) : an array of ints. */
    int *channel_map;

    sf_count_t filelength; /* Overall length of (embedded) file. */
    sf_count_t fileoffset; /* Offset in number of bytes from beginning of file. */

    sf_count_t dataoffset; /* Offset in number of bytes from beginning of file. */
    sf_count_t datalength; /* Length in bytes of the audio data. */
    sf_count_t dataend; /* Offset to file tailer. */

    int blockwidth; /* Size in bytes of one set of interleaved samples. */
    int bytewidth; /* Size in bytes of one sample (one channel). */

    void *dither;
    void *interleave;

    int last_op; /* Last operation; either SFM_READ or SFM_WRITE */
    sf_count_t read_current;
    sf_count_t write_current;

    /* This is a pointer to dynamically allocated file
	** container format specific data.
	*/
    void *container_data;

    /* This is a pointer to dynamically allocated file
	** codec format specific data.
	*/
    void *codec_data;

    SF_DITHER_INFO write_dither;
    SF_DITHER_INFO read_dither;

    int norm_double;
    int norm_float;

    int auto_header;

    int ieee_replace;

    /* A set of file specific function pointers */
    size_t (*read_short)(struct SF_PRIVATE *, short *ptr, size_t len);
    size_t (*read_int)(struct SF_PRIVATE *, int *ptr, size_t len);
    size_t (*read_float)(struct SF_PRIVATE *, float *ptr, size_t len);
    size_t (*read_double)(struct SF_PRIVATE *, double *ptr, size_t len);

    size_t (*write_short)(struct SF_PRIVATE *, const short *ptr, size_t len);
    size_t (*write_int)(struct SF_PRIVATE *, const int *ptr, size_t len);
    size_t (*write_float)(struct SF_PRIVATE *, const float *ptr, size_t len);
    size_t (*write_double)(struct SF_PRIVATE *, const double *ptr, size_t len);

    sf_count_t (*seek)(struct SF_PRIVATE *, int mode, sf_count_t samples_from_start);
    int (*write_header)(struct SF_PRIVATE *, int calc_length);
    size_t (*command)(struct SF_PRIVATE *, int command, void *data, size_t datasize);
    int (*byterate)(struct SF_PRIVATE *);

    /*
	**	Separate close functions for the codec and the container.
	**	The codec close function is always called first.
	*/
    int (*codec_close)(struct SF_PRIVATE *);
    int (*container_close)(struct SF_PRIVATE *);

    char *format_desc;

    /* Chunk get/set. */
    SF_CHUNK_ITERATOR *iterator;

    struct READ_CHUNKS rchunks;
	struct WRITE_CHUNKS wchunks;

    int (*set_chunk)(struct SF_PRIVATE *, const SF_CHUNK_INFO *chunk_info);
    SF_CHUNK_ITERATOR *(*next_chunk_iterator)(struct SF_PRIVATE *, SF_CHUNK_ITERATOR *iterator);
    int (*get_chunk_size)(struct SF_PRIVATE *, const SF_CHUNK_ITERATOR *iterator,
                          SF_CHUNK_INFO *chunk_info);
    int (*get_chunk_data)(struct SF_PRIVATE *, const SF_CHUNK_ITERATOR *iterator,
                          SF_CHUNK_INFO *chunk_info);

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

    int fopen();
    int set_stdio();
    int file_valid();
    void set_file(int fd);
    void init_files();

    SNDFILE *open_file(SF_INFO *sfinfo);

    int close();

    sf_count_t fseek(sf_count_t offset, int whence);
    size_t fread(void *ptr, size_t bytes, size_t count);
    size_t fwrite(const void *ptr, size_t bytes, size_t count);
    sf_count_t fgets(char *buffer, size_t bufsize);
    sf_count_t ftell();
    sf_count_t get_filelen();

    void fsync();

    SF_BOOL is_pipe();

    int ftruncate(sf_count_t len);
    int fclose();
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
    SFE_NO_EMBED_SUPPORT,
    SFE_NO_EMBEDDED_RDWR,
    SFE_NO_PIPE_WRITE,

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
    SFE_OPEN_PIPE_RDWR,
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
    SFE_AU_EMBED_BAD_LEN,

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
    SFE_VOC_NO_PIPE,

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
    SFE_XI_NO_PIPE,

    SFE_HTK_NO_PIPE,

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
    SFE_WVE_NO_PIPE,

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

    SFE_MAX_ERROR /* This must be last in list. */
};

/* Allocate and initialize the SF_PRIVATE struct. */
SF_PRIVATE *psf_allocate(void);

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

sf_count_t psf_decode_frame_count(SF_PRIVATE *psf);

/* Functions used in the write function for updating the peak chunk. */

void peak_update_short(SF_PRIVATE *psf, short *ptr, size_t items);
void peak_update_int(SF_PRIVATE *psf, int *ptr, size_t items);
void peak_update_double(SF_PRIVATE *psf, double *ptr, size_t items);

/* Functions defined in command.c. */

int psf_get_format_simple_count(void);
int psf_get_format_simple(SF_FORMAT_INFO *data);

int psf_get_format_info(SF_FORMAT_INFO *data);

int psf_get_format_major_count(void);
int psf_get_format_major(SF_FORMAT_INFO *data);

int psf_get_format_subtype_count(void);
int psf_get_format_subtype(SF_FORMAT_INFO *data);

void psf_generate_format_desc(SF_PRIVATE *psf);

double psf_calc_signal_max(SF_PRIVATE *psf, int normalize);
int psf_calc_max_all_channels(SF_PRIVATE *psf, double *peaks, int normalize);

int psf_get_signal_max(SF_PRIVATE *psf, double *peak);
int psf_get_max_all_channels(SF_PRIVATE *psf, double *peaks);

/* Functions in strings.c. */

const char *psf_get_string(SF_PRIVATE *psf, int str_type);
int psf_set_string(SF_PRIVATE *psf, int str_type, const char *str);
int psf_store_string(SF_PRIVATE *psf, int str_type, const char *str);
int psf_location_string_count(const SF_PRIVATE *psf, int location);

/* Default seek function. Use for PCM and float encoded data. */
sf_count_t psf_default_seek(SF_PRIVATE *psf, int mode, sf_count_t samples_from_start);

int macos_guess_file_type(SF_PRIVATE *psf, const char *filename);

/*------------------------------------------------------------------------------------
**	File I/O functions which will allow access to large files (> 2 Gig) on
**	some 32 bit OSes. Implementation in file_io.c.
*/

#define SENSIBLE_SIZE (0x40000000)

static void psf_log_syserr(SF_PRIVATE *psf, int error);

SF_VIRTUAL_IO *psf_get_vio();

/*
void psf_fclearerr (SF_PRIVATE *psf) ;
int psf_ferror (SF_PRIVATE *psf) ;
*/

/*------------------------------------------------------------------------------------
** Functions for reading and writing different file formats.
*/

int aiff_open(SF_PRIVATE *psf);
int au_open(SF_PRIVATE *psf);
int avr_open(SF_PRIVATE *psf);
int htk_open(SF_PRIVATE *psf);
int ircam_open(SF_PRIVATE *psf);
int mat4_open(SF_PRIVATE *psf);
int mat5_open(SF_PRIVATE *psf);
int nist_open(SF_PRIVATE *psf);
int paf_open(SF_PRIVATE *psf);
int pvf_open(SF_PRIVATE *psf);
int raw_open(SF_PRIVATE *psf);
int sds_open(SF_PRIVATE *psf);
int svx_open(SF_PRIVATE *psf);
int voc_open(SF_PRIVATE *psf);
int w64_open(SF_PRIVATE *psf);
int wav_open(SF_PRIVATE *psf);
int xi_open(SF_PRIVATE *psf);
int flac_open(SF_PRIVATE *psf);
int caf_open(SF_PRIVATE *psf);
int mpc2k_open(SF_PRIVATE *psf);
int rf64_open(SF_PRIVATE *psf);

int ogg_vorbis_open(SF_PRIVATE *psf);
int ogg_speex_open(SF_PRIVATE *psf);
int ogg_pcm_open(SF_PRIVATE *psf);
int ogg_opus_open(SF_PRIVATE *psf);
int ogg_open(SF_PRIVATE *psf);

/* In progress. Do not currently work. */

int mpeg_open(SF_PRIVATE *psf);
int rx2_open(SF_PRIVATE *psf);
int txw_open(SF_PRIVATE *psf);
int wve_open(SF_PRIVATE *psf);
int dwd_open(SF_PRIVATE *psf);

/*------------------------------------------------------------------------------------
**	Init functions for a number of common data encodings.
*/

int pcm_init(SF_PRIVATE *psf);
int ulaw_init(SF_PRIVATE *psf);
int alaw_init(SF_PRIVATE *psf);
int float32_init(SF_PRIVATE *psf);
int double64_init(SF_PRIVATE *psf);
int dwvw_init(SF_PRIVATE *psf, int bitwidth);
int gsm610_init(SF_PRIVATE *psf);
int nms_adpcm_init(SF_PRIVATE *psf);
int vox_adpcm_init(SF_PRIVATE *psf);
int flac_init(SF_PRIVATE *psf);
int g72x_init(SF_PRIVATE *psf);
int alac_init(SF_PRIVATE *psf, const struct ALAC_DECODER_INFO *info);

int dither_init(SF_PRIVATE *psf, int mode);

int wavlike_ima_init(SF_PRIVATE *psf, int blockalign, int samplesperblock);
int wavlike_msadpcm_init(SF_PRIVATE *psf, int blockalign, int samplesperblock);

int aiff_ima_init(SF_PRIVATE *psf, int blockalign, int samplesperblock);

int interleave_init(SF_PRIVATE *psf);

/*------------------------------------------------------------------------------------
** Chunk logging functions.
*/

SF_CHUNK_ITERATOR *psf_get_chunk_iterator(SF_PRIVATE *psf, const char *marker_str);
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
void psf_get_cues(SF_PRIVATE *psf, void *data, size_t datasize);

SF_INSTRUMENT *psf_instrument_alloc(void);

void psf_sanitize_string(char *cptr, int len);

/* Generate the current date as a string. */
void psf_get_date_str(char *str, int maxlen);

struct AUDIO_DETECT
{
    int channels;
    int endianness;
};

int audio_detect(SF_PRIVATE *psf, struct AUDIO_DETECT *ad, const unsigned char *data, int datalen);
int id3_skip(SF_PRIVATE *psf);

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
