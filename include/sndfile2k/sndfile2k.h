/*
** Copyright (C) 1999-2016 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2017 evpobr <evpobr@gmail.com>
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

/** @brief SndFile2K C API reference
 *
 *  @file sndfile2k.h
 *
 */

#pragma once

#ifndef SNDFILE2K_H
#define SNDFILE2K_H

/* This is the version 1.0.X header file. */
#define SNDFILE_1

#include <stdio.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Set of format bit flags.
 *
 * The SF_INFO::format field of the ::SF_INFO structure is made up of the
 * bit-wise OR of a major format type (values between @c 0x10000 and
 * @c 0x08000000), a minor format type (with values less than @c 0x10000) and an
 * optional endianness value.
 *
 * ::SF_FORMAT_SUBMASK, ::SF_FORMAT_TYPEMASK and ::SF_FORMAT_ENDMASK bit masks
 * can be used to separate major, minor format types and endianness value.
 *
 * @note Not all combinations of endianness and major and minor file types are
 * valid. You can use sf_format_check() function to check format flags
 * combination.
 *
 * @sa SF_INFO
 * @sa sf_format_check()
 */
typedef enum SF_FORMAT
{
    // Major formats.

    //! Microsoft WAV format (little endian default)
    SF_FORMAT_WAV = 0x010000,
    //! Apple/SGI AIFF format (big endian)
    SF_FORMAT_AIFF = 0x020000,
    //! Sun/NeXT AU format (big endian)
    SF_FORMAT_AU = 0x030000,
    //! RAW PCM data
    SF_FORMAT_RAW = 0x040000,
    //! Ensoniq PARIS file format
    SF_FORMAT_PAF = 0x050000,
    //! Amiga IFF / SVX8 / SV16 format
    SF_FORMAT_SVX = 0x060000,
    //! Sphere NIST format
    SF_FORMAT_NIST = 0x070000,
    //! VOC files
    SF_FORMAT_VOC = 0x080000,
    //! Berkeley/IRCAM/CARL
    SF_FORMAT_IRCAM = 0x0A0000,
    //! Sonic Foundry's 64 bit RIFF/WAV
    SF_FORMAT_W64 = 0x0B0000,
    //! Matlab (tm) V4.2 / GNU Octave 2.0
    SF_FORMAT_MAT4 = 0x0C0000,
    //! Matlab (tm) V5.0 / GNU Octave 2.1
    SF_FORMAT_MAT5 = 0x0D0000,
    //! Portable Voice Format
    SF_FORMAT_PVF = 0x0E0000,
    //! Fasttracker 2 Extended Instrument
    SF_FORMAT_XI = 0x0F0000,
    //! HMM Tool Kit format
    SF_FORMAT_HTK = 0x100000,
    //! Midi Sample Dump Standard
    SF_FORMAT_SDS = 0x110000,
    //! Audio Visual Research
    SF_FORMAT_AVR = 0x120000,
    //! MS WAVE with WAVEFORMATEX
    SF_FORMAT_WAVEX = 0x130000,
    //! FLAC lossless file format
    SF_FORMAT_FLAC = 0x170000,
    //! Core Audio File format
    SF_FORMAT_CAF = 0x180000,
    //! Psion WVE format
    SF_FORMAT_WVE = 0x190000,
    //! Xiph OGG container
    SF_FORMAT_OGG = 0x200000,
    //! Akai MPC 2000 sampler
    SF_FORMAT_MPC2K = 0x210000,
    //! RF64 WAV file
    SF_FORMAT_RF64 = 0x220000,

    // Subtypes from here on

    //! Signed 8 bit data
    SF_FORMAT_PCM_S8 = 0x0001,
    //! Signed 16 bit data
    SF_FORMAT_PCM_16 = 0x0002,
    //! Signed 24 bit data
    SF_FORMAT_PCM_24 = 0x0003,
    //! Signed 32 bit data
    SF_FORMAT_PCM_32 = 0x0004,
    //! Unsigned 8 bit data (WAV and RAW only)
    SF_FORMAT_PCM_U8 = 0x0005,
    //! 32 bit float data
    SF_FORMAT_FLOAT = 0x0006,
    //! 64 bit float data
    SF_FORMAT_DOUBLE = 0x0007,
    //! U-Law encoded
    SF_FORMAT_ULAW = 0x0010,
    //! A-Law encoded
    SF_FORMAT_ALAW = 0x0011,
    //! IMA ADPCM
    SF_FORMAT_IMA_ADPCM = 0x0012,
    //! Microsoft ADPCM
    SF_FORMAT_MS_ADPCM = 0x0013,
    //! GSM 6.10 encoding
    SF_FORMAT_GSM610 = 0x0020,
    //! OKI / Dialogix ADPCM
    SF_FORMAT_VOX_ADPCM = 0x0021,
    //! 16kbs NMS G721-variant encoding
    SF_FORMAT_NMS_ADPCM_16 = 0x0022,
    //! 24kbs NMS G721-variant encoding
    SF_FORMAT_NMS_ADPCM_24 = 0x0023,
    //! 32kbs NMS G721-variant encoding
    SF_FORMAT_NMS_ADPCM_32 = 0x0024,
    //! 32kbs G721 ADPCM encoding
    SF_FORMAT_G721_32 = 0x0030,
    //! 24kbs G723 ADPCM encoding
    SF_FORMAT_G723_24 = 0x0031,
    //! 40kbs G723 ADPCM encoding
    SF_FORMAT_G723_40 = 0x0032,
    //! 12 bit Delta Width Variable Word encoding
    SF_FORMAT_DWVW_12 = 0x0040,
    //! 16 bit Delta Width Variable Word encoding
    SF_FORMAT_DWVW_16 = 0x0041,
    //! 24 bit Delta Width Variable Word encoding
    SF_FORMAT_DWVW_24 = 0x0042,
    //! N bit Delta Width Variable Word encoding
    SF_FORMAT_DWVW_N = 0x0043,
    //! 8 bit differential PCM (XI only)
    SF_FORMAT_DPCM_8 = 0x0050,
    //! 16 bit differential PCM (XI only)
    SF_FORMAT_DPCM_16 = 0x0051,
    //! Xiph Vorbis encoding
    SF_FORMAT_VORBIS = 0x0060,
    //! Apple Lossless Audio Codec (16 bit)
    SF_FORMAT_ALAC_16 = 0x0070,
    //! Apple Lossless Audio Codec (20 bit)
    SF_FORMAT_ALAC_20 = 0x0071,
    //! Apple Lossless Audio Codec (24 bit)
    SF_FORMAT_ALAC_24 = 0x0072,
    //! Apple Lossless Audio Codec (32 bit)
    SF_FORMAT_ALAC_32 = 0x0073,

    // Endian-ness options

    //! Default file endian-ness
    SF_ENDIAN_FILE = 0x00000000,
    //! Force little endian-ness
    SF_ENDIAN_LITTLE = 0x10000000,
    //! Force big endian-ness
    SF_ENDIAN_BIG = 0x20000000,
    //! Force CPU endian-ness
    SF_ENDIAN_CPU = 0x30000000,
    //! Major format submask
    SF_FORMAT_SUBMASK = 0x0000FFFF,
    //! Minor format submask
    SF_FORMAT_TYPEMASK = 0x0FFF0000,
    //! Endianness submask
    SF_FORMAT_ENDMASK = 0xF0000000
} SF_FORMAT;

/** Defines command constants for sf_command()
 *
 * @ingroup command
 */
