/*
** Copyright (C) 1999-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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

#include <config.h>

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include "sf_unistd.h"
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <memory.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include "sndfile2k/sndfile2k.h"
#include "sfendian.h"
#include "common.h"
#include "sndfile_error.h"

#include <stdlib.h>
#include <stdio.h>

#include <cassert>
#include <algorithm>

using namespace std;

#define INITIAL_HEADER_SIZE (256)

SndFile::SndFile()
{
    m_unique_id = psf_rand_int32();
        m_header.ptr = (unsigned char *)calloc(1, INITIAL_HEADER_SIZE);
    if (!m_header.ptr)
        throw sf::sndfile_error(SFE_MALLOC_FAILED);
    m_header.len = INITIAL_HEADER_SIZE;
    seek_from_start = psf_default_seek;
}

int SndFile::open(const char *filename, SF_FILEMODE mode, SF_INFO *sfinfo)
{
    sf::ref_ptr<SF_STREAM> stream;
    m_error = psf_open_file_stream(filename, mode, stream.get_address_of());
    if (m_error == SFE_NO_ERROR)
        m_error = open(stream, mode, sfinfo);

    return m_error;
}

int SndFile::open(sf::ref_ptr<SF_STREAM> &stream, SF_FILEMODE mode, SF_INFO *sfinfo)
{
    if (m_is_open)
        return SFE_ALREADY_INITIALIZED;
    if (!stream)
        return SFE_BAD_VIRTUAL_IO;

    if (mode != SFM_READ &&
        mode != SFM_WRITE &&
        mode != SFM_RDWR)
        return SFE_BAD_OPEN_MODE;

    if (!sfinfo)
        return SFE_BAD_SF_INFO_PTR;

    m_stream = stream;
    m_stream->ref();
    m_mode = mode;
    m_last_op = m_mode;
    memcpy(&sf, sfinfo, sizeof(SF_INFO));

    sf.sections = 1;
    sf.seekable = SF_TRUE;

    /* Set bytewidth if known. */
    switch (SF_CODEC(sf.format))
    {
    case SF_FORMAT_PCM_S8:
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_ULAW:
    case SF_FORMAT_ALAW:
    case SF_FORMAT_DPCM_8:
        m_bytewidth = 1;
        break;

    case SF_FORMAT_PCM_16:
    case SF_FORMAT_DPCM_16:
        m_bytewidth = 2;
        break;

    case SF_FORMAT_PCM_24:
        m_bytewidth = 3;
        break;

    case SF_FORMAT_PCM_32:
    case SF_FORMAT_FLOAT:
        m_bytewidth = 4;
        break;

    case SF_FORMAT_DOUBLE:
        m_bytewidth = 8;
        break;
    };

    m_filelength = m_stream->get_filelen();
    if (m_filelength == SF_COUNT_MAX)
        log_printf("Length : unknown\n");
    else
        log_printf("Length : %D\n", m_filelength);

    m_stream->seek(0, SF_SEEK_SET);

    m_is_open = true;
    return SFE_NO_ERROR;
}

bool SndFile::is_open() const
{
    return m_is_open;
}

void SndFile::close()
{
    if (codec_close)
    {
        m_error = codec_close(this);
        /* To prevent it being called in psf->container_close(). */
        codec_close = nullptr;
    };

    if (container_close)
        m_error = container_close(this);

    if (m_stream)
        m_stream->unref();

    /* For an ISO C compliant implementation it is ok to free a NULL pointer. */
    free(m_header.ptr);
    free(m_container_data);
    free(m_codec_data);
    free(m_interleave);
    free(m_dither);
    free(m_loop_info);
    free(m_instrument);
    m_cues.clear();
    m_channel_map.clear();
    free(m_format_desc);
    free(m_strings.storage);

    if (m_wchunks.chunks)
        for (uint32_t k = 0; k < m_wchunks.used; k++)
            free(m_wchunks.chunks[k].data);
    free(m_rchunks.chunks);
    free(m_wchunks.chunks);
    free(m_iterator);
    m_is_open = false;
}

SndFile::~SndFile()
{
    close();
}

int SndFile::bump_header_allocation(sf_count_t needed)
{
    size_t newlen;
    size_t smallest = INITIAL_HEADER_SIZE;

    if (needed > m_header.len)
        newlen = 2 * std::max(needed, static_cast<sf_count_t>(smallest));
    else
        newlen = 2 * m_header.len;

    if (newlen > 100 * 1024)
    {
        log_printf("Request for header allocation of %D denied.\n", newlen);
        return 1;
    }

    void *ptr = realloc(m_header.ptr, newlen);
    if (!ptr)
    {
        log_printf("realloc (%p, %z) failed\n", m_header.ptr, newlen);
        m_error = SFE_MALLOC_FAILED;
        return 1;
    };

    /* Always zero-out new header memory to avoid un-initializer memory accesses. */
    if (newlen > m_header.len)
        memset((char *)ptr + m_header.len, 0, newlen - m_header.len);

    m_header.ptr = (unsigned char *)ptr;
    m_header.len = newlen;
    return 0;
}

/*
 * psf_log_printf allows libsndfile internal functions to print to an internal parselog which
 * can later be displayed.
 * The format specifiers are as for printf but without the field width and other modifiers.
 * Printing is performed to the parselog char array of the SndFile class.
 * Printing is done in such a way as to guarantee that the log never overflows the end of the
 * parselog array.
 */

void SndFile::log_putchar(char ch)
{
    if (m_parselog.indx < SIGNED_SIZEOF(m_parselog.buf) - 1)
    {
        m_parselog.buf[m_parselog.indx++] = ch;
        m_parselog.buf[m_parselog.indx] = 0;
    };
    return;
}

