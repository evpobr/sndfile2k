/*
** Copyright (C) 2006-2016 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "common.h"
#include "test_main.h"
#include "ref_ptr.h"

/*
** This is a bit rough, but it is the nicest way to do it.
*/

#define cmp_test(line, ival, tval, str) \
    \
do \
if(ival != tval)                        \
    \
{                                  \
        printf(str, line, ival, tval);  \
        exit(1);                        \
    \
}                                  \
    while (0)                           \
        ;

static void conversion_test(char endian)
{
    std::unique_ptr<SndFile> psf;
    const char *filename = "conversion.bin";
    int64_t i64 = INT64_C(0x0123456789abcdef), t64 = 0;
    char format_str[16];
    char test_name[64];
    char i8 = 12, t8 = 0;
    short i16 = 0x123, t16 = 0;
    int i24 = 0x23456, t24 = 0;
    int i32 = 0x0a0b0c0d, t32 = 0;
    int bytes;

    snprintf(format_str, sizeof(format_str), "%c12348", endian);

    snprintf(test_name, sizeof(test_name), "Testing %s conversions", endian == 'e' ? "little endian" : "big endian");
    print_test_name(test_name);

    SF_INFO sfinfo = { 0 };
    psf = std::unique_ptr<SndFile>(new SndFile());
    if (psf->open(filename, SFM_WRITE, &sfinfo) != 0)
    {
        printf("\n\nError : failed to open file '%s' for write.\n\n", filename);
        exit(1);
    };

    psf->binheader_writef(format_str, i8, i16, i24, i32, i64);
    psf->fwrite(psf->m_header.ptr, 1, psf->m_header.indx);
    psf.reset();

    sfinfo = { 0 };
    psf = std::unique_ptr<SndFile>(new SndFile());
    if (psf->open(filename, SFM_READ, &sfinfo) != 0)
    {
        printf("\n\nError : failed to open file '%s' for read.\n\n", filename);
        exit(1);
    };

    bytes = psf->binheader_readf(format_str, &t8, &t16, &t24, &t32, &t64);

    if (bytes != 18)
    {
        printf("\n\nLine %d : read %d bytes.\n\n", __LINE__, bytes);
        exit(1);
    };

    cmp_test(__LINE__, i8, t8, "\n\nLine %d : 8 bit int failed %d -> %d.\n\n");
    cmp_test(__LINE__, i16, t16, "\n\nLine %d : 16 bit int failed 0x%x -> 0x%x.\n\n");
    cmp_test(__LINE__, i24, t24, "\n\nLine %d : 24 bit int failed 0x%x -> 0x%x.\n\n");
    cmp_test(__LINE__, i32, t32, "\n\nLine %d : 32 bit int failed 0x%x -> 0x%x.\n\n");
    cmp_test(__LINE__, i64, t64, "\n\nLine %d : 64 bit int failed 0x%" PRIx64 "x -> 0x%" PRIx64 "x.\n\n");

    remove(filename);
    puts("ok");
}

void test_conversions(void)
{
    conversion_test('E');
    conversion_test('e');
}