typedef enum SF_COMMAND
{
    /** Gets library version string
     *
     * @param[in] sndfile Not used
     * @param[out] data A pointer to a char buffer
     * @param[in] datasize The size of the buffer
     *
     * @return Length of string in characters
     */
    SFC_GET_LIB_VERSION = 0x1000,

    /** Gets the internal per-file operation log.
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[out] data A pointer to a char buffer
     * @param[in] datasize The size of the buffer
     *
     * This log buffer can often contain a good reason for why libsndfile failed
     * to open a particular file.
     *
     * @return Length of string in characters
     */
    SFC_GET_LOG_INFO = 0x1001,

    /** Gets ::SF_INFO of given sound file
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[out] data A pointer to a ::SF_INFO struct
     * @param[in] datasize @c sizeof(SF_INFO)
     *
     * @return ::SF_ERR_NO_ERROR on succes, error code otherwise
     */
    SFC_GET_CURRENT_SF_INFO = 0x1002,

    /** Gets the current double normalization mode
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize Not used
     *
     * @return ::SF_TRUE if double normalization if enabled, ::SF_FALSE
     * otherwise.
     */
    SFC_GET_NORM_DOUBLE = 0x1010,

    /** Gets the current float normalization mode
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize Not used
     *
     * @return ::SF_TRUE if float normalization if enabled, ::SF_FALSE
     * otherwise.
     */
    SFC_GET_NORM_FLOAT = 0x1011,

    /** Sets the current double normalization mode
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize ::SF_TRUE or ::SF_FALSE
     *
     * This parameter is ::SF_FALSE by default.
     *
     * Double normalization changes behavior on sf_read_double(),
     * sf_readf_double(), sf_write_double() and sf_writef_double() functions.
     *
     * For read operations setting normalisation to ::SF_TRUE means that the
     * data from all subsequent reads will be be normalised to the range
     * @c [-1.0, 1.0].
     *
     * For write operations, setting normalisation to ::SF_TRUE means than all
     * data supplied to the double write functions should be in the range
     * @c [-1.0, 1.0] and will be scaled for the file format as necessary.
     *
     * For both cases, setting normalisation to #SF_FALSE means that no scaling
     * will take place.
     *
     * @return ::SF_TRUE if double normalization if enabled, ::SF_FALSE
     * otherwise.
     */
    SFC_SET_NORM_DOUBLE = 0x1012,

    /** Sets the current float normalization mode
     * @param[in] sndfile A valid SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize ::SF_TRUE or ::SF_FALSE
     *
     * This parameter is ::SF_FALSE by default.
     *
     * Float normalization changes behavior on sf_read_float(), sf_readf_float(),
     * sf_write_float() and sf_writef_float() functions.
     *
     * For read operations setting normalisation to #SF_TRUE means that the
     * data from all subsequent reads will be be normalised to the range
     * @c [-1.0, 1.0].
     *
     * For write operations, setting normalisation to #SF_TRUE means than all
     * data supplied to the double write functions should be in the range
     * @c [-1.0, 1.0] and will be scaled for the file format as necessary.
     *
     * For both cases, setting normalisation to #SF_FALSE means that no scaling
     * will take place.
     *
     * @return ::SF_TRUE if float normalization if enabled, ::SF_FALSE
     * otherwise.
     */
    SFC_SET_NORM_FLOAT = 0x1013,

    /** Sets/clears the scale factor when integer (short/int) data is read from
     * a file containing floating point data.
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize ::SF_TRUE or ::SF_FALSE
     *
     * @return The previous @c SFC_SET_SCALE_FLOAT_INT_READ setting for this
     * file.
     */
    SFC_SET_SCALE_FLOAT_INT_READ = 0x1014,

    /** Sets/clears the scale factor when integer (short/int) data is written to
     * a file as floating point data.
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize ::SF_TRUE or ::SF_FALSE
     *
     * @return The previous ::SFC_SET_SCALE_INT_FLOAT_WRITE setting for this
     * file.
     */
    SFC_SET_SCALE_INT_FLOAT_WRITE = 0x1015,

    /** Retrieves the number of simple formats supported by libsndfile.
     *
     * @param[in] sndfile Not used
     * @param[in] data A pointer to an int variable
     * @param[in] datasize sizeof(int)
     *
     * @return ::SF_ERR_NO_ERROR.
     */
    SFC_GET_SIMPLE_FORMAT_COUNT = 0x1020,

    /** Retrieves information about a simple format.
     *
     * @param[in] sndfile Not used
     * @param[in] data A pointer to an  ::SF_FORMAT_INFO struct
     * @param[in] datasize @c sizeof(SF_FORMAT_INFO)
     *
     * ::SF_FORMAT_INFO::format field should be set to the format number,
     * retrieved by ::SFC_GET_SIMPLE_FORMAT_COUNT command.
     *
     * The value of the format field of the SF_FORMAT_INFO struct will be a
     * value which can be placed in the ::SF_INFO struct::format field when a
     * file is to be opened for write.
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_GET_SIMPLE_FORMAT = 0x1021,

    /** Retrieves information about a major or subtype format
     *
     * @param[in] sndfile Not used
     * @param[in] data A pointer to an  ::SF_FORMAT_INFO struct
     * @param[in] datasize @c sizeof(SF_FORMAT_INFO)
     *
     * When sf_command() is called with #SFC_GET_FORMAT_INFO, the @c format
     * field is examined and if (@c format & #SF_FORMAT_TYPEMASK) is a valid
     * format then the struct is filled in with information about the given
     * major type.
     *
     * If (@c format & #SF_FORMAT_TYPEMASK) is #SF_FALSE and (@c format &
     * #SF_FORMAT_SUBMASK) is a valid subtype format then the struct is filled
     * in with information about the given subtype.
     *
     * \code{.c}
     *  SF_FORMAT_INFO format_info;
     *
     *  format_info.format = SF_FORMAT_WAV;
     *  sf_command(sndfile, SFC_GET_FORMAT_INFO, &format_info, sizeof (format_info));
     *  printf("%08x  %s %s\n", format_info.format, format_info.name, format_info.extension);
     *
     *  format_info.format = SF_FORMAT_ULAW;
     *  sf_command (sndfile, SFC_GET_FORMAT_INFO, &format_info, sizeof (format_info));
     *  printf ("%08x  %s\n", format_info.format, format_info.name);
     * \endcode
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_GET_FORMAT_INFO = 0x1028,

    /** Gets the number of major formats
     *
     * @param[in] sndfile Not used
     * @param[in] data A pointer to an @c int
     * @param[in] datasize @c sizeof(int)
     *
     * @return #SF_ERR_NO_ERROR.
     */
    SFC_GET_FORMAT_MAJOR_COUNT = 0x1030,

    /** Gets information about a major format type
     *
     * @param[in] sndfile Not used
     * @param[in] data A pointer to an ::SF_FORMAT_INFO struct
     * @param[in] datasize @c sizeof(SF_FORMAT_INFO)
     *
     * The value of the @c format field will be one of the major format
     * identifiers such as #SF_FORMAT_WAV or #SF_FORMAT_AIFF.
     *
     * The name field will contain a @c char* pointer to the name of the string,
     * eg. "WAV (Microsoft)".
     *
     * The extension field will contain the most commonly used file extension
     * for that file type.
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_GET_FORMAT_MAJOR = 0x1031,

    /** Gets the number of subformats
     *
     * @param[in] sndfile Not used
     * @param[in] data A pointer to an @c int
     * @param[in] datasize @c sizeof(int)
     *
     * @return #SF_ERR_NO_ERROR.
     */
    SFC_GET_FORMAT_SUBTYPE_COUNT = 0x1032,

    /** Gets information about a subformat
     *
     * @param[in] sndfile Not used
     * @param[out] data A pointer to an ::SF_FORMAT_INFO struct
     * @param[in] datasize @c sizeof(SF_FORMAT_INFO)
     *
     * This command enumerates the subtypes (this function does not translate a
     * subtype into a string describing that subtype). A typical use case might
     * be retrieving a string description of all subtypes so that a dialog box
     * can be filled in.
     *
     * On return the value of the @c format field will be one of the major
     * format identifiers such as #SF_FORMAT_WAV or #SF_FORMAT_AIFF.
     *
     * The @c name field will contain a @c char* pointer to the name of the
     * string; for instance "WAV (Microsoft)" or "AIFF (Apple/SGI)".
     *
     * The @c extension field will be a @c NULL pointer.
     *
     * @return #SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_GET_FORMAT_SUBTYPE = 0x1033,

    /** Calculates the measured maximum signal value
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data a pointer to a @c double
     * @param[in] datasize @c sizeof(double)
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_CALC_SIGNAL_MAX = 0x1040,

    /** Calculates the measured normalised maximum signal value
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data a pointer to a @c double
     * @param[in] datasize @c sizeof(double)
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_CALC_NORM_SIGNAL_MAX = 0x1041,

    /** Calculates the peak value (ie a single number) for each channel
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data a pointer to a @c double array
     * @param[in] datasize @c sizeof(double) * number of channels
     *
     * @return #SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_CALC_MAX_ALL_CHANNELS = 0x1042,

    /** Calculates the normalised peak for each channel
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data a pointer to a @c double array
     * @param[in] datasize @c sizeof(double) * number of channels
     *
     * @return #SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_CALC_NORM_MAX_ALL_CHANNELS = 0x1043,

    /** Gets the peak value for the file as stored in the file header
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data a pointer to a @c double
     * @param[in] datasize @c sizeof(double)
     *
     * @return #SF_TRUE if the file header contained the peak value, #SF_FALSE
     * otherwise.
     */
    SFC_GET_SIGNAL_MAX = 0x1044,

    /** Gets the peak value for each channel as stored in the file header
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data a pointer to a @c double array
     * @param[in] datasize @c sizeof(double) * number of channels
     *
     * @return #SF_TRUE if the file header contains per channel peak values for
     * the file, #SF_FALSE otherwise.
     */
    SFC_GET_MAX_ALL_CHANNELS = 0x1045,

    /** Switches the code for adding the PEAK chunk to WAV and AIFF files on or
     * off
     *
     * @param[in] sndfile Not used
     * @param[out] data A pointer to an ::SF_FORMAT_INFO struct
     * @param[in] datasize @c sizeof(SF_FORMAT_INFO)
     *
     * By default, WAV and AIFF files which contain floating point data
     * (subtype #SF_FORMAT_FLOAT or #SF_FORMAT_DOUBLE) have a PEAK chunk.
     * By using this command, the addition of a PEAK chunk can be turned on or
     * off.
     *
     * @warning This call must be made before any data is written to the file.
     *
     * @retval ::SF_TRUE if the peak chunk will be written after this call.
     * @retval ::SF_FALSE if the peak chunk will not be written after this call.
     */
    SFC_SET_ADD_PEAK_CHUNK = 0x1050,

    /** Not used
     *
     * @return #SF_ERR_NO_ERROR.
     */
    SFC_SET_ADD_HEADER_PAD_CHUNK = 0x1051,

    /** Forces update of the file header to reflect the data written so far when
     * a file is open for write
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize Not used
     *
     * The header of an audio file is normally written by libsndfile when the
     * file is closed using sf_close().
     *
     * There are however situations where large files are being generated and it
     * would be nice to have valid data in the header before the file is
     * complete. Using this command will update the file header to reflect the
     * amount of data written to the file so far. Other programs opening the
     * file for read (before any more data is written) will then read a valid
     * sound file header.
     *
     * @return ::SF_ERR_NO_ERROR.
     */
    SFC_UPDATE_HEADER_NOW = 0x1060,

    /** Used when a file is open for write, this command will cause the file
     * header to be updated after each write to the file.
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize #SF_TRUE or #SF_FALSE
     *
     * Similar to ::SFC_UPDATE_HEADER_NOW but updates the header at the end of
     * every call to the @c sf_write* functions.
     *
     * @return ::SF_TRUE if auto update header is now on; ::SF_FALSE otherwise.
     */
    SFC_SET_UPDATE_HEADER_AUTO = 0x1061,

    /** Truncates a file open for write or for read/write
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data A pointer to a variable of ::sf_count_t type
     * @param[in] datasize sizeof(sf_count_t)
     *
     * Truncate the file to the number of frames specified by the ::sf_count_t
     * pointed to by data. After this command, both the read and the write
     * pointer will be at the new end of the file. This command will fail
     * (returning non-zero) if the requested truncate position is beyond the end
     * of the file.
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_FILE_TRUNCATE = 0x1080,

    /** Changes the data start offset for files opened up as ::SF_FORMAT_RAW
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data A pointer to a variable of ::sf_count_t type
     * @param[in] datasize sizeof(sf_count_t)
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_SET_RAW_START_OFFSET = 0x1090,

    /** Controls dithering on write
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data A pointer to a ::SF_DITHER_INFO struct
     * @param[in] datasize sizeof(SF_DITHER_INFO)
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_SET_DITHER_ON_WRITE = 0x10A0,

    /** Controls dithering on read
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data A pointer to a ::SF_DITHER_INFO struct
     * @param[in] datasize sizeof(SF_DITHER_INFO)
     *
     * @return ::SF_ERR_NO_ERROR on succes, negative error code otherwise.
     */
    SFC_SET_DITHER_ON_READ = 0x10A1,

    /** Not used
     *
     * @return ::SF_ERR_NO_ERROR.
     */
    SFC_GET_DITHER_INFO_COUNT = 0x10A2,

    /** Not used
     *
     * @return ::SF_ERR_NO_ERROR.
     */
    SFC_GET_DITHER_INFO = 0x10A3,

    /** Turns on/off automatic clipping when doing floating point to integer
     * conversion
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize ::SF_TRUE or ::SF_FALSE
     *
     * @return #SF_TRUE is clipping is now on or #SF_FALSE otherwise.
     */
    SFC_SET_CLIPPING = 0x10C0,

    /** Retrieves current clipping setting
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize Not used
     *
     * @return ::SF_TRUE is clipping is on or ::SF_FALSE otherwise.
     */
    SFC_GET_CLIPPING = 0x10C1,

    /** Gets the cue marker count
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data A pointer to a variable of @c uint32_t type
     * @param[in] datasize sizeof(uint32_t)
     *
     * @return ::SF_TRUE if the file header contains cue marker information for
     * the file, ::SF_FALSE otherwise.
     */
    SFC_GET_CUE_COUNT = 0x10CD,

    /** Gets instrument information
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data A pointer to a ::SF_INSTRUMENT struct
     * @param[in] datasize sizeof(SF_INSTRUMENT)
     *
     * This command retrieves instrument information from file including MIDI
     * base note, keyboard mapping and looping informations(start/stop and
     * mode).
     *
     * @return ::SF_TRUE if the file header contains instrument information for
     * the file, ::SF_FALSE otherwise.
     */
    SFC_GET_INSTRUMENT = 0x10D0,
    /** Sets instrument information
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data A pointer to a ::SF_INSTRUMENT struct
     * @param[in] datasize sizeof(SF_INSTRUMENT)
     *
     * @return ::SF_TRUE if the file header contains instrument information for
     * the file, ::SF_FALSE otherwise.
     */
    SFC_SET_INSTRUMENT = 0x10D1,

    /** Sets loop information
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data A pointer to a ::SF_LOOP struct
     * @param[in] datasize sizeof(SF_LOOP)
     *
     * @return ::SF_TRUE if the file header contains loop information for the
     * file, ::SF_FALSE otherwise.
     */
    SFC_GET_LOOP_INFO = 0x10E0,

    /** Gets the channel mapping info
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data A pointer to pointer to an int
     * @param[in] datasize datasize sizeof (double) * number_of_channels
     *
     * When succeeded ::SFC_GET_CHANNEL_MAP_INFO fills data array with channel
     * mapping values.
     *
     * @return ::SF_TRUE on success, ::SF_FALSE otherwise.
     */
    SFC_GET_CHANNEL_MAP_INFO = 0x1100,
    /** Sets the channel mapping info
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data A pointer to pointer to an int
     * @param[in] datasize sizeof (double) * number_of_channels
     *
     * @return ::SF_TRUE on success, ::SF_FALSE otherwise.
     */
    SFC_SET_CHANNEL_MAP_INFO = 0x1101,

    /** Determines if raw data read using sf_read_raw() needs to be end swapped
     * on the host CPU.
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize Not used
     *
     * @return ::SF_TRUE on success, ::SF_FALSE otherwise.
     */
    SFC_RAW_DATA_NEEDS_ENDSWAP = 0x1110,

    // Support for Wavex Ambisonics Format

    /** Sets the GUID of a new WAVEX file to indicate an Ambisonics format.
     *
     * This command turns on (::SF_AMBISONIC_B_FORMAT) or off
     * (::SF_AMBISONIC_NONE) encoding.
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize One of ::SF_AMBISONIC_MODE values
     *
     * @note Ambisonics feature currently is supported with ::SF_FORMAT_WAVEX
     * only.
     *
     * @return The ::SF_AMBISONIC_MODE value that has just been set or zero if
     * the file format does not support ambisonic encoding.
     */
    SFC_WAVEX_SET_AMBISONIC = 0x1200,
    /** Tests if the current file has the GUID of a WAVEX file for any of the
     * Ambisonic formats.
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize Not used
     *
     * \note Ambisonics feature currently is supported with #SF_FORMAT_WAVEX
     * only.
     * @return One of ::SF_AMBISONIC_MODE values if Ambisonics is supported,
     * @c 0 otherwise.
     *
     * \sa ::SFC_WAVEX_SET_AMBISONIC
     * \sa ::SF_AMBISONIC_MODE
     *
     */
    SFC_WAVEX_GET_AMBISONIC = 0x1201,

    /** Controls auto downgrade from RF64 to WAV
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data Not used
     * @param[in] datasize ::SF_TRUE or ::SF_FALSE
     *
     * The EBU recomendation is that when writing RF64 files and the resulting
     * file is less than 4Gig in size, it should be downgraded to a WAV file
     * (WAV files have a maximum size of 4Gig). libsndfile doesn't follow the
     * EBU recommendations exactly, , mainly because the test suite needs to be
     * able test reading/writing RF64 files without having to generate files
     * larger than 4 gigabytes.
     *
     * @note This command should be issued before the first bit of audio data
     * has been written to the file. Calling this command after audio data has
     * been written will return the current value of this setting, but will not
     * allow it to be changed.   
     *
     * @return ::SF_TRUE if mode is set, ::SF_FALSE otherwise.
     */
    SFC_RF64_AUTO_DOWNGRADE = 0x1210,

    /** Sets the Variable Bit Rate encoding quality
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data A pointer to a double value
     * @param[in] datasize sizeof(double)
     *
     * The encoding quality value should be between 0.0 (lowest quality) and 1.0
     * (highest quality). Currenly this command is only implemented for FLAC and
     * Ogg/Vorbis files. It has no effect on un-compressed file formats.
     *
     * @return ::SF_TRUE if mode is set, ::SF_FALSE otherwise.
     */
    SFC_SET_VBR_ENCODING_QUALITY = 0x1300,
    /** Sets compression level
     *
     * @param[in] sndfile A valid ::SNDFILE* pointer
     * @param[in] data A pointer to a double value
     * @param[in] datasize sizeof(double)
     *
     * The compression level should be between 0.0 (minimum compression level)
     * and 1.0 (highest compression level). Currenly this command is only
     * implemented for FLAC and Ogg/Vorbis files. It has no effect on 
     * un-compressed file formats.
     *
     * @return ::SF_TRUE if mode is set, ::SF_FALSE otherwise.
     */
    SFC_SET_COMPRESSION_LEVEL = 0x1301,

    /** Internal, do not use
     */
    SFC_TEST_IEEE_FLOAT_REPLACE = 0x6001,

    /** Gets cue marker information from file
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[out] data A pointer to an array of ::SF_CUE_POINT structs.
     * @param[in] datasize Count of ::SF_CUE_POINT structs in array
     *
     * @return Number of ::SF_CUE_POINT actually retrieved.
     */
    SFC_GET_CUE_POINTS = 0x1388,
    /** Sets cue marker information for the file
     *
     * @param[in] sndfile a valid ::SNDFILE* pointer
     * @param[in] data A pointer to an array of ::SF_CUE_POINT structs
     * @param[in] datasize Count of ::SF_CUE_POINT structs in array
     *
     * @return ::SF_TRUE on success, ::SF_FALSE otherwise.
     */
    SFC_SET_CUE_POINTS = 0x1389,
} SF_COMMAND;