void SndFile::log_printf(const char *format, ...)
{
    va_list ap;
    uint32_t u;
    int d, tens, shift, width, width_specifier, left_align, slen;
    char c, *strptr, istr[5], lead_char, sign_char;

    va_start(ap, format);

    while ((c = *format++))
    {
        if (c != '%')
        {
            log_putchar(c);
            continue;
        };

        if (format[0] == '%') /* Handle %% */
        {
            log_putchar('%');
            format++;
            continue;
        };

        sign_char = 0;
        left_align = SF_FALSE;
        while (1)
        {
            switch (format[0])
            {
            case ' ':
            case '+':
                sign_char = format[0];
                format++;
                continue;

            case '-':
                left_align = SF_TRUE;
                format++;
                continue;

            default:
                break;
            };

            break;
        };

        if (format[0] == 0)
            break;

        lead_char = ' ';
        if (format[0] == '0')
            lead_char = '0';

        width_specifier = 0;
        while ((c = *format++) && isdigit(c))
            width_specifier = width_specifier * 10 + (c - '0');

        switch (c)
        {
        case 0: /* NULL character. */
            va_end(ap);
            return;

        case 's': /* string */
            strptr = va_arg(ap, char *);
            if (strptr == NULL)
                break;
            slen = strlen(strptr);
            width_specifier = width_specifier >= slen ? width_specifier - slen : 0;
            if (left_align == SF_FALSE)
                while (width_specifier-- > 0)
                    log_putchar(' ');
            while (*strptr)
                log_putchar(*strptr++);
            while (width_specifier-- > 0)
                log_putchar(' ');
            break;

        case 'd': /* int */
            d = va_arg(ap, int);

            if (d < 0)
            {
                d = -d;
                sign_char = '-';
                if (lead_char != '0' && left_align == SF_FALSE)
                    width_specifier--;
            };

            tens = 1;
            width = 1;
            while (d / tens >= 10)
            {
                tens *= 10;
                width++;
            };

            width_specifier -= width;

            if (sign_char == ' ')
            {
                log_putchar(' ');
                width_specifier--;
            };

            if (left_align == SF_FALSE && lead_char != '0')
            {
                if (sign_char == '+')
                    width_specifier--;

                while (width_specifier-- > 0)
                    log_putchar(lead_char);
            };

            if (sign_char == '+' || sign_char == '-')
            {
                log_putchar(sign_char);
                width_specifier--;
            };

            if (left_align == SF_FALSE)
                while (width_specifier-- > 0)
                    log_putchar(lead_char);

            while (tens > 0)
            {
                log_putchar('0' + d / tens);
                d %= tens;
                tens /= 10;
            };

            while (width_specifier-- > 0)
                log_putchar(lead_char);
            break;

        case 'D': /* sf_count_t */
        {
            sf_count_t D, Tens;

            D = va_arg(ap, sf_count_t);

            if (D == 0)
            {
                while (--width_specifier > 0)
                    log_putchar(lead_char);
                log_putchar('0');
                break;
            }
            if (D < 0)
            {
                log_putchar('-');
                D = -D;
            };
            Tens = 1;
            width = 1;
            while (D / Tens >= 10)
            {
                Tens *= 10;
                width++;
            };

            while (width_specifier > width)
            {
                log_putchar(lead_char);
                width_specifier--;
            };

            while (Tens > 0)
            {
                log_putchar('0' + D / Tens);
                D %= Tens;
                Tens /= 10;
            };
        };
        break;

        case 'u': /* unsigned int */
            u = va_arg(ap, unsigned int);

            tens = 1;
            width = 1;
            while (u / tens >= 10)
            {
                tens *= 10;
                width++;
            };

            width_specifier -= width;

            if (sign_char == ' ')
            {
                log_putchar(' ');
                width_specifier--;
            };

            if (left_align == SF_FALSE && lead_char != '0')
            {
                if (sign_char == '+')
                    width_specifier--;

                while (width_specifier-- > 0)
                    log_putchar(lead_char);
            };

            if (sign_char == '+' || sign_char == '-')
            {
                log_putchar(sign_char);
                width_specifier--;
            };

            if (left_align == SF_FALSE)
                while (width_specifier-- > 0)
                    log_putchar(lead_char);

            while (tens > 0)
            {
                log_putchar('0' + u / tens);
                u %= tens;
                tens /= 10;
            };

            while (width_specifier-- > 0)
                log_putchar(lead_char);
            break;

        case 'c': /* char */
            c = va_arg(ap, int) & 0xFF;
            log_putchar(c);
            break;

        case 'x': /* hex */
        case 'X': /* hex */
            d = va_arg(ap, int);

            if (d == 0)
            {
                while (--width_specifier > 0)
                    log_putchar(lead_char);
                log_putchar('0');
                break;
            };
            shift = 28;
            width = (width_specifier < 8) ? 8 : width_specifier;
            while (!((((uint32_t)0xF) << shift) & d))
            {
                shift -= 4;
                width--;
            };

            while (width > 0 && width_specifier > width)
            {
                log_putchar(lead_char);
                width_specifier--;
            };

            while (shift >= 0)
            {
                c = (d >> shift) & 0xF;
                log_putchar((c > 9) ? c + 'A' - 10 : c + '0');
                shift -= 4;
            };
            break;

        case 'M': /* int2str */
            d = va_arg(ap, int);
            if (CPU_IS_LITTLE_ENDIAN)
            {
                istr[0] = d & 0xFF;
                istr[1] = (d >> 8) & 0xFF;
                istr[2] = (d >> 16) & 0xFF;
                istr[3] = (d >> 24) & 0xFF;
            }
            else
            {
                istr[3] = d & 0xFF;
                istr[2] = (d >> 8) & 0xFF;
                istr[1] = (d >> 16) & 0xFF;
                istr[0] = (d >> 24) & 0xFF;
            };
            istr[4] = 0;
            strptr = istr;
            while (*strptr)
            {
                c = *strptr++;
                log_putchar(c);
            };
            break;

        default:
            log_putchar('*');
            log_putchar(c);
            log_putchar('*');
            break;
        } /* switch */
    } /* while */

    va_end(ap);
    return;
}

/*-----------------------------------------------------------------------------------------------
**  ASCII header printf functions.
**  Some formats (ie NIST) use ascii text in their headers.
**  Format specifiers are the same as the standard printf specifiers (uses vsnprintf).
**  If this generates a compile error on any system, the author should be notified
**  so an alternative vsnprintf can be provided.
*/

void SndFile::asciiheader_printf(const char *format, ...)
{
    va_list argptr;

    int maxlen = strlen((char *)m_header.ptr);
    char *start = ((char *)m_header.ptr) + maxlen;
    maxlen = m_header.len - maxlen;

    va_start(argptr, format);
    vsnprintf(start, maxlen, format, argptr);
    va_end(argptr);

    /* Make sure the string is properly terminated. */
    start[maxlen - 1] = 0;

    m_header.indx = strlen((char *)m_header.ptr);

    return;
}

/*-----------------------------------------------------------------------------------------------
**  Binary header writing functions. Returns number of bytes written.
**
**  Format specifiers for psf_binheader_writef are as follows
**		m	- marker - four bytes - no endian manipulation
**
**		e   - all following numerical values will be little endian
**		E   - all following numerical values will be big endian
**
**		t   - all following O types will be truncated to 4 bytes
**		T   - switch off truncation of all following O types
**
**		1	- single byte value
**		2	- two byte value
**		3	- three byte value
**		4	- four byte value
**		8	- eight byte value (sometimes written as 4 bytes)
**
**		s   - string preceded by a four byte length
**		S   - string including null terminator
**      p   - a Pascal string
**
**		f	- floating point data
**		d	- double precision floating point data
**		h	- 16 binary bytes value
**
**		b	- binary data (see below)
**		z   - zero bytes (ses below)
**		j	- jump forwards or backwards
**
**	To write a word followed by an int (both little endian) use:
**		psf_binheader_writef ("e24", wordval, longval) ;
**
**	To write binary data use:
**		psf_binheader_writef ("b", &bindata, sizeof (bindata)) ;
**
**	To write N zero bytes use:
**			NOTE: due to platform issues (ie x86-64) you should cast the
**			argument to size_t or ensure the variable type is size_t.
**		psf_binheader_writef ("z", N) ;
*/

