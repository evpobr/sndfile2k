/*
** Copyright (C) 2002-2017 Erik de Castro Lopo <erikd@mega-nerd.com>
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

/* Microsoft declares some 'unistd.h' functions in 'io.h'. */

#pragma once

/* Some defines that microsoft 'forgot' to implement. */

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifndef R_OK
#define R_OK (4) /* Test for read permission.  */
#endif

#ifndef W_OK
#define W_OK (2) /* Test for write permission.  */
#endif

#ifndef X_OK
#ifdef WIN32
#define X_OK (0)
#else
#define X_OK 1 /* execute permission - unsupported in windows*/
#endif
#endif

#ifndef F_OK
#define F_OK (0) /* Test for existence.  */
#endif

#ifndef S_IRWXU
#define S_IRWXU (0000700) /* rwx, owner */
#endif

#ifndef S_IRUSR
#define S_IRUSR (0000400) /* read permission, owner */
#endif

#ifndef S_IWUSR
#define S_IWUSR (0000200) /* write permission, owner */
#endif

#ifndef S_IXUSR
#define S_IXUSR (0000100) /* execute/search permission, owner */
#endif

/* Windows doesn't have group permissions so set all these to zero. */
#define S_IRWXG (0) /* rwx, group */
#define S_IRGRP (0) /* read permission, group */
#define S_IWGRP (0) /* write permission, grougroup */
#define S_IXGRP (0) /* execute/search permission, group */

/* Windows doesn't have others permissions so set all these to zero. */
#define S_IRWXO (0) /* rwx, other */
#define S_IROTH (0) /* read permission, other */
#define S_IWOTH (0) /* write permission, other */
#define S_IXOTH (0) /* execute/search permission, other */

#if !defined(S_ISFIFO)
#if defined(_S_IFMT) && defined(_S_IFIFO)
#define S_ISFIFO(mode) (((mode)&_S_IFMT) == _S_IFIFO)
#else
#define S_ISFIFO(mode) (0)
#endif
#endif

#if !defined(S_ISSOCK)
#if defined(_S_IFMT) && defined(_S_ISOCK)
#define S_ISSOCK(mode) (((mode)&_S_IFMT) == _S_ISOCK)
#else
#define S_ISSOCK(mode) (0)
#endif
#endif

#if !defined(S_ISREG)
#if defined(_S_IFREG)
#define S_ISREG(mode) (((mode)&_S_IFREG) == _S_IFREG)
#else
#define S_ISREG(mode) (0)
#endif
#endif

/*
**	Don't know if these are still needed.
**
** #define _IFMT  _S_IFMT
** #define _IFREG _S_IFREG
*/
