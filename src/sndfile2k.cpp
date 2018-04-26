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

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdbool.h>
#include <math.h>

#include "sndfile2k/sndfile2k.hpp"
#include "sfendian.h"
#include "common.h"
#include "sndfile_error.h"
#include "ref_ptr.h"

#include <algorithm>
#include <memory>

#ifdef __APPLE__
/*
**	Detect if a compile for a universal binary is being attempted and barf if it is.
**	See the URL below for the rationale.
*/
#ifdef __BIG_ENDIAN__
#if (CPU_IS_LITTLE_ENDIAN == 1)
#error "Universal binary compile detected. See http://www.mega-nerd.com/libsndfile/FAQ.html#Q018"
#endif
#endif

#ifdef __LITTLE_ENDIAN__
#if (CPU_IS_BIG_ENDIAN == 1)
#error "Universal binary compile detected. See http://www.mega-nerd.com/libsndfile/FAQ.html#Q018"
#endif
#endif
#endif

typedef struct
{
    int error;
    const char *str;
} ErrorStruct;

static ErrorStruct SndfileErrors[] = {
    /* Public error values and their associated strings. */
    {SF_ERR_NO_ERROR, "No Error."},
    {SF_ERR_UNRECOGNISED_FORMAT, "Format not recognised."},
    {SF_ERR_SYSTEM, "System error." /* Often replaced. */},
    {SF_ERR_MALFORMED_FILE, "Supported file format but file is malformed."},
    {SF_ERR_UNSUPPORTED_ENCODING, "Supported file format but unsupported encoding."},

    /* Private error values and their associated strings. */
    {SFE_ZERO_MAJOR_FORMAT, "Error : major format is 0."},
    {SFE_ZERO_MINOR_FORMAT, "Error : minor format is 0."},
    {SFE_BAD_FILE, "File does not exist or is not a regular file (possibly a pipe?)."},
    {SFE_BAD_FILE_READ, "File exists but no data could be read."},
    {SFE_OPEN_FAILED, "Could not open file."},
    {SFE_BAD_SNDFILE_PTR, "Not a valid SNDFILE* pointer."},
    {SFE_BAD_SF_INFO_PTR, "NULL SF_INFO pointer passed to libsndfile."},
    {SFE_BAD_SF_INCOMPLETE, "SndFile class incomplete and end of header parsing."},
    {SFE_BAD_FILE_PTR, "Bad FILE pointer."},
    {SFE_BAD_INT_PTR, "Internal error, Bad pointer."},
    {SFE_BAD_STAT_SIZE, "Error : software was misconfigured at compile time "
                        "(sizeof statbuf.st_size)."},
    {SFE_NO_TEMP_DIR, "Error : Could not file temp dir."},

    {SFE_MALLOC_FAILED, "Internal malloc () failed."},
    {SFE_UNIMPLEMENTED, "File contains data in an unimplemented format."},
    {SFE_BAD_READ_ALIGN, "Attempt to read a non-integer number of channels."},
    {SFE_BAD_WRITE_ALIGN, "Attempt to write a non-integer number of channels."},
    {SFE_NOT_READMODE, "Read attempted on file currently open for write."},
    {SFE_NOT_WRITEMODE, "Write attempted on file currently open for read."},
    {SFE_BAD_MODE_RW, "Error : This file format does not support read/write mode."},
    {SFE_BAD_SF_INFO, "Internal error : SF_INFO struct incomplete."},
    {SFE_BAD_OFFSET, "Error : supplied offset beyond end of file."},
    {SFE_BAD_VIRTUAL_IO, "Error : bad pointer on SF_VIRTUAL_IO struct."},
    {SFE_BAD_BROADCAST_INFO_SIZE, "Error : bad coding_history_size in SF_BROADCAST_INFO struct."},
    {SFE_BAD_BROADCAST_INFO_TOO_BIG, "Error : SF_BROADCAST_INFO struct too large."},
    {SFE_BAD_CART_INFO_SIZE, "Error: SF_CART_INFO struct too large."},
    {SFE_BAD_CART_INFO_TOO_BIG, "Error: bad tag_text_size in SF_CART_INFO struct."},
    {SFE_INTERLEAVE_MODE, "Attempt to write to file with non-interleaved data."},
    {SFE_INTERLEAVE_SEEK, "Bad karma in seek during interleave read operation."},
    {SFE_INTERLEAVE_READ, "Bad karma in read during interleave read operation."},

    {SFE_INTERNAL, "Unspecified internal error."},
    {SFE_BAD_COMMAND_PARAM, "Bad parameter passed to function sf_command."},
    {SFE_BAD_ENDIAN, "Bad endian-ness. Try default endian-ness"},
    {SFE_CHANNEL_COUNT_ZERO, "Channel count is zero."},
    {SFE_CHANNEL_COUNT, "Too many channels specified."},
    {SFE_CHANNEL_COUNT_BAD, "Bad channel count."},

    {SFE_BAD_SEEK, "Internal psf_fseek() failed."},
    {SFE_NOT_SEEKABLE, "Seek attempted on unseekable file type."},
    {SFE_AMBIGUOUS_SEEK, "Error : combination of file open mode and seek command is ambiguous."},
    {SFE_WRONG_SEEK, "Error : invalid seek parameters."},
    {SFE_SEEK_FAILED, "Error : parameters OK, but psf_seek() failed."},

    {SFE_BAD_OPEN_MODE, "Error : bad mode parameter for file open."},
    {SFE_RDWR_POSITION, "Error on RDWR position (cryptic)."},
    {SFE_RDWR_BAD_HEADER, "Error : Cannot open file in read/write mode due to "
                          "string data in header."},
    {SFE_CMD_HAS_DATA, "Error : Command fails because file already has audio data."},

    {SFE_STR_NO_SUPPORT, "Error : File type does not support string data."},
    {SFE_STR_NOT_WRITE, "Error : Trying to set a string when file is not in write mode."},
    {SFE_STR_MAX_DATA, "Error : Maximum string data storage reached."},
    {SFE_STR_MAX_COUNT, "Error : Maximum string data count reached."},
    {SFE_STR_BAD_TYPE, "Error : Bad string data type."},
    {SFE_STR_NO_ADD_END, "Error : file type does not support strings added at end of file."},
    {SFE_STR_BAD_STRING, "Error : bad string."},
    {SFE_STR_WEIRD, "Error : Weird string error."},

    {SFE_WAV_NO_RIFF, "Error in WAV file. No 'RIFF' chunk marker."},
    {SFE_WAV_NO_WAVE, "Error in WAV file. No 'WAVE' chunk marker."},
    {SFE_WAV_NO_FMT, "Error in WAV/W64/RF64 file. No 'fmt ' chunk marker."},
    {SFE_WAV_BAD_FMT, "Error in WAV/W64/RF64 file. Malformed 'fmt ' chunk."},
    {SFE_WAV_FMT_SHORT, "Error in WAV/W64/RF64 file. Short 'fmt ' chunk."},

    {SFE_WAV_BAD_FACT, "Error in WAV file. 'fact' chunk out of place."},
    {SFE_WAV_BAD_PEAK, "Error in WAV file. Bad 'PEAK' chunk."},
    {SFE_WAV_PEAK_B4_FMT, "Error in WAV file. 'PEAK' chunk found before 'fmt ' chunk."},

    {SFE_WAV_BAD_FORMAT, "Error in WAV file. Errors in 'fmt ' chunk."},
    {SFE_WAV_BAD_BLOCKALIGN, "Error in WAV file. Block alignment in 'fmt ' chunk is incorrect."},
    {SFE_WAV_NO_DATA, "Error in WAV file. No 'data' chunk marker."},
    {SFE_WAV_BAD_LIST, "Error in WAV file. Malformed LIST chunk."},
    {SFE_WAV_UNKNOWN_CHUNK, "Error in WAV file. File contains an unknown chunk marker."},
    {SFE_WAV_WVPK_DATA, "Error in WAV file. Data is in WAVPACK format."},

    {SFE_WAV_ADPCM_NOT4BIT, "Error in ADPCM WAV file. Invalid bit width."},
    {SFE_WAV_ADPCM_CHANNELS, "Error in ADPCM WAV file. Invalid number of channels."},
    {SFE_WAV_ADPCM_SAMPLES, "Error in ADPCM WAV file. Invalid number of samples per block."},
    {SFE_WAV_GSM610_FORMAT, "Error in GSM610 WAV file. Invalid format chunk."},
    {SFE_WAV_NMS_FORMAT, "Error in NMS ADPCM WAV file. Invalid format chunk."},

    {SFE_AIFF_NO_FORM, "Error in AIFF file, bad 'FORM' marker."},
    {SFE_AIFF_AIFF_NO_FORM, "Error in AIFF file, 'AIFF' marker without 'FORM'."},
    {SFE_AIFF_COMM_NO_FORM, "Error in AIFF file, 'COMM' marker without 'FORM'."},
    {SFE_AIFF_SSND_NO_COMM, "Error in AIFF file, 'SSND' marker without 'COMM'."},
    {SFE_AIFF_UNKNOWN_CHUNK, "Error in AIFF file, unknown chunk."},
    {SFE_AIFF_COMM_CHUNK_SIZE, "Error in AIFF file, bad 'COMM' chunk size."},
    {SFE_AIFF_BAD_COMM_CHUNK, "Error in AIFF file, bad 'COMM' chunk."},
    {SFE_AIFF_PEAK_B4_COMM, "Error in AIFF file. 'PEAK' chunk found before 'COMM' chunk."},
    {SFE_AIFF_BAD_PEAK, "Error in AIFF file. Bad 'PEAK' chunk."},
    {SFE_AIFF_NO_SSND, "Error in AIFF file, bad 'SSND' chunk."},
    {SFE_AIFF_NO_DATA, "Error in AIFF file, no sound data."},
    {SFE_AIFF_RW_SSND_NOT_LAST,
     "Error in AIFF file, RDWR only possible if SSND chunk at end of file."},

    {SFE_AU_UNKNOWN_FORMAT, "Error in AU file, unknown format."},
    {SFE_AU_NO_DOTSND, "Error in AU file, missing '.snd' or 'dns.' marker."},

    {SFE_RAW_READ_BAD_SPEC, "Error while opening RAW file for read. Must "
                            "specify format and channels.\n"
                            "Possibly trying to open unsupported format."},
    {SFE_RAW_BAD_BITWIDTH, "Error. RAW file bitwidth must be a multiple of 8."},
    {SFE_RAW_BAD_FORMAT, "Error. Bad format field in SF_INFO struct when "
                         "opening a RAW file for read."},

    {SFE_PAF_NO_MARKER, "Error in PAF file, no marker."},
    {SFE_PAF_VERSION, "Error in PAF file, bad version."},
    {SFE_PAF_UNKNOWN_FORMAT, "Error in PAF file, unknown format."},
    {SFE_PAF_SHORT_HEADER, "Error in PAF file. File shorter than minimal header."},
    {SFE_PAF_BAD_CHANNELS, "Error in PAF file. Bad channel count."},

    {SFE_SVX_NO_FORM, "Error in 8SVX / 16SV file, no 'FORM' marker."},
    {SFE_SVX_NO_BODY, "Error in 8SVX / 16SV file, no 'BODY' marker."},
    {SFE_SVX_NO_DATA, "Error in 8SVX / 16SV file, no sound data."},
    {SFE_SVX_BAD_COMP, "Error in 8SVX / 16SV file, unsupported compression format."},
    {SFE_SVX_BAD_NAME_LENGTH, "Error in 8SVX / 16SV file, NAME chunk too long."},

    {SFE_NIST_BAD_HEADER, "Error in NIST file, bad header."},
    {SFE_NIST_CRLF_CONVERISON,
     "Error : NIST file damaged by Windows CR -> CRLF conversion process."},
    {SFE_NIST_BAD_ENCODING, "Error in NIST file, unsupported compression format."},

    {SFE_VOC_NO_CREATIVE, "Error in VOC file, no 'Creative Voice File' marker."},
    {SFE_VOC_BAD_FORMAT, "Error in VOC file, bad format."},
    {SFE_VOC_BAD_VERSION, "Error in VOC file, bad version number."},
    {SFE_VOC_BAD_MARKER, "Error in VOC file, bad marker in file."},
    {SFE_VOC_BAD_SECTIONS, "Error in VOC file, incompatible VOC sections."},
    {SFE_VOC_MULTI_SAMPLERATE, "Error in VOC file, more than one sample rate defined."},
    {SFE_VOC_MULTI_SECTION,
     "Unimplemented VOC file feature, file contains multiple sound sections."},
    {SFE_VOC_MULTI_PARAM, "Error in VOC file, file contains multiple bit or channel widths."},
    {SFE_VOC_SECTION_COUNT, "Error in VOC file, too many sections."},

    {SFE_IRCAM_NO_MARKER, "Error in IRCAM file, bad IRCAM marker."},
    {SFE_IRCAM_BAD_CHANNELS, "Error in IRCAM file, bad channel count."},
    {SFE_IRCAM_UNKNOWN_FORMAT, "Error in IRCAM file, unknown encoding format."},

    {SFE_W64_64_BIT, "Error in W64 file, file contains 64 bit offset."},
    {SFE_W64_NO_RIFF, "Error in W64 file. No 'riff' chunk marker."},
    {SFE_W64_NO_WAVE, "Error in W64 file. No 'wave' chunk marker."},
    {SFE_W64_NO_DATA, "Error in W64 file. No 'data' chunk marker."},
    {SFE_W64_ADPCM_NOT4BIT, "Error in ADPCM W64 file. Invalid bit width."},
    {SFE_W64_ADPCM_CHANNELS, "Error in ADPCM W64 file. Invalid number of channels."},
    {SFE_W64_GSM610_FORMAT, "Error in GSM610 W64 file. Invalid format chunk."},

    {SFE_MAT4_BAD_NAME, "Error in MAT4 file. No variable name."},
    {SFE_MAT4_NO_SAMPLERATE, "Error in MAT4 file. No sample rate."},

    {SFE_MAT5_BAD_ENDIAN, "Error in MAT5 file. Not able to determine endian-ness."},
    {SFE_MAT5_NO_BLOCK, "Error in MAT5 file. Bad block structure."},
    {SFE_MAT5_SAMPLE_RATE, "Error in MAT5 file. Not able to determine sample rate."},

    {SFE_PVF_NO_PVF1, "Error in PVF file. No PVF1 marker."},
    {SFE_PVF_BAD_HEADER, "Error in PVF file. Bad header."},
    {SFE_PVF_BAD_BITWIDTH, "Error in PVF file. Bad bit width."},

    {SFE_XI_BAD_HEADER, "Error in XI file. Bad header."},
    {SFE_XI_EXCESS_SAMPLES, "Error in XI file. Excess samples in file."},

    {SFE_SDS_NOT_SDS, "Error : not an SDS file."},
    {SFE_SDS_BAD_BIT_WIDTH, "Error : bad bit width for SDS file."},

    {SFE_FLAC_BAD_HEADER, "Error : bad flac header."},
    {SFE_FLAC_NEW_DECODER, "Error : problem while creating flac decoder."},
    {SFE_FLAC_INIT_DECODER, "Error : problem with initialization of the flac decoder."},
    {SFE_FLAC_LOST_SYNC, "Error : flac decoder lost sync."},
    {SFE_FLAC_BAD_SAMPLE_RATE, "Error : flac does not support this sample rate."},
    {SFE_FLAC_CHANNEL_COUNT_CHANGED, "Error : flac channel changed mid stream."},
    {SFE_FLAC_UNKOWN_ERROR, "Error : unknown error in flac decoder."},

    {SFE_WVE_NOT_WVE, "Error : not a WVE file."},

    {SFE_DWVW_BAD_BITWIDTH, "Error : Bad bit width for DWVW encoding. Must be 12, 16 or 24."},
    {SFE_G72X_NOT_MONO, "Error : G72x encoding does not support more than 1 channel."},
    {SFE_NMS_ADPCM_NOT_MONO, "Error : NMS ADPCM encoding does not support more than 1 channel."},

    {SFE_VORBIS_ENCODER_BUG, "Error : Sample rate chosen is known to trigger a "
                             "Vorbis encoder bug on this CPU."},

    {SFE_RF64_NOT_RF64, "Error : Not an RF64 file."},
    {SFE_RF64_PEAK_B4_FMT, "Error in RF64 file. 'PEAK' chunk found before 'fmt ' chunk."},
    {SFE_RF64_NO_DATA, "Error in RF64 file. No 'data' chunk marker."},

    {SFE_ALAC_FAIL_TMPFILE, "Error : Failed to open tmp file for ALAC encoding."},

    {SFE_BAD_CHUNK_PTR, "Error : Bad SF_CHUNK_INFO pointer."},
    {SFE_UNKNOWN_CHUNK, "Error : Unknown chunk marker."},
    {SFE_BAD_CHUNK_FORMAT,
     "Error : Reading/writing chunks from this file format is not supported."},
    {SFE_BAD_CHUNK_MARKER, "Error : Bad chunk marker."},
    {SFE_BAD_CHUNK_DATA_PTR, "Error : Bad data pointer in SF_CHUNK_INFO struct."},
    {SFE_FILENAME_TOO_LONG, "Error : Supplied filename too long."},
    {SFE_NEGATIVE_RW_LEN, "Error : Length parameter passed to read/write is negative."},

    {SFE_ALREADY_INITIALIZED, "Error : Already initialized." },

    {SFE_MAX_ERROR, "Maximum error number."},
    {SFE_MAX_ERROR + 1, NULL}};