/**
 * \ingroup tagging
 */

/** Defines field type constants
 */
typedef enum SF_STR_FIELD
{
    //! Title
    SF_STR_TITLE = 0x01,
    //! Copyright
    SF_STR_COPYRIGHT = 0x02,
    //! Software
    SF_STR_SOFTWARE = 0x03,
    //! Artist
    SF_STR_ARTIST = 0x04,
    //! Comment
    SF_STR_COMMENT = 0x05,
    //! Date
    SF_STR_DATE = 0x06,
    //! Album
    SF_STR_ALBUM = 0x07,
    //! License
    SF_STR_LICENSE = 0x08,
    //! Track number
    SF_STR_TRACKNUMBER = 0x09,
    //! Genre
    SF_STR_GENRE = 0x10
} SF_STR_FIELD;

/** Defines start index of of tags fields
 */
#define SF_STR_FIRST SF_STR_TITLE
/** Defines last index of of tags fields
 */
#define SF_STR_LAST SF_STR_GENRE

/** \brief Defines boolean constants
 *
 */
typedef enum SF_BOOL
{
    //! False
    SF_FALSE = 0,
    //! True
    SF_TRUE = 1,
} SF_BOOL;

/** \brief Defines the kind of file access
 *
 * \ingroup file-base
 */
