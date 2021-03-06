/*
** Copyright (C) 1999-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2008 George Blood Audio
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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cstdint>

using namespace std;

#include "sndfile2k/sndfile2k.h"

#include "common.h"

#define BUFFER_LEN (4096)

#define MIN(x, y) ((x) < (y) ? (x) : (y))

void sfe_copy_data_fp(SNDFILE *outfile, SNDFILE *infile, int channels, int normalize)
{
    static double data[BUFFER_LEN], max;
    sf_count_t frames, readcount, k;

    frames = BUFFER_LEN / channels;
    readcount = frames;

    sf_command(infile, SFC_CALC_SIGNAL_MAX, &max, sizeof(max));

    if (!normalize && max < 1.0)
    {
        while (readcount > 0)
        {
            readcount = sf_readf_double(infile, data, frames);
            sf_writef_double(outfile, data, readcount);
        };
    }
    else
    {
        sf_command(infile, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);

        while (readcount > 0)
        {
            readcount = sf_readf_double(infile, data, frames);
            for (k = 0; k < readcount * channels; k++)
                data[k] /= max;
            sf_writef_double(outfile, data, readcount);
        };
    };

    return;
}

void sfe_copy_data_int(SNDFILE *outfile, SNDFILE *infile, int channels)
{
    static int data[BUFFER_LEN];
    int frames, readcount;

    frames = BUFFER_LEN / channels;
    readcount = frames;

    while (readcount > 0)
    {
        readcount = sf_readf_int(infile, data, frames);
        sf_writef_int(outfile, data, readcount);
    };

    return;
}

static void update_strings(SNDFILE *outfile, const METADATA_INFO *info)
{
    if (info->title != NULL)
        sf_set_string(outfile, SF_STR_TITLE, info->title);

    if (info->copyright != NULL)
        sf_set_string(outfile, SF_STR_COPYRIGHT, info->copyright);

    if (info->artist != NULL)
        sf_set_string(outfile, SF_STR_ARTIST, info->artist);

    if (info->comment != NULL)
        sf_set_string(outfile, SF_STR_COMMENT, info->comment);

    if (info->date != NULL)
        sf_set_string(outfile, SF_STR_DATE, info->date);

    if (info->album != NULL)
        sf_set_string(outfile, SF_STR_ALBUM, info->album);

    if (info->license != NULL)
        sf_set_string(outfile, SF_STR_LICENSE, info->license);
}

void sfe_apply_metadata_changes(const char *filenames[2], const METADATA_INFO *info)
{
    SNDFILE *infile = NULL, *outfile = NULL;
    SF_INFO sfinfo;
    METADATA_INFO tmpinfo;
    int error_code = 0;

    memset(&sfinfo, 0, sizeof(sfinfo));
    memset(&tmpinfo, 0, sizeof(tmpinfo));

    if (filenames[1] == NULL)
    {
        sf_open(filenames[0], SFM_RDWR, &sfinfo, &outfile);
        infile = outfile;
    }
    else
    {
        sf_open(filenames[0], SFM_READ, &sfinfo, &infile);

        /* Output must be WAV. */
        sfinfo.format = SF_FORMAT_WAV | (SF_FORMAT_SUBMASK & sfinfo.format);
        sf_open(filenames[1], SFM_WRITE, &sfinfo, &outfile);
    };

    if (infile == NULL)
    {
        printf("Error : Not able to open input file '%s' : %s\n", filenames[0],
               sf_strerror(infile));
        error_code = 1;
        goto cleanup_exit;
    };

    if (outfile == NULL)
    {
        printf("Error : Not able to open output file '%s' : %s\n", filenames[1],
               sf_strerror(outfile));
        error_code = 1;
        goto cleanup_exit;
    };

    if (infile != outfile)
    {
        int infileminor = SF_FORMAT_SUBMASK & sfinfo.format;

        /* If the input file is not the same as the output file, copy the data. */
        if ((infileminor == SF_FORMAT_DOUBLE) || (infileminor == SF_FORMAT_FLOAT))
            sfe_copy_data_fp(outfile, infile, sfinfo.channels, SF_FALSE);
        else
            sfe_copy_data_int(outfile, infile, sfinfo.channels);
    };

    update_strings(outfile, info);