/*------------------------------------------------------------------------------
*/

bool format_from_extension(const char *path, SF_INFO *sfinfo);
bool guess_file_type(sf::ref_ptr<SF_STREAM> &stream, SF_INFO *sfinfo);
int validate_sfinfo(SF_INFO *sfinfo);
int validate_psf(SndFile *psf);
void save_header_info(SndFile *psf);

/*------------------------------------------------------------------------------
** Private (static) variables.
*/

int sf_errno = 0;
char sf_parselog[SF_BUFFER_LEN] = {0};
static char sf_syserr[SF_SYSERR_LEN] = {0};

/*------------------------------------------------------------------------------
**	Public functions.
*/

int sf_open(const char *path, SF_FILEMODE mode, SF_INFO *sfinfo, SNDFILE **sndfile)
{
    // Input parameters check

    if (mode != SFM_READ && mode != SFM_WRITE && mode != SFM_RDWR)
    {
        sf_errno = SFE_BAD_OPEN_MODE;
        return sf_errno;
    };

    if (!sfinfo)
    {
        sf_errno = SFE_BAD_SF_INFO_PTR;
        return sf_errno;
    };

    if (!sndfile)
    {
        sf_errno = SFE_BAD_FILE_PTR;
        return sf_errno;
    }
    *sndfile = nullptr;

    // Handle open modes

    // Read mode
    if (mode == SFM_READ)
    {
        // For RAW files sfinfo parameter must be properly set
        if ((SF_CONTAINER(sfinfo->format)) == SF_FORMAT_RAW)
        {
            if (sf_format_check(sfinfo) == 0)
            {
                sf_errno = SFE_RAW_BAD_FORMAT;
                return sf_errno;
            };
        }
        // For any other format in read mode sfinfo parameter is
        // ignored, set its fields to zero.
        else
        {
            memset(sfinfo, 0, sizeof(SF_INFO));
        }
    };
    
    SndFile *psf = nullptr;
    try
    {
        psf = new SndFile();

        sf::ref_ptr<SF_STREAM> stream;
        int error = psf_open_file_stream(path, mode, stream.get_address_of());
        if (error != SFE_NO_ERROR)
            throw sf::sndfile_error(error);

        // Need this to detect if we create new file (filelength = 0).
        sf_count_t filelength = stream->get_filelen();

        if (mode == SFM_WRITE || (mode == SFM_RDWR && filelength == 0))
        {
            // If the file is being opened for write or RDWR and the file is currently
            // empty, then the SF_INFO struct must contain valid data.
            if ((SF_CONTAINER(sfinfo->format)) == 0)
                throw sf::sndfile_error(SFE_ZERO_MAJOR_FORMAT);

            if ((SF_CODEC(sfinfo->format)) == 0)
                throw sf::sndfile_error(SFE_ZERO_MINOR_FORMAT);

            if (sf_format_check(sfinfo) == 0)
                throw sf::sndfile_error(SFE_BAD_OPEN_FORMAT);
        }
        else if ((SF_CONTAINER(sfinfo->format)) != SF_FORMAT_RAW)
        {
            // If type RAW has not been specified then need to figure out file type.
            if (!guess_file_type(stream, sfinfo))
            {
                if (!format_from_extension(path, sfinfo))
                {
                    throw sf::sndfile_error(SFE_BAD_OPEN_FORMAT);
                }
            }
        };

        // Here we have format detected

        psf->log_printf("File : %s\n", path);
        strcpy(psf->m_path, path);

        psf->open(stream, mode, sfinfo);
        if (!psf->is_open())
            throw sf::sndfile_error(psf->m_error);

        /* Call the initialisation function for the relevant file type. */
        switch (SF_CONTAINER(psf->sf.format))
        {
        case SF_FORMAT_WAV:
        case SF_FORMAT_WAVEX:
            error = wav_open(psf);
            break;

        case SF_FORMAT_AIFF:
            error = aiff_open(psf);
            break;

        case SF_FORMAT_AU:
            error = au_open(psf);
            break;

        case SF_FORMAT_RAW:
            error = raw_open(psf);
            break;

        case SF_FORMAT_W64:
            error = w64_open(psf);
            break;

        case SF_FORMAT_RF64:
            error = rf64_open(psf);
            break;

        case SF_FORMAT_PAF:
            error = paf_open(psf);
            break;

        case SF_FORMAT_SVX:
            error = svx_open(psf);
            break;

        case SF_FORMAT_NIST:
            error = nist_open(psf);
            break;

        case SF_FORMAT_IRCAM:
            error = ircam_open(psf);
            break;

        case SF_FORMAT_VOC:
            error = voc_open(psf);
            break;

        case SF_FORMAT_SDS:
            error = sds_open(psf);
            break;

        case SF_FORMAT_OGG:
            error = ogg_open(psf);
            break;

        case SF_FORMAT_TXW:
            error = txw_open(psf);
            break;

        case SF_FORMAT_WVE:
            error = wve_open(psf);
            break;

        case SF_FORMAT_DWD:
            error = dwd_open(psf);
            break;

        case SF_FORMAT_MAT4:
            error = mat4_open(psf);
            break;

        case SF_FORMAT_MAT5:
            error = mat5_open(psf);
            break;

        case SF_FORMAT_PVF:
            error = pvf_open(psf);
            break;

        case SF_FORMAT_XI:
            error = xi_open(psf);
            break;

        case SF_FORMAT_HTK:
            error = htk_open(psf);
            break;

        case SF_FORMAT_REX2:
            error = rx2_open(psf);
            break;

        case SF_FORMAT_AVR:
            error = avr_open(psf);
            break;

        case SF_FORMAT_FLAC:
            error = flac_open(psf);
            break;

        case SF_FORMAT_CAF:
            error = caf_open(psf);
            break;

        case SF_FORMAT_MPC2K:
            error = mpc2k_open(psf);
            break;

        default:
            error = SF_ERR_UNRECOGNISED_FORMAT;
        };

        if (error != SFE_NO_ERROR)
            throw sf::sndfile_error(error);

        if (mode == SFM_RDWR && sf_format_check(&psf->sf) == 0)
            throw sf::sndfile_error(SFE_BAD_MODE_RW);

        if (validate_sfinfo(&psf->sf) == 0)
        {
            psf->log_SF_INFO();
            save_header_info(psf);
            throw sf::sndfile_error(SFE_BAD_SF_INFO);
        };

        if (validate_psf(psf) == 0)
        {
            save_header_info(psf);
            throw sf::sndfile_error(SFE_INTERNAL);
        };

        psf->m_read_current = 0;
        psf->m_write_current = 0;
        if (mode == SFM_RDWR)
        {
            psf->m_write_current = psf->sf.frames;
            psf->m_have_written = psf->sf.frames > 0 ? true : false;
        };

        memcpy(sfinfo, &psf->sf, sizeof(SF_INFO));

        if (mode == SFM_WRITE)
        {
            /* Zero out these fields. */
            sfinfo->frames = 0;
            sfinfo->sections = 0;
            sfinfo->seekable = 0;
        };

        *sndfile = static_cast<SNDFILE *>(psf);
        psf->ref();

        return SF_ERR_NO_ERROR;
    }
    catch (sf::sndfile_error &e)
    {
        delete psf;

        return e.error();
    };
}