/* These macros may seem a bit messy but do prevent problems with processors which
** seg. fault when asked to write an int or short to a non-int/short aligned address.
*/

void SndFile::header_put_byte(char x)
{
    m_header.ptr[m_header.indx++] = x;
}

#if (CPU_IS_BIG_ENDIAN == 1)
void SndFile::header_put_marker(int x)
{
    header.ptr[header.indx++] = (x >> 24);
    header.ptr[header.indx++] = (x >> 16);
    header.ptr[header.indx++] = (x >> 8);
    header.ptr[header.indx++] = x;
}

#elif (CPU_IS_LITTLE_ENDIAN == 1)
void SndFile::header_put_marker(int x)
{
    m_header.ptr[m_header.indx++] = x;
    m_header.ptr[m_header.indx++] = (x >> 8);
    m_header.ptr[m_header.indx++] = (x >> 16);
    m_header.ptr[m_header.indx++] = (x >> 24);
}

#else
#error "Cannot determine endian-ness of processor."
#endif

void SndFile::header_put_be_short(int x)
{
    m_header.ptr[m_header.indx++] = (x >> 8);
    m_header.ptr[m_header.indx++] = x;
}

void SndFile::header_put_le_short(int x)
{
    m_header.ptr[m_header.indx++] = x;
    m_header.ptr[m_header.indx++] = (x >> 8);
}

void SndFile::header_put_be_3byte(int x) 
{
    m_header.ptr[m_header.indx++] = (x >> 16);
    m_header.ptr[m_header.indx++] = (x >> 8);
    m_header.ptr[m_header.indx++] = x;
}

void SndFile::header_put_le_3byte(int x)
{
    m_header.ptr[m_header.indx++] = x;
    m_header.ptr[m_header.indx++] = (x >> 8);
    m_header.ptr[m_header.indx++] = (x >> 16);
}

void SndFile::header_put_be_int(int x)
{
    m_header.ptr[m_header.indx++] = (x >> 24);
    m_header.ptr[m_header.indx++] = (x >> 16);
    m_header.ptr[m_header.indx++] = (x >> 8);
    m_header.ptr[m_header.indx++] = x;
}

void SndFile::header_put_le_int(int x)
{
    m_header.ptr[m_header.indx++] = x;
    m_header.ptr[m_header.indx++] = (x >> 8);
    m_header.ptr[m_header.indx++] = (x >> 16);
    m_header.ptr[m_header.indx++] = (x >> 24);
}

void SndFile::header_put_be_8byte(sf_count_t x)
{
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 56);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 48);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 40);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 32);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 24);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 16);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 8);
    m_header.ptr[m_header.indx++] = (unsigned char)x;
}

void SndFile::header_put_le_8byte(sf_count_t x)
{
    m_header.ptr[m_header.indx++] = (unsigned char)x;
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 8);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 16);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 24);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 32);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 40);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 48);
    m_header.ptr[m_header.indx++] = (unsigned char)(x >> 56);
}

