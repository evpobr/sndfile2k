/*
** Copyright (C) 2008-2016 Erik de Castro Lopo <erikd@mega-nerd.com>
** Copyright (C) 2008-2010 George Blood Audio
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "sndfile2k/sndfile2k.h"

#include "common.h"

#define BUFFER_LEN (1 << 16)

static void usage_exit(const char *progname, int exit_code);
static void missing_param(const char *option);
static void read_localtime(struct tm *timedata);

int main(int argc, char *argv[])
{
    METADATA_INFO info;
    struct tm timedata;
    const char *progname;
    const char *filenames[2] = {NULL, NULL};
    char date[128];
    int k;

    /* Store the program name. */
    progname = program_name(argv[0]);

    /* Check if we've been asked for help. */
    if (argc < 3 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
        usage_exit(progname, 0);

    /* Set all fields of the struct to zero bytes. */
    memset(&info, 0, sizeof(info));

    /* Get the time in case we need it later. */
    read_localtime(&timedata);

    for (k = 1; k < argc; k++)
    {
        if (argv[k][0] != '-')
        {
            if (filenames[0] == NULL)
                filenames[0] = argv[k];
            else if (filenames[1] == NULL)
                filenames[1] = argv[k];
            else
            {
                printf("Error : Already have two file names on the command "
                       "line and then found '%s'.\n\n",
                       argv[k]);
                usage_exit(progname, 1);
            };
            continue;
        };

#define HANDLE_STR_ARG(cmd, field)      \
    if (strcmp(argv[k], cmd) == 0)      \
    {                                   \
        k++;                            \
        if (k == argc)                  \
            missing_param(argv[k - 1]); \
        info.field = argv[k];           \
        continue;                       \
    };

        HANDLE_STR_ARG("--str-comment", comment);
        HANDLE_STR_ARG("--str-title", title);
        HANDLE_STR_ARG("--str-copyright", copyright);
        HANDLE_STR_ARG("--str-artist", artist);
        HANDLE_STR_ARG("--str-date", date);
        HANDLE_STR_ARG("--str-album", album);
        HANDLE_STR_ARG("--str-license", license);

        if (strcmp(argv[k], "--str-auto-date") == 0)
        {
            snprintf(date, sizeof(date), "%04d-%02d-%02d", timedata.tm_year + 1900,
                     timedata.tm_mon + 1, timedata.tm_mday);

            info.date = strdup(date);
            continue;
        };

        printf("Error : Don't know what to do with command line arg '%s'.\n\n", argv[k]);
        usage_exit(progname, 1);
    };

    if (filenames[0] == NULL)
    {
        printf("Error : No input file specificed.\n\n");
        exit(1);
    };

    if (filenames[1] != NULL && strcmp(filenames[0], filenames[1]) == 0)
    {
        printf("Error : Input and output files are the same.\n\n");
        exit(1);
    };

    sfe_apply_metadata_changes(filenames, &info);

    return 0;
}

/*
 * Print version and usage.
 */

static void usage_exit(const char *progname, int exit_code)
{
    printf("\nUsage :\n\n"
           "  %s [options] <file>\n"
           "  %s [options] <input file> <output file>\n"
           "\n",
           progname, progname);

    puts("Where an option is made up of a pair of a field to set (one of\n"
         "the metadata fields below) and a string. Fields are\n"
         "as follows :\n");
    puts("    --str-comment            Set the metadata comment.\n"
         "    --str-title              Set the metadata title.\n"
         "    --str-copyright          Set the metadata copyright.\n"
         "    --str-artist             Set the metadata artist.\n"
         "    --str-date               Set the metadata date.\n"
         "    --str-album              Set the metadata album.\n"
         "    --str-license            Set the metadata license.\n");

    puts("There are also the following arguments which do not take a\n"
         "parameter :\n\n"
         "    --str-auto-date          Set the metadata date to current "
         "date.\n");

    puts("Most of the above operations can be done in-place on an existing\n"
         "file. If any operation cannot be performed, the application will\n"
         "exit with an appropriate error message.\n");

    printf("Using %s.\n\n", sf_version_string());
    exit(exit_code);
}

static void missing_param(const char *option)
{
    printf("Error : Option '%s' needs a parameter but doesn't seem to have "
           "one.\n\n",
           option);
    exit(1);
}

static void read_localtime(struct tm *timedata)
{
    time_t current;

    time(&current);
    memset(timedata, 0, sizeof(struct tm));

#if defined(HAVE_LOCALTIME_R)
    /* If the re-entrant version is available, use it. */
    localtime_r(&current, timedata);
#elif defined(HAVE_LOCALTIME)
    {
        struct tm *tmptr;
        /* Otherwise use the standard one and copy the data to local storage. */
        if ((tmptr = localtime(&current)) != NULL)
            memcpy(timedata, tmptr, sizeof(struct tm));
    }
#endif

    return;
}