int sf_open_stream(SF_STREAM *stream, SF_FILEMODE mode, SF_INFO *sfinfo, SNDFILE **sndfile)
{
    if (!stream)
        return SFE_BAD_FILE_PTR;
   // Input parameters check

    if (mode != SFM_READ && mode != SFM_WRITE && mode != SFM_RDWR)
    {
        sf_errno = SFE_BAD_OPEN_MODE;
        return sf_errno;
    };

    if (!sfinfo)
    {
        sf_errno = SFE_BAD_SF_INFO_PTR;
        return sf_errno;
    };

    if (!sndfile)
    {
        sf_errno = SFE_BAD_FILE_PTR;
        return sf_errno;
    }
    *sndfile = nullptr;

    // Handle open modes

    // Read mode
    if (mode == SFM_READ)
    {
        // For RAW files sfinfo parameter must be properly set
        if ((SF_CONTAINER(sfinfo->format)) == SF_FORMAT_RAW)
        {
            if (sf_format_check(sfinfo) == 0)
            {
                sf_errno = SFE_RAW_BAD_FORMAT;
                return sf_errno;
            };
        }
        // For any other format in read mode sfinfo parameter is
        // ignored, set its fields to zero.
        else
        {
            memset(sfinfo, 0, sizeof(SF_INFO));
        }
    };
    
    SndFile *psf = nullptr;
    try
    {
        psf = new SndFile();

        sf::ref_ptr<SF_STREAM> s;
        s.copy(stream);

        // Need this to detect if we create new file (filelength = 0).
        sf_count_t filelength = s->get_filelen();

        if (mode == SFM_WRITE || (mode == SFM_RDWR && filelength == 0))
        {
            // If the file is being opened for write or RDWR and the file is currently
            // empty, then the SF_INFO struct must contain valid data.
            if ((SF_CONTAINER(sfinfo->format)) == 0)
                throw sf::sndfile_error(SFE_ZERO_MAJOR_FORMAT);

            if ((SF_CODEC(sfinfo->format)) == 0)
                throw sf::sndfile_error(SFE_ZERO_MINOR_FORMAT);

            if (sf_format_check(sfinfo) == 0)
                throw sf::sndfile_error(SFE_BAD_OPEN_FORMAT);
        }
        else if ((SF_CONTAINER(sfinfo->format)) != SF_FORMAT_RAW)
        {
            // If type RAW has not been specified then need to figure out file type.
            if (!guess_file_type(s, sfinfo))
            {
                throw sf::sndfile_error(SFE_BAD_OPEN_FORMAT);
            }
        };

        // Here we have format detected

        psf->open(s, mode, sfinfo);
        if (!psf->is_open())
            throw sf::sndfile_error(psf->m_error);

        int error = SFE_NO_ERROR;
        /* Call the initialisation function for the relevant file type. */
        switch (SF_CONTAINER(psf->sf.format))
        {
        case SF_FORMAT_WAV:
        case SF_FORMAT_WAVEX:
            error = wav_open(psf);
            break;

        case SF_FORMAT_AIFF:
            error = aiff_open(psf);
            break;

        case SF_FORMAT_AU:
            error = au_open(psf);
            break;

        case SF_FORMAT_RAW:
            error = raw_open(psf);
            break;

        case SF_FORMAT_W64:
            error = w64_open(psf);
            break;

        case SF_FORMAT_RF64:
            error = rf64_open(psf);
            break;

        case SF_FORMAT_PAF:
            error = paf_open(psf);
            break;

        case SF_FORMAT_SVX:
            error = svx_open(psf);
            break;

        case SF_FORMAT_NIST:
            error = nist_open(psf);
            break;

        case SF_FORMAT_IRCAM:
            error = ircam_open(psf);
            break;

        case SF_FORMAT_VOC:
            error = voc_open(psf);
            break;

        case SF_FORMAT_SDS:
            error = sds_open(psf);
            break;

        case SF_FORMAT_OGG:
            error = ogg_open(psf);
            break;

        case SF_FORMAT_TXW:
            error = txw_open(psf);
            break;

        case SF_FORMAT_WVE:
            error = wve_open(psf);
            break;

        case SF_FORMAT_DWD:
            error = dwd_open(psf);
            break;

        case SF_FORMAT_MAT4:
            error = mat4_open(psf);
            break;

        case SF_FORMAT_MAT5:
            error = mat5_open(psf);
            break;

        case SF_FORMAT_PVF:
            error = pvf_open(psf);
            break;

        case SF_FORMAT_XI:
            error = xi_open(psf);
            break;

        case SF_FORMAT_HTK:
            error = htk_open(psf);
            break;

        case SF_FORMAT_REX2:
            error = rx2_open(psf);
            break;

        case SF_FORMAT_AVR:
            error = avr_open(psf);
            break;

        case SF_FORMAT_FLAC:
            error = flac_open(psf);
            break;

        case SF_FORMAT_CAF:
            error = caf_open(psf);
            break;

        case SF_FORMAT_MPC2K:
            error = mpc2k_open(psf);
            break;

        default:
            error = SF_ERR_UNRECOGNISED_FORMAT;
        };

        if (error != SFE_NO_ERROR)
            throw sf::sndfile_error(error);

        if (mode == SFM_RDWR && sf_format_check(&psf->sf) == 0)
            throw sf::sndfile_error(SFE_BAD_MODE_RW);

        if (validate_sfinfo(&psf->sf) == 0)
        {
            psf->log_SF_INFO();
            save_header_info(psf);
            throw sf::sndfile_error(SFE_BAD_SF_INFO);
        };

        if (validate_psf(psf) == 0)
        {
            save_header_info(psf);
            throw sf::sndfile_error(SFE_INTERNAL);
        };

        psf->m_read_current = 0;
        psf->m_write_current = 0;
        if (mode == SFM_RDWR)
        {
            psf->m_write_current = psf->sf.frames;
            psf->m_have_written = psf->sf.frames > 0 ? true : false;
        };

        memcpy(sfinfo, &psf->sf, sizeof(SF_INFO));

        if (mode == SFM_WRITE)
        {
            /* Zero out these fields. */
            sfinfo->frames = 0;
            sfinfo->sections = 0;
            sfinfo->seekable = 0;
        };

        *sndfile = static_cast<SNDFILE *>(psf);
        psf->ref();

        return SF_ERR_NO_ERROR;
    }
    catch (sf::sndfile_error &e)
    {
        delete psf;

        return e.error();
    };
} /* sf_open_virtual */

int sf_close(SNDFILE *sndfile)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    sndfile->unref();
    return 0;
} /* sf_close */

void sf_write_sync(SNDFILE *sndfile)
{
    if (sndfile)
        sndfile->writeSync();
} /* sf_write_sync */

/*==============================================================================
*/

const char *sf_error_number(int errnum)
{
    static const char *bad_errnum =
        "No error defined for this error number. This is a bug in libsndfile.";
    int k;

    if (errnum == SFE_MAX_ERROR)
        return SndfileErrors[0].str;

    if (errnum < 0 || errnum > SFE_MAX_ERROR)
    {
        /* This really shouldn't happen in release versions. */
        printf("Not a valid error number (%d).\n", errnum);
        return bad_errnum;
    };

    for (k = 0; SndfileErrors[k].str; k++)
        if (errnum == SndfileErrors[k].error)
            return SndfileErrors[k].str;

    return bad_errnum;
} /* sf_error_number */

const char *sf_strerror(SNDFILE *sndfile)
{
    SndFile *psf = NULL;
    int errnum;

    if (sndfile == NULL)
    {
        errnum = sf_errno;
        if (errnum == SFE_SYSTEM && sf_syserr[0])
            return sf_syserr;
    }
    else
    {
        psf = (SndFile *)sndfile;

        if (psf->m_Magick != SNDFILE_MAGICK)
            return "sf_strerror : Bad magic number.";

        errnum = psf->m_error;

        if (errnum == SFE_SYSTEM && psf->m_syserr[0])
            return psf->m_syserr;
    };

    return sf_error_number(errnum);
} /* sf_strerror */

/*------------------------------------------------------------------------------
*/

int sf_error(SNDFILE *sndfile)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    return sndfile->getError();
} /* sf_error */

