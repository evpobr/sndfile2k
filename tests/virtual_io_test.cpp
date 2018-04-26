/*
** Copyright (C) 1999-2011 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#include <fcntl.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>

#include "sndfile2k/sndfile2k.h"

#include "utils.h"

#include "ref_ptr.h"

static void vio_test(const char *fname, int format);

int main(void)
{
    vio_test("vio_pcm16.wav", SF_FORMAT_WAV | SF_FORMAT_PCM_16);
    vio_test("vio_pcm24.aiff", SF_FORMAT_AIFF | SF_FORMAT_PCM_24);
    vio_test("vio_float.au", SF_FORMAT_AU | SF_FORMAT_FLOAT);
    vio_test("vio_pcm24.paf", SF_FORMAT_PAF | SF_FORMAT_PCM_24);

    return 0;
} /* main */

/*==============================================================================
*/

bool flush_done = false;

class MemoryStream: public SF_STREAM
{
    unsigned long m_ref = 0;

    sf_count_t m_offset = 0;
    sf_count_t m_length = 0;
    unsigned char m_data[16 * 1024] = {0};

public:

    unsigned long ref() override
    {
        unsigned long ref_counter = 0;

        ref_counter = ++m_ref;

        return ref_counter;
    }

    void unref() override
    {
        m_ref--;
        if (m_ref == 0)
            delete this;
    }

    sf_count_t get_filelen() override
    {
        return m_length;
    }

    sf_count_t seek(sf_count_t offset, int whence) override
    {
        switch (whence)
        {
        case SEEK_SET:
            m_offset = offset;
            break;

        case SEEK_CUR:
            m_offset = m_offset + offset;
            break;

        case SEEK_END:
            m_offset = m_length + offset;
            break;
        default:
            break;
        };

        return m_offset;
    }

    sf_count_t read(void *ptr, sf_count_t count) override
    {
        /*
        **	This will brack badly for files over 2Gig in length, but
        **	is sufficient for testing.
        */
        if (m_offset + count > m_length)
            count = m_length - m_offset;

        memcpy(ptr, m_data + m_offset, count);
        m_offset += count;

        return count;
    }

    sf_count_t write(const void *ptr, sf_count_t count) override
    {
        /*
        **	This will break badly for files over 2Gig in length, but
        **	is sufficient for testing.
        */
        if (m_offset >= SIGNED_SIZEOF(m_data))
            return 0;

        if (m_offset + count > SIGNED_SIZEOF(m_data))
            count = sizeof(m_data) - m_offset;

        memcpy(m_data + m_offset, ptr, (size_t)count);
        m_offset += count;

        if (m_offset > m_length)
            m_length = m_offset;

        return count;
    }

    sf_count_t tell() override
    {
        return m_offset;
    }

    void flush() override
    {
        flush_done = true;
    }

    int set_filelen(sf_count_t len) override
    {
        return 0;
    }

};

/*==============================================================================
*/

static void gen_short_data(short *data, int len, int start)
{
    int k;

    for (k = 0; k < len; k++)
        data[k] = start + k;
} /* gen_short_data */

static void check_short_data(short *data, int len, int start, int line)
{
    int k;

    for (k = 0; k < len; k++)
        if (data[k] != start + k)
        {
            printf("\n\nLine %d : data [%d] = %d (should be %d).\n\n", line, k,
                   data[k], start + k);
            exit(1);
        };
} /* gen_short_data */

/*------------------------------------------------------------------------------
*/

static void vio_test(const char *fname, int format)
{
    static short data[256];

    sf::ref_ptr<SF_STREAM> vio;
    SNDFILE *file;
    SF_INFO sfinfo;

    print_test_name("virtual i/o test", fname);

    memset(&sfinfo, 0, sizeof(sfinfo));
    sfinfo.format = format;
    sfinfo.channels = 2;
    sfinfo.samplerate = 44100;

    MemoryStream *ms = new MemoryStream();
    vio.copy(ms);
    vio->ref();

    int error = sf_open_stream(vio.get(), SFM_WRITE, &sfinfo, &file);
    if (error != SF_ERR_NO_ERROR)
    {
        printf("\n\nLine %d : sf_open_write failed with error : ", __LINE__);
        fflush(stdout);
        puts(sf_strerror(NULL));
        exit(1);
    };

    if (vio->get_filelen() < 0)
    {
        printf("\n\nLine %d : vfget_filelen returned negative length.\n\n",
               __LINE__);
        exit(1);
    };

    gen_short_data(data, ARRAY_LEN(data), 0);
    sf_write_short(file, data, ARRAY_LEN(data));

    gen_short_data(data, ARRAY_LEN(data), 1);
    sf_write_short(file, data, ARRAY_LEN(data));

    gen_short_data(data, ARRAY_LEN(data), 2);
    sf_write_short(file, data, ARRAY_LEN(data));

    /* Test flush */
    sf_write_sync(file);
    sf_close(file);
    if (!flush_done)
        printf("\n\nLine %d : sf_write_sync failed.", __LINE__);


    /* Now test read. */
    vio->seek(SF_SEEK_SET, 0);
    memset(&sfinfo, 0, sizeof(sfinfo));

    error = sf_open_stream(vio.get(), SFM_READ, &sfinfo, &file);
    if (error != SF_ERR_NO_ERROR)
    {
        printf("\n\nLine %d : sf_open_write failed with error : ", __LINE__);
        fflush(stdout);
        puts(sf_strerror(NULL));

        exit(1);
    };

    sf_read_short(file, data, ARRAY_LEN(data));
    check_short_data(data, ARRAY_LEN(data), 0, __LINE__);

    sf_read_short(file, data, ARRAY_LEN(data));
    check_short_data(data, ARRAY_LEN(data), 1, __LINE__);

    sf_read_short(file, data, ARRAY_LEN(data));
    check_short_data(data, ARRAY_LEN(data), 2, __LINE__);

    sf_close(file);
    vio->unref();

    puts("ok");
} /* vio_test */