int SndFile::binheader_writef(const char *format, ...)
{
    va_list argptr;
    sf_count_t countdata;
    unsigned long longdata;
    unsigned int data;
    float floatdata;
    double doubledata;
    void *bindata;
    size_t size;
    char c, *strptr;
    int count = 0, trunc_8to4;

    trunc_8to4 = SF_FALSE;

    va_start(argptr, format);

    while ((c = *format++))
    {
        if (m_header.indx + 16 >= m_header.len && bump_header_allocation(16))
            return count;

        switch (c)
        {
        case ' ': /* Do nothing. Just used to space out format string. */
            break;

        case 'e': /* All conversions are now from LE to host. */
            m_rwf_endian = SF_ENDIAN_LITTLE;
            break;

        case 'E': /* All conversions are now from BE to host. */
            m_rwf_endian = SF_ENDIAN_BIG;
            break;

        case 't': /* All 8 byte values now get written as 4 bytes. */
            trunc_8to4 = SF_TRUE;
            break;

        case 'T': /* All 8 byte values now get written as 8 bytes. */
            trunc_8to4 = SF_FALSE;
            break;

        case 'm':
            data = va_arg(argptr, unsigned int);
            header_put_marker(data);
            count += 4;
            break;

        case '1':
            data = va_arg(argptr, unsigned int);
            header_put_byte(data);
            count += 1;
            break;

        case '2':
            data = va_arg(argptr, unsigned int);
            if (m_rwf_endian == SF_ENDIAN_BIG)
            {
                header_put_be_short(data);
            }
            else
            {
                header_put_le_short(data);
            };
            count += 2;
            break;

        case '3': /* tribyte */
            data = va_arg(argptr, unsigned int);
            if (m_rwf_endian == SF_ENDIAN_BIG)
            {
                header_put_be_3byte(data);
            }
            else
            {
                header_put_le_3byte(data);
            };
            count += 3;
            break;

        case '4':
            data = va_arg(argptr, unsigned int);
            if (m_rwf_endian == SF_ENDIAN_BIG)
            {
                header_put_be_int(data);
            }
            else
            {
                header_put_le_int(data);
            };
            count += 4;
            break;

        case '8':
            countdata = va_arg(argptr, sf_count_t);
            if (m_rwf_endian == SF_ENDIAN_BIG && trunc_8to4 == SF_FALSE)
            {
                header_put_be_8byte(countdata);
                count += 8;
            }
            else if (m_rwf_endian == SF_ENDIAN_LITTLE && trunc_8to4 == SF_FALSE)
            {
                header_put_le_8byte(countdata);
                count += 8;
            }
            else if (m_rwf_endian == SF_ENDIAN_BIG && trunc_8to4 == SF_TRUE)
            {
                longdata = countdata & 0xFFFFFFFF;
                header_put_be_int(longdata);
                count += 4;
            }
            else if (m_rwf_endian == SF_ENDIAN_LITTLE && trunc_8to4 == SF_TRUE)
            {
                longdata = countdata & 0xFFFFFFFF;
                header_put_le_int(longdata);
                count += 4;
            }
            break;

        case 'f':
            /* Floats are passed as doubles. Is this always true? */
            floatdata = (float)va_arg(argptr, double);
            if (m_rwf_endian == SF_ENDIAN_BIG)
                float32_be_write(floatdata, m_header.ptr + m_header.indx);
            else
                float32_le_write(floatdata, m_header.ptr + m_header.indx);
            m_header.indx += 4;
            count += 4;
            break;

        case 'd':
            doubledata = va_arg(argptr, double);
            if (m_rwf_endian == SF_ENDIAN_BIG)
                double64_be_write(doubledata, m_header.ptr + m_header.indx);
            else
                double64_le_write(doubledata, m_header.ptr + m_header.indx);
            m_header.indx += 8;
            count += 8;
            break;

        case 's':
            /* Write a C string (guaranteed to have a zero terminator). */
            strptr = va_arg(argptr, char *);
            size = strlen(strptr) + 1;

            if (m_header.indx + 4 + (sf_count_t)size + (sf_count_t)(size & 1) > m_header.len && bump_header_allocation(4 + size + (size & 1)))
                return count;

            if (m_rwf_endian == SF_ENDIAN_BIG)
                header_put_be_int(size + (size & 1));
            else
                header_put_le_int(size + (size & 1));
            memcpy(&(m_header.ptr[m_header.indx]), strptr, size);
            size += (size & 1);
            m_header.indx += size;
            m_header.ptr[m_header.indx - 1] = 0;
            count += 4 + size;
            break;

        case 'S':
            /*
			**	Write an AIFF style string (no zero terminator but possibly
			**	an extra pad byte if the string length is odd).
			*/
            strptr = va_arg(argptr, char *);
            size = strlen(strptr);
            if (m_header.indx + 4 + (sf_count_t)size + (sf_count_t)(size & 1) > m_header.len && bump_header_allocation(4 + size + (size & 1)))
                return count;
            if (m_rwf_endian == SF_ENDIAN_BIG)
                header_put_be_int(size);
            else
                header_put_le_int(size);
            memcpy(&(m_header.ptr[m_header.indx]), strptr, size + (size & 1));
            size += (size & 1);
            m_header.indx += size;
            count += 4 + size;
            break;

        case 'p':
            /* Write a PASCAL string (as used by AIFF files).
			*/
            strptr = va_arg(argptr, char *);
            size = strlen(strptr);
            size = (size & 1) ? size : size + 1;
            size = (size > 254) ? 254 : size;

            if (m_header.indx + 1 + (sf_count_t)size > m_header.len && bump_header_allocation(1 + size))
                return count;

            header_put_byte(size);
            memcpy(&(m_header.ptr[m_header.indx]), strptr, size);
            m_header.indx += size;
            count += 1 + size;
            break;

        case 'b':
            bindata = va_arg(argptr, void *);
            size = va_arg(argptr, size_t);

            if (m_header.indx + (sf_count_t)size > m_header.len && bump_header_allocation(size))
                return count;

            memcpy(&(m_header.ptr[m_header.indx]), bindata, size);
            m_header.indx += size;
            count += size;
            break;

        case 'z':
            size = va_arg(argptr, size_t);

            if (m_header.indx + (sf_count_t)size > m_header.len && bump_header_allocation(size))
                return count;

            count += size;
            while (size)
            {
                m_header.ptr[m_header.indx] = 0;
                m_header.indx++;
                size--;
            };
            break;

        case 'h':
            bindata = va_arg(argptr, void *);
            memcpy(&(m_header.ptr[m_header.indx]), bindata, 16);
            m_header.indx += 16;
            count += 16;
            break;

        case 'j': /* Jump forwards/backwards by specified amount. */
            size = va_arg(argptr, size_t);

            if (m_header.indx + (sf_count_t)size > m_header.len && bump_header_allocation(size))
                return count;

            m_header.indx += size;
            count += size;
            break;

        case 'o': /* Jump to specified offset. */
            size = va_arg(argptr, size_t);

            if ((sf_count_t)size >= m_header.len && bump_header_allocation(size))
                return count;

            m_header.indx = size;
            break;

        default:
            log_printf("*** Invalid format specifier `%c'\n", c);
            m_error = SFE_INTERNAL;
            break;
        };
    };

    va_end(argptr);
    return count;
}

/*-----------------------------------------------------------------------------------------------
**  Binary header reading functions. Returns number of bytes read.
**
**	Format specifiers are the same as for header write function above with the following
**	additions:
**
**		p   - jump a given number of position from start of file.
**
**	If format is NULL, psf_binheader_readf returns the current offset.
*/

#if (CPU_IS_BIG_ENDIAN == 1)
#define GET_MARKER(ptr) \
    ((((uint32_t)(ptr)[0]) << 24) | ((ptr)[1] << 16) | ((ptr)[2] << 8) | ((ptr)[3]))

#elif (CPU_IS_LITTLE_ENDIAN == 1)
#define GET_MARKER(ptr) \
    (((ptr)[0]) | ((ptr)[1] << 8) | ((ptr)[2] << 16) | (((uint32_t)(ptr)[3]) << 24))

#else
#error "Cannot determine endian-ness of processor."
#endif

#define GET_LE_SHORT(ptr) (((ptr)[1] << 8) | ((ptr)[0]))
#define GET_BE_SHORT(ptr) (((ptr)[0] << 8) | ((ptr)[1]))

#define GET_LE_3BYTE(ptr) (((ptr)[2] << 16) | ((ptr)[1] << 8) | ((ptr)[0]))
#define GET_BE_3BYTE(ptr) (((ptr)[0] << 16) | ((ptr)[1] << 8) | ((ptr)[2]))

#define GET_LE_INT(ptr) (((ptr)[3] << 24) | ((ptr)[2] << 16) | ((ptr)[1] << 8) | ((ptr)[0]))

#define GET_BE_INT(ptr) (((ptr)[0] << 24) | ((ptr)[1] << 16) | ((ptr)[2] << 8) | ((ptr)[3]))

#define GET_LE_8BYTE(ptr)                                              \
    ((((sf_count_t)(ptr)[7]) << 56) | (((sf_count_t)(ptr)[6]) << 48) | \
     (((sf_count_t)(ptr)[5]) << 40) | (((sf_count_t)(ptr)[4]) << 32) | \
     (((sf_count_t)(ptr)[3]) << 24) | (((sf_count_t)(ptr)[2]) << 16) | \
     (((sf_count_t)(ptr)[1]) << 8) | ((ptr)[0]))