/*------------------------------------------------------------------------------
*/

int sf_perror(SNDFILE *sndfile)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    int errnum = sndfile->getError();

    fprintf(stderr, "%s\n", sf_error_number(errnum));
    return SFE_NO_ERROR;
} /* sf_perror */

/*------------------------------------------------------------------------------
*/

int sf_error_str(SNDFILE *sndfile, char *str, size_t maxlen)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    int errnum = sndfile->getError();

    snprintf(str, maxlen, "%s", sf_error_number(errnum));

    return SFE_NO_ERROR;
}

/*==============================================================================
*/

int sf_format_check(const SF_INFO *info)
{
    int subformat, endian;

    subformat = SF_CODEC(info->format);
    endian = SF_ENDIAN(info->format);

    /* This is the place where each file format can check if the suppiled
	** SF_INFO struct is valid.
	** Return 0 on failure, 1 ons success.
	*/

    if (info->channels < 1 || info->channels > SF_MAX_CHANNELS)
        return 0;

    if (info->samplerate < 0)
        return 0;

    switch (SF_CONTAINER(info->format))
    {
    case SF_FORMAT_WAV:
        /* WAV now allows both endian, RIFF or RIFX (little or big respectively) */
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if ((subformat == SF_FORMAT_IMA_ADPCM || subformat == SF_FORMAT_MS_ADPCM) &&
            info->channels <= 2)
            return 1;
        if (subformat == SF_FORMAT_GSM610 && info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_G721_32 && info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        if ((subformat == SF_FORMAT_NMS_ADPCM_16 || subformat == SF_FORMAT_NMS_ADPCM_24 ||
             subformat == SF_FORMAT_NMS_ADPCM_32) && info->channels == 1)
            return 1;
        break;

    case SF_FORMAT_WAVEX:
        if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        break;

    case SF_FORMAT_AIFF:
        /* AIFF does allow both endian-nesses for PCM data.*/
        if (subformat == SF_FORMAT_PCM_16 || subformat == SF_FORMAT_PCM_24 ||
            subformat == SF_FORMAT_PCM_32)
            return 1;
        /* For other encodings reject any endian-ness setting. */
        if (endian != 0)
            return 0;
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_S8)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        if ((subformat == SF_FORMAT_DWVW_12 || subformat == SF_FORMAT_DWVW_16 ||
             subformat == SF_FORMAT_DWVW_24) &&
            info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_GSM610 && info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_IMA_ADPCM && (info->channels == 1 || info->channels == 2))
            return 1;
        break;

    case SF_FORMAT_AU:
        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        if (subformat == SF_FORMAT_G721_32 && info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_G723_24 && info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_G723_40 && info->channels == 1)
            return 1;
        break;

    case SF_FORMAT_CAF:
        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        if (subformat == SF_FORMAT_ALAC_16 || subformat == SF_FORMAT_ALAC_20)
            return 1;
        if (subformat == SF_FORMAT_ALAC_24 || subformat == SF_FORMAT_ALAC_32)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        break;

    case SF_FORMAT_RAW:
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_S8 ||
            subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        if (subformat == SF_FORMAT_ALAW || subformat == SF_FORMAT_ULAW)
            return 1;
        if ((subformat == SF_FORMAT_DWVW_12 || subformat == SF_FORMAT_DWVW_16 ||
             subformat == SF_FORMAT_DWVW_24) &&
            info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_GSM610 && info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_VOX_ADPCM && info->channels == 1)
            return 1;
        if ((subformat == SF_FORMAT_NMS_ADPCM_16 || subformat == SF_FORMAT_NMS_ADPCM_24 ||
	     subformat == SF_FORMAT_NMS_ADPCM_32) && info->channels == 1)
            return 1 ;
        break;

    case SF_FORMAT_PAF:
        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16 ||
            subformat == SF_FORMAT_PCM_24)
            return 1;
        break;

    case SF_FORMAT_SVX:
        /* SVX only supports writing mono SVX files. */
        if (info->channels > 1)
            return 0;
        /* Always big endian. */
        if (endian == SF_ENDIAN_LITTLE || endian == SF_ENDIAN_CPU)
            return 0;

        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        break;

    case SF_FORMAT_NIST:
        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        break;

    case SF_FORMAT_IRCAM:
        if (info->channels > 256)
            return 0;
        if (subformat == SF_FORMAT_PCM_16 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW ||
            subformat == SF_FORMAT_FLOAT)
            return 1;
        break;

    case SF_FORMAT_VOC:
        if (info->channels > 2)
            return 0;
        /* VOC is strictly little endian. */
        if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        break;

    case SF_FORMAT_W64:
        /* W64 is strictly little endian. */
        if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if ((subformat == SF_FORMAT_IMA_ADPCM || subformat == SF_FORMAT_MS_ADPCM) &&
            info->channels <= 2)
            return 1;
        if (subformat == SF_FORMAT_GSM610 && info->channels == 1)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        break;

    case SF_FORMAT_MAT4:
        if (subformat == SF_FORMAT_PCM_16 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        break;

    case SF_FORMAT_MAT5:
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16 ||
            subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        break;

    case SF_FORMAT_PVF:
        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16 ||
            subformat == SF_FORMAT_PCM_32)
            return 1;
        break;

    case SF_FORMAT_XI:
        if (info->channels != 1)
            return 0;
        if (subformat == SF_FORMAT_DPCM_8 || subformat == SF_FORMAT_DPCM_16)
            return 1;
        break;

    case SF_FORMAT_HTK:
        if (info->channels != 1)
            return 0;
        /* HTK is strictly big endian. */
        if (endian == SF_ENDIAN_LITTLE || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_16)
            return 1;
        break;

    case SF_FORMAT_SDS:
        if (info->channels != 1)
            return 0;
        /* SDS is strictly big endian. */
        if (endian == SF_ENDIAN_LITTLE || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16 ||
            subformat == SF_FORMAT_PCM_24)
            return 1;
        break;

    case SF_FORMAT_AVR:
        if (info->channels > 2)
            return 0;
        /* SDS is strictly big endian. */
        if (endian == SF_ENDIAN_LITTLE || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_S8 ||
            subformat == SF_FORMAT_PCM_16)
            return 1;
        break;

    case SF_FORMAT_FLAC:
        /* FLAC can't do more than 8 channels. */
        if (info->channels > 8)
            return 0;
        if (endian != SF_ENDIAN_FILE)
            return 0;
        if (subformat == SF_FORMAT_PCM_S8 || subformat == SF_FORMAT_PCM_16 ||
            subformat == SF_FORMAT_PCM_24)
            return 1;
        break;

    case SF_FORMAT_WVE:
        if (info->channels > 1)
            return 0;
        /* WVE is strictly big endian. */
        if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_ALAW)
            return 1;
        break;

    case SF_FORMAT_OGG:
        if (endian != SF_ENDIAN_FILE)
            return 0;
        if (subformat == SF_FORMAT_VORBIS)
            return 1;
        break;

    case SF_FORMAT_MPC2K:
        if (info->channels > 2)
            return 0;
        /* MPC2000 is strictly little endian. */
        if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_16)
            return 1;
        break;

    case SF_FORMAT_RF64:
        if (endian == SF_ENDIAN_BIG || endian == SF_ENDIAN_CPU)
            return 0;
        if (subformat == SF_FORMAT_PCM_U8 || subformat == SF_FORMAT_PCM_16)
            return 1;
        if (subformat == SF_FORMAT_PCM_24 || subformat == SF_FORMAT_PCM_32)
            return 1;
        if (subformat == SF_FORMAT_ULAW || subformat == SF_FORMAT_ALAW)
            return 1;
        if (subformat == SF_FORMAT_FLOAT || subformat == SF_FORMAT_DOUBLE)
            return 1;
        break;
    default:
        break;
    };

    return 0;
} /* sf_format_check */

/*------------------------------------------------------------------------------
*/

const char *sf_version_string(void)
{
#if ENABLE_EXPERIMENTAL_CODE
    return PACKAGE_NAME "-" PACKAGE_VERSION "-exp";
#else
    return PACKAGE_NAME "-" PACKAGE_VERSION;
#endif
}

/*------------------------------------------------------------------------------
*/

int sf_command(SNDFILE *sndfile, int command, void *data, int datasize)
{
    /* This set of commands do not need the sndfile parameter. */
    switch (command)
    {
    case SFC_GET_LIB_VERSION:
        if (data == NULL)
        {
            return 0;
        };
        snprintf((char *)data, datasize, "%s", sf_version_string());
        return strlen((char *)data);

    case SFC_GET_SIMPLE_FORMAT_COUNT:
        if (data == NULL || datasize != SIGNED_SIZEOF(int))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        *((int *)data) = psf_get_format_simple_count();
        return 0;

    case SFC_GET_SIMPLE_FORMAT:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_simple((SF_FORMAT_INFO *)data);

    case SFC_GET_FORMAT_MAJOR_COUNT:
        if (data == NULL || datasize != SIGNED_SIZEOF(int))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        *((int *)data) = psf_get_format_major_count();
        return 0;

    case SFC_GET_FORMAT_MAJOR:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_major((SF_FORMAT_INFO *)data);

    case SFC_GET_FORMAT_SUBTYPE_COUNT:
        if (data == NULL || datasize != SIGNED_SIZEOF(int))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        *((int *)data) = psf_get_format_subtype_count();
        return 0;

    case SFC_GET_FORMAT_SUBTYPE:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_subtype((SF_FORMAT_INFO *)data);

    case SFC_GET_FORMAT_INFO:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_info((SF_FORMAT_INFO *)data);
    };

    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    return sndfile->command(command, data, datasize);
}

sf_count_t sf_seek(SNDFILE *sndfile, sf_count_t offset, int whence)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;
    
    return sndfile->seek(offset, whence);
}

const char *sf_get_string(SNDFILE *sndfile, int str_type)
{
    if (!sndfile)
        return nullptr;

    return sndfile->getString(str_type);
} /* sf_get_string */

int sf_set_string(SNDFILE *sndfile, int str_type, const char *str)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    return sndfile->setString(str_type, str);
}

int sf_current_byterate(SNDFILE *sndfile)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    return sndfile->getCurrentByterate();
}

sf_count_t sf_read_raw(SNDFILE *sndfile, void *ptr, sf_count_t bytes)
{
    if (!sndfile)
        return 0;

    return sndfile->readRaw(ptr, bytes);
}

sf_count_t sf_read_short(SNDFILE *sndfile, short *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->readShortSamples(ptr, len);
}

sf_count_t sf_readf_short(SNDFILE *sndfile, short *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->readShortFrames(ptr, frames);
}

sf_count_t sf_read_int(SNDFILE *sndfile, int *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->readIntSamples(ptr, len);
}

sf_count_t sf_readf_int(SNDFILE *sndfile, int *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->readIntFrames(ptr, frames);
}

sf_count_t sf_read_float(SNDFILE *sndfile, float *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->readFloatSamples(ptr, len);
}

