/*
** Copyright (C) 2007-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#else
#include "sf_unistd.h"
#endif

#include <string.h>
#include <errno.h>

#include "common.h"
#include "sfendian.h"

#include "test_main.h"

static unsigned char float_le_mono[] = {
    0x36, 0x86, 0x21, 0x44, 0xB5, 0xB4, 0x49, 0x44, 0xA2, 0xC0, 0x71, 0x44,
    0x7B, 0xD1, 0x8C, 0x44, 0x54, 0xAA, 0xA0, 0x44, 0x60, 0x67, 0xB4, 0x44,
    0x22, 0x05, 0xC8, 0x44, 0x29, 0x80, 0xDB, 0x44, 0x04, 0xD5, 0xEE, 0x44,
    0x27, 0x00, 0x01, 0x45, 0x50, 0x7F, 0x0A, 0x45, 0x53, 0xE6, 0x13, 0x45,
    0x85, 0x33, 0x1D, 0x45, 0x43, 0x65, 0x26, 0x45, 0xEC, 0x79, 0x2F, 0x45,
    0xE3, 0x6F, 0x38, 0x45, 0x98, 0x45, 0x41, 0x45, 0x77, 0xF9, 0x49, 0x45,
    0xF6, 0x89, 0x52, 0x45, 0x8F, 0xF5, 0x5A, 0x45, 0xC9, 0x3A, 0x63, 0x45,
    0x28, 0x58, 0x6B, 0x45, 0x3C, 0x4C, 0x73, 0x45, 0x9F, 0x15, 0x7B, 0x45,
    0x75, 0x59, 0x81, 0x45, 0x64, 0x11, 0x85, 0x45, 0xF1, 0xB1, 0x88, 0x45,
    0x78, 0x3A, 0x8C, 0x45, 0x58, 0xAA, 0x8F, 0x45, 0xF2, 0x00, 0x93, 0x45,
    0xB2, 0x3D, 0x96, 0x45, 0x01, 0x60, 0x99, 0x45, 0x50, 0x67, 0x9C, 0x45,
    0x15, 0x53, 0x9F, 0x45, 0xCC, 0x22, 0xA2, 0x45, 0xF0, 0xD5, 0xA4, 0x45,
    0x07, 0x6C, 0xA7, 0x45, 0x9C, 0xE4, 0xA9, 0x45, 0x3D, 0x3F, 0xAC, 0x45,
    0x7A, 0x7B, 0xAE, 0x45, 0xF2, 0x98, 0xB0, 0x45, 0x3C, 0x97, 0xB2, 0x45,
    0x02, 0x76, 0xB4, 0x45, 0xEC, 0x34, 0xB6, 0x45, 0xA8, 0xD3, 0xB7, 0x45,
    0xEB, 0x51, 0xB9, 0x45, 0x6F, 0xAF, 0xBA, 0x45, 0xF5, 0xEB, 0xBB, 0x45,
    0x41, 0x07, 0xBD, 0x45, 0x21, 0x01, 0xBE, 0x45, 0x64, 0xD9, 0xBE, 0x45,
    0xE3, 0x8F, 0xBF, 0x45, 0x7E, 0x24, 0xC0, 0x45, 0x15, 0x97, 0xC0, 0x45,
    0x92, 0xE7, 0xC0, 0x45, 0xE8, 0x15, 0xC1, 0x45, 0x7E, 0x24, 0xC0, 0x45,
    0x15, 0x97, 0xC0, 0x45, 0x92, 0xE7, 0xC0, 0x45, 0xE8, 0x15, 0xC1, 0x45,
    0x7E, 0x24, 0xC0, 0x45, 0x15, 0x97, 0xC0, 0x45, 0x92, 0xE7, 0xC0, 0x45,
    0xE8, 0x15, 0xC1, 0x45,
};

static unsigned char int24_32_le_stereo[] = {
    0x00, 0xE7, 0xFB, 0xFF, 0x00, 0x7C, 0xFD, 0xFF, 0x00, 0xA2, 0xFC, 0xFF,
    0x00, 0x2B, 0xFC, 0xFF, 0x00, 0xF3, 0xFD, 0xFF, 0x00, 0x19, 0xFB, 0xFF,
    0x00, 0xA5, 0xFE, 0xFF, 0x00, 0x8D, 0xFA, 0xFF, 0x00, 0x91, 0xFF, 0xFF,
    0x00, 0xB5, 0xFA, 0xFF, 0x00, 0x91, 0x00, 0x00, 0x00, 0x5E, 0xFB, 0xFF,
    0x00, 0xD9, 0x01, 0x00, 0x00, 0x82, 0xFB, 0xFF, 0x00, 0xDF, 0x03, 0x00,
    0x00, 0x44, 0xFC, 0xFF, 0x00, 0x1C, 0x05, 0x00, 0x00, 0x77, 0xFC, 0xFF,
    0x00, 0x8D, 0x06, 0x00, 0x00, 0x4F, 0xFC, 0xFF, 0x00, 0x84, 0x07, 0x00,
    0x00, 0x74, 0xFC, 0xFF, 0x00, 0x98, 0x08, 0x00, 0x00, 0x33, 0xFD, 0xFF,
    0x00, 0xB9, 0x09, 0x00, 0x00, 0x48, 0xFF, 0xFF, 0x00, 0xD1, 0x0A, 0x00,
    0x00, 0x10, 0x02, 0x00, 0x00, 0x28, 0x0C, 0x00, 0x00, 0xA2, 0x05, 0x00,
    0x00, 0xA7, 0x0C, 0x00, 0x00, 0x45, 0x08, 0x00, 0x00, 0x44, 0x0D, 0x00,
    0x00, 0x1A, 0x0A, 0x00, 0x00, 0x65, 0x0D, 0x00, 0x00, 0x51, 0x0B, 0x00,
    0x00, 0x8B, 0x0D, 0x00, 0x00, 0x18, 0x0B, 0x00, 0x00, 0x37, 0x0E, 0x00,
    0x00, 0x24, 0x0B, 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0xDD, 0x0A, 0x00,
    0x00, 0x83, 0x10, 0x00, 0x00, 0x31, 0x0A, 0x00, 0x00, 0x07, 0x12, 0x00,
    0x00, 0xC0, 0x08, 0x00, 0x00, 0xF7, 0x12, 0x00, 0x00, 0x47, 0x06, 0x00,
    0x00, 0xDD, 0x12, 0x00, 0x00, 0x6A, 0x03, 0x00, 0x00, 0xD5, 0x11, 0x00,
    0x00, 0x99, 0x00, 0x00, 0x00, 0x01, 0x10, 0x00, 0x00, 0xC5, 0xFE, 0xFF,
    0x00, 0xF4, 0x0D, 0x00, 0x00, 0x97, 0xFD, 0xFF, 0x00, 0x62, 0x0B, 0x00,
    0x00, 0x75, 0xFC, 0xFF, 0x00, 0xE9, 0x08, 0x00, 0x00, 0xC0, 0xFB, 0xFF,
    0x00, 0x80, 0x06, 0x00, 0x00, 0x3C, 0xFB, 0xFF, 0x00, 0xDA, 0x03, 0x00,
    0x00, 0xE4, 0xFA, 0xFF, 0x00, 0xEB, 0x01, 0x00, 0x00, 0x21, 0xFB, 0xFF,
    0x00, 0x20, 0x00, 0x00, 0x00, 0xE7, 0xFB, 0xFF,
};

void test_audio_detect(void)
{
    SF_PRIVATE psf;
    AUDIO_DETECT ad;
    int errors = 0;

    print_test_name("Testing audio detect");

    memset(&psf, 0, sizeof(psf));

    ad.endianness = SF_ENDIAN_LITTLE;
    ad.channels = 1;
    if (audio_detect(&psf, &ad, float_le_mono, sizeof(float_le_mono)) !=
        SF_FORMAT_FLOAT)
    {
        puts("    float_le_mono");
        errors++;
    };

    ad.endianness = SF_ENDIAN_LITTLE;
    ad.channels = 2;
    if (audio_detect(&psf, &ad, int24_32_le_stereo,
                     sizeof(int24_32_le_stereo)) != SF_FORMAT_PCM_32)
    {
        if (errors == 0)
            puts("\nFailed tests :\n");
        puts("    int24_32_le_stereo");
        errors++;
    };

    if (errors != 0)
    {
        printf("\n    Errors : %d\n\n", errors);
        exit(1);
    };

    puts("ok");

    return;
}