#define GET_BE_8BYTE(ptr)                                              \
    ((((sf_count_t)(ptr)[0]) << 56) | (((sf_count_t)(ptr)[1]) << 48) | \
     (((sf_count_t)(ptr)[2]) << 40) | (((sf_count_t)(ptr)[3]) << 32) | \
     (((sf_count_t)(ptr)[4]) << 24) | (((sf_count_t)(ptr)[5]) << 16) | \
     (((sf_count_t)(ptr)[6]) << 8) | ((ptr)[7]))

size_t SndFile::header_read(void *ptr, size_t bytes)
{
    size_t count = 0;

    if (m_header.indx + bytes >= m_header.len && bump_header_allocation(bytes))
        return count;

    if (m_header.indx + bytes > m_header.end)
    {
        count = fread(m_header.ptr + m_header.end, 1, bytes - (m_header.end - m_header.indx));
        if (count != bytes - (m_header.end - m_header.indx))
        {
            log_printf("Error : psf->fread returned short count.\n");
            return count;
        };
        m_header.end += count;
    };

    memcpy(ptr, m_header.ptr + m_header.indx, bytes);
    m_header.indx += bytes;

    return bytes;
}

int SndFile::header_gets(char *ptr, int bufsize)
{
    if (m_header.indx + bufsize >= m_header.len && bump_header_allocation(bufsize))
        return 0;

    int k;
    for (k = 0; k < bufsize - 1; k++)
    {
        if (m_header.indx < m_header.end)
        {
            ptr[k] = m_header.ptr[m_header.indx];
            m_header.indx++;
        }
        else
        {
            m_header.end += fread(m_header.ptr + m_header.end, 1, 1);
            ptr[k] = m_header.ptr[m_header.indx];
            m_header.indx = m_header.end;
        };

        if (ptr[k] == '\n')
            break;
    };

    ptr[k] = 0;

    return k;
}

void SndFile::binheader_seekf(sf_count_t position, SF_SEEK_MODE whence)
{
	switch (whence)
	{
	case SEEK_SET:
		if (m_header.indx + position >= m_header.len)
			bump_header_allocation(position);
		if (position > m_header.len)
		{
			/* Too much header to cache so just seek instead. */
			fseek(position, whence);
			return;
		};
		if (position > m_header.end)
			m_header.end += fread(m_header.ptr + m_header.end, 1, position - m_header.end);
		m_header.indx = position;
		break;

	case SEEK_CUR:
		if (m_header.indx + position >= m_header.len)
			bump_header_allocation(position);

		if (m_header.indx + position < 0)
			break;

		if (m_header.indx >= m_header.len)
		{
			fseek(position, whence);
			return;
		};

		if (m_header.indx + position <= m_header.end)
		{
			m_header.indx += position;
			break;
		};

		if (m_header.indx + position > m_header.len)
		{
			/* Need to jump this without caching it. */
			m_header.indx = m_header.end;
			fseek(position, SEEK_CUR);
			break;
		};

		m_header.end += fread(m_header.ptr + m_header.end, 1, position - (m_header.end - m_header.indx));
		m_header.indx = m_header.end;
		break;

	case SEEK_END:
	default:
		log_printf("Bad whence param in header_seek().\n");
		break;
	};

	return;
}

int SndFile::binheader_readf(char const *format, ...)
{
    va_list argptr;
    sf_count_t *countptr, countdata;
    unsigned char *ucptr, sixteen_bytes[16];
    unsigned int *intptr, intdata;
    unsigned short *shortptr;
    char *charptr;
    float *floatptr;
    double *doubleptr;
    char c;
    int byte_count = 0, count = 0;

    if (!format)
        return ftell();

    va_start(argptr, format);

    while ((c = *format++))
    {
        if (m_header.indx + 16 >= m_header.len && bump_header_allocation(16))
            return count;

        switch (c)
        {
        case 'e': /* All conversions are now from LE to host. */
            m_rwf_endian = SF_ENDIAN_LITTLE;
            break;

        case 'E': /* All conversions are now from BE to host. */
            m_rwf_endian = SF_ENDIAN_BIG;
            break;

        case 'm': /* 4 byte marker value eg 'RIFF' */
            intptr = va_arg(argptr, unsigned int *);
            *intptr = 0;
            ucptr = (unsigned char *)intptr;
            byte_count += header_read(ucptr, sizeof(int));
            *intptr = GET_MARKER(ucptr);
            break;

        case 'h':
            intptr = va_arg(argptr, unsigned int *);
            *intptr = 0;
            ucptr = (unsigned char *)intptr;
            byte_count += header_read(sixteen_bytes, sizeof(sixteen_bytes));
            {
                int k;
                intdata = 0;
                for (k = 0; k < 16; k++)
                    intdata ^= sixteen_bytes[k] << k;
            }
            *intptr = intdata;
            break;

        case '1':
            charptr = va_arg(argptr, char *);
            *charptr = 0;
            byte_count += header_read(charptr, sizeof(char));
            break;

        case '2': /* 2 byte value with the current endian-ness */
            shortptr = va_arg(argptr, unsigned short *);
            *shortptr = 0;
            ucptr = (unsigned char *)shortptr;
            byte_count += header_read(ucptr, sizeof(short));
            if (m_rwf_endian == SF_ENDIAN_BIG)
                *shortptr = GET_BE_SHORT(ucptr);
            else
                *shortptr = GET_LE_SHORT(ucptr);
            break;

        case '3': /* 3 byte value with the current endian-ness */
            intptr = va_arg(argptr, unsigned int *);
            *intptr = 0;
            byte_count += header_read(sixteen_bytes, 3);
            if (m_rwf_endian == SF_ENDIAN_BIG)
                *intptr = GET_BE_3BYTE(sixteen_bytes);
            else
                *intptr = GET_LE_3BYTE(sixteen_bytes);
            break;

        case '4': /* 4 byte value with the current endian-ness */
            intptr = va_arg(argptr, unsigned int *);
            *intptr = 0;
            ucptr = (unsigned char *)intptr;
            byte_count += header_read(ucptr, sizeof(int));
            if (m_rwf_endian == SF_ENDIAN_BIG)
                *intptr = psf_get_be32(ucptr, 0);
            else
                *intptr = psf_get_le32(ucptr, 0);
            break;

        case '8': /* 8 byte value with the current endian-ness */
            countptr = va_arg(argptr, sf_count_t *);
            *countptr = 0;
            byte_count += header_read(sixteen_bytes, 8);
            if (m_rwf_endian == SF_ENDIAN_BIG)
                countdata = psf_get_be64(sixteen_bytes, 0);
            else
                countdata = psf_get_le64(sixteen_bytes, 0);
            *countptr = countdata;
            break;

        case 'f': /* Float conversion */
            floatptr = va_arg(argptr, float *);
            *floatptr = 0.0;
            byte_count += header_read(floatptr, sizeof(float));
            if (m_rwf_endian == SF_ENDIAN_BIG)
                *floatptr = float32_be_read((unsigned char *)floatptr);
            else
                *floatptr = float32_le_read((unsigned char *)floatptr);
            break;

        case 'd': /* double conversion */
            doubleptr = va_arg(argptr, double *);
            *doubleptr = 0.0;
            byte_count += header_read(doubleptr, sizeof(double));
            if (m_rwf_endian == SF_ENDIAN_BIG)
                *doubleptr = double64_be_read((unsigned char *)doubleptr);
            else
                *doubleptr = double64_le_read((unsigned char *)doubleptr);
            break;

        case 's':
            log_printf("Format conversion 's' not implemented yet.\n");
            /*
			strptr = va_arg (argptr, char *) ;
			size   = strlen (strptr) + 1 ;
			size  += (size & 1) ;
			longdata = H2LE_32 (size) ;
			get_int (psf, longdata) ;
			memcpy (&(psf->header.ptr [psf->header.indx]), strptr, size) ;
			psf->header.indx += size ;
			*/
            break;

        case 'b': /* Raw bytes */
            charptr = va_arg(argptr, char *);
            count = va_arg(argptr, size_t);
            memset(charptr, 0, count);
            byte_count += header_read(charptr, count);
            break;

        case 'G':
            charptr = va_arg(argptr, char *);
            count = va_arg(argptr, size_t);
            memset(charptr, 0, count);

            if (m_header.indx + count >= m_header.len && bump_header_allocation(count))
                return 0;

            byte_count += header_gets(charptr, count);
            break;

        case 'z':
            log_printf("Format conversion 'z' not implemented yet.\n");
            /*
			size    = va_arg (argptr, size_t) ;
			while (size)
			{	psf->header.ptr [psf->header.indx] = 0 ;
				psf->header.indx ++ ;
				size -- ;
				} ;
			*/
            break;

        default:
            log_printf("*** Invalid format specifier `%c'\n", c);
            m_error = SFE_INTERNAL;
            break;
        };
    };

    va_end(argptr);

    return byte_count;
}