sf_count_t sf_readf_float(SNDFILE *sndfile, float *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->readFloatFrames(ptr, frames);
}

sf_count_t sf_read_double(SNDFILE *sndfile, double *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->readDoubleSamples(ptr, len);
}

sf_count_t sf_readf_double(SNDFILE *sndfile, double *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->readDoubleFrames(ptr, frames);
}

sf_count_t sf_write_raw(SNDFILE *sndfile, const void *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->writeRaw(ptr, len);
}

sf_count_t sf_write_short(SNDFILE *sndfile, const short *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->writeShortSamples(ptr, len);
}

sf_count_t sf_writef_short(SNDFILE *sndfile, const short *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->writeShortFrames(ptr, frames);
}

sf_count_t sf_write_int(SNDFILE *sndfile, const int *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->writeIntSamples(ptr, len);
}

sf_count_t sf_writef_int(SNDFILE *sndfile, const int *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->writeIntFrames(ptr, frames);
}

sf_count_t sf_write_float(SNDFILE *sndfile, const float *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->writeFroatSamples(ptr, len);
}

sf_count_t sf_writef_float(SNDFILE *sndfile, const float *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->writeFloatFrames(ptr, frames);
}

sf_count_t sf_write_double(SNDFILE *sndfile, const double *ptr, sf_count_t len)
{
    if (!sndfile)
        return 0;

    return sndfile->writeDoubleSamples(ptr, len);
}

sf_count_t sf_writef_double(SNDFILE *sndfile, const double *ptr, sf_count_t frames)
{
    if (!sndfile)
        return 0;

    return sndfile->writeDoubleFrames(ptr, frames);
}

bool format_from_extension(const char *path, SF_INFO *sfinfo)
{
    char *cptr;
    char buffer[16];
    int format = 0;

    if ((cptr = (char *)strrchr(path, '.')) == NULL)
        return false;

    cptr++;
    if (strlen(cptr) > sizeof(buffer) - 1)
        return false;

    psf_strlcpy(buffer, sizeof(buffer), cptr);
    buffer[sizeof(buffer) - 1] = 0;

    /* Convert everything in the buffer to lower case. */
    cptr = buffer;
    while (*cptr)
    {
        *cptr = tolower(*cptr);
        cptr++;
    };

    cptr = buffer;

    if (strcmp(cptr, "au") == 0)
    {
        sfinfo->channels = 1;
        sfinfo->samplerate = 8000;
        sfinfo->format = SF_FORMAT_RAW | SF_FORMAT_ULAW;
        return true;
    }
    else if (strcmp(cptr, "snd") == 0)
    {
        sfinfo->channels = 1;
        sfinfo->samplerate = 8000;
        sfinfo->format = SF_FORMAT_RAW | SF_FORMAT_ULAW;
        return true;
    }
    else if (strcmp(cptr, "vox") == 0 || strcmp(cptr, "vox8") == 0)
    {
        sfinfo->channels = 1;
        sfinfo->samplerate = 8000;
        sfinfo->format = SF_FORMAT_RAW | SF_FORMAT_VOX_ADPCM;
        return true;
    }
    else if (strcmp(cptr, "vox6") == 0)
    {
        sfinfo->channels = 1;
        sfinfo->samplerate = 6000;
        sfinfo->format = SF_FORMAT_RAW | SF_FORMAT_VOX_ADPCM;
        return true;
    }
    else if (strcmp(cptr, "gsm") == 0)
    {
        sfinfo->channels = 1;
        sfinfo->samplerate = 8000;
        sfinfo->format = SF_FORMAT_RAW | SF_FORMAT_GSM610;
        return true;
    }

    return false;
}

bool guess_file_type(sf::ref_ptr<SF_STREAM> &stream, SF_INFO *sfinfo)
{
    assert(stream);
    assert(sfinfo != nullptr);

    uint32_t buffer[3], format;
    stream->seek(0, SF_SEEK_SET);
    if (stream->read(&buffer, SIGNED_SIZEOF(buffer)) != SIGNED_SIZEOF(buffer))
        return false;

    if ((buffer[0] == MAKE_MARKER('R', 'I', 'F', 'F') ||
         buffer[0] == MAKE_MARKER('R', 'I', 'F', 'X')) &&
        buffer[2] == MAKE_MARKER('W', 'A', 'V', 'E'))
    {
        sfinfo->format = SF_FORMAT_WAV;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('F', 'O', 'R', 'M'))
    {
        if (buffer[2] == MAKE_MARKER('A', 'I', 'F', 'F') ||
            buffer[2] == MAKE_MARKER('A', 'I', 'F', 'C'))
        {
            sfinfo->format = SF_FORMAT_AIFF;
            return true;
        }
        if (buffer[2] == MAKE_MARKER('8', 'S', 'V', 'X') ||
            buffer[2] == MAKE_MARKER('1', '6', 'S', 'V'))
        {
            sfinfo->format = SF_FORMAT_SVX;
            return true;
        }
        return false;
    };

    if (buffer[0] == MAKE_MARKER('.', 's', 'n', 'd') ||
        buffer[0] == MAKE_MARKER('d', 'n', 's', '.'))
    {
        sfinfo->format = SF_FORMAT_AU;
        return true;
    }

    if ((buffer[0] == MAKE_MARKER('f', 'a', 'p', ' ') ||
         buffer[0] == MAKE_MARKER(' ', 'p', 'a', 'f')))
    {
        sfinfo->format = SF_FORMAT_PAF;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('N', 'I', 'S', 'T'))
    {
        sfinfo->format = SF_FORMAT_NIST;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('C', 'r', 'e', 'a') &&
        buffer[1] == MAKE_MARKER('t', 'i', 'v', 'e'))
    {
        sfinfo->format = SF_FORMAT_VOC;
        return true;
    }

    if ((buffer[0] & MAKE_MARKER(0xFF, 0xFF, 0xF8, 0xFF)) == MAKE_MARKER(0x64, 0xA3, 0x00, 0x00) ||
        (buffer[0] & MAKE_MARKER(0xFF, 0xF8, 0xFF, 0xFF)) == MAKE_MARKER(0x00, 0x00, 0xA3, 0x64))
    {
        sfinfo->format = SF_FORMAT_IRCAM;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('r', 'i', 'f', 'f'))
    {
        sfinfo->format = SF_FORMAT_W64;
        return true;
    }

    if (buffer[0] == MAKE_MARKER(0, 0, 0x03, 0xE8) && buffer[1] == MAKE_MARKER(0, 0, 0, 1) &&
        buffer[2] == MAKE_MARKER(0, 0, 0, 1))
    {
        sfinfo->format = SF_FORMAT_MAT4;
        return true;
    }

    if (buffer[0] == MAKE_MARKER(0, 0, 0, 0) && buffer[1] == MAKE_MARKER(1, 0, 0, 0) &&
        buffer[2] == MAKE_MARKER(1, 0, 0, 0))
    {
        sfinfo->format = SF_FORMAT_MAT4;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('M', 'A', 'T', 'L') &&
        buffer[1] == MAKE_MARKER('A', 'B', ' ', '5'))
    {
        sfinfo->format = SF_FORMAT_MAT5;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('P', 'V', 'F', '1'))
    {
        sfinfo->format = SF_FORMAT_PVF;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('E', 'x', 't', 'e') &&
        buffer[1] == MAKE_MARKER('n', 'd', 'e', 'd') &&
        buffer[2] == MAKE_MARKER(' ', 'I', 'n', 's'))
    {
        sfinfo->format = SF_FORMAT_XI;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('c', 'a', 'f', 'f') &&
        buffer[2] == MAKE_MARKER('d', 'e', 's', 'c'))
    {
        sfinfo->format = SF_FORMAT_CAF;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('O', 'g', 'g', 'S'))
    {
        sfinfo->format = SF_FORMAT_OGG;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('A', 'L', 'a', 'w') &&
        buffer[1] == MAKE_MARKER('S', 'o', 'u', 'n') &&
        buffer[2] == MAKE_MARKER('d', 'F', 'i', 'l'))
    {
        sfinfo->format = SF_FORMAT_WVE;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('D', 'i', 'a', 'm') &&
        buffer[1] == MAKE_MARKER('o', 'n', 'd', 'W') &&
        buffer[2] == MAKE_MARKER('a', 'r', 'e', ' '))
    {
        sfinfo->format = SF_FORMAT_DWD;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('L', 'M', '8', '9') || buffer[0] == MAKE_MARKER('5', '3', 0, 0))
    {
        sfinfo->format = SF_FORMAT_TXW;
        return true;
    }

    if ((buffer[0] & MAKE_MARKER(0xFF, 0xFF, 0x80, 0xFF)) == MAKE_MARKER(0xF0, 0x7E, 0, 0x01))
    {
        sfinfo->format = SF_FORMAT_SDS;
        return true;
    }

    if ((buffer[0] & MAKE_MARKER(0xFF, 0xFF, 0, 0)) == MAKE_MARKER(1, 4, 0, 0))
    {
        sfinfo->format = SF_FORMAT_MPC2K;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('C', 'A', 'T', ' ') &&
        buffer[2] == MAKE_MARKER('R', 'E', 'X', '2'))
    {
        sfinfo->format = SF_FORMAT_REX2;
        return true;
    }

    if (buffer[0] == MAKE_MARKER(0x30, 0x26, 0xB2, 0x75) &&
        buffer[1] == MAKE_MARKER(0x8E, 0x66, 0xCF, 0x11))
    {
        return false /*-SF_FORMAT_WMA-*/;
    }

    sf_count_t filelength = stream->get_filelen();
    /* HMM (Hidden Markov Model) Tool Kit. */
    if (buffer[2] == MAKE_MARKER(0, 2, 0, 0) &&
        2 * ((int64_t)BE2H_32(buffer[0])) + 12 == filelength)
    {
        sfinfo->format = SF_FORMAT_HTK;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('f', 'L', 'a', 'C'))
    {
        sfinfo->format = SF_FORMAT_FLAC;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('2', 'B', 'I', 'T'))
    {
        sfinfo->format = SF_FORMAT_AVR;
        return true;
    }

    if (buffer[0] == MAKE_MARKER('R', 'F', '6', '4') &&
        buffer[2] == MAKE_MARKER('W', 'A', 'V', 'E'))
    {
        sfinfo->format = SF_FORMAT_RF64;
        return true;
    }

    /* Turtle Beach SMP 16-bit */
    if (buffer[0] == MAKE_MARKER('S', 'O', 'U', 'N') &&
        buffer[1] == MAKE_MARKER('D', ' ', 'S', 'A'))
    {
        return false;
    }

    /* Yamaha sampler format. */
    if (buffer[0] == MAKE_MARKER('S', 'Y', '8', '0') ||
        buffer[0] == MAKE_MARKER('S', 'Y', '8', '5'))
    {
        return false;
    }

    if (buffer[0] == MAKE_MARKER('a', 'j', 'k', 'g'))
    {
        return false /*-SF_FORMAT_SHN-*/;
    }

    return false;
}

