/*
** Copyright (C) 2001-2016 Erik de Castro Lopo <erikd@mega-nerd.com>
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
#include <string.h>
#include <math.h>

#include "sndfile2k/sndfile2k.h"
#include "common.h"

#define STRINGS_DEBUG (0)

int SF_PRIVATE::store_string(int str_type, const char *str)
{
    char new_str[128];
    size_t str_len;
    int k, str_flags;

    if (str == NULL)
        return SFE_STR_BAD_STRING;

    str_len = strlen(str);

    /* A few extra checks for write mode. */
    if (m_mode == SFM_WRITE || m_mode == SFM_RDWR)
    {
        if ((m_strings.flags & SF_STR_ALLOW_START) == 0)
            return SFE_STR_NO_SUPPORT;
        if (m_have_written && (m_strings.flags & SF_STR_ALLOW_END) == 0)
            return SFE_STR_NO_SUPPORT;
        /* Only allow zero length strings for software. */
        if (str_type != SF_STR_SOFTWARE && str_len == 0)
            return SFE_STR_BAD_STRING;
    };

    /* Find the next free slot in table. */
    for (k = 0; k < SF_MAX_STRINGS; k++)
    {
        /* If we find a matching entry clear it. */
        if (m_strings.data[k].type == str_type)
            m_strings.data[k].type = -1;

        if (m_strings.data[k].type == 0)
            break;
    };

    /* Determine flags */
    str_flags = SF_STR_LOCATE_START;
    if (m_mode == SFM_RDWR || m_have_written)
    {
        if ((m_strings.flags & SF_STR_ALLOW_END) == 0)
            return SFE_STR_NO_ADD_END;
        str_flags = SF_STR_LOCATE_END;
    };

    /* More sanity checking. */
    if (k >= SF_MAX_STRINGS)
        return SFE_STR_MAX_COUNT;

    if (k == 0 && m_strings.storage_used != 0)
    {
        log_printf("SFE_STR_WEIRD : k == 0 && m_strings.storage_used != 0\n");
        return SFE_STR_WEIRD;
    };

    if (k != 0 && m_strings.storage_used == 0)
    {
        log_printf("SFE_STR_WEIRD : k != 0 && m_strings.storage_used == 0\n");
        return SFE_STR_WEIRD;
    };

    /* Special case for the first string. */
    if (k == 0)
        m_strings.storage_used = 0;

    switch (str_type)
    {
    case SF_STR_SOFTWARE:
        /* In write mode, want to append libsndfile-version to string. */
        if (m_mode == SFM_WRITE || m_mode == SFM_RDWR)
        {
            if (strstr(str, PACKAGE_NAME) == NULL)
            {
                /*
				 * If the supplied string does not already contain a
				 * libsndfile-X.Y.Z component, then add it.
				 */
                if (strlen(str) == 0)
                    snprintf(new_str, sizeof(new_str), "%s-%s", PACKAGE_NAME, PACKAGE_VERSION);
                else
                    snprintf(new_str, sizeof(new_str), "%s (%s-%s)", str, PACKAGE_NAME,
                             PACKAGE_VERSION);
            }
            else
                snprintf(new_str, sizeof(new_str), "%s", str);

            str = new_str;
        };
        break;

    case SF_STR_TITLE:
    case SF_STR_COPYRIGHT:
    case SF_STR_ARTIST:
    case SF_STR_COMMENT:
    case SF_STR_DATE:
    case SF_STR_ALBUM:
    case SF_STR_LICENSE:
    case SF_STR_TRACKNUMBER:
    case SF_STR_GENRE:
        break;

    default:
        log_printf("%s : SFE_STR_BAD_TYPE\n", __func__);
        return SFE_STR_BAD_TYPE;
    };

    /* Plus one to catch string terminator. */
    str_len = strlen(str) + 1;

    if (m_strings.storage_used + str_len + 1 > m_strings.storage_len)
    {
        char *temp = m_strings.storage;
        size_t newlen = 2 * m_strings.storage_len + str_len + 1;

        newlen = newlen < 256 ? 256 : newlen;

        if ((m_strings.storage = (char *)realloc(temp, newlen)) == NULL)
        {
            m_strings.storage = temp;
            return SFE_MALLOC_FAILED;
        };

        m_strings.storage_len = newlen;
    };

    m_strings.data[k].type = str_type;
    m_strings.data[k].offset = m_strings.storage_used;
    m_strings.data[k].flags = str_flags;

    memcpy(m_strings.storage + m_strings.storage_used, str, str_len);
    m_strings.storage_used += str_len;

    m_strings.flags |= str_flags;

#if STRINGS_DEBUG
    printf("storage_used         : %zd / %zd\n", m_strings.storage_used,
           m_strings.storage_len);
    psf_hexdump(m_strings.storage, m_strings.storage_used);
#endif

    return 0;
}

int SF_PRIVATE::set_string(int str_type, const char *str)
{
    if (m_mode == SFM_READ)
        return SFE_STR_NOT_WRITE;

    return store_string(str_type, str);
}

const char *SF_PRIVATE::get_string(int str_type)
{
    for (int k = 0; k < SF_MAX_STRINGS; k++)
        if (str_type == m_strings.data[k].type)
            return m_strings.storage + m_strings.data[k].offset;

    return NULL;
}

int SF_PRIVATE::location_string_count(int location)
{
    int count = 0;

    for (int k = 0; k < SF_MAX_STRINGS; k++)
        if (m_strings.data[k].type > 0 && m_strings.data[k].flags & location)
            count++;

    return count;
}
