/*
** Copyright (C) 1999-2018 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "sf_unistd.h"

#include "sndfile2k/sndfile2k.h"

#include "utils.h"

#define BUFFER_SIZE (1 << 15)
#define SHORT_BUFFER (256)

static void error_number_test(void)
{
    const char *noerror, *errstr;
    int k;

    print_test_name("error_number_test", "");

    noerror = sf_error_number(0);

    for (k = 1; k < 300; k++)
    {
        errstr = sf_error_number(k);

        /* Test for termination condition. */
        if (errstr == noerror)
            break;

        /* Test for error. */
        if (strstr(errstr, "This is a bug in libsndfile."))
        {
            printf("\n\nError : error number %d : %s\n\n\n", k, errstr);
            exit(1);
        };
    };

    puts("ok");
    return;
}

static void error_value_test(void)
{
    static unsigned char aiff_data[0x1b0] = {
        'F', 'O', 'R', 'M', 0x00, 0x00, 0x01, 0xA8, /* FORM length */

        'A', 'I', 'F', 'C',
    };

    const char *filename = "error.aiff";
    SNDFILE *file;
    SF_INFO sfinfo;
    int error_num;

    print_test_name("error_value_test", filename);

    dump_data_to_file(filename, aiff_data, sizeof(aiff_data));

    memset(&sfinfo, 0, sizeof(sfinfo));

    error_num = sf_open(filename, SFM_READ, &sfinfo, &file);
    if (file != NULL)
    {
        printf("\n\nLine %d : Should not have been able to open this file.\n\n", __LINE__);
        sf_close(file);
        exit(1);
    };

    if (error_num <= 1 || error_num > 300)
    {
        printf("\n\nLine %d : Should not have had an error number of %d.\n\n", __LINE__, error_num);
        exit(1);
    };

    remove(filename);
    puts("ok");
    return;
}

static void no_file_test(const char *filename)
{
    SNDFILE *sndfile;
    SF_INFO sfinfo;

    print_test_name(__func__, filename);

    unlink(filename);

    memset(&sfinfo, 0, sizeof(sfinfo));
    sf_open(filename, SFM_READ, &sfinfo, &sndfile);

    exit_if_true(sndfile != NULL, "\n\nLine %d : should not have received a valid SNDFILE* pointer.\n", __LINE__);

    unlink(filename);
    puts("ok");
}

static void zero_length_test(const char *filename)
{
    SNDFILE *sndfile;
    SF_INFO sfinfo;
    FILE *file;

    print_test_name(__func__, filename);

    /* Create a zero length file. */
    file = fopen(filename, "w");
    exit_if_true(file == NULL, "\n\nLine %d : fopen ('%s') failed.\n", __LINE__, filename);
    fclose(file);

    memset(&sfinfo, 0, sizeof(sfinfo));
    sf_open(filename, SFM_READ, &sfinfo, &sndfile);

    exit_if_true(sndfile != NULL, "\n\nLine %d : should not have received a valid SNDFILE* pointer.\n", __LINE__);

    exit_if_true(0 && sf_error(NULL) != SF_ERR_UNRECOGNISED_FORMAT, "\n\nLine %3d : Error : %s\n       should be : %s\n", __LINE__,
                 sf_strerror(NULL), sf_error_number(SF_ERR_UNRECOGNISED_FORMAT));

    unlink(filename);
    puts("ok");
}

static void bad_wav_test(const char *filename)
{
    SNDFILE *sndfile;
    SF_INFO sfinfo;

    FILE *file;
    const char data[] = "RIFF    WAVEfmt            ";

    print_test_name(__func__, filename);

    if ((file = fopen(filename, "w")) == NULL)
    {
        printf("\n\nLine %d : fopen returned NULL.\n", __LINE__);
        exit(1);
    };

    exit_if_true(fwrite(data, sizeof(data), 1, file) != 1, "\n\nLine %d : fwrite failed.\n", __LINE__);
    fclose(file);

    memset(&sfinfo, 0, sizeof(sfinfo));
    sf_open(filename, SFM_READ, &sfinfo, &sndfile);

    if (sndfile)
    {
        printf("\n\nLine %d : should not have received a valid SNDFILE* "
               "pointer.\n",
               __LINE__);
        exit(1);
    };

    unlink(filename);
    puts("ok");
}

static void unrecognised_test(void)
{
    const char *filename = "unrecognised.bin";

    print_test_name(__func__, filename);

    FILE *file = fopen(filename, "wb");
    exit_if_true(file == NULL, "\n\nLine %d : fopen ('%s') failed : %s\n", __LINE__, filename, strerror(errno));
    fputs("Unrecognised file", file);
    fclose(file);

    SF_INFO sfinfo;
    memset(&sfinfo, 0, sizeof(sfinfo));
    SNDFILE *sndfile;
    int errnum = sf_open(filename, SFM_READ, &sfinfo, &sndfile);

    exit_if_true(sndfile != NULL, "\n\nLine %d : SNDFILE* pointer (%p) should ne NULL.\n", __LINE__, sndfile);

    exit_if_true(errnum != SF_ERR_UNRECOGNISED_FORMAT, "\n\nLine %d : error (%d) should have been SF_ERR_UNRECOGNISED_FORMAT (%d).\n",
                 __LINE__, errnum, SF_ERR_UNRECOGNISED_FORMAT);

    unlink(filename);
    puts("ok");
}

int main(void)
{
    error_number_test();
    error_value_test();

    no_file_test("no_file.wav");
    zero_length_test("zero_length.wav");
    bad_wav_test("bad_wav.wav");

    unrecognised_test();

    return 0;
}