int validate_sfinfo(SF_INFO *sfinfo)
{
    if (sfinfo->samplerate < 1)
        return 0;
    if (sfinfo->frames < 0)
        return 0;
    if (sfinfo->channels < 1)
        return 0;
    if ((SF_CONTAINER(sfinfo->format)) == 0)
        return 0;
    if ((SF_CODEC(sfinfo->format)) == 0)
        return 0;
    if (sfinfo->sections < 1)
        return 0;
    return 1;
}

int validate_psf(SndFile *psf)
{
    if (psf->m_datalength < 0)
    {
        psf->log_printf("Invalid SndFile field : m_datalength == %D.\n", psf->m_datalength);
        return 0;
    };
    if (psf->m_dataoffset < 0)
    {
        psf->log_printf("Invalid SndFile field : m_dataoffset == %D.\n", psf->m_dataoffset);
        return 0;
    };
    if (psf->m_blockwidth && psf->m_blockwidth != psf->sf.channels * psf->m_bytewidth)
    {
        psf->log_printf("Invalid SndFile field : sf.channels * m_bytewidth == %d.\n",
                       psf->sf.channels * psf->m_bytewidth);
        return 0;
    };
    return 1;
}

void save_header_info(SndFile *psf)
{
    snprintf(sf_parselog, sizeof(sf_parselog), "%s", psf->m_parselog.buf);
}

/*==============================================================================
** Chunk getting and setting.
** This works for AIFF, CAF, RF64 and WAV.
** It doesn't work for W64 because W64 uses weird GUID style chunk markers.
*/

int sf_set_chunk(SNDFILE *sndfile, const SF_CHUNK_INFO *chunk_info)
{
    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    return sndfile->setChunk(chunk_info);
}

SF_CHUNK_ITERATOR *sf_get_chunk_iterator(SNDFILE *sndfile, const SF_CHUNK_INFO *chunk_info)
{
    if (!sndfile)
        return nullptr;

    return sndfile->getChunkIterator(chunk_info);
}

SF_CHUNK_ITERATOR *sf_next_chunk_iterator(SF_CHUNK_ITERATOR *iterator)
{
    SNDFILE *sndfile = iterator ? iterator->sndfile : nullptr;

    if (!sndfile)
        return nullptr;

    return sndfile->getNextChunkIterator(iterator);
}

int sf_get_chunk_size(const SF_CHUNK_ITERATOR *iterator, SF_CHUNK_INFO *chunk_info)
{
    SNDFILE *sndfile = iterator ? iterator->sndfile : nullptr;

    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    return sndfile->getChunkSize(iterator, chunk_info);
}

int sf_get_chunk_data(const SF_CHUNK_ITERATOR *iterator, SF_CHUNK_INFO *chunk_info)
{
    SNDFILE *sndfile = iterator ? iterator->sndfile : nullptr;

    if (!sndfile)
        return SFE_BAD_SNDFILE_PTR;

    return sndfile->getChunkData(iterator, chunk_info);
}

unsigned long SndFile::ref()
{
    m_error = SFE_NO_ERROR;

    unsigned long ref_counter = 0;

    ref_counter = ++m_ref;

    return ref_counter;
}

void SndFile::unref()
{
    m_error = SFE_NO_ERROR;

    m_ref--;
    if (m_ref == 0)
        delete this;
}

sf_count_t SndFile::getFrames(void) const
{
    return sf.frames;
}

int SndFile::getFormat(void) const
{
    return sf.format;
}

int SndFile::getChannels(void) const
{
    return sf.channels;
}

int SndFile::getSamplerate(void) const
{
    return sf.samplerate;
}

int SndFile::getError(void) const
{
    return m_error;
}

const char *SndFile::getErrorString(void) const
{
    return sf_error_number(m_error);
}

int SndFile::command(int command, void *data, int datasize)
{
    m_error = SFE_NO_ERROR;

    double quality;
    int old_value;

    /* This set of commands do not need the sndfile parameter. */
    switch (command)
    {
    case SFC_GET_LIB_VERSION:
        if (data == NULL)
        {
            return 0;
        };
        snprintf((char *)data, datasize, "%s", sf_version_string());
        return strlen((char *)data);

    case SFC_GET_SIMPLE_FORMAT_COUNT:
        if (data == NULL || datasize != SIGNED_SIZEOF(int))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        *((int *)data) = psf_get_format_simple_count();
        return 0;

    case SFC_GET_SIMPLE_FORMAT:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_simple((SF_FORMAT_INFO *)data);

    case SFC_GET_FORMAT_MAJOR_COUNT:
        if (data == NULL || datasize != SIGNED_SIZEOF(int))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        *((int *)data) = psf_get_format_major_count();
        return 0;

    case SFC_GET_FORMAT_MAJOR:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_major((SF_FORMAT_INFO *)data);

    case SFC_GET_FORMAT_SUBTYPE_COUNT:
        if (data == NULL || datasize != SIGNED_SIZEOF(int))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        *((int *)data) = psf_get_format_subtype_count();
        return 0;

    case SFC_GET_FORMAT_SUBTYPE:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_subtype((SF_FORMAT_INFO *)data);

    case SFC_GET_FORMAT_INFO:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_FORMAT_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        return psf_get_format_info((SF_FORMAT_INFO *)data);
    };

    switch (command)
    {
    case SFC_SET_NORM_FLOAT:
        old_value = m_norm_float;
        m_norm_float = (datasize) ? SF_TRUE : SF_FALSE;
        return old_value;

    case SFC_GET_CURRENT_SF_INFO:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_INFO))
            return (sf_errno = SFE_BAD_COMMAND_PARAM);
        memcpy(data, &sf, sizeof(SF_INFO));
        break;

    case SFC_SET_NORM_DOUBLE:
        old_value = m_norm_double;
        m_norm_double = (datasize) ? SF_TRUE : SF_FALSE;
        return old_value;

    case SFC_GET_NORM_FLOAT:
        return m_norm_float;

    case SFC_GET_NORM_DOUBLE:
        return m_norm_double;

    case SFC_SET_SCALE_FLOAT_INT_READ:
        old_value = m_float_int_mult;

        m_float_int_mult = (datasize != 0) ? SF_TRUE : SF_FALSE;
        if (m_float_int_mult && m_float_max < 0.0)
            /* Scale to prevent wrap-around distortion. */
            m_float_max = (float)((32768.0 / 32767.0) * psf_calc_signal_max(this, SF_FALSE));
        return old_value;

    case SFC_SET_SCALE_INT_FLOAT_WRITE:
        old_value = m_scale_int_float;
        m_scale_int_float = (datasize != 0) ? SF_TRUE : SF_FALSE;
        return old_value;

    case SFC_SET_ADD_PEAK_CHUNK:
    {
        int format = SF_CONTAINER(sf.format);

        /* Only WAV and AIFF support the PEAK chunk. */
        switch (format)
        {
        case SF_FORMAT_AIFF:
        case SF_FORMAT_CAF:
        case SF_FORMAT_WAV:
        case SF_FORMAT_WAVEX:
        case SF_FORMAT_RF64:
            break;

        default:
            return SF_FALSE;
        };

        format = SF_CODEC(sf.format);

        /* Only files containg the following data types support the PEAK chunk. */
        if (format != SF_FORMAT_FLOAT && format != SF_FORMAT_DOUBLE)
            return SF_FALSE;
    };
    /* Can only do this is in SFM_WRITE mode. */
    if (m_mode != SFM_WRITE && m_mode != SFM_RDWR)
        return SF_FALSE;
    /* If data has already been written this must fail. */
    if (m_have_written)
    {
        m_error = SFE_CMD_HAS_DATA;
        return SF_FALSE;
    };
    /* Everything seems OK, so set psf->has_peak and re-write header. */
    if (datasize == SF_FALSE && m_peak_info != NULL)
    {
        if (m_peak_info)
        {
            m_peak_info.reset();
        }
    }
    else if (!m_peak_info)
    {
        m_peak_info = std::unique_ptr<PEAK_INFO>(new PEAK_INFO(sf.channels));
    };

    if (write_header)
        write_header(this, SF_TRUE);
    return datasize;

    case SFC_SET_ADD_HEADER_PAD_CHUNK:
        return SF_FALSE;

    case SFC_GET_LOG_INFO:
        if (data == NULL)
            return 0;
        snprintf((char *)data, datasize, "%s", m_parselog.buf);
        return strlen((char *)data);

    case SFC_CALC_SIGNAL_MAX:
        if (data == NULL || datasize != sizeof(double))
            return (m_error = SFE_BAD_COMMAND_PARAM);
        *((double *)data) = psf_calc_signal_max(this, SF_FALSE);
        break;

    case SFC_CALC_NORM_SIGNAL_MAX:
        if (data == NULL || datasize != sizeof(double))
            return (m_error = SFE_BAD_COMMAND_PARAM);
        *((double *)data) = psf_calc_signal_max(this, SF_TRUE);
        break;

    case SFC_CALC_MAX_ALL_CHANNELS:
        if (data == NULL || datasize != SIGNED_SIZEOF(double) * sf.channels)
            return (m_error = SFE_BAD_COMMAND_PARAM);
        return psf_calc_max_all_channels(this, (double *)data, SF_FALSE);

    case SFC_CALC_NORM_MAX_ALL_CHANNELS:
        if (data == NULL || datasize != SIGNED_SIZEOF(double) * sf.channels)
            return (m_error = SFE_BAD_COMMAND_PARAM);
        return psf_calc_max_all_channels(this, (double *)data, SF_TRUE);

    case SFC_GET_SIGNAL_MAX:
        if (data == NULL || datasize != sizeof(double))
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };
        return psf_get_signal_max(this, (double *)data);

    case SFC_GET_MAX_ALL_CHANNELS:
        if (data == NULL || datasize != SIGNED_SIZEOF(double) * sf.channels)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };
        return psf_get_max_all_channels(this, (double *)data);

    case SFC_UPDATE_HEADER_NOW:
        if (write_header)
            write_header(this, SF_TRUE);
        break;

    case SFC_SET_UPDATE_HEADER_AUTO:
        m_auto_header = datasize ? SF_TRUE : SF_FALSE;
        return m_auto_header;
        break;

    case SFC_SET_DITHER_ON_WRITE:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_DITHER_INFO))
            return (m_error = SFE_BAD_COMMAND_PARAM);
        memcpy(&m_write_dither, data, sizeof(m_write_dither));
        if (m_mode == SFM_WRITE || m_mode == SFM_RDWR)
            dither_init(this, SFM_WRITE);
        break;

    case SFC_SET_DITHER_ON_READ:
        if (data == NULL || datasize != SIGNED_SIZEOF(SF_DITHER_INFO))
            return (m_error = SFE_BAD_COMMAND_PARAM);
        memcpy(&m_read_dither, data, sizeof(m_read_dither));
        if (m_mode == SFM_READ || m_mode == SFM_RDWR)
            dither_init(this, SFM_READ);
        break;

    case SFC_FILE_TRUNCATE:
        if (m_mode != SFM_WRITE && m_mode != SFM_RDWR)
            return SF_TRUE;
        if (datasize != sizeof(sf_count_t))
            return SF_TRUE;
        if (data == NULL || datasize != sizeof(sf_count_t))
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        }
        else
        {
            sf_count_t position;

            position = *((sf_count_t *)data);

            if (seek(position, SF_SEEK_SET) != position)
                return SF_TRUE;

            sf.frames = position;

            position = fseek(0, SEEK_CUR);

            return ftruncate(position);
        };
        break;

    case SFC_SET_RAW_START_OFFSET:
        if (data == NULL || datasize != sizeof(sf_count_t))
            return (m_error = SFE_BAD_COMMAND_PARAM);

        if ((SF_CONTAINER(sf.format)) != SF_FORMAT_RAW)
            return (m_error = SFE_BAD_COMMAND_PARAM);

        m_dataoffset = *((sf_count_t *)data);
        seek(0, SF_SEEK_CUR);
        break;

        /* Lite remove start */
    case SFC_TEST_IEEE_FLOAT_REPLACE:
        m_ieee_replace = (datasize) ? SF_TRUE : SF_FALSE;
        if ((SF_CODEC(sf.format)) == SF_FORMAT_FLOAT)
            float32_init(this);
        else if ((SF_CODEC(sf.format)) == SF_FORMAT_DOUBLE)
            double64_init(this);
        else
            return (m_error = SFE_BAD_COMMAND_PARAM);
        break;
        /* Lite remove end */

    case SFC_SET_CLIPPING:
        m_add_clipping = (datasize) ? true : false;
        return m_add_clipping;

    case SFC_GET_CLIPPING:
        return m_add_clipping;

    case SFC_GET_LOOP_INFO:
        if (datasize != sizeof(SF_LOOP_INFO) || data == NULL)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };
        if (m_loop_info == NULL)
            return SF_FALSE;
        memcpy(data, m_loop_info, sizeof(SF_LOOP_INFO));
        return SF_TRUE;

    case SFC_GET_CUE_COUNT:
        if (datasize != sizeof(uint32_t) || data == NULL)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };
        if (!m_cues.empty())
        {
            *((uint32_t *)data) = m_cues.size();
            return SF_TRUE;
        };
        return SF_FALSE;

    case SFC_GET_CUE_POINTS:
    {
        {
            if (!data || datasize <= 0)
            {
                m_error = SFE_BAD_COMMAND_PARAM;
                return SF_FALSE;
            }
            SF_CUE_POINT *in_cue = reinterpret_cast<SF_CUE_POINT *>(data);
            size_t in_cue_count = (std::min)(m_cues.size(), static_cast<size_t>(datasize));

            memcpy(data, m_cues.data(), in_cue_count * sizeof(SF_CUE_POINT));

            if (!data) {
                m_error = SFE_MALLOC_FAILED;
                return SF_FALSE;
            };
            return in_cue_count;
        }
    }

    case SFC_SET_CUE_POINTS:
    {
        if (m_have_written)
        {
            m_error = SFE_CMD_HAS_DATA;
            return SF_FALSE;
        };
        if (!data && datasize <= 0)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };
        if (!data && datasize == 0)
            return SF_TRUE;
        SF_CUE_POINT *in_cues = (SF_CUE_POINT *)data;
        size_t in_cue_count = (size_t)datasize;

        m_cues.clear();
        for (size_t i = 0; i <= in_cue_count; i++)
        {
            m_cues.push_back(in_cues[i]);
        }

        //sndfile->cues.resize(in_cue_count);
        //sndfile->cues.insert(sndfile->cues.begin(), in_cues, in_cues + 1);
        return SF_TRUE;
    }

    case SFC_GET_INSTRUMENT:
        if (datasize != sizeof(SF_INSTRUMENT) || data == NULL)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };
        if (m_instrument == NULL)
            return SF_FALSE;
        memcpy(data, m_instrument, sizeof(SF_INSTRUMENT));
        return SF_TRUE;

    case SFC_SET_INSTRUMENT:
        /* If data has already been written this must fail. */
        if (m_have_written)
        {
            m_error = SFE_CMD_HAS_DATA;
            return SF_FALSE;
        };
        if (datasize != sizeof(SF_INSTRUMENT) || data == NULL)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };

        if (m_instrument == NULL && (m_instrument = psf_instrument_alloc()) == NULL)
        {
            m_error = SFE_MALLOC_FAILED;
            return SF_FALSE;
        };
        memcpy(m_instrument, data, sizeof(SF_INSTRUMENT));
        return SF_TRUE;

    case SFC_RAW_DATA_NEEDS_ENDSWAP:
        return m_data_endswap;

    case SFC_GET_CHANNEL_MAP_INFO:
        if (m_channel_map.empty())
            return SF_FALSE;

        if (data == NULL || datasize != SIGNED_SIZEOF(m_channel_map[0]) * sf.channels)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };

        memcpy(data, m_channel_map.data(), datasize);
        return SF_TRUE;

    case SFC_SET_CHANNEL_MAP_INFO:
        if (m_have_written)
        {
            m_error = SFE_CMD_HAS_DATA;
            return SF_FALSE;
        };
        if (data == NULL || datasize != SIGNED_SIZEOF(m_channel_map[0]) * sf.channels)
        {
            m_error = SFE_BAD_COMMAND_PARAM;
            return SF_FALSE;
        };

        {
            int *iptr;

            for (iptr = (int *)data; iptr < (int *)data + sf.channels; iptr++)
            {
                if (*iptr <= SF_CHANNEL_MAP_INVALID || *iptr >= SF_CHANNEL_MAP_MAX)
                {
                    m_error = SFE_BAD_COMMAND_PARAM;
                    return SF_FALSE;
                };
            };
        };

        m_channel_map.resize(datasize);
        memcpy(m_channel_map.data(), data, datasize);

        /*
        **	Pass the command down to the container's command handler.
        **	Don't pass user data, use validated psf->channel_map data instead.
        */
        if (on_command)
            return on_command(this, command, NULL, 0);
        return SF_FALSE;

    case SFC_SET_VBR_ENCODING_QUALITY:
        if (data == NULL || datasize != sizeof(double))
            return SF_FALSE;

        quality = *((double *)data);
        quality = 1.0 - (std::max)(0.0, (std::min)(1.0, quality));
        return this->command(SFC_SET_COMPRESSION_LEVEL, &quality, sizeof(quality));

    default:
        /* Must be a file specific command. Pass it on. */
        if (on_command)
            return this->on_command(this, command, data, datasize);

        log_printf("*** sf_command : cmd = 0x%X\n", command);
        return m_error = SFE_BAD_COMMAND_PARAM;
    };

    return 0;
}