typedef enum SF_FILEMODE
{
    //! Read mode
    SFM_READ = 0x10,
    //! Write mode
    SFM_WRITE = 0x20,
    //! Read/write mode
    SFM_RDWR = 0x30,
} SF_FILEMODE;

/** Defines Ambisonics format constants
 *
 * @sa ::SFC_WAVEX_GET_AMBISONIC, ::SFC_WAVEX_GET_AMBISONIC
 */
typedef enum SF_AMBISONIC_MODE
{
    //! None
    SF_AMBISONIC_NONE = 0x40,
    //! B-format
    SF_AMBISONIC_B_FORMAT = 0x41
} SF_AMBISONIC_MODE;

/** @brief Defines public error values
 *
 * These are guaranteed to remain unchanged for the duration of the library
 * major version number.
 *
 * There are also a large number of private error numbers which are internal to
 * the library which can change at any time.
 *
 * @ingroup error
 */
typedef enum SF_ERR
{
    //! No error (success)
    SF_ERR_NO_ERROR = 0,
    //! Format not recognised
    SF_ERR_UNRECOGNISED_FORMAT = 1,
    //! System error
    SF_ERR_SYSTEM = 2,
    //! Supported file format but file is malformed
    SF_ERR_MALFORMED_FILE = 3,
    //! Supported file format but unsupported encoding
    SF_ERR_UNSUPPORTED_ENCODING = 4
} SF_ERR;

/**
 * Defines channel map constants
 */