sf_count_t psf_default_seek(SndFile *psf, int UNUSED(mode), sf_count_t samples_from_start)
{
    if (!(psf->m_blockwidth && psf->m_dataoffset >= 0))
    {
        psf->m_error = SFE_BAD_SEEK;
        return PSF_SEEK_ERROR;
    };

    if (!psf->sf.seekable)
    {
        psf->m_error = SFE_NOT_SEEKABLE;
        return PSF_SEEK_ERROR;
    };

    sf_count_t position = psf->m_dataoffset + psf->m_blockwidth * samples_from_start;

    sf_count_t retval = psf->fseek(position, SEEK_SET);
    if (retval != position)
    {
        psf->m_error = SFE_SEEK_FAILED;
        return PSF_SEEK_ERROR;
    };

    return samples_from_start;
}

void psf_hexdump(const void *ptr, int len)
{
    const char *data = (const char *)ptr;

    if (!data)
        return;
    if (len <= 0)
        return;

    puts("");
    for (int k = 0; k < len; k += 16)
    {
        char ascii[17];
        memset(ascii, ' ', sizeof(ascii));

        printf("%08X: ", k);

        int m = 0;
        for (m = 0; m < 16 && k + m < len; m++)
        {
            printf(m == 8 ? " %02X " : "%02X ", data[k + m] & 0xFF);
            ascii[m] = psf_isprint(data[k + m]) ? data[k + m] : '.';
        };

        if (m <= 8)
            printf(" ");
        for (; m < 16; m++)
            printf("   ");

        ascii[16] = 0;
        printf(" %s\n", ascii);
    };

    puts("");
}

void SndFile::log_SF_INFO()
{
    log_printf("---------------------------------\n");

    log_printf(" Sample rate :   %d\n", sf.samplerate);
    if (sf.frames == SF_COUNT_MAX)
        log_printf(" Frames      :   unknown\n");
    else
        log_printf(" Frames      :   %D\n", sf.frames);
    log_printf(" Channels    :   %d\n", sf.channels);

    log_printf(" Format      :   0x%X\n", sf.format);
    log_printf(" Sections    :   %d\n", sf.sections);
    log_printf(" Seekable    :   %s\n", sf.seekable ? "TRUE" : "FALSE");

    log_printf("---------------------------------\n");
}

void *psf_memset(void *s, int c, sf_count_t len)
{
    char *ptr = (char *)s;

    while (len > 0)
    {
        int setcount = (len > 0x10000000) ? 0x10000000 : (int)len;

        memset(ptr, c, setcount);

        ptr += setcount;
        len -= setcount;
    };

    return s;
}

/*
** Clang refuses to do sizeof (SF_CUES_VAR (cue_count)) so we have to manually
** bodgy something up instead.
*/

SF_CUE_POINT *psf_cues_alloc(uint32_t cue_count)
{
    return (SF_CUE_POINT *)calloc(cue_count, sizeof(SF_CUE_POINT));
}

SF_CUE_POINT *psf_cues_dup(const SF_CUE_POINT *cues, uint32_t cue_count)
{
    SF_CUE_POINT *pnew = psf_cues_alloc(cue_count);
    memcpy(pnew, cues, sizeof(SF_CUE_POINT) * cue_count);
    return pnew;
}

SF_INSTRUMENT *psf_instrument_alloc(void)
{
    SF_INSTRUMENT *instr = (SF_INSTRUMENT *)calloc(1, sizeof(SF_INSTRUMENT));

    if (instr == NULL)
        return NULL;

    /* Set non-zero default values. */
    instr->basenote = -1;
    instr->velocity_lo = -1;
    instr->velocity_hi = -1;
    instr->key_lo = -1;
    instr->key_hi = -1;

    return instr;
}

void psf_sanitize_string(char *cptr, int len)
{
    do
    {
        len--;
        cptr[len] = psf_isprint(cptr[len]) ? cptr[len] : '.';
    } while (len > 0);
}

void psf_get_date_str(char *str, int maxlen)
{
    time_t current;
    struct tm timedata, *tmptr;

    time(&current);

#if defined(HAVE_GMTIME_R)
    /* If the re-entrant version is available, use it. */
    tmptr = gmtime_r(&current, &timedata);
#elif defined(HAVE_GMTIME)
    /* Otherwise use the standard one and copy the data to local storage. */
    tmptr = gmtime(&current);
    memcpy(&timedata, tmptr, sizeof(timedata));
#else
    tmptr = NULL;
#endif

    if (tmptr)
        snprintf(str, maxlen, "%4d-%02d-%02d %02d:%02d:%02d UTC", 1900 + timedata.tm_year,
                 timedata.tm_mon, timedata.tm_mday, timedata.tm_hour, timedata.tm_min,
                 timedata.tm_sec);
    else
        snprintf(str, maxlen, "Unknown date");

    return;
}