sf_count_t SndFile::seek(sf_count_t frames, int whence)
{
    m_error = SFE_NO_ERROR;

    sf_count_t seek_offset = 0, retval;

    if (!sf.seekable)
    {
        m_error = SFE_NOT_SEEKABLE;
        return PSF_SEEK_ERROR;
    };

    /* If the whence parameter has a mode ORed in, check to see that
	** it makes sense.
	*/
    if (((whence & SFM_MASK) == SFM_WRITE && m_mode == SFM_READ) ||
        ((whence & SFM_MASK) == SFM_READ && m_mode == SFM_WRITE))
    {
        m_error = SFE_WRONG_SEEK;
        return PSF_SEEK_ERROR;
    };

    /* Convert all SEEK_CUR and SEEK_END into seek_from_start to be
	** used with SEEK_SET.
	*/
    switch (whence)
    {
    /* The SEEK_SET behaviour is independant of mode. */
    case SEEK_SET:
    case SEEK_SET | SFM_READ:
    case SEEK_SET | SFM_WRITE:
    case SEEK_SET | SFM_RDWR:
        seek_offset = frames;
        break;

    /* The SEEK_CUR is a little more tricky. */
    case SEEK_CUR:
        if (frames == 0)
        {
            if (m_mode == SFM_READ)
                return m_read_current;
            if (m_mode == SFM_WRITE)
                return m_write_current;
        };
        if (m_mode == SFM_READ)
            seek_offset = m_read_current + frames;
        else if (m_mode == SFM_WRITE || m_mode == SFM_RDWR)
            seek_offset = m_write_current + frames;
        else
            m_error = SFE_AMBIGUOUS_SEEK;
        break;

    case SEEK_CUR | SFM_READ:
        if (frames == 0)
            return m_read_current;
        seek_offset = m_read_current + frames;
        break;

    case SEEK_CUR | SFM_WRITE:
        if (frames == 0)
            return m_write_current;
        seek_offset = m_write_current + frames;
        break;

    /* The SEEK_END */
    case SEEK_END:
    case SEEK_END | SFM_READ:
    case SEEK_END | SFM_WRITE:
        seek_offset = sf.frames + frames;
        break;

    default:
        m_error = SFE_BAD_SEEK;
        break;
    };

    if (m_error)
        return PSF_SEEK_ERROR;

    if (m_mode == SFM_RDWR || m_mode == SFM_WRITE)
    {
        if (seek_offset < 0)
        {
            m_error = SFE_BAD_SEEK;
            return PSF_SEEK_ERROR;
        };
    }
    else if (seek_offset < 0 || seek_offset > sf.frames)
    {
        m_error = SFE_BAD_SEEK;
        return PSF_SEEK_ERROR;
    };

    if (seek_from_start)
    {
        int new_mode = (whence & SFM_MASK) ? (whence & SFM_MASK) : m_mode;

        retval = seek_from_start(this, new_mode, seek_offset);

        switch (new_mode)
        {
        case SFM_READ:
            m_read_current = retval;
            break;
        case SFM_WRITE:
            m_write_current = retval;
            break;
        case SFM_RDWR:
            m_read_current = retval;
            m_write_current = retval;
            new_mode = SFM_READ;
            break;
        };

        m_last_op = new_mode;

        return retval;
    };

    m_error = SFE_AMBIGUOUS_SEEK;
    return PSF_SEEK_ERROR;
}

void SndFile::writeSync(void)
{
    m_error = SFE_NO_ERROR;

    fsync();
};

int SndFile::setString(int str_type, const char *str)
{
    m_error = SFE_NO_ERROR;

    return set_string(str_type, str);
};

const char *SndFile::getString(int str_type) const
{
    return get_string(str_type);
};

sf_count_t SndFile::readShortSamples(short *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_READ_ALIGN;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, items * sizeof(short));
        return 0; /* End of file. */
    };

    if (!read_short || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_short(this, ptr, items);

    if (m_read_current + count / sf.channels <= sf.frames)
    {
        m_read_current += count / sf.channels;
    }
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = items - count;
        psf_memset(ptr + count, 0, extra * sizeof(short));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count;
};

sf_count_t SndFile::readIntSamples(int *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_READ_ALIGN;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, items * sizeof(int));
        return 0;
    };

    if (!read_int || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_int(this, ptr, items);

    if (m_read_current + count / sf.channels <= sf.frames)
        m_read_current += count / sf.channels;
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = items - count;
        psf_memset(ptr + count, 0, extra * sizeof(int));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count;
}

