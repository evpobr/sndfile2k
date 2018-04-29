/*
** Copyright (C) 2001-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "config.h"

#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sf_unistd.h"

#include <math.h>

#include "sndfile2k/sndfile2k.h"

#include "sfendian.h"
#include "utils.h"

#define BUFFER_LEN (1 << 10)
#define LOG_BUFFER_SIZE (1024)
#define data_MARKER MAKE_MARKER('d', 'a', 't', 'a')

static void float_norm_test(const char *filename);
static void double_norm_test(const char *filename);
static void format_tests(void);
static void calc_peak_test(int filetype, const char *filename, int channels);
static void truncate_test(const char *filename, int filetype);
static void instrument_test(const char *filename, int filetype);
static void cue_points_test(const char *filename, int filetype);
static void channel_map_test(const char *filename, int filetype);
static void current_sf_info_test(const char *filename);
static void raw_needs_endswap_test(const char *filename, int filetype);

/* Force the start of this buffer to be double aligned. Sparc-solaris will
** choke if its not.
*/

static int int_data[BUFFER_LEN];
static float float_data[BUFFER_LEN];
static double double_data[BUFFER_LEN];

int main(int argc, char *argv[])
{
    int do_all = 0;
    int test_count = 0;

    if (argc != 2)
    {
        printf("Usage : %s <test>\n", argv[0]);
        printf("    Where <test> is one of the following:\n");
        printf("           ver     - test sf_command (SFC_GETLIB_VERSION)\n");
        printf("           norm    - test floating point normalisation\n");
        printf("           format  - test format string commands\n");
        printf("           peak    - test peak calculation\n");
        printf("           trunc   - test file truncation\n");
        printf("           inst    - test set/get of SF_INSTRUMENT.\n");
        printf("           cue     - test set/get of SF_CUES and SF_CUE_POINTS.\n");
        printf("           chanmap - test set/get of channel map data..\n");
        printf("           rawend  - test SFC_RAW_NEEDS_ENDSWAP.\n");
        printf("           all     - perform all tests\n");
        exit(1);
    };

    do_all = !strcmp(argv[1], "all");

    if (do_all || strcmp(argv[1], "ver") == 0)
    {
        char buffer[128];

        print_test_name("version_test", "(none)");
        buffer[0] = 0;
        sf_command(NULL, SFC_GET_LIB_VERSION, buffer, sizeof(buffer));
        if (strlen(buffer) < 1)
        {
            printf("Line %d: could not retrieve lib version.\n", __LINE__);
            exit(1);
        };
        puts("ok");
        test_count++;
    };

    if (do_all || strcmp(argv[1], "norm") == 0)
    {
        /*	Preliminary float/double normalisation tests. More testing
		**	is done in the program 'floating_point_test'.
		*/
        float_norm_test("float.wav");
        double_norm_test("double.wav");
        test_count++;
    };

    if (do_all || strcmp(argv[1], "peak") == 0)
    {
        calc_peak_test(SF_ENDIAN_BIG | SF_FORMAT_RAW, "be-peak.raw", 1);
        calc_peak_test(SF_ENDIAN_LITTLE | SF_FORMAT_RAW, "le-peak.raw", 1);
        calc_peak_test(SF_ENDIAN_BIG | SF_FORMAT_RAW, "be-peak.raw", 7);
        calc_peak_test(SF_ENDIAN_LITTLE | SF_FORMAT_RAW, "le-peak.raw", 7);
        test_count++;
    };

    if (do_all || !strcmp(argv[1], "format"))
    {
        format_tests();
        test_count++;
    };

    if (do_all || strcmp(argv[1], "trunc") == 0)
    {
        truncate_test("truncate.raw", SF_FORMAT_RAW | SF_FORMAT_PCM_32);
        truncate_test("truncate.au", SF_FORMAT_AU | SF_FORMAT_PCM_16);
        test_count++;
    };

    if (do_all || strcmp(argv[1], "inst") == 0)
    {
        instrument_test("instrument.wav", SF_FORMAT_WAV | SF_FORMAT_PCM_16);
        /*-instrument_test ("instrument.aiff" , SF_FORMAT_AIFF | SF_FORMAT_PCM_24) ;-*/
        /*-instrument_test ("instrument.xi", SF_FORMAT_XI | SF_FORMAT_DPCM_16) ;-*/
        test_count++;
    };

    if (do_all || strcmp(argv[1], "cue") == 0)
    {
        cue_points_test("cue.wav", SF_FORMAT_WAV | SF_FORMAT_PCM_16);
        cue_points_test("cue.aiff", SF_FORMAT_AIFF | SF_FORMAT_PCM_24);

        test_count++;
    };

    if (do_all || strcmp(argv[1], "current_sf_info") == 0)
    {
        current_sf_info_test("current.wav");
        test_count++;
    };

    if (do_all || strcmp(argv[1], "chanmap") == 0)
    {
        channel_map_test("chanmap.wavex", SF_FORMAT_WAVEX | SF_FORMAT_PCM_16);
        channel_map_test("chanmap.rf64", SF_FORMAT_RF64 | SF_FORMAT_PCM_16);
        channel_map_test("chanmap.aifc", SF_FORMAT_AIFF | SF_FORMAT_PCM_16);
        channel_map_test("chanmap.caf", SF_FORMAT_CAF | SF_FORMAT_PCM_16);
        test_count++;
    };

    if (do_all || strcmp(argv[1], "rawend") == 0)
    {
        raw_needs_endswap_test("raw_end.wav", SF_FORMAT_WAV);
        raw_needs_endswap_test("raw_end.wavex", SF_FORMAT_WAVEX);
        raw_needs_endswap_test("raw_end.rifx", SF_ENDIAN_BIG | SF_FORMAT_WAV);
        raw_needs_endswap_test("raw_end.aiff", SF_FORMAT_AIFF);
        raw_needs_endswap_test("raw_end.aiff_le", SF_ENDIAN_LITTLE | SF_FORMAT_AIFF);
        test_count++;
    };

    if (test_count == 0)
    {
        printf("Mono : ************************************\n");
        printf("Mono : *  No '%s' test defined.\n", argv[1]);
        printf("Mono : ************************************\n");
        return 1;
    };

    return 0;
}

