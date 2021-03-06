/* Set to 1 if the compile is GNU GCC. */
#cmakedefine01 COMPILER_IS_GCC

/* Target processor clips on negative float to int conversion. */
#cmakedefine01 CPU_CLIPS_NEGATIVE

/* Target processor clips on positive float to int conversion. */
#cmakedefine01 CPU_CLIPS_POSITIVE

/* Target processor is big endian. */
#cmakedefine01 CPU_IS_BIG_ENDIAN

/* Target processor is little endian. */
#cmakedefine01 CPU_IS_LITTLE_ENDIAN

/* Set to 1 to enable experimental code. */
#cmakedefine01 ENABLE_EXPERIMENTAL_CODE

/* Define if you have the <alsa/asoundlib.h> header file. */
#cmakedefine HAVE_ALSA_ASOUNDLIB_H

/* Define if you have the <byteswap.h> header file. */
#cmakedefine HAVE_BYTESWAP_H

/* Define if you have the S_IRGRP define. */
#cmakedefine HAVE_DECL_S_IRGRP

/* Define if you have the <direct.h> header file. */
#cmakedefine HAVE_DIRECT_H

/* Define if FLAC and Vorbis support enabled. */
#cmakedefine HAVE_XIPH_CODECS

/* Define if Speex support enabled. */
#cmakedefine HAVE_SPEEX

/* Define if you have the `fstat' function. */
#cmakedefine HAVE_FSTAT

/* Define if you have the `fstat64' function. */
#cmakedefine HAVE_FSTAT64

/* Define if you have the `fsync' function. */
#cmakedefine HAVE_FSYNC

/* Define if you have the `gettimeofday' function. */
#cmakedefine HAVE_GETTIMEOFDAY

/* Define if you have the `gmtime' function. */
#cmakedefine HAVE_GMTIME

/* Define if you have the `gmtime_r' function. */
#cmakedefine HAVE_GMTIME_R

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine HAVE_IO_H

/* Define if you have the <wchar.h> header file. */
#cmakedefine HAVE_WCHAR_H

/* Define if you have the <locale.h> header file. */
#cmakedefine HAVE_LOCALE_H

/* Define if you have the `localtime' function. */
#cmakedefine HAVE_LOCALTIME

/* Define if you have the `localtime_r' function. */
#cmakedefine HAVE_LOCALTIME_R

/* Define if you have C99's lrintf function. */
#cmakedefine HAVE_LRINTF

/* Define if you have the `setlocale' function. */
#cmakedefine HAVE_SETLOCALE

/* Define if <sndio.h> is available. */
#cmakedefine HAVE_SNDIO_H

/* Define if if you have libsqlite3. */
#cmakedefine HAVE_SQLITE3

/* Define if the system has the type `ssize_t'. */
#cmakedefine HAVE_SSIZE_T

/* Define if you have the <sys/time.h> header file. */
#cmakedefine HAVE_SYS_TIME_H

/* Define if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H

/* Define if you have <sys/wait.h> that is POSIX.1 compatible. */
#cmakedefine HAVE_SYS_WAIT_H

/* Define if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H

/* Define if you have the `waitpid' function. */
#cmakedefine HAVE_WAITPID

/* Set to 1 if compiling for OpenBSD */
#cmakedefine01 OS_IS_OPENBSD

/* Define to the full name of this package. */
#define PACKAGE_NAME "@PACKAGE_NAME@"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@PROJECT_VERSION@"

/* The size of `int', as computed by sizeof. */
@SIZEOF_INT_CODE@

/* The size of `int64_t', as computed by sizeof. */
@SIZEOF_INT64_T_CODE@

/* The size of `long', as computed by sizeof. */
@SIZEOF_LONG_CODE@

/* The size of `long long', as computed by sizeof. */
@SIZEOF_LONG_LONG_CODE@

/* The size of `short', as computed by sizeof. */
@SIZEOF_SHORT_CODE@

/* The size of `size_t', as computed by sizeof. */
@SIZEOF_SIZE_T_CODE@

/* The size of `ssize_t', as computed by sizeof. */
@SIZEOF_SSIZE_T_CODE@

/* The size of `void*', as computed by sizeof. */
@SIZEOF_VOIDP_CODE@

/* The size of `wchar_t', as computed by sizeof. */
@SIZEOF_WCHAR_T_CODE@

/* Set to 1 to use the native windows API */
#cmakedefine01 USE_WINDOWS_API

/* Version number of package */
#define VERSION "@PROJECT_VERSION@"

/* Define if windows DLL is being built. */
#cmakedefine WIN32_TARGET_DLL

/* Target processor is big endian. */
#cmakedefine WORDS_BIGENDIAN

#ifndef HAVE_SSIZE_T
#define ssize_t intptr_t
#endif