cleanup_exit:

    if (outfile != NULL && outfile != infile)
        sf_close(outfile);

    if (infile != NULL)
        sf_close(infile);

    if (error_code)
        exit(error_code);

    return;
}

typedef struct
{
    const char *ext;
    int len;
    int format;
} OUTPUT_FORMAT_MAP;

static OUTPUT_FORMAT_MAP format_map[] = {
    {"wav", 0, SF_FORMAT_WAV},
    {"aif", 3, SF_FORMAT_AIFF},
    {"au", 0, SF_FORMAT_AU},
    {"snd", 0, SF_FORMAT_AU},
    {"raw", 0, SF_FORMAT_RAW},
    {"gsm", 0, SF_FORMAT_RAW},
    {"vox", 0, SF_FORMAT_RAW},
    {"paf", 0, SF_FORMAT_PAF | SF_ENDIAN_BIG},
    {"fap", 0, SF_FORMAT_PAF | SF_ENDIAN_LITTLE},
    {"svx", 0, SF_FORMAT_SVX},
    {"nist", 0, SF_FORMAT_NIST},
    {"sph", 0, SF_FORMAT_NIST},
    {"voc", 0, SF_FORMAT_VOC},
    {"ircam", 0, SF_FORMAT_IRCAM},
    {"sf", 0, SF_FORMAT_IRCAM},
    {"w64", 0, SF_FORMAT_W64},
    {"mat", 0, SF_FORMAT_MAT4},
    {"mat4", 0, SF_FORMAT_MAT4},
    {"mat5", 0, SF_FORMAT_MAT5},
    {"pvf", 0, SF_FORMAT_PVF},
    {"xi", 0, SF_FORMAT_XI},
    {"htk", 0, SF_FORMAT_HTK},
    {"sds", 0, SF_FORMAT_SDS},
    {"avr", 0, SF_FORMAT_AVR},
    {"wavex", 0, SF_FORMAT_WAVEX},
    {"flac", 0, SF_FORMAT_FLAC},
    {"caf", 0, SF_FORMAT_CAF},
    {"wve", 0, SF_FORMAT_WVE},
    {"prc", 0, SF_FORMAT_WVE},
    {"ogg", 0, SF_FORMAT_OGG},
    {"oga", 0, SF_FORMAT_OGG},
    {"mpc", 0, SF_FORMAT_MPC2K},
    {"rf64", 0, SF_FORMAT_RF64},
};

int sfe_file_type_of_ext(const char *str, int format)
{
    char buffer[16];
    const char *cptr;
    int k;

    format &= SF_FORMAT_SUBMASK;

    if ((cptr = strrchr((char *)str, '.')) == NULL)
        return 0;

    strncpy(buffer, cptr + 1, 15);
    buffer[15] = 0;

    for (k = 0; buffer[k]; k++)
        buffer[k] = tolower((buffer[k]));

    if (strcmp(buffer, "gsm") == 0)
        return SF_FORMAT_RAW | SF_FORMAT_GSM610;

    if (strcmp(buffer, "vox") == 0)
        return SF_FORMAT_RAW | SF_FORMAT_VOX_ADPCM;

    for (k = 0; k < (int)(sizeof(format_map) / sizeof(format_map[0])); k++)
    {
        if (format_map[k].len > 0 && strncmp(buffer, format_map[k].ext, format_map[k].len) == 0)
            return format_map[k].format | format;
        else if (strcmp(buffer, format_map[k].ext) == 0)
            return format_map[k].format | format;
    };

    /* Default if all the above fails. */
    return (SF_FORMAT_WAV | SF_FORMAT_PCM_24);
}

void sfe_dump_format_map(void)
{
    SF_FORMAT_INFO info;
    int k;

    for (k = 0; k < ARRAY_LEN(format_map); k++)
    {
        info.format = format_map[k].format;
        sf_command(NULL, SFC_GET_FORMAT_INFO, &info, sizeof(info));
        printf("        %-10s : %s\n", format_map[k].ext, info.name == NULL ? "????" : info.name);
    };
}

const char *program_name(const char *argv0)
{
    const char *tmp;

    tmp = strrchr(argv0, '/');
    argv0 = tmp ? tmp + 1 : argv0;

    /* Remove leading libtool name mangling. */
    if (strstr(argv0, "lt-") == argv0)
        return argv0 + 3;

    return argv0;
}