typedef enum SF_CHANNEL_MAP
{
    SF_CHANNEL_MAP_INVALID = 0,
    SF_CHANNEL_MAP_MONO = 1,
    SF_CHANNEL_MAP_LEFT,
    SF_CHANNEL_MAP_RIGHT,
    SF_CHANNEL_MAP_CENTER,
    SF_CHANNEL_MAP_FRONT_LEFT,
    SF_CHANNEL_MAP_FRONT_RIGHT,
    SF_CHANNEL_MAP_FRONT_CENTER,
    SF_CHANNEL_MAP_REAR_CENTER,
    SF_CHANNEL_MAP_REAR_LEFT,
    SF_CHANNEL_MAP_REAR_RIGHT,
    SF_CHANNEL_MAP_LFE,
    SF_CHANNEL_MAP_FRONT_LEFT_OF_CENTER,
    SF_CHANNEL_MAP_FRONT_RIGHT_OF_CENTER,
    SF_CHANNEL_MAP_SIDE_LEFT,
    SF_CHANNEL_MAP_SIDE_RIGHT,
    SF_CHANNEL_MAP_TOP_CENTER,
    SF_CHANNEL_MAP_TOP_FRONT_LEFT,
    SF_CHANNEL_MAP_TOP_FRONT_RIGHT,
    SF_CHANNEL_MAP_TOP_FRONT_CENTER,
    SF_CHANNEL_MAP_TOP_REAR_LEFT,
    SF_CHANNEL_MAP_TOP_REAR_RIGHT,
    SF_CHANNEL_MAP_TOP_REAR_CENTER,

    SF_CHANNEL_MAP_AMBISONIC_B_W,
    SF_CHANNEL_MAP_AMBISONIC_B_X,
    SF_CHANNEL_MAP_AMBISONIC_B_Y,
    SF_CHANNEL_MAP_AMBISONIC_B_Z,

    SF_CHANNEL_MAP_MAX
} SF_CHANNEL_MAP;

/** Represents opened sound file
 *
 * @ingroup file-base
 *
 * Pointer to @c ::SNDFILE is returned by set of file open finctions, including
 * sf_open(), sf_open_fd() and sf_open_virtual(). Most of API functions take
 * @c SNDFILE pointer as required parameter.
 *
 * Make no assumptions about @c SNDFILE internal structure, it is subject to
 * change in future releases.
 *
 * Any created @c SNDFILE should be destroyed by sf_close() function to avoid
 * memory leaks.
 */
typedef struct SF_PRIVATE SNDFILE;

/** Represents file offset type
 *
 * This type is always 64-bit unsigned integer.
 */
typedef int64_t sf_count_t;

/** Defines maximum value of sf_count_t
 */
#define SF_COUNT_MAX INT64_MAX

/** Contains information about format of sound file
 */
typedef struct SF_INFO
{
    //! The count of frames
    sf_count_t frames;
    //! The sampling rate in Hz
    int samplerate;
    //! The count of channels
    int channels;
    //! Combination of major and minor format flags. See ::SF_FORMAT for details.
    int format;
    //! Unused.
    int sections;
    //! Indicates that sound file is seekable if set to non-zero value
    int seekable;
} SF_INFO;

/** Contains information about the sound file format
 *
 * Using this interface will allow applications to support new file formats
 * and encoding types when libsndfile is upgraded, without requiring
 * re-compilation of the application.
 *
 * @ingroup command
 *
 */
typedef struct SF_FORMAT_INFO
{
    //! Sound file format characterictics, see ::SF_FORMAT for details
    int format;
    //! Format description
    const char *name;
    //! Format file extension
    const char *extension;
} SF_FORMAT_INFO;

/** Defines dither type constants
 */
typedef enum SF_DITHER_TYPE
{
    //! Default level
    SFD_DEFAULT_LEVEL = 0,
    //! Custom level
    SFD_CUSTOM_LEVEL = 0x40000000,

    //! No dither
    SFD_NO_DITHER = 500,
    //! White
    SFD_WHITE = 501,
    //! Triangular probability density function Dither
    SFD_TRIANGULAR_PDF = 502
} SF_DITHER_TYPE;

/** Contains information about dithering
 */
typedef struct SF_DITHER_INFO
{
    //! Dither type, see ::SF_DITHER_TYPE for details
    int type;
    //! Dither level
    double level;
    //! Dither name
    const char *name;
} SF_DITHER_INFO;

/** Contains CUE marker information
 */
typedef struct SF_CUE_POINT
{
    int32_t indx;
    uint32_t position;
    int32_t fcc_chunk;
    int32_t chunk_start;
    int32_t block_start;
    uint32_t sample_offset;
    char name[256];
} SF_CUE_POINT;

/** Defines loop mode constants
 *
 * @sa ::SF_INSTRUMENT
 */
typedef enum SF_LOOP_MODE
{
    //! None
    SF_LOOP_NONE = 800,
    //! Forward
    SF_LOOP_FORWARD,
    //! Backward
    SF_LOOP_BACKWARD,
    //! Alternating
    SF_LOOP_ALTERNATING
} SF_LOOP_MODE;

/** Contains instrument information
 */
typedef struct SF_INSTRUMENT
{
    int gain;
    char basenote, detune;
    char velocity_lo, velocity_hi;
    char key_lo, key_hi;
    int loop_count;

    struct
    {
        int mode;
        uint32_t start;
        uint32_t end;
        uint32_t count;
    } loops[16]; // make variable in a sensible way
} SF_INSTRUMENT;

/** Defines loop information
 */
typedef struct SF_LOOP_INFO
{
    short time_sig_num; // any positive integer    > 0
    short time_sig_den; // any positive power of 2 > 0
    int loop_mode; // see SF_LOOP enum

    /* 
     * this is NOT the amount of quarter notes !!!
     * a full bar of 4/4 is 4 beats
     * a full bar of 7/8 is 7 beats
     */
    int num_beats;

    /*
     * suggestion, as it can be calculated using other fields:
     * file's length, file's sampleRate and our time_sig_den
     * -> bpms are always the amount of _quarter notes_ per minute
     */
    float bpm;

    int root_key; // MIDI note, or -1 for None
    int future[6];
} SF_LOOP_INFO;

/** Type of user defined function that increases reference count
 *
 * @param[in] user_data User defined value.
 *
 * @return Reference count.
 */
typedef unsigned long (*sf_ref_callback)(void *user_data);
/** Type of user defined function that decreases reference count
 *
 * @param[in] user_data User defined value.
 */
typedef void (*sf_unref_callback)(void *user_data);

/** Type of user defined function that returns file size
 *
 * @ingroup file-virt
 *
 * @param[in] user_data User defined value.
 *
 * @return Size of file in bytes on success, @c -1 otherwise.
 */
typedef sf_count_t (*sf_vio_get_filelen)(void *user_data);

/** Type of user defined seek function
 *
 * @ingroup file-virt
 *
 * @param[in] offset Offset to seek.
 * @param[in] whence Seek mode, one of ::SF_SEEK_MODE values.
 * @param[in] user_data User defined value.
 *
 * @c offset must be non-negative if @c whence is ::SF_SEEK_SET or ::SF_SEEK_CUR
 * or non-positive if @c whence is ::SF_SEEK_END.
 *
 * @return Non-negative offset from start on the file on success, @c -1
 * otherwise.
 */
typedef sf_count_t (*sf_vio_seek)(sf_count_t offset, int whence, void *user_data);
/** Type of user defined read function
 *
 * @ingroup file-virt
 *
 * @param[in] ptr Pointer to an allocated block memory.
 * @param[in] count Size in bytes of /c ptr.
 * @param[in] user_data User defined value.
 *
 * @return Count of bytes actually read or zero otherwise.
 */
typedef sf_count_t (*sf_vio_read)(void *ptr, sf_count_t count, void *user_data);

/** Type of user defined write function
 *
 * @ingroup file-virt
 *
 * @param[in] ptr Pointer to an allocated block memory.
 * @param[in] count Size in bytes of @c ptr.
 * @param[in] user_data User defined value.
 *
 * @return Count of bytes actually written or zero otherwise.
 */