int subformat_to_bytewidth(int format)
{
    switch (format)
    {
    case SF_FORMAT_PCM_U8:
    case SF_FORMAT_PCM_S8:
        return 1;
    case SF_FORMAT_PCM_16:
        return 2;
    case SF_FORMAT_PCM_24:
        return 3;
    case SF_FORMAT_PCM_32:
    case SF_FORMAT_FLOAT:
        return 4;
    case SF_FORMAT_DOUBLE:
        return 8;
    };

    return 0;
}

int s_bitwidth_to_subformat(int bits)
{
    static int array[] = {SF_FORMAT_PCM_S8, SF_FORMAT_PCM_16, SF_FORMAT_PCM_24, SF_FORMAT_PCM_32};

    if (bits < 8 || bits > 32)
        return 0;

    return array[((bits + 7) / 8) - 1];
}

int u_bitwidth_to_subformat(int bits)
{
    static int array[] = {SF_FORMAT_PCM_U8, SF_FORMAT_PCM_16, SF_FORMAT_PCM_24, SF_FORMAT_PCM_32};

    if (bits < 8 || bits > 32)
        return 0;

    return array[((bits + 7) / 8) - 1];
}

/*
**	psf_rand_int32 : Not crypto quality, but more than adequate for things
**	like stream serial numbers in Ogg files or the unique_id field of the
**	SndFile class.
*/

int32_t psf_rand_int32(void)
{
    static uint64_t value = 0;

    if (value == 0)
    {
#ifdef HAVE_GETTIMEOFDAY
        struct timeval tv;
        gettimeofday(&tv, NULL);
        value = tv.tv_sec + tv.tv_usec;
#else
        value = time(NULL);
#endif
    };

    int count = 4 + (value & 7);
    for (int k = 0; k < count; k++)
        value = (11117 * value + 211231) & 0x7fffffff;

    return (int32_t)value;
}

void append_snprintf(char *dest, size_t maxlen, const char *fmt, ...)
{
    size_t len = strlen(dest);

    if (len < maxlen)
    {
        va_list ap;

        va_start(ap, fmt);
        vsnprintf(dest + len, maxlen - len, fmt, ap);
        va_end(ap);
    };

    return;
}

void psf_strlcpy_crlf(char *dest, const char *src, size_t destmax, size_t srcmax)
{
    /* Must be minus 2 so it can still expand a single trailing '\n' or '\r'. */
    char *destend = dest + destmax - 2;
    const char *srcend = src + srcmax;

    while (dest < destend && src < srcend)
    {
        if ((src[0] == '\r' && src[1] == '\n') || (src[0] == '\n' && src[1] == '\r'))
        {
            *dest++ = '\r';
            *dest++ = '\n';
            src += 2;
            continue;
        };

        if (src[0] == '\r')
        {
            *dest++ = '\r';
            *dest++ = '\n';
            src += 1;
            continue;
        };

        if (src[0] == '\n')
        {
            *dest++ = '\r';
            *dest++ = '\n';
            src += 1;
            continue;
        };

        *dest++ = *src++;
    };

    /* Make sure dest is terminated. */
    *dest = 0;
}

sf_count_t psf_decode_frame_count(SndFile *psf)
{
    sf_count_t count, readlen, total = 0;
    BUF_UNION ubuf;

    /* If the file is too long, just return SF_COUNT_MAX. */
    if (psf->m_datalength > 0x1000000)
        return SF_COUNT_MAX;

    psf->fseek(psf->m_dataoffset, SEEK_SET);

    readlen = ARRAY_LEN(ubuf.ibuf) / psf->sf.channels;
    readlen *= psf->sf.channels;

    while ((count = psf->read_int(psf, ubuf.ibuf, readlen)) > 0)
        total += count;

    psf->fseek(psf->m_dataoffset, SEEK_SET);

    return total / psf->sf.channels;
}

#define CASE_NAME(x) \
    case x:          \
        return #x;   \
        break;

const char *str_of_major_format(int format)
{
    switch (SF_CONTAINER(format))
    {
        CASE_NAME(SF_FORMAT_WAV);
        CASE_NAME(SF_FORMAT_AIFF);
        CASE_NAME(SF_FORMAT_AU);
        CASE_NAME(SF_FORMAT_RAW);
        CASE_NAME(SF_FORMAT_PAF);
        CASE_NAME(SF_FORMAT_SVX);
        CASE_NAME(SF_FORMAT_NIST);
        CASE_NAME(SF_FORMAT_VOC);
        CASE_NAME(SF_FORMAT_IRCAM);
        CASE_NAME(SF_FORMAT_W64);
        CASE_NAME(SF_FORMAT_MAT4);
        CASE_NAME(SF_FORMAT_MAT5);
        CASE_NAME(SF_FORMAT_PVF);
        CASE_NAME(SF_FORMAT_XI);
        CASE_NAME(SF_FORMAT_HTK);
        CASE_NAME(SF_FORMAT_SDS);
        CASE_NAME(SF_FORMAT_AVR);
        CASE_NAME(SF_FORMAT_WAVEX);
        CASE_NAME(SF_FORMAT_FLAC);
        CASE_NAME(SF_FORMAT_CAF);
        CASE_NAME(SF_FORMAT_WVE);
        CASE_NAME(SF_FORMAT_OGG);
    default:
        break;
    };

    return "BAD_MAJOR_FORMAT";
}

const char *str_of_minor_format(int format)
{
    switch (SF_CODEC(format))
    {
        CASE_NAME(SF_FORMAT_PCM_S8);
        CASE_NAME(SF_FORMAT_PCM_16);
        CASE_NAME(SF_FORMAT_PCM_24);
        CASE_NAME(SF_FORMAT_PCM_32);
        CASE_NAME(SF_FORMAT_PCM_U8);
        CASE_NAME(SF_FORMAT_FLOAT);
        CASE_NAME(SF_FORMAT_DOUBLE);
        CASE_NAME(SF_FORMAT_ULAW);
        CASE_NAME(SF_FORMAT_ALAW);
        CASE_NAME(SF_FORMAT_IMA_ADPCM);
        CASE_NAME(SF_FORMAT_MS_ADPCM);
        CASE_NAME(SF_FORMAT_GSM610);
        CASE_NAME(SF_FORMAT_VOX_ADPCM);
        CASE_NAME(SF_FORMAT_NMS_ADPCM_16);
        CASE_NAME(SF_FORMAT_NMS_ADPCM_24);
        CASE_NAME(SF_FORMAT_NMS_ADPCM_32);
        CASE_NAME(SF_FORMAT_G721_32);
        CASE_NAME(SF_FORMAT_G723_24);
        CASE_NAME(SF_FORMAT_G723_40);
        CASE_NAME(SF_FORMAT_DWVW_12);
        CASE_NAME(SF_FORMAT_DWVW_16);
        CASE_NAME(SF_FORMAT_DWVW_24);
        CASE_NAME(SF_FORMAT_DWVW_N);
        CASE_NAME(SF_FORMAT_DPCM_8);
        CASE_NAME(SF_FORMAT_DPCM_16);
        CASE_NAME(SF_FORMAT_VORBIS);
    default:
        break;
    };

    return "BAD_MINOR_FORMAT";
}