const char *sfe_endian_name(int format)
{
    switch (format & SF_FORMAT_ENDMASK)
    {
    case SF_ENDIAN_FILE:
        return "file";
    case SF_ENDIAN_LITTLE:
        return "little";
    case SF_ENDIAN_BIG:
        return "big";
    case SF_ENDIAN_CPU:
        return "cpu";
    default:
        break;
    };

    return "unknown";
}

const char *sfe_container_name(int format)
{
    switch (format & SF_FORMAT_TYPEMASK)
    {
    case SF_FORMAT_WAV:
        return "WAV";
    case SF_FORMAT_AIFF:
        return "AIFF";
    case SF_FORMAT_AU:
        return "AU";
    case SF_FORMAT_RAW:
        return "RAW";
    case SF_FORMAT_PAF:
        return "PAF";
    case SF_FORMAT_SVX:
        return "SVX";
    case SF_FORMAT_NIST:
        return "NIST";
    case SF_FORMAT_VOC:
        return "VOC";
    case SF_FORMAT_IRCAM:
        return "IRCAM";
    case SF_FORMAT_W64:
        return "W64";
    case SF_FORMAT_MAT4:
        return "MAT4";
    case SF_FORMAT_MAT5:
        return "MAT5";
    case SF_FORMAT_PVF:
        return "PVF";
    case SF_FORMAT_XI:
        return "XI";
    case SF_FORMAT_HTK:
        return "HTK";
    case SF_FORMAT_SDS:
        return "SDS";
    case SF_FORMAT_AVR:
        return "AVR";
    case SF_FORMAT_WAVEX:
        return "WAVEX";
    case SF_FORMAT_FLAC:
        return "FLAC";
    case SF_FORMAT_CAF:
        return "CAF";
    case SF_FORMAT_WVE:
        return "WVE";
    case SF_FORMAT_OGG:
        return "OGG";
    case SF_FORMAT_MPC2K:
        return "MPC2K";
    case SF_FORMAT_RF64:
        return "RF64";
    default:
        break;
    };

    return "unknown";
}

const char *sfe_codec_name(int format)
{
    switch (format & SF_FORMAT_SUBMASK)
    {
    case SF_FORMAT_PCM_S8:
        return "signed 8 bit PCM";
    case SF_FORMAT_PCM_16:
        return "16 bit PCM";
    case SF_FORMAT_PCM_24:
        return "24 bit PCM";
    case SF_FORMAT_PCM_32:
        return "32 bit PCM";
    case SF_FORMAT_PCM_U8:
        return "unsigned 8 bit PCM";
    case SF_FORMAT_FLOAT:
        return "32 bit float";
    case SF_FORMAT_DOUBLE:
        return "64 bit double";
    case SF_FORMAT_ULAW:
        return "u-law";
    case SF_FORMAT_ALAW:
        return "a-law";
    case SF_FORMAT_IMA_ADPCM:
        return "IMA ADPCM";
    case SF_FORMAT_MS_ADPCM:
        return "MS ADPCM";
    case SF_FORMAT_GSM610:
        return "gsm610";
    case SF_FORMAT_VOX_ADPCM:
        return "Vox ADPCM";
    case SF_FORMAT_G721_32:
        return "g721 32kbps";
    case SF_FORMAT_G723_24:
        return "g723 24kbps";
    case SF_FORMAT_G723_40:
        return "g723 40kbps";
    case SF_FORMAT_DWVW_12:
        return "12 bit DWVW";
    case SF_FORMAT_DWVW_16:
        return "16 bit DWVW";
    case SF_FORMAT_DWVW_24:
        return "14 bit DWVW";
    case SF_FORMAT_DWVW_N:
        return "DWVW";
    case SF_FORMAT_DPCM_8:
        return "8 bit DPCM";
    case SF_FORMAT_DPCM_16:
        return "16 bit DPCM";
    case SF_FORMAT_VORBIS:
        return "Vorbis";
    case SF_FORMAT_ALAC_16:
        return "16 bit ALAC";
    case SF_FORMAT_ALAC_20:
        return "20 bit ALAC";
    case SF_FORMAT_ALAC_24:
        return "24 bit ALAC";
    case SF_FORMAT_ALAC_32:
        return "32 bit ALAC";
    default:
        break;
    };
    return "unknown";
}