typedef sf_count_t (*sf_vio_write)(const void *ptr, sf_count_t count, void *user_data);
/** Type of user defined function that returns current position in file
*
* @ingroup file-base
*
* @param[in] user_data User defined value.
*
* @return File offset on success, @c -1 otherwise.
*/
typedef sf_count_t (*sf_vio_tell)(void *user_data);
/** Type of user defined function that forces the writing of data to disk.
*
* @ingroup file - base
*
* @param[in] user_data User defined value.
*
* @return File offset on success, @c - 1 otherwise.
*/
typedef void (*sf_vio_flush)(void *user_data);
/** Type of user defined function truncates stream size.
*
* @ingroup file - base
*
* @param[in] user_data User defined value.
* @param[in] len New size of stream.
*
* @return New file size on success, @c - 1 otherwise.
*/
typedef int (*sf_vio_set_filelen)(void *user_data, sf_count_t len);

/** Defines virtual file I/O context
 *
 * @ingroup file-virt
 *
 * @c SF_VIRTUAL_IO contains pointers to user-defined functions used in Virtual
 * I/O.
 *
 * @c get_filelen, @c seek and @c tell fields must always be set; @c read field
 * is required for read and read/write mode; @c write field should be set for
 * write mode.
 *
 */
typedef struct SF_VIRTUAL_IO
{
    //! Pointer to a used defined function that returns file size
    sf_vio_get_filelen get_filelen;
    //! Pointer to a user defined seek function
    sf_vio_seek seek;
    //! Pointer to a user defined read function
    sf_vio_read read;
    //! Pointer to a user defined write function
    sf_vio_write write;
    //! Pointer to a user defined function that returns file position
    sf_vio_tell tell;
    //! Pointer to a user defined function that forces the writing of data to disk
    sf_vio_flush flush;
	//! Pointer to a user defined function that truncates stream
	sf_vio_set_filelen set_filelen;
	//! Pointer to a user defined function that increases reference count.
	sf_ref_callback ref;
	//! Pointer to a user defined function that decreases reference count.
	sf_unref_callback unref;
} SF_VIRTUAL_IO;

#include "sndfile2k_export.h"

/** @defgroup file-base Basic sound file manipulation
*
* API for opening, closing and flushing sound files.
*
*  @{
*/

/** Opens the specified sound file using path
 *
 * @param[in] path Path to the file
 * @param[in] mode File open mode
 * @param[in,out] sfinfo Format information
 *
 * The @p path is byte encoded, but may be utf-8 on Linux, while on
 * Mac OS X it will use the filesystem character set. On Windows, there is
 * also a Windows specific sf_wchar_open() that takes a @c UTF16_BE encoded
 * filename.
 *
 * The @p emode specifies the kind of access that is requested for the file,
 * one of ::SF_FILEMODE values.
 *
 * When opening a file for read, the SF_INFO::format field should be set to 
 * zero before passing to sf_open(). The only exception to this is the case of
 * @c RAW files where the caller has to set the @p samplerate, @p channels and
 * @p format fields to valid values. All other fields of the structure are
 * filled in by the library.
 *
 * When opening a file for write, the caller must fill in structure members 
 * SF_INFO::samplerate, SF_INFO::channels, and SF_INFO::format.
 *
 * All calls to sf_open() should be matched with a call to sf_close().
 *
 * @return A valid pointer to a #SNDFILE object on success, @c NULL
 * otherwise.
 *
 * @sa sf_wchar_open(), sf_open_fd(), sf_open_virtual()
 * @sa sf_close()
 */
SNDFILE2K_EXPORT SNDFILE *sf_open(const char *path, SF_FILEMODE mode, SF_INFO *sfinfo);

/** Opens sound file using @c POSIX file descriptor
 *
 * @param[in] fd File descriptor
 * @param[in] mode File open mode
 * @param[in,out] sfinfo Format information
 * @param[in] close_desc File descriptor close mode
 *
 * sf_open_fd() is similar to sf_open(), but takes @c POSIX file descriptor
 * of already opened file instead of path.
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
 *
 * When sf_close() is called, the file descriptor is only closed if the
 * @c close_desc parameter was ::SF_TRUE when the sf_open_fd()
 * function was called.
 *
 * @note On Microsoft Windows, this function does not work if the application
 * and the sndfile2k DLL are linked to different versions of the Microsoft C runtime DLL.
 *
 * @return A valid pointer to a #SNDFILE object on success, @c NULL
 * otherwise.
 *
 * @deprecated This function and will be removed in next major release.
 * Use sf_open_virtual_ex() function instead.
 *
 * @sa sf_open(), sf_wchar_open(), sf_open_virtual()
 * @sa sf_close()
 */
SNDFILE2K_EXPORT SNDFILE *sf_open_fd(int fd, SF_FILEMODE mode, SF_INFO *sfinfo, int close_desc);

/** @defgroup file-virt Virtual I/O
 *
 * SndFile2K calls the callbacks provided by the ::SF_VIRTUAL_IO structure when
 * opening, reading and writing to the virtual file context.
 *
 *  @{
 */

/** Opens sound file using Virtual I/O context
 *
 * @param[in] sfvirtual Virtual I/O context
 * @param[in] mode File open mode
 * @param[in,out] sfinfo Format information
 * @param[in] user_data User data
 *
 * sf_open_fd() is similar to sf_open(), but takes Virtual I/O context
 * of already opened file instead of path.
 *
 * Care should be taken to ensure that the mode of the file represented by the
 * descriptor matches the @c mode argument.
 *
 * @return A valid pointer to a #SNDFILE object on success, @c NULL
 * otherwise.
 *
 * @deprecated This function is deprecated and will be removed in next major release.
 * Use sf_open_virtual_ex() instead.
 *
 * @sa sf_open(), sf_wchar_open(), sf_open_fd(), sf_open_virtual_ex()
 * @sa sf_close()
 */
SNDFILE2K_EXPORT SNDFILE *sf_open_virtual(SF_VIRTUAL_IO *sfvirtual, SF_FILEMODE mode, SF_INFO *sfinfo,
                                          void *user_data);

/** Opens sound file using Virtual I/O context
 *
 * @param[in] sfvirtual Virtual I/O context
 * @param[in] mode File open mode
 * @param[in,out] sfinfo Format information
 * @param[in] user_data User data
 *
 * sf_open_virtual_ex() is similar to sf_open(), but takes Virtual I/O context
 * of already opened file instead of path.
 *
 * sf_open_virtual_ex() uses new SF_VIRTUAL_IO members: SF_VIRTUAL_IO::flush,
 * SF_VIRTUAL_IO::set_filelen, SF_VIRTUAL_IO::ref and SF_VIRTUAL_IO::unref.
 * Anyway, all this callbaks are not required and
 * could be set to @c NULL.
 *
 * - SF_VIRTUAL_IO::flush is helpful in write mode, performing flashing cache data
 * to disk.
 * - SF_VIRTUAL_IO::set_filelen truncates file to desired size by the library.
 * - SF_VIRTUAL_IO::ref and SF_VIRTUAL_IO::unref could be implemented to control
 *   stream life time. When these members are implemented, library will call @c ref
 *   when file is opened, and call @c unref before closing file. You can pass as
 *   @p user_data pointer to struct with some counter member, your implementation
 *   of @c ref must increase this value, and @unref must decrease it and when it
 *	 reaches zero close stream and free resources. Now, when initial value is zero,
 *	 library will call @ref and set counter to @c 1, holding reference. In sf_close()
 *	 @c unref will be called, setting counter to @c 0 and calls yours cleanup code.
 *
 *
 * Care should be taken to ensure that the mode of the file represented by the
 * descriptor matches the @c mode argument.
 *
 * @return A valid pointer to a #SNDFILE object on success, @c NULL
 * otherwise.
 *
 * @sa sf_open(), sf_wchar_open(), sf_open_fd(), sf_open_virtual()
 * @sa sf_close()
 */
