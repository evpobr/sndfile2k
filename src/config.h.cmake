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

/* Will be defined if flac, ogg and vorbis are available. */
#cmakedefine HAVE_EXTERNAL_XIPH_LIBS

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

/* Define if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H

/* Define to 1 if you have the <io.h> header file. */
#cmakedefine HAVE_IO_H

/* Define if you have the <locale.h> header file. */
#cmakedefine HAVE_LOCALE_H

/* Define if you have the `localtime' function. */
#cmakedefine HAVE_LOCALTIME

/* Define if you have the `localtime_r' function. */
#cmakedefine HAVE_LOCALTIME_R

/* Define if you have C99's lrintf function. */
#cmakedefine HAVE_LRINTF

/* Define if you have the `pipe' function. */
#cmakedefine HAVE_PIPE

/* Define if you have the `setlocale' function. */
#cmakedefine HAVE_SETLOCALE

/* Define if <sndio.h> is available. */
#cmakedefine HAVE_SNDIO_H

/* Define if if you have libsqlite3. */
#cmakedefine HAVE_SQLITE3

/* Define if the system has the type `ssize_t'. */
#cmakedefine HAVE_SSIZE_T

/* Define if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H

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

/* Set to 1 if compiling for Win32 */
#cmakedefine01 OS_IS_WIN32

/* Name of package */
#define PACKAGE "@CPACK_PACKAGE_NAME@"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "@PACKAGE_BUGREPORT@"

/* Define to the full name of this package. */
#define PACKAGE_NAME "@CPACK_PACKAGE_NAME@"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "@CPACK_PACKAGE_NAME@ @CPACK_PACKAGE_VERSION_FULL@"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "@CPACK_PACKAGE_NAME@"

/* Define to the home page for this package. */
#define PACKAGE_URL "@PACKAGE_URL@"

/* Define to the version of this package. */
#define PACKAGE_VERSION "@CPACK_PACKAGE_VERSION_FULL@"

/* The size of `int', as computed by sizeof. */
@SIZEOF_INT_CODE@

/* The size of `int64_t', as computed by sizeof. */
@SIZEOF_INT64_T_CODE@

/* The size of `long', as computed by sizeof. */
@SIZEOF_LONG_CODE@

/* The size of `long long', as computed by sizeof. */
@SIZEOF_LONG_LONG_CODE@

/* The size of `off_t', as computed by sizeof. */
@SIZEOF_OFF_T_CODE@

/* Set to sizeof (long) if unknown. */
@SIZEOF_SF_COUNT_T_CODE@

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

/* Set to long if unknown. */
#define TYPEOF_SF_COUNT_T @TYPEOF_SF_COUNT_T@

/* Set to 1 to use the native windows API */
#cmakedefine01 USE_WINDOWS_API

/* Version number of package */
#define VERSION "@PROJECT_VERSION@"

/* Define if windows DLL is being built. */
#cmakedefine WIN32_TARGET_DLL

/* Target processor is big endian. */
#cmakedefine WORDS_BIGENDIAN

/* Define as `__inline' or '__inline__' if that's what the C compiler calls it, or to nothing if it is not supported. */
@INLINE_CODE@

#ifndef HAVE_SSIZE_T
#define ssize_t intptr_t
#endif
