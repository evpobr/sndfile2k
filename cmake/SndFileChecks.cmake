include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(CheckTypeSize)
include(TestBigEndian)

include(ClipMode)
include(TestLargeFiles)

if(NOT WIN32)
  test_large_files(_LARGEFILES)

  if(LARGE_FILES_DEFINITIONS)
    add_definitions(${LARGE_FILES_DEFINITIONS})
  endif()
endif()

if(NOT WIN32)
  find_package(ALSA)
  if(ALSA_FOUND)
    set(HAVE_ALSA_ASOUNDLIB_H TRUE)
  else()
    find_package(Sndio)
    set(HAVE_SNDIO_H ${SNDIO_FOUND})
  endif(ALSA_FOUND)
endif()

find_package(Vorbis)
find_package(FLAC)
if(VORBISENC_FOUND AND FLAC_FOUND)
  set(HAVE_EXTERNAL_XIPH_LIBS 1)
endif()

find_package(Speex)

find_package(SQLite3)
if(SQLITE3_FOUND)
  set(HAVE_SQLITE3 1)
endif()

if(BUILD_PROGRAMS)
  if(WIN32)
    set(WINMM_LIBRARY "winmm")
  elseif(CYGWIN)
    set(WINMM_LIBRARY "libwinmm")
  endif()
endif()

check_include_file(byteswap.h       HAVE_BYTESWAP_H)
check_include_file(direct.h         HAVE_DIRECT_H)
check_include_file(inttypes.h       HAVE_INTTYPES_H)
if(BUILD_TESTING)
  check_include_file(locale.h       HAVE_LOCALE_H)
  if(NOT WIN32)
    check_include_file(sys/wait.h   HAVE_SYS_WAIT_H)
  endif()
endif()
check_include_file(stdint.h         HAVE_STDINT_H)
check_include_file(sys/time.h       HAVE_SYS_TIME_H)
check_include_file(sys/types.h      HAVE_SYS_TYPES_H)
check_include_file(unistd.h         HAVE_UNISTD_H)
if(NOT HAVE_UNISTD_H)
  check_include_file(io.h           HAVE_IO_H)
endif()
check_include_file(wchar.h          HAVE_WCHAR_H)
check_type_size(int64_t             SIZEOF_INT64_T)
check_type_size(int                 SIZEOF_INT)
check_type_size(long                SIZEOF_LONG)
check_type_size(long\ long          SIZEOF_LONG_LONG)
check_type_size(short               SIZEOF_SHORT)
check_type_size(size_t              SIZEOF_SIZE_T)
check_type_size(ssize_t             SIZEOF_SSIZE_T)
check_type_size(void*               SIZEOF_VOIDP)
check_type_size(wchar_t             SIZEOF_WCHAR_T)

if((SIZEOF_OFF_T EQUAL 8) OR WIN32)
  set(TYPEOF_SF_COUNT_T "int64_t")
  set(SIZEOF_SF_COUNT_T 8)
else()
  message("")
  message("*** The configure process has determined that this system is capable")
  message("*** of Large File Support but has not been able to find a type which")
  message("*** is an unambiguous 64 bit file offset.")
  message("*** Please contact the author to help resolve this problem.")
  message("")
  message(FATAL_ERROR "Bad file offset type.")
endif()

find_library(M_LIBRARY m)
if(M_LIBRARY)
  # Check if he need to link 'm' for math functions
  check_library_exists(m floor "" LIBM_REQUIRED)
  if(LIBM_REQUIRED)
    list(APPEND CMAKE_REQUIRED_LIBRARIES ${M_LIBRARY})
  else()
    unset(M_LIBRARY)
  endif()
endif()
mark_as_advanced(M_LIBRARY)

check_function_exists(fstat         HAVE_FSTAT)
check_function_exists(fstat64       HAVE_FSTAT64)
check_function_exists(fsync         HAVE_FSYNC)
check_function_exists(gettimeofday  HAVE_GETTIMEOFDAY)
check_function_exists(gmtime_r      HAVE_GMTIME_R)
if(NOT HAVE_GMTIME_R)
  check_function_exists(gmtime      HAVE_GMTIME)
endif()
check_function_exists(localtime_r   HAVE_LOCALTIME_R)
if(NOT HAVE_LOCALTIME_R)
  check_function_exists(localtime   HAVE_LOCALTIME)
endif()
check_function_exists(pipe          HAVE_PIPE)
if(BUILD_TESTING)
  check_function_exists(setlocale   HAVE_SETLOCALE)
  if(HAVE_SYS_WAIT_H)
    check_function_exists(waitpid   HAVE_WAITPID)
  endif()
endif()
check_function_exists(lrintf        HAVE_LRINTF)

check_symbol_exists(S_IRGRP sys/stat.h HAVE_DECL_S_IRGRP)

test_big_endian(WORDS_BIGENDIAN)
if(WORDS_BIGENDIAN)
  set(WORDS_BIGENDIAN 1)
  set(CPU_IS_BIG_ENDIAN 1)
else(${LITTLE_ENDIAN})
  set(CPU_IS_LITTLE_ENDIAN 1)
endif()

if(WIN32)
  set(USE_WINDOWS_API 1)
  if(BUILD_SHARED_LIBS)
    set(WIN32_TARGET_DLL 1)
  endif()
if(MINGW)
  set(__USE_MINGW_ANSI_STDIO 1)
endif(MINGW)
endif(WIN32)

if(${CMAKE_SYSTEM_NAME} STREQUAL "OpenBSD")
  set(OS_IS_OPENBSD 1)
endif()


if(CMAKE_COMPILER_IS_GNUCC OR(CMAKE_C_COMPILER_ID MATCHES "Clang"))
  set(COMPILER_IS_GCC 1)
endif()

if(ENABLE_EXPERIMENTAL)
  set(ENABLE_EXPERIMENTAL_CODE 1)
endif()

if(ENABLE_CPU_CLIP)
  clip_mode()
endif()

if(MSVC)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
endif(MSVC)

if(ENABLE_STATIC_RUNTIME)
  if(MSVC)
    foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
      endif(${flag_var} MATCHES "/MD")
    endforeach(flag_var)
  endif(MSVC)
  if(MINGW)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_C_FLAGS} -static-libgcc -s")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS} -static-libgcc -static-libstdc++ -s")
  endif(MINGW)
elseif(NOT ENABLE_STATIC_RUNTIME)
  if(MSVC)
    foreach(flag_var
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
      CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
      CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
      CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
      if(${flag_var} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${flag_var} "${${flag_var}}")
      endif(${flag_var} MATCHES "/MT")
    endforeach(flag_var)
  endif(MSVC)
  if(MINGW)
    set(CMAKE_C_FLAGS "")
    set(CMAKE_CXX_FLAGS "")
    set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
    set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
  endif(MINGW)
endif(ENABLE_STATIC_RUNTIME)