SNDFILE2K_EXPORT SNDFILE *sf_open_virtual_ex(SF_VIRTUAL_IO *sfvirtual, SF_FILEMODE mode, SF_INFO *sfinfo, void *user_data);

/** @}*/

/** @}*/

/** \defgroup error Error handling
 *
 *  @{
 */

/** Returns the current error code for the given ::SNDFILE
 *
 * @param[in] sndfile Pointer to a sound file state
 *
 * The error code may be one of ::SF_ERR values, or any one of many other
 * internal error values.
 * 
 * Applications should only test the return value against ::SF_ERR values as
 * the internal error values are subject to change at any time.
 *
 * For errors not in the above list, the function sf_error_number() can be
 * used to convert it to an error string.
 *
 * @return Error number.
 *
 * @sa sf_error_number()
 */
SNDFILE2K_EXPORT int sf_error(SNDFILE *sndfile);

/** Returns textual description of  the current error code for the given ::SNDFILE
 *
 * @param[in] sndfile Pointer to a sound file state
 *
 * @return Pointer to the NULL-terminated error description string.
 *
 * @sa sf_error()
 */
SNDFILE2K_EXPORT const char *sf_strerror(SNDFILE *sndfile);

/** Returns text description of error code
*
* @param[in] errnum Error code
*
* @return Pointer to the NULL-terminated error description string.
*
* @sa sf_error()
*/
SNDFILE2K_EXPORT const char *sf_error_number(int errnum);

/** Returns internal error code for the given ::SNDFILE
 *
 * @param[in] sndfile Pointer to a sound file state
 *
 * The error code may be one of ::SF_ERR values, or any one of many other
 * internal error values.
 *
 * @deprecated sf_perror() is deprecated and will be dropped from the library
 * at some later date.
 *
 * @return Error code.
 *
 * @\sa sf_error_str()
 */
SNDFILE2K_EXPORT int sf_perror(SNDFILE *sndfile);

/** Fills provided buffer with text description of error code for the given
 * ::SNDFILE
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] str Pointer to an allocated string buffer
 * @param[in] len Size in bytes of @c str
 *
 * The error code may be one of ::SF_ERR values, or any one of many other
 * internal error values.
 *
 * @deprecated sf_error_str() is deprecated and will be dropped from the
 * library at some later date.
 *
 * @return Zero on success or non-zero otherwise.
 *
 * @sa sf_perror()
 */
SNDFILE2K_EXPORT int sf_error_str(SNDFILE *sndfile, char *str, size_t len);

/** @}*/

/** API to retrieve information from or change aspects of the library behaviour.
 *
 * Examples include retrieving a string containing the library version or
 * changing the scaling applied to floating point sample data during read and
 * write. Most of these operations are performed on a per-file basis.
 *
 * @defgroup command Command interface
 *
 *  @{
 */

/** Gets or sets parameters of library or sound file.
 *
 * @param[in] sndfile Pointer to a sound file state or @c NULL
 * @param command Command ID see ::SF_COMMAND for details
 * @param data Command specific, could be @c NULL
 * @param datasize Size of @c data, could be @c 0
 *
 * @return Command-specific, see particular command description.
 */
SNDFILE2K_EXPORT int sf_command(SNDFILE *sndfile, int command, void *data, int datasize);

/** @}*/

/** Checks correctness of SF_INFO::format field
 *
 * To open sound file in write mode you need to set up ::SF_INFO struct, which
 * field SF_INFO::format is constructed as combination of major, minor format
 * and endiannes. To be sure that format is correct and will be accepted you
 * can use this function.
 *
 * @return ::SF_TRUE if format is valid, ::SF_FALSE otherwise.
 *
 * @sa ::SF_INFO
 * @sa ::SF_FORMAT
 */
SNDFILE2K_EXPORT int sf_format_check(const SF_INFO *info);

/** Defines file seek mode constants
 *
 * @ingroup file-base
 */
typedef enum SF_SEEK_MODE
{
    //! Seek from beginning
    SF_SEEK_SET = SEEK_SET,
    //! Seek from current position
    SF_SEEK_CUR = SEEK_CUR,
    //! Seek from end
    SF_SEEK_END = SEEK_END
} SF_SEEK_MODE;

/** Changes position of sound file
 *
 * @ingroup file-base
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] frames Count of frames
 * @param[in] whence Seek origin
 *
 * The file seek functions work much like lseek in @c unistd.h with the exception
 * that the non-audio data is ignored and the seek only moves within the audio
 * data section of the file. In addition, seeks are defined in number of
 * (multichannel) frames. Therefore, a seek in a stereo file from the current
 * position forward with an offset of 1 would skip forward by one sample of both
 * channels.
 *
 * Internally, library keeps track of the read and write locations using
 * separate read and write pointers. If a file has been opened with a mode of
 * ::SFM_RDWR, bitwise OR-ing the standard whence values above with either
 * ::SFM_READ or ::SFM_WRITE allows the read and write pointers to be modified
 * separately. If the @c SEEK_* values are used on their own, the read and write
 * pointers are both modified.
 *
 * @note The frames offset can be negative and in fact should be when
 * ::SF_SEEK_END is used for the @c whence parameter.
 *
 * @return Offset in (multichannel) frames from the start of the audio data or
 * @c -1 if an error occured (ie an attempt is made to seek beyond the start or
 * end of the file).
 */
SNDFILE2K_EXPORT sf_count_t sf_seek(SNDFILE *sndfile, sf_count_t frames, int whence);

/** @defgroup tagging Tags manipulation
 *
 * @brief API for retrieving and setting string data within sound files.
 *
 * @note Not all file types support this features.
 *
 *  @{
 */

/** Sets string field for given sound file
 *
 * @param[in] sndfile Pointer to a sound file state
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
SNDFILE2K_EXPORT int sf_set_string(SNDFILE *sndfile, int str_type, const char *str);

/** Gets string field from given sound file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] str_type Type of field to retrieve, one of ::SF_STR_FIELD values
 *
 * @return Zero-terminated string in utf-8 encoding on success or @c NULL
 * otherwise.
 */
SNDFILE2K_EXPORT const char *sf_get_string(SNDFILE *sndfile, int str_type);

/** @}*/

/** Returns the library version string
 *
 * @return NULL-terminated string with version of library (@c X.Y.Z format, with 
  @c -exp postfix if experimental features are enabled.)
 */
SNDFILE2K_EXPORT const char *sf_version_string(void);

/** Returns current byterate of the file
 *
 * @param[in] sndfile Pointer to a sound file state
 *
 * @note This feature could be unsupported for particular format.
 *
 * @return Byterate on success, @c -1 otherwise.
 */
SNDFILE2K_EXPORT int sf_current_byterate(SNDFILE *sndfile);

/** Read raw bytes from sound file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block memory.
 * @param[in] bytes Count of bytes to read
 *
 * @note This function is not for general use. No type conversion or
 * byte-swapping performed, data is returned as it is.
 *
 * @return Number of bytes actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_read_raw(SNDFILE *sndfile, void *ptr, sf_count_t bytes);

/** Writes raw bytes to sound file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] bytes Count of bytes to write
 *
 * @note This function is not for general use. No type conversion or
 * byte-swapping performed, data is written as it is.
 *
 * @return Number of bytes actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_write_raw(SNDFILE *sndfile, const void *ptr, sf_count_t bytes);

/** @defgroup read-write-frames Read/Write frames
 *
 * @brief API for reading and writing data in particular format as frames.
 *
 * This set of high-level functions read/write data in terms of frame and/or
 * performing type conversion and byte-swapping if necessary.
 *
 * One frame is one item (sample) for each channel. For example, to read @c 10
 * frames as integers (32-bit) from stereo (2 channles) sound file you need to
 * allocate buffer of @c 20 integers size and call sf_readf_int() function with
 * @c frames parameter set to @c 10.
 *
 * When file format data type differs from function type, e.g you read WAV file
 * with @c float data, data type conversion if automatically performed.
 *
 * When endianness of file data differs from host system endianness,
 * byte-swapping is performed and no additional manipulations are required.
 *
 *  @{
 */