sf_count_t SndFile::readFloatSamples(float *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_READ_ALIGN;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, items * sizeof(float));
        return 0;
    };

    if (!read_float || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_float(this, ptr, items);

    if (m_read_current + count / sf.channels <= sf.frames)
    {
        m_read_current += count / sf.channels;
    }
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = items - count;
        psf_memset(ptr + count, 0, extra * sizeof(float));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count;
}

sf_count_t SndFile::readDoubleSamples(double *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_READ_ALIGN;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, items * sizeof(double));
        return 0;
    };

    if (!read_double || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_double(this, ptr, items);

    if (m_read_current + count / sf.channels <= sf.frames)
    {
        m_read_current += count / sf.channels;
    }
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = items - count;
        psf_memset(ptr + count, 0, extra * sizeof(double));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count;
}

sf_count_t SndFile::writeShortSamples(const short *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_WRITE_ALIGN;
        return 0;
    };

    if (!write_short || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_short(this, ptr, items);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count;
}

sf_count_t SndFile::writeIntSamples(const int *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_WRITE_ALIGN;
        return 0;
    };

    if (!write_int || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_int(this, ptr, items);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count;
}

sf_count_t SndFile::writeFroatSamples(const float *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_WRITE_ALIGN;
        return 0;
    };

    if (!write_float || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_float(this, ptr, items);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count;
}

sf_count_t SndFile::writeDoubleSamples(const double *ptr, sf_count_t items)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (items == 0)
        return 0;

    if (items <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (items % sf.channels)
    {
        m_error = SFE_BAD_WRITE_ALIGN;
        return 0;
    };

    if (!write_double || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_double(this, ptr, items);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count;
}

sf_count_t SndFile::readShortFrames(short *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, frames * sf.channels * sizeof(short));
        return 0; /* End of file. */
    };

    if (!read_short || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_short(this, ptr, frames * sf.channels);

    if (m_read_current + count / sf.channels <= sf.frames)
    {
        m_read_current += count / sf.channels;
    }
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = frames * sf.channels - count;
        psf_memset(ptr + count, 0, extra * sizeof(short));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count / sf.channels;
}

sf_count_t SndFile::readIntFrames(int *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, frames * sf.channels * sizeof(int));
        return 0;
    };

    if (!read_int || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_int(this, ptr, frames * sf.channels);

    if (m_read_current + count / sf.channels <= sf.frames)
    {
        m_read_current += count / sf.channels;
    }
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = frames * sf.channels - count;
        psf_memset(ptr + count, 0, extra * sizeof(int));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count / sf.channels;
}

sf_count_t SndFile::readFloatFrames(float *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, frames * sf.channels * sizeof(float));
        return 0;
    };

    if (!read_float || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_float(this, ptr, frames * sf.channels);

    if (m_read_current + count / sf.channels <= sf.frames)
    {
        m_read_current += count / sf.channels;
    }
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = frames * sf.channels - count;
        psf_memset(ptr + count, 0, extra * sizeof(float));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count / sf.channels;
}

sf_count_t SndFile::readDoubleFrames(double *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, frames * sf.channels * sizeof(double));
        return 0;
    };

    if (!read_double || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = read_double(this, ptr, frames * sf.channels);

    if (m_read_current + count / sf.channels <= sf.frames)
    {
        m_read_current += count / sf.channels;
    }
    else
    {
        count = (sf.frames - m_read_current) * sf.channels;
        extra = frames * sf.channels - count;
        psf_memset(ptr + count, 0, extra * sizeof(double));
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count / sf.channels;
}

sf_count_t SndFile::writeShortFrames(const short *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (!write_short || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_short(this, ptr, frames * sf.channels);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count / sf.channels;
}

sf_count_t SndFile::writeIntFrames(const int *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (!write_int || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_int(this, ptr, frames * sf.channels);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count / sf.channels;
}

sf_count_t SndFile::writeFloatFrames(const float *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (!write_float || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_float(this, ptr, frames * sf.channels);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count / sf.channels;
}

sf_count_t SndFile::writeDoubleFrames(const double *ptr, sf_count_t frames)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;

    if (frames == 0)
        return 0;

    if (frames <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (!write_double || !seek_from_start)
    {
        m_error = SFE_UNIMPLEMENTED;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        if ((m_error = write_header(this, SF_FALSE)))
            return 0;
    };
    m_have_written = true;

    count = write_double(this, ptr, frames * sf.channels);

    m_write_current += count / sf.channels;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count / sf.channels;
}

int SndFile::getCurrentByterate() const
{
    /* This should cover all PCM and floating point formats. */
    if (m_bytewidth)
        return sf.samplerate * sf.channels * m_bytewidth;

    if (byterate)
        return byterate(const_cast<SndFile *>(this));

    switch (SF_CODEC(sf.format))
    {
    case SF_FORMAT_IMA_ADPCM:
    case SF_FORMAT_MS_ADPCM:
    case SF_FORMAT_VOX_ADPCM:
        return (sf.samplerate * sf.channels) / 2;

    case SF_FORMAT_GSM610:
        return (sf.samplerate * sf.channels * 13000) / 8000;

    case SF_FORMAT_NMS_ADPCM_16:
        return sf.samplerate / 4 + 10;

    case SF_FORMAT_NMS_ADPCM_24:
        return sf.samplerate * 3 / 8 + 10;

    case SF_FORMAT_NMS_ADPCM_32:
        return sf.samplerate / 2 + 10;

    case SF_FORMAT_G721_32: /* 32kbs G721 ADPCM encoding. */
        return (sf.samplerate * sf.channels) / 2;

    case SF_FORMAT_G723_24: /* 24kbs G723 ADPCM encoding. */
        return (sf.samplerate * sf.channels * 3) / 8;

    case SF_FORMAT_G723_40: /* 40kbs G723 ADPCM encoding. */
        return (sf.samplerate * sf.channels * 5) / 8;

    default:
        break;
    };

    return -1;
}

sf_count_t SndFile::readRaw(void *ptr, sf_count_t bytes)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count, extra;
    int bytewidth, blockwidth;

    if (bytes == 0)
        return 0;

    bytewidth = (m_bytewidth > 0) ? m_bytewidth : 1;
    blockwidth = (m_blockwidth > 0) ? m_blockwidth : 1;

    if (m_mode == SFM_WRITE)
    {
        m_error = SFE_NOT_READMODE;
        return 0;
    };

    if (bytes < 0 || m_read_current >= sf.frames)
    {
        psf_memset(ptr, 0, bytes);
        return 0;
    };

    if (bytes % (sf.channels * bytewidth))
    {
        m_error = SFE_BAD_READ_ALIGN;
        return 0;
    };

    if (m_last_op != SFM_READ)
        if (seek_from_start(this, SFM_READ, m_read_current) < 0)
            return 0;

    count = fread(ptr, 1, bytes);

    if (m_read_current + count / blockwidth <= sf.frames)
    {
        m_read_current += count / blockwidth;
    }
    else
    {
        count = (sf.frames - m_read_current) * blockwidth;
        extra = bytes - count;
        psf_memset(((char *)ptr) + count, 0, extra);
        m_read_current = sf.frames;
    };

    m_last_op = SFM_READ;

    return count;
}

sf_count_t SndFile::writeRaw(const void *ptr, sf_count_t bytes)
{
    m_error = SFE_NO_ERROR;

    sf_count_t count;
    int bytewidth, blockwidth;

    if (bytes == 0)
        return 0;

    if (bytes <= 0)
    {
        m_error = SFE_NEGATIVE_RW_LEN;
        return 0;
    };

    bytewidth = (m_bytewidth > 0) ? m_bytewidth : 1;
    blockwidth = (m_blockwidth > 0) ? m_blockwidth : 1;

    if (m_mode == SFM_READ)
    {
        m_error = SFE_NOT_WRITEMODE;
        return 0;
    };

    if (bytes % (sf.channels * bytewidth))
    {
        m_error = SFE_BAD_WRITE_ALIGN;
        return 0;
    };

    if (m_last_op != SFM_WRITE)
        if (seek_from_start(this, SFM_WRITE, m_write_current) < 0)
            return 0;

    if (!m_have_written && write_header)
    {
        m_error = write_header(this, SF_FALSE);
        if (m_error != SFE_NO_ERROR)
            return 0;
    };
    m_have_written = true;

    count = fwrite(ptr, 1, bytes);

    m_write_current += count / blockwidth;

    m_last_op = SFM_WRITE;

    if (m_write_current > sf.frames)
    {
        sf.frames = m_write_current;
        m_dataend = 0;
    };

    if (m_auto_header && write_header)
        write_header(this, SF_TRUE);

    return count;
}

int SndFile::setChunk(const SF_CHUNK_INFO *chunk_info)
{
    m_error = SFE_NO_ERROR;

    if (chunk_info == NULL || chunk_info->data == NULL)
    {
        m_error = SFE_BAD_CHUNK_PTR;
        return m_error;
    }

    if (set_chunk != nullptr)
        return set_chunk(const_cast<SndFile *>(this), chunk_info);

    return SFE_BAD_CHUNK_FORMAT;
}

SF_CHUNK_ITERATOR *SndFile::getChunkIterator(const SF_CHUNK_INFO *chunk_info)
{
    m_error = SFE_NO_ERROR;

    if (chunk_info)
        return psf_get_chunk_iterator(this, chunk_info->id);

    return psf_get_chunk_iterator(this, NULL);
}

SF_CHUNK_ITERATOR *SndFile::getNextChunkIterator(SF_CHUNK_ITERATOR *iterator)
{
    m_error = SFE_NO_ERROR;

    SNDFILE *sndfile = iterator ? iterator->sndfile : nullptr;

    if (!sndfile)
        return nullptr;

    if (next_chunk_iterator)
        return next_chunk_iterator(this, iterator);

    return NULL;
}

int SndFile::getChunkSize(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info)
{
    m_error = SFE_NO_ERROR;

    SNDFILE *sndfile = it ? it->sndfile : NULL;

    if (!sndfile || (sndfile != this))
        return SFE_BAD_SNDFILE_PTR;

    if (chunk_info == NULL)
        return SFE_BAD_CHUNK_PTR;

    if (get_chunk_size)
        return get_chunk_size(this, it, chunk_info);

    return SFE_BAD_CHUNK_FORMAT;
}

int SndFile::getChunkData(const SF_CHUNK_ITERATOR *it, SF_CHUNK_INFO *chunk_info)
{
    m_error = SFE_NO_ERROR;

    SNDFILE *sndfile = it ? it->sndfile : NULL;

    if (!sndfile || (sndfile != this))
        return SFE_BAD_SNDFILE_PTR;

    if (chunk_info == NULL || chunk_info->data == NULL)
        return SFE_BAD_CHUNK_PTR;

    if (get_chunk_data)
        return get_chunk_data(this, it, chunk_info);

    return SFE_BAD_CHUNK_FORMAT;
}