const char *str_of_open_mode(int mode)
{
    switch (mode)
    {
        CASE_NAME(SFM_READ);
        CASE_NAME(SFM_WRITE);
        CASE_NAME(SFM_RDWR);

    default:
        break;
    };

    return "BAD_MODE";
}

const char *str_of_endianness(int end)
{
    switch (end)
    {
        CASE_NAME(SF_ENDIAN_BIG);
        CASE_NAME(SF_ENDIAN_LITTLE);
        CASE_NAME(SF_ENDIAN_CPU);
    default:
        break;
    };

    /* Zero length string for SF_ENDIAN_FILE. */
    return "";
}

void psf_f2s_array(const float *src, short *dest, size_t count, int normalize)
{
    float normfact = (float)(normalize ? (1.0 * 0x7FFF) : 1.0);
    while (count)
    {
        count--;
        dest[count] = (short)lrintf(src[count] * normfact);
    }

    return;
}

void psf_f2s_clip_array(const float *src, short *dest, size_t count, int normalize)
{
    float normfact = (float)(normalize ? (1.0 * 0x8000) : 1.0);

    while (count)
    {
        count--;
        float scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFF))
        {
            dest[count] = 0x7FFF;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x1000))
        {
            dest[count] = 0x8000;
            continue;
        };

        dest[count] = (short)lrintf(scaled_value);
    };

    return;
}

void psf_d2s_array(const double *src, short *dest, size_t count, int normalize)
{
    double normfact = normalize ? (1.0 * 0x7FFF) : 1.0;
    while (count)
    {
        count--;
        dest[count] = (short)lrint(src[count] * normfact);
    }

    return;
}

void psf_d2s_clip_array(const double *src, short *dest, size_t count, int normalize)
{
    double normfact = normalize ? (1.0 * 0x8000) : 1.0;

    while (count)
    {
        count--;
        double scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFF))
        {
            dest[count] = 0x7FFF;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x1000))
        {
            dest[count] = 0x8000;
            continue;
        };

        dest[count] = (short)lrint(scaled_value);
    };

    return;
}

void psf_f2i_array(const float *src, int *dest, size_t count, int normalize)
{
    float normfact = (float)(normalize ? (1.0 * 0x7FFFFFFF) : 1.0);
    while (count)
    {
        count--;
        dest[count] = lrintf(src[count] * normfact);
    }

    return;
}

void psf_f2i_clip_array(const float *src, int *dest, size_t count, int normalize)
{
    double normfact = (float)(normalize ? (8.0 * 0x10000000) : 1.0);

    while (count)
    {
        count--;
        double scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            dest[count] = 0x7FFFFFFF;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            dest[count] = 0x80000000;
            continue;
        };

        dest[count] = lrintf(scaled_value);
    };

    return;
}

void psf_d2i_array(const double *src, int *dest, size_t count, int normalize)
{
    double normfact = normalize ? (1.0 * 0x7FFFFFFF) : 1.0;
    while (count)
    {
        count--;
        dest[count] = lrint(src[count] * normfact);
    }

    return;
}

void psf_d2i_clip_array(const double *src, int *dest, size_t count, int normalize)
{
    double normfact = normalize ? (8.0 * 0x10000000) : 1.0;

    while (count)
    {
        count--;
        double scaled_value = src[count] * normfact;
        if (CPU_CLIPS_POSITIVE == 0 && scaled_value >= (1.0 * 0x7FFFFFFF))
        {
            dest[count] = 0x7FFFFFFF;
            continue;
        };
        if (CPU_CLIPS_NEGATIVE == 0 && scaled_value <= (-8.0 * 0x10000000))
        {
            dest[count] = 0x80000000;
            continue;
        };

        dest[count] = lrint(scaled_value);
    };

    return;
}

FILE *psf_open_tmpfile(char *fname, size_t fnamelen)
{
    const char *tmpdir;
    FILE *file;

#ifdef _WIN32
    tmpdir = getenv("TEMP");
#else
    tmpdir = getenv("TMPDIR");
#endif
    tmpdir = tmpdir == NULL ? "/tmp" : tmpdir;

    if (tmpdir && access(tmpdir, R_OK | W_OK | X_OK) == 0)
    {
        snprintf(fname, fnamelen, "%s/%x%x-alac.tmp", tmpdir, psf_rand_int32(), psf_rand_int32());
        if ((file = fopen(fname, "wb+")) != NULL)
            return file;
    };

    snprintf(fname, fnamelen, "%x%x-alac.tmp", psf_rand_int32(), psf_rand_int32());
    if ((file = fopen(fname, "wb+")) != NULL)
        return file;

    memset(fname, 0, fnamelen);
    return NULL;
}

sf_count_t SndFile::get_filelen()
{
    assert(m_stream);

    return m_stream->get_filelen();
}

int SndFile::file_valid()
{
    return (m_stream) ? SF_TRUE : SF_FALSE;
}

sf_count_t SndFile::fseek(sf_count_t offset, int whence)
{
    if (m_stream)
        return m_stream->seek(offset, whence);
    else
        return -1;
}

size_t SndFile::fread(void *ptr, size_t bytes, size_t items)
{
    assert(m_stream);

    if (!ptr)
        return 0;
    if (items * bytes <= 0)
        return 0;
    else if (m_stream)
        return m_stream->read(ptr, bytes * items) / bytes;
    else
        return 0;
}

size_t SndFile::fwrite(const void *ptr, size_t bytes, size_t items)
{
    if (!ptr)
        return 0;
    if (items * bytes <= 0)
        return 0;
    else if (m_stream)
        return m_stream->write(ptr, bytes * items) / bytes;
    else
        return 0;
}

sf_count_t SndFile::ftell()
{
    assert(m_stream);

    return m_stream->tell();
}

void SndFile::fsync()
{
    if (m_mode == SFM_WRITE || m_mode == SFM_RDWR)
        m_stream->flush();
}

int SndFile::ftruncate(sf_count_t len)
{
    return m_stream->set_filelen(len);
}