/** Reads short (16-bit) frames from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] frames Count of frames to read
 *
 * @return Number of frames actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_readf_short(SNDFILE *sndfile, short *ptr, sf_count_t frames);
/** Writes short (16-bit) frames to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block memory.
 * @param[in] frames Count of frames to write
 *
 * @return Number of frames actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_writef_short(SNDFILE *sndfile, const short *ptr, sf_count_t frames);

/** Reads integer (32-bit) frames from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] frames Count of frames to read
 *
 * @return Number of frames actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_readf_int(SNDFILE *sndfile, int *ptr, sf_count_t frames);
/** Writes integer (32-bit) frames to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] frames Count of frames to write
 *
 * @return Number of frames actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_writef_int(SNDFILE *sndfile, const int *ptr, sf_count_t frames);

/** Reads float (32-bit) frames from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] frames Count of frames to read
 *
 * @return Number of frames actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_readf_float(SNDFILE *sndfile, float *ptr, sf_count_t frames);
/** Writes float (32-bit) frames to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] frames Count of frames to write
 *
 * @return Number of frames actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_writef_float(SNDFILE *sndfile, const float *ptr, sf_count_t frames);

/** Reads double (64-bit) frames from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] frames Count of frames to read
 *
 * @return Number of frames actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_readf_double(SNDFILE *sndfile, double *ptr, sf_count_t frames);
/** Writes double (64-bit) frames to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] frames Count of frames to write
 *
 * @return Number of frames actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_writef_double(SNDFILE *sndfile, const double *ptr,
                                             sf_count_t frames);

/** @}*/

/** @defgroup read-write-items Read/Write items
 *
 * @brief API for reading and writing data in particular format as items.
 *
 * This set of high-level functions read/write data in terms of sample and/or
 * performing type conversion and byte-swapping if necessary.
 *
 * One item is one sample for each channel. For example, to read @c 10
 * samples as integers (32-bit) from stereo (2 channles) sound file you need to
 * allocate buffer of @c 10 integers size and call sf_read_int() function with
 * @c items parameter set to @c 10.
 *
 * When file format data type differs from function type, e.g you read WAV file
 * with @c float data, data type conversion if automatically performed.
 *
 * When endianness of file data differs from host system endianness,
 * byte-swapping is performed and no additional manipulations are required.
 *
 *  @{
 */

/** Reads short (16-bit) samples from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to read
 *
 * @return Number of samples actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_read_short(SNDFILE *sndfile, short *ptr, sf_count_t items);
/** Writes short (16-bit) samples to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to write
 *
 * @return Number of samples actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_write_short(SNDFILE *sndfile, const short *ptr, sf_count_t items);

/** Reads integer (32-bit) samples from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to read
 *
 * @return Number of samples actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_read_int(SNDFILE *sndfile, int *ptr, sf_count_t items);
/** Writes integer (32-bit) samples to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to write
 *
 * @return Number of samples actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_write_int(SNDFILE *sndfile, const int *ptr, sf_count_t items);

/** Reads float (32-bit) samples from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to read
 *
 * @return Number of samples actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_read_float(SNDFILE *sndfile, float *ptr, sf_count_t items);
/** Writes float (32-bit) samples to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to write
 *
 * @return Number of samples actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_write_float(SNDFILE *sndfile, const float *ptr, sf_count_t items);

/** Reads double (64-bit) samples from file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[out] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to read
 *
 * @return Number of samples actually read.
 */
SNDFILE2K_EXPORT sf_count_t sf_read_double(SNDFILE *sndfile, double *ptr, sf_count_t items);
/** Writes double (64-bit) samples to file
 *
 * @param[in] sndfile Pointer to a sound file state
 * @param[in] ptr Pointer to an allocated block of memory.
 * @param[in] items Count of samples to write
 *
 * @return Number of samples actually written.
 */
SNDFILE2K_EXPORT sf_count_t sf_write_double(SNDFILE *sndfile, const double *ptr, sf_count_t items);
/** @}*/

/** Closes the ::SNDFILE object
 *
 * @param[in] sndfile Pointer to a sound file state
 *
 * @return Zero on success, error code otherwise.
 *
 * \sa sf_open(), sf_wchar_open(), sf_open_fd(), sf_open_virtual()
 */
SNDFILE2K_EXPORT int sf_close(SNDFILE *sndfile);

/** Forces writing of data to disk
 *
 * @ingroup file-base
 *
 * @param[in] sndfile Pointer to a sound file state
 *
 * If the file is opened #SFM_WRITE or #SFM_RDWR, call fsync() on the file to
 * force the writing of data to disk. If the file is opened ::SFM_READ no
 * action is taken.
 */
SNDFILE2K_EXPORT void sf_write_sync(SNDFILE *sndfile);

#if (defined(_WIN32) || defined(__CYGWIN__))

/** Opens file using unicode filename
 *
 * @ingroup file-base
 *
 * @param[in] wpath Path to the file
 * @param[in] mode File open mode
 * @param[in,out] sfinfo Format information
 *
 * sf_wchar_open() is similar to sf_open(), but takes @c wchar_t string as
 * path.
 *
 * @note This function is Windows-specific.
 *
 * @return Returns a valid pointer to a #SNDFILE object on success, @c NULL
 * otherwise.
 *
 * @sa sf_open(), sf_open_fd(), sf_open_virtual()
 * @sa sf_close()
 */
SNDFILE2K_EXPORT SNDFILE *sf_wchar_open(const wchar_t *wpath, SF_FILEMODE mode, SF_INFO *sfinfo);
#endif

/** @defgroup chunks Chunks API
 *
 * Getting and setting of chunks from within a sound file.
 *
 * These functions allow the getting and setting of chunks within a sound file
 * (for those formats which allow it).
 *
 * These functions fail safely. Specifically, they will not allow you to overwrite
 * existing chunks or add extra versions of format specific reserved chunks but
 * should allow you to retrieve any and all chunks (may not be implemented for
 * all chunks or all file formats).
 *
 *  @{
 */

/** Describes sound file chunk
 */
struct SF_CHUNK_INFO
{
    char id[64]; //! The chunk identifier
    unsigned id_size; //! The size of the chunk identifier
    unsigned datalen; //! The size of that data
    void *data; //! Pointer to the data
};

typedef struct SF_CHUNK_INFO SF_CHUNK_INFO;

/** Sets the specified chunk info
 *
 * @param[in] sndfile Pointer to a sound file state
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
SNDFILE2K_EXPORT int sf_set_chunk(SNDFILE *sndfile, const SF_CHUNK_INFO *chunk_info);

/** An opaque structure to an iterator over the all chunks of a given id
 */
typedef struct SF_CHUNK_ITERATOR SF_CHUNK_ITERATOR;

/** Gets an iterator for all chunks matching chunk_info.
 *
 * @param[in] sndfile Pointer to a sound file state
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
SNDFILE2K_EXPORT SF_CHUNK_ITERATOR *sf_get_chunk_iterator(SNDFILE *sndfile,
                                                          const SF_CHUNK_INFO *chunk_info);

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
SNDFILE2K_EXPORT SF_CHUNK_ITERATOR *sf_next_chunk_iterator(SF_CHUNK_ITERATOR *iterator);

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
SNDFILE2K_EXPORT int sf_get_chunk_size(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info);

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
SNDFILE2K_EXPORT int sf_get_chunk_data(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info);

/** @}*/

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* SNDFILE2K_H */