static void float_norm_test(const char *filename)
{
    SNDFILE *file;
    SF_INFO sfinfo;
    unsigned int k;

    print_test_name("float_norm_test", filename);

    sfinfo.samplerate = 44100;
    sfinfo.format = (SF_FORMAT_RAW | SF_FORMAT_PCM_16);
    sfinfo.channels = 1;
    sfinfo.frames = BUFFER_LEN;

    /* Create float_data with all values being less than 1.0. */
    for (k = 0; k < BUFFER_LEN / 2; k++)
        float_data[k] = (k + 5) / (2.0 * BUFFER_LEN);
    for (k = BUFFER_LEN / 2; k < BUFFER_LEN; k++)
        float_data[k] = (k + 5);

    if (sf_open(filename, SFM_WRITE, &sfinfo, &file) != SF_ERR_NO_ERROR)
    {
        printf("Line %d: sf_open_write failed with error : ", __LINE__);
        fflush(stdout);
        puts(sf_strerror(NULL));
        exit(1);
    };

    /* Normalisation is on by default so no need to do anything here. */

    if ((k = sf_write_float(file, float_data, BUFFER_LEN / 2)) != BUFFER_LEN / 2)
    {
        printf("Line %d: sf_write_float failed with short write (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    /* Turn normalisation off. */
    sf_command(file, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);

    if ((k = sf_write_float(file, float_data + BUFFER_LEN / 2, BUFFER_LEN / 2)) != BUFFER_LEN / 2)
    {
        printf("Line %d: sf_write_float failed with short write (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    sf_close(file);

    /* sfinfo struct should still contain correct data. */
    if (sf_open(filename, SFM_READ, &sfinfo, &file) != SF_ERR_NO_ERROR)
    {
        printf("Line %d: sf_open_read failed with error : ", __LINE__);
        fflush(stdout);
        puts(sf_strerror(NULL));
        exit(1);
    };

    if (sfinfo.format != (SF_FORMAT_RAW | SF_FORMAT_PCM_16))
    {
        printf("Line %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, (SF_FORMAT_RAW | SF_FORMAT_PCM_16),
               sfinfo.format);
        exit(1);
    };

    if (sfinfo.frames != BUFFER_LEN)
    {
        printf("\n\nLine %d: Incorrect number of.frames in file. (%d => %" PRId64 ")\n", __LINE__, BUFFER_LEN, sfinfo.frames);
        exit(1);
    };

    if (sfinfo.channels != 1)
    {
        printf("Line %d: Incorrect number of channels in file.\n", __LINE__);
        exit(1);
    };

    /* Read float_data and check that it is normalised (ie default). */
    if ((k = sf_read_float(file, float_data, BUFFER_LEN)) != BUFFER_LEN)
    {
        printf("\n\nLine %d: sf_read_float failed with short read (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    for (k = 0; k < BUFFER_LEN; k++)
        if (float_data[k] >= 1.0)
        {
            printf("\n\nLine %d: float_data [%d] == %f which is greater than "
                   "1.0\n",
                   __LINE__, k, float_data[k]);
            exit(1);
        };

    /* Seek to start of file, turn normalisation off, read float_data and check again. */
    sf_seek(file, 0, SEEK_SET);
    sf_command(file, SFC_SET_NORM_FLOAT, NULL, SF_FALSE);

    if ((k = sf_read_float(file, float_data, BUFFER_LEN)) != BUFFER_LEN)
    {
        printf("\n\nLine %d: sf_read_float failed with short read (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    for (k = 0; k < BUFFER_LEN; k++)
        if (float_data[k] < 1.0)
        {
            printf("\n\nLine %d: float_data [%d] == %f which is less than 1.0\n", __LINE__, k, float_data[k]);
            exit(1);
        };

    /* Seek to start of file, turn normalisation on, read float_data and do final check. */
    sf_seek(file, 0, SEEK_SET);
    sf_command(file, SFC_SET_NORM_FLOAT, NULL, SF_TRUE);

    if ((k = sf_read_float(file, float_data, BUFFER_LEN)) != BUFFER_LEN)
    {
        printf("\n\nLine %d: sf_read_float failed with short read (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    for (k = 0; k < BUFFER_LEN; k++)
        if (float_data[k] > 1.0)
        {
            printf("\n\nLine %d: float_data [%d] == %f which is greater than 1.0\n", __LINE__, k, float_data[k]);
            exit(1);
        };

    sf_close(file);

    unlink(filename);

    printf("ok\n");
}

static void double_norm_test(const char *filename)
{
    SNDFILE *file;
    SF_INFO sfinfo;
    unsigned int k;

    print_test_name("double_norm_test", filename);

    sfinfo.samplerate = 44100;
    sfinfo.format = (SF_FORMAT_RAW | SF_FORMAT_PCM_16);
    sfinfo.channels = 1;
    sfinfo.frames = BUFFER_LEN;

    /* Create double_data with all values being less than 1.0. */
    for (k = 0; k < BUFFER_LEN / 2; k++)
        double_data[k] = (k + 5) / (2.0 * BUFFER_LEN);
    for (k = BUFFER_LEN / 2; k < BUFFER_LEN; k++)
        double_data[k] = (k + 5);

    if (sf_open(filename, SFM_WRITE, &sfinfo, &file) != SF_ERR_NO_ERROR)
    {
        printf("Line %d: sf_open_write failed with error : ", __LINE__);
        fflush(stdout);
        puts(sf_strerror(NULL));
        exit(1);
    };

    /* Normailsation is on by default so no need to do anything here. */
    /*-sf_command (file, "set-norm-double", "true", 0) ;-*/

    if ((k = sf_write_double(file, double_data, BUFFER_LEN / 2)) != BUFFER_LEN / 2)
    {
        printf("Line %d: sf_write_double failed with short write (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    /* Turn normalisation off. */
    sf_command(file, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);

    if ((k = sf_write_double(file, double_data + BUFFER_LEN / 2, BUFFER_LEN / 2)) != BUFFER_LEN / 2)
    {
        printf("Line %d: sf_write_double failed with short write (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    sf_close(file);

    if (sf_open(filename, SFM_READ, &sfinfo, &file) != SF_ERR_NO_ERROR)
    {
        printf("Line %d: sf_open_read failed with error : ", __LINE__);
        fflush(stdout);
        puts(sf_strerror(NULL));
        exit(1);
    };

    if (sfinfo.format != (SF_FORMAT_RAW | SF_FORMAT_PCM_16))
    {
        printf("Line %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, (SF_FORMAT_RAW | SF_FORMAT_PCM_16),
               sfinfo.format);
        exit(1);
    };

    if (sfinfo.frames != BUFFER_LEN)
    {
        printf("\n\nLine %d: Incorrect number of.frames in file. (%d => %" PRId64 ")\n", __LINE__, BUFFER_LEN, sfinfo.frames);
        exit(1);
    };

    if (sfinfo.channels != 1)
    {
        printf("Line %d: Incorrect number of channels in file.\n", __LINE__);
        exit(1);
    };

    /* Read double_data and check that it is normalised (ie default). */
    if ((k = sf_read_double(file, double_data, BUFFER_LEN)) != BUFFER_LEN)
    {
        printf("\n\nLine %d: sf_read_double failed with short read (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    for (k = 0; k < BUFFER_LEN; k++)
        if (double_data[k] >= 1.0)
        {
            printf("\n\nLine %d: double_data [%d] == %f which is greater than "
                   "1.0\n",
                   __LINE__, k, double_data[k]);
            exit(1);
        };

    /* Seek to start of file, turn normalisation off, read double_data and check again. */
    sf_seek(file, 0, SEEK_SET);
    sf_command(file, SFC_SET_NORM_DOUBLE, NULL, SF_FALSE);

    if ((k = sf_read_double(file, double_data, BUFFER_LEN)) != BUFFER_LEN)
    {
        printf("\n\nLine %d: sf_read_double failed with short read (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    for (k = 0; k < BUFFER_LEN; k++)
        if (double_data[k] < 1.0)
        {
            printf("\n\nLine %d: double_data [%d] == %f which is less than 1.0\n", __LINE__, k, double_data[k]);
            exit(1);
        };

    /* Seek to start of file, turn normalisation on, read double_data and do final check. */
    sf_seek(file, 0, SEEK_SET);
    sf_command(file, SFC_SET_NORM_DOUBLE, NULL, SF_TRUE);

    if ((k = sf_read_double(file, double_data, BUFFER_LEN)) != BUFFER_LEN)
    {
        printf("\n\nLine %d: sf_read_double failed with short read (%d ->%d)\n", __LINE__, BUFFER_LEN, k);
        exit(1);
    };

    for (k = 0; k < BUFFER_LEN; k++)
        if (double_data[k] > 1.0)
        {
            printf("\n\nLine %d: double_data [%d] == %f which is greater than "
                   "1.0\n",
                   __LINE__, k, double_data[k]);
            exit(1);
        };

    sf_close(file);

    unlink(filename);

    printf("ok\n");
}

static void format_tests(void)
{
    SF_FORMAT_INFO format_info;
    SF_INFO sfinfo;
    const char *last_name;
    int k, count;

    print_test_name("format_tests", "(null)");

    /* Clear out SF_INFO struct and set channels > 0. */
    memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.channels = 1;

    /* First test simple formats. */

    sf_command(NULL, SFC_GET_SIMPLE_FORMAT_COUNT, &count, sizeof(int));

    if (count < 0 || count > 30)
    {
        printf("Line %d: Weird count.\n", __LINE__);
        exit(1);
    };

    format_info.format = 0;
    sf_command(NULL, SFC_GET_SIMPLE_FORMAT, &format_info, sizeof(format_info));

    last_name = format_info.name;
    for (k = 1; k < count; k++)
    {
        format_info.format = k;
        sf_command(NULL, SFC_GET_SIMPLE_FORMAT, &format_info, sizeof(format_info));
        if (strcmp(last_name, format_info.name) >= 0)
        {
            printf("\n\nLine %d: format names out of sequence `%s' < `%s'.\n", __LINE__, last_name, format_info.name);
            exit(1);
        };
        sfinfo.format = format_info.format;

        if (!sf_format_check(&sfinfo))
        {
            printf("\n\nLine %d: sf_format_check failed.\n", __LINE__);
            printf("        Name : %s\n", format_info.name);
            printf("        Format      : 0x%X\n", sfinfo.format);
            printf("        Channels    : 0x%X\n", sfinfo.channels);
            printf("        Sample Rate : 0x%X\n", sfinfo.samplerate);
            exit(1);
        };
        last_name = format_info.name;
    };
    format_info.format = 666;
    sf_command(NULL, SFC_GET_SIMPLE_FORMAT, &format_info, sizeof(format_info));

    /* Now test major formats. */
    sf_command(NULL, SFC_GET_FORMAT_MAJOR_COUNT, &count, sizeof(int));

    if (count < 0 || count > 30)
    {
        printf("Line %d: Weird count.\n", __LINE__);
        exit(1);
    };

    format_info.format = 0;
    sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info, sizeof(format_info));

    last_name = format_info.name;
    for (k = 1; k < count; k++)
    {
        format_info.format = k;
        sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info, sizeof(format_info));
        if (strcmp(last_name, format_info.name) >= 0)
        {
            printf("\n\nLine %d: format names out of sequence (%d) `%s' < `%s'.\n", __LINE__, k, last_name, format_info.name);
            exit(1);
        };

        last_name = format_info.name;
    };
    format_info.format = 666;
    sf_command(NULL, SFC_GET_FORMAT_MAJOR, &format_info, sizeof(format_info));

    /* Now test subtype formats. */
    sf_command(NULL, SFC_GET_FORMAT_SUBTYPE_COUNT, &count, sizeof(int));

    if (count < 0 || count > 30)
    {
        printf("Line %d: Weird count.\n", __LINE__);
        exit(1);
    };

    format_info.format = 0;
    sf_command(NULL, SFC_GET_FORMAT_SUBTYPE, &format_info, sizeof(format_info));

    last_name = format_info.name;
    for (k = 1; k < count; k++)
    {
        format_info.format = k;
        sf_command(NULL, SFC_GET_FORMAT_SUBTYPE, &format_info, sizeof(format_info));
    };
    format_info.format = 666;
    sf_command(NULL, SFC_GET_FORMAT_SUBTYPE, &format_info, sizeof(format_info));

    printf("ok\n");
}

static void calc_peak_test(int filetype, const char *filename, int channels)
{
    SNDFILE *file;
    SF_INFO sfinfo;
    char label[128];
    int k, format;
    sf_count_t buffer_len, frame_count;
    double peak;

    snprintf(label, sizeof(label), "calc_peak_test (%d channels)", channels);
    print_test_name(label, filename);

    format = filetype | SF_FORMAT_PCM_16;

    buffer_len = BUFFER_LEN - (BUFFER_LEN % channels);
    frame_count = buffer_len / channels;

    sfinfo.samplerate = 44100;
    sfinfo.format = format;
    sfinfo.channels = channels;
    sfinfo.frames = frame_count;

    /* Create double_data with max value of 0.5. */
    for (k = 0; k < buffer_len; k++)
        double_data[k] = (k + 1) / (2.0 * buffer_len);

    file = test_open_file_or_die(filename, SFM_WRITE, &sfinfo, __LINE__);

    test_writef_double_or_die(file, 0, double_data, frame_count, __LINE__);

    sf_close(file);

    file = test_open_file_or_die(filename, SFM_READ, &sfinfo, __LINE__);

    if (sfinfo.format != format)
    {
        printf("Line %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, format, sfinfo.format);
        exit(1);
    };

    if (sfinfo.frames != frame_count)
    {
        printf("\n\nLine %d: Incorrect number of frames in file. (%" PRId64 " => %" PRId64 ")\n", __LINE__, frame_count,
               sfinfo.frames);
        exit(1);
    };

    if (sfinfo.channels != channels)
    {
        printf("Line %d: Incorrect number of channels in file.\n", __LINE__);
        exit(1);
    };

    sf_command(file, SFC_CALC_SIGNAL_MAX, &peak, sizeof(peak));
    if (fabs(peak - (1 << 14)) > 1.0)
    {
        printf("Line %d : Peak value should be %d (is %f).\n", __LINE__, (1 << 14), peak);
        exit(1);
    };

    sf_command(file, SFC_CALC_NORM_SIGNAL_MAX, &peak, sizeof(peak));
    if (fabs(peak - 0.5) > 4e-5)
    {
        printf("Line %d : Peak value should be %f (is %f).\n", __LINE__, 0.5, peak);
        exit(1);
    };

    sf_close(file);

    format = (filetype | SF_FORMAT_FLOAT);
    sfinfo.samplerate = 44100;
    sfinfo.format = format;
    sfinfo.channels = channels;
    sfinfo.frames = frame_count;

    /* Create double_data with max value of 0.5. */
    for (k = 0; k < buffer_len; k++)
        double_data[k] = (k + 1) / (2.0 * buffer_len);

    file = test_open_file_or_die(filename, SFM_WRITE, &sfinfo, __LINE__);

    test_writef_double_or_die(file, 0, double_data, frame_count, __LINE__);

    sf_close(file);

    file = test_open_file_or_die(filename, SFM_READ, &sfinfo, __LINE__);

    if (sfinfo.format != format)
    {
        printf("Line %d: Returned format incorrect (0x%08X => 0x%08X).\n", __LINE__, format, sfinfo.format);
        exit(1);
    };

    if (sfinfo.frames != frame_count)
    {
        printf("\n\nLine %d: Incorrect number of.frames in file. (%" PRId64 " => %" PRId64 ")\n", __LINE__, frame_count,
               sfinfo.frames);
        exit(1);
    };

    if (sfinfo.channels != channels)
    {
        printf("Line %d: Incorrect number of channels in file.\n", __LINE__);
        exit(1);
    };

    sf_command(file, SFC_CALC_SIGNAL_MAX, &peak, sizeof(peak));
    if (fabs(peak - 0.5) > 1e-5)
    {
        printf("Line %d : Peak value should be %f (is %f).\n", __LINE__, 0.5, peak);
        exit(1);
    };

    sf_command(file, SFC_CALC_NORM_SIGNAL_MAX, &peak, sizeof(peak));
    if (fabs(peak - 0.5) > 1e-5)
    {
        printf("Line %d : Peak value should be %f (is %f).\n", __LINE__, 0.5, peak);
        exit(1);
    };

    sf_close(file);

    unlink(filename);

    printf("ok\n");
}

static void truncate_test(const char *filename, int filetype)
{
    SNDFILE *file;
    SF_INFO sfinfo;
    sf_count_t len;

    print_test_name("truncate_test", filename);

    sfinfo.samplerate = 11025;
    sfinfo.format = filetype;
    sfinfo.channels = 2;

    file = test_open_file_or_die(filename, SFM_RDWR, &sfinfo, __LINE__);

    test_write_int_or_die(file, 0, int_data, BUFFER_LEN, __LINE__);

    len = 100;
    if (sf_command(file, SFC_FILE_TRUNCATE, &len, sizeof(len)))
    {
        printf("Line %d: sf_command (SFC_FILE_TRUNCATE) returned error.\n", __LINE__);
        exit(1);
    };

    test_seek_or_die(file, 0, SEEK_CUR, len, 2, __LINE__);
    test_seek_or_die(file, 0, SEEK_END, len, 2, __LINE__);

    sf_close(file);

    unlink(filename);
    puts("ok");
}

static void instrumet_rw_test(const char *filename)
{
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    SF_INSTRUMENT inst;
    memset(&sfinfo, 0, sizeof(SF_INFO));

    sndfile = test_open_file_or_die(filename, SFM_RDWR, &sfinfo, __LINE__);

    if (sf_command(sndfile, SFC_GET_INSTRUMENT, &inst, sizeof(inst)) == SF_TRUE)
    {
        inst.basenote = 22;

        if (sf_command(sndfile, SFC_SET_INSTRUMENT, &inst, sizeof(inst)) == SF_TRUE)
            printf("Sucess: [%s] updated\n", filename);
        else
            printf("Error: SFC_SET_INSTRUMENT on [%s] [%s]\n", filename, sf_strerror(sndfile));
    }
    else
        printf("Error: SFC_GET_INSTRUMENT on [%s] [%s]\n", filename, sf_strerror(sndfile));

    if (sf_command(sndfile, SFC_UPDATE_HEADER_NOW, NULL, 0) != 0)
        printf("Error: SFC_UPDATE_HEADER_NOW on [%s] [%s]\n", filename, sf_strerror(sndfile));

    sf_write_sync(sndfile);
    sf_close(sndfile);

    return;
}

static void instrument_test(const char *filename, int filetype)
{
    static SF_INSTRUMENT write_inst = {2, /* gain */
                                       3, /* detune */
                                       4, /* basenote */
                                       5,
                                       6, /* key low and high */
                                       7,
                                       8, /* velocity low and high */
                                       2, /* loop_count */
                                       {
                                           {801, 2, 3, 0},
                                           {801, 3, 4, 0},
                                       }};
    SF_INSTRUMENT read_inst;
    SNDFILE *file;
    SF_INFO sfinfo;

    print_test_name("instrument_test", filename);

    sfinfo.samplerate = 11025;
    sfinfo.format = filetype;
    sfinfo.channels = 1;

    file = test_open_file_or_die(filename, SFM_WRITE, &sfinfo, __LINE__);
    if (sf_command(file, SFC_SET_INSTRUMENT, &write_inst, sizeof(write_inst)) == SF_FALSE)
    {
        printf("\n\nLine %d : sf_command (SFC_SET_INSTRUMENT) failed.\n\n", __LINE__);
        exit(1);
    };
    test_write_double_or_die(file, 0, double_data, BUFFER_LEN, __LINE__);
    sf_close(file);

    memset(&read_inst, 0, sizeof(read_inst));

    file = test_open_file_or_die(filename, SFM_READ, &sfinfo, __LINE__);
    if (sf_command(file, SFC_GET_INSTRUMENT, &read_inst, sizeof(read_inst)) == SF_FALSE)
    {
        printf("\n\nLine %d : sf_command (SFC_GET_INSTRUMENT) failed.\n\n", __LINE__);
        exit(1);
        return;
    };
    check_log_buffer_or_die(file, __LINE__);
    sf_close(file);

    if ((filetype & SF_FORMAT_TYPEMASK) == SF_FORMAT_WAV)
    {
        /*
		 * For all the fields that WAV doesn't support, modify the
		 * write_inst struct to hold the default value that the WAV
		 * module should hold.
		 */
        write_inst.key_lo = write_inst.velocity_lo = 0;
        write_inst.key_hi = write_inst.velocity_hi = 127;
        write_inst.gain = 1;
    };

    if ((filetype & SF_FORMAT_TYPEMASK) == SF_FORMAT_XI)
    {
        /*
		 * For all the fields that XI doesn't support, modify the
		 * write_inst struct to hold the default value that the XI
		 * module should hold.
		 */
        write_inst.basenote = 0;
        write_inst.detune = 0;
        write_inst.key_lo = write_inst.velocity_lo = 0;
        write_inst.key_hi = write_inst.velocity_hi = 127;
        write_inst.gain = 1;
    };

    if (memcmp(&write_inst, &read_inst, sizeof(write_inst)) != 0)
    {
        printf("\n\nLine %d : instrument comparison failed.\n\n", __LINE__);
        printf("W  Base Note : %u\n"
               "   Detune    : %u\n"
               "   Low  Note : %u\tHigh Note : %u\n"
               "   Low  Vel. : %u\tHigh Vel. : %u\n"
               "   Gain      : %d\tCount     : %d\n"
               "   mode      : %d\n"
               "   start     : %d\tend       : %d\tcount  :%d\n"
               "   mode      : %d\n"
               "   start     : %d\tend       : %d\tcount  :%d\n\n",
               write_inst.basenote, write_inst.detune, write_inst.key_lo, write_inst.key_hi, write_inst.velocity_lo,
               write_inst.velocity_hi, write_inst.gain, write_inst.loop_count, write_inst.loops[0].mode, write_inst.loops[0].start,
               write_inst.loops[0].end, write_inst.loops[0].count, write_inst.loops[1].mode, write_inst.loops[1].start,
               write_inst.loops[1].end, write_inst.loops[1].count);
        printf("R  Base Note : %u\n"
               "   Detune    : %u\n"
               "   Low  Note : %u\tHigh Note : %u\n"
               "   Low  Vel. : %u\tHigh Vel. : %u\n"
               "   Gain      : %d\tCount     : %d\n"
               "   mode      : %d\n"
               "   start     : %d\tend       : %d\tcount  :%d\n"
               "   mode      : %d\n"
               "   start     : %d\tend       : %d\tcount  :%d\n\n",
               read_inst.basenote, read_inst.detune, read_inst.key_lo, read_inst.key_hi, read_inst.velocity_lo,
               read_inst.velocity_hi, read_inst.gain, read_inst.loop_count, read_inst.loops[0].mode, read_inst.loops[0].start,
               read_inst.loops[0].end, read_inst.loops[0].count, read_inst.loops[1].mode, read_inst.loops[1].start,
               read_inst.loops[1].end, read_inst.loops[1].count);

        if ((filetype & SF_FORMAT_TYPEMASK) != SF_FORMAT_XI)
            exit(1);
    };

    if (0)
        instrumet_rw_test(filename);

    unlink(filename);
    puts("ok");
}

static void sf_cue_point_set(SF_CUE_POINT *cue_point,
                             int32_t indx,
                             uint32_t position,
                             int32_t fcc_chunk,
                             int32_t chunk_start,
                             int32_t block_start,
                             uint32_t sample_offset,
                             const char *name)
{
    cue_point->indx = indx;
    cue_point->position = position;
    cue_point->fcc_chunk = fcc_chunk;
    cue_point->chunk_start = chunk_start;
    cue_point->block_start = block_start;
    cue_point->sample_offset = sample_offset;
    strncpy(cue_point->name, name, 256);
}

static void cue_points_test(const char *filename, int filetype)
{
    SF_CUE_POINT write_cue[2];
    SNDFILE *file;
    SF_INFO sfinfo;

    if (filetype == (SF_FORMAT_WAV | SF_FORMAT_PCM_16))
    {
        sf_cue_point_set(&write_cue[0], 1, 0, data_MARKER, 0, 0, 1, "");
        sf_cue_point_set(&write_cue[1], 2, 0, data_MARKER, 0, 0, 2, "");
    }
    else
    {
        sf_cue_point_set(&write_cue[0], 1, 0, data_MARKER, 0, 0, 1, "Cue1");
        sf_cue_point_set(&write_cue[1], 2, 0, data_MARKER, 0, 0, 2, "Cue2");
    }

    print_test_name("cue_point_test", filename);

    sfinfo.samplerate = 11025;
    sfinfo.format = filetype;
    sfinfo.channels = 1;

    file = test_open_file_or_die(filename, SFM_WRITE, &sfinfo, __LINE__);
    if (sf_command(file, SFC_SET_CUE_POINTS, write_cue, 2) == SF_FALSE)
    {
        printf("\n\nLine %d : sf_command (SFC_SET_CUE_POINTS) failed.\n\n", __LINE__);
        exit(1);
    };
    test_write_double_or_die(file, 0, double_data, BUFFER_LEN, __LINE__);
    sf_close(file);

    SF_CUE_POINT read_cue[2] = { 0 };
    file = test_open_file_or_die(filename, SFM_READ, &sfinfo, __LINE__);
    if (sf_command(file, SFC_GET_CUE_POINTS, read_cue, 2) == SF_FALSE)
    {
        printf("\n\nLine %d : sf_command (SFC_SET_CUE_POINTS) failed.\n\n", __LINE__);
        exit(1);
        return;
    };
    check_log_buffer_or_die(file, __LINE__);
    sf_close(file);

    if (memcmp(&write_cue, &read_cue, sizeof(write_cue)) != 0)
    {
        printf("\n\nLine %d : cue comparison failed.\n\n", __LINE__);
        printf("W  indx          : %d\n"
               "   position      : %u\n"
               "   fcc_chunk     : %x\n"
               "   chunk_start   : %d\n"
               "   block_start   : %d\n"
               "   sample_offset : %u\n"
               "   name          : %s\n"
               "   indx          : %d\n"
               "   position      : %u\n"
               "   fcc_chunk     : %x\n"
               "   chunk_start   : %d\n"
               "   block_start   : %d\n"
               "   sample_offset : %u\n"
               "   name          : %s\n",
               write_cue[0].indx, write_cue[0].position,
               write_cue[0].fcc_chunk, write_cue[0].chunk_start, write_cue[0].block_start,
               write_cue[0].sample_offset, write_cue[0].name, write_cue[1].indx,
               write_cue[1].position, write_cue[1].fcc_chunk, write_cue[1].chunk_start,
               write_cue[1].block_start, write_cue[1].sample_offset, write_cue[1].name);
        printf("R  indx          : %d\n"
               "   position      : %u\n"
               "   fcc_chunk     : %x\n"
               "   chunk_start   : %d\n"
               "   block_start   : %d\n"
               "   sample_offset : %u\n"
               "   name          : %s\n"
               "   indx          : %d\n"
               "   position      : %u\n"
               "   fcc_chunk     : %x\n"
               "   chunk_start   : %d\n"
               "   block_start   : %d\n"
               "   sample_offset : %u\n"
               "   name          : %s\n",
               read_cue[0].indx, read_cue[0].position, read_cue[0].fcc_chunk,
               read_cue[0].chunk_start, read_cue[0].block_start, read_cue[0].sample_offset,
               read_cue[0].name, read_cue[1].indx, read_cue[1].position,
               read_cue[1].fcc_chunk, read_cue[1].chunk_start, read_cue[1].block_start,
               read_cue[1].sample_offset, read_cue[1].name);

        exit(1);
    };

    unlink(filename);
    puts("ok");
}

static void current_sf_info_test(const char *filename)
{
    SNDFILE *outfile, *infile;
    SF_INFO outinfo, ininfo;

    print_test_name("current_sf_info_test", filename);

    outinfo.samplerate = 44100;
    outinfo.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_16);
    outinfo.channels = 1;
    outinfo.frames = 0;

    outfile = test_open_file_or_die(filename, SFM_WRITE, &outinfo, __LINE__);
    sf_command(outfile, SFC_SET_UPDATE_HEADER_AUTO, NULL, 0);

    exit_if_true(outinfo.frames != 0, "\n\nLine %d : Initial sfinfo.frames is not zero.\n\n", __LINE__);

    test_write_double_or_die(outfile, 0, double_data, BUFFER_LEN, __LINE__);
    sf_command(outfile, SFC_GET_CURRENT_SF_INFO, &outinfo, sizeof(outinfo));

    exit_if_true(outinfo.frames != BUFFER_LEN, "\n\nLine %d : Initial sfinfo.frames (%" PRId64 ") should be %d.\n\n", __LINE__,
                 outinfo.frames, BUFFER_LEN);

    /* Read file making sure no channel map exists. */
    memset(&ininfo, 0, sizeof(ininfo));
    infile = test_open_file_or_die(filename, SFM_READ, &ininfo, __LINE__);

    test_write_double_or_die(outfile, 0, double_data, BUFFER_LEN, __LINE__);

    sf_command(infile, SFC_GET_CURRENT_SF_INFO, &ininfo, sizeof(ininfo));

    exit_if_true(ininfo.frames != BUFFER_LEN, "\n\nLine %d : Initial sfinfo.frames (%" PRId64 ") should be %d.\n\n", __LINE__,
                 ininfo.frames, BUFFER_LEN);

    sf_close(outfile);
    sf_close(infile);

    unlink(filename);
    puts("ok");
}

static void channel_map_test(const char *filename, int filetype)
{
    SNDFILE *file;
    SF_INFO sfinfo;
    int channel_map_read[4],
        channel_map_write[4] = {SF_CHANNEL_MAP_LEFT, SF_CHANNEL_MAP_RIGHT, SF_CHANNEL_MAP_LFE, SF_CHANNEL_MAP_REAR_CENTER};

    print_test_name("channel_map_test", filename);

    memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.samplerate = 11025;
    sfinfo.format = filetype;
    sfinfo.channels = ARRAY_LEN(channel_map_read);

    switch (filetype & SF_FORMAT_TYPEMASK)
    {
    /* WAVEX and RF64 have a default channel map, even if you don't specify one. */
    case SF_FORMAT_WAVEX:
    case SF_FORMAT_RF64:
        /* Write file without channel map. */
        file = test_open_file_or_die(filename, SFM_WRITE, &sfinfo, __LINE__);
        test_write_double_or_die(file, 0, double_data, BUFFER_LEN, __LINE__);
        sf_close(file);

        /* Read file making default channel map exists. */
        file = test_open_file_or_die(filename, SFM_READ, &sfinfo, __LINE__);
        exit_if_true(sf_command(file, SFC_GET_CHANNEL_MAP_INFO, channel_map_read, sizeof(channel_map_read)) == SF_FALSE,
                     "\n\nLine %d : sf_command (SFC_GET_CHANNEL_MAP_INFO) should not have failed.\n\n",
                     __LINE__);
        check_log_buffer_or_die(file, __LINE__);
        sf_close(file);
        break;

    default:
        break;
    };

    /* Write file with a channel map. */
    file = test_open_file_or_die(filename, SFM_WRITE, &sfinfo, __LINE__);
    exit_if_true(sf_command(file, SFC_SET_CHANNEL_MAP_INFO, channel_map_write, sizeof(channel_map_write)) == SF_FALSE,
                 "\n\nLine %d : sf_command (SFC_SET_CHANNEL_MAP_INFO) failed.\n\n", __LINE__);
    test_write_double_or_die(file, 0, double_data, BUFFER_LEN, __LINE__);
    sf_close(file);

    /* Read file making sure no channel map exists. */
    file = test_open_file_or_die(filename, SFM_READ, &sfinfo, __LINE__);
    exit_if_true(sf_command(file, SFC_GET_CHANNEL_MAP_INFO, channel_map_read, sizeof(channel_map_read)) != SF_TRUE,
                 "\n\nLine %d : sf_command (SFC_GET_CHANNEL_MAP_INFO) failed.\n\n", __LINE__);
    check_log_buffer_or_die(file, __LINE__);
    sf_close(file);

    exit_if_true(memcmp(channel_map_read, channel_map_write, sizeof(channel_map_read)) != 0,
                 "\n\nLine %d : Channel map read does not match channel map written.\n\n",
                 __LINE__);

    unlink(filename);
    puts("ok");
}

static void raw_needs_endswap_test(const char *filename, int filetype)
{
    static int subtypes[] = {SF_FORMAT_FLOAT, SF_FORMAT_DOUBLE, SF_FORMAT_PCM_16, SF_FORMAT_PCM_24, SF_FORMAT_PCM_32};
    SNDFILE *file;
    SF_INFO sfinfo;
    unsigned k;
    int needs_endswap;

    print_test_name(__func__, filename);

    for (k = 0; k < ARRAY_LEN(subtypes); k++)
    {
        if (filetype == (SF_ENDIAN_LITTLE | SF_FORMAT_AIFF))
            switch (subtypes[k])
            {
            /* Little endian AIFF does not AFAIK support fl32 and fl64. */
            case SF_FORMAT_FLOAT:
            case SF_FORMAT_DOUBLE:
                continue;
            default:
                break;
            };

        memset(&sfinfo, 0, sizeof(sfinfo));
        sfinfo.samplerate = 11025;
        sfinfo.format = filetype | subtypes[k];
        sfinfo.channels = 1;

        file = test_open_file_or_die(filename, SFM_WRITE, &sfinfo, __LINE__);
        test_write_double_or_die(file, 0, double_data, BUFFER_LEN, __LINE__);
        sf_close(file);

        memset(&sfinfo, 0, sizeof(sfinfo));
        file = test_open_file_or_die(filename, SFM_READ, &sfinfo, __LINE__);

        needs_endswap = sf_command(file, SFC_RAW_DATA_NEEDS_ENDSWAP, NULL, 0);

        switch (filetype)
        {
        case SF_FORMAT_WAV:
        case SF_FORMAT_WAVEX:
        case SF_FORMAT_AIFF | SF_ENDIAN_LITTLE:
            exit_if_true(needs_endswap != CPU_IS_BIG_ENDIAN,
                         "\n\nLine %d : SFC_RAW_DATA_NEEDS_ENDSWAP failed for "
                         "(%d | %d).\n\n",
                         __LINE__, filetype, k);
            break;

        case SF_FORMAT_AIFF:
        case SF_FORMAT_WAV | SF_ENDIAN_BIG:
            exit_if_true(needs_endswap != CPU_IS_LITTLE_ENDIAN,
                         "\n\nLine %d : SFC_RAW_DATA_NEEDS_ENDSWAP failed for "
                         "(%d | %d).\n\n",
                         __LINE__, filetype, k);
            break;

        default:
            printf("\n\nLine %d : bad format value %d.\n\n", __LINE__, filetype);
            exit(1);
            break;
        };

        sf_close(file);
    };

    unlink(filename);
    puts("ok");
}
