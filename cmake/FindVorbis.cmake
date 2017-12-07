# - Find vorbis
# Find the native vorbis includes and libraries
#
#  VORBIS_INCLUDE_DIRS - where to find vorbis.h, etc.
#  VORBIS_LIBRARIES    - List of libraries when using vorbis(file).
#  Vorbis_FOUND        - True if vorbis found.

if(VORBIS_INCLUDE_DIR)
  # Already in cache, be silent
  set(VORBIS_FIND_QUIETLY TRUE)
endif(VORBIS_INCLUDE_DIR)

if(VORBIS_FIND_QUIETLY)
  set(_FIND_OGG_ARG QUIET)
endif()

find_package(Ogg ${_FIND_OGG_ARG})

find_package(PkgConfig QUIET)
pkg_check_modules(PC_VORBIS QUIET vorbis)
pkg_check_modules(PC_VORBISFILE QUIET vorbisfile)
pkg_check_modules(PC_VORBISENC QUIET vorbisenc)

find_path(VORBIS_INCLUDE_DIR vorbis/codec.h
  HINTS ${PC_VORBIS_INCLUDEDIR} ${PC_VORBIS_INCLUDE_DIRS} ${VORBIS_ROOT}
  PATH_SUFFIXES include)
find_path(VORBISFILE_INCLUDE_DIR vorbis/vorbisfile.h
  HINTS ${PC_VORBISFILE_INCLUDEDIR} ${PC_VORBISFILE_INCLUDE_DIRS} ${VORBIS_ROOT}
  PATH_SUFFIXES include)
find_path(VORBISENC_INCLUDE_DIR vorbis/vorbisenc.h
  HINTS ${PC_VORBISENC_INCLUDEDIR} ${PC_VORBISENC_INCLUDE_DIRS} ${VORBIS_ROOT}
  PATH_SUFFIXES include)
# MSVC built vorbis may be named vorbis_static
# The provided project files name the library with the lib prefix.
find_library(VORBIS_LIBRARY
  NAMES vorbis vorbis_static libvorbis libvorbis_static
  HINTS ${PC_VORBIS_LIBDIR} ${PC_VORBIS_LIBRARY_DIRS} ${VORBIS_ROOT}
  PATH_SUFFIXES lib)
find_library(VORBISFILE_LIBRARY
  NAMES vorbisfile vorbisfile_static libvorbisfile libvorbisfile_static
  HINTS ${PC_VORBISFILE_LIBDIR} ${PC_VORBISFILE_LIBRARY_DIRS} ${VORBIS_ROOT}
  PATH_SUFFIXES lib)
find_library(VORBISENC_LIBRARY
  NAMES vorbisenc vorbisenc_static libvorbisenc libvorbisenc_static
  HINTS ${PC_VORBISENC_LIBDIR} ${PC_VORBISENC_LIBRARY_DIRS} ${VORBIS_ROOT}
  PATH_SUFFIXES lib)

# Handle the QUIETLY and REQUIRED arguments and set VORBIS_FOUND
# to TRUE if all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Vorbis DEFAULT_MSG VORBIS_INCLUDE_DIR VORBIS_LIBRARY OGG_FOUND)
find_package_handle_standard_args(VorbisFile DEFAULT_MSG VORBISFILE_INCLUDE_DIR VORBISFILE_LIBRARY VORBIS_FOUND)
find_package_handle_standard_args(VorbisEnc DEFAULT_MSG VORBISENC_INCLUDE_DIR VORBIS_FOUND)

if(VORBIS_FOUND)
  set(VORBIS_INCLUDE_DIRS ${VORBIS_INCLUDE_DIR} ${OGG_INCLUDE_DIRS})
  set(VORBIS_LIBRARIES ${VORBIS_LIBRARY} ${OGG_LIBRARIES})

  if(NOT TARGET Vorbis)
    add_library(Vorbis UNKNOWN IMPORTED)
    set_target_properties(Vorbis PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${VORBIS_INCLUDE_DIR}
      IMPORTED_LOCATION ${VORBIS_LIBRARY}
      INTERFACE_LINK_LIBRARIES Ogg)
  endif()
endif(VORBIS_FOUND)

if(VORBISFILE_FOUND)
  set(VORBISFILE_INCLUDE_DIRS ${VORBISFILE_INCLUDE_DIR} ${OGG_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS})
  set(VORBISFILE_LIBRARIES ${VORBISFILE_LIBRARY} ${OGG_LIBRARIES} ${VORBIS_LIBRARIES})

  if(NOT TARGET VorbisFile)
    add_library(VorbisFile UNKNOWN IMPORTED)
    set_target_properties(VorbisFile PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${VORBISFILE_INCLUDE_DIR}
      IMPORTED_LOCATION ${VORBISFILE_LIBRARY}
      INTERFACE_LINK_LIBRARIES Vorbis)
  endif()
endif()

if(VORBISENC_FOUND)
  set(VORBISENC_INCLUDE_DIRS ${VORBISENC_INCLUDE_DIR} ${OGG_INCLUDE_DIRS} ${VORBIS_INCLUDE_DIRS})
  set(VORBISFILE_LIBRARIES ${VORBISENC_LIBRARY} ${OGG_LIBRARIES} ${VORBIS_LIBRARIES})

  if(NOT TARGET VorbisEnc)
    add_library(VorbisEnc UNKNOWN IMPORTED)
    set_target_properties(VorbisEnc PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${VORBISENC_INCLUDE_DIR}
      IMPORTED_LOCATION ${VORBISENC_LIBRARY}
      INTERFACE_LINK_LIBRARIES Vorbis)
  endif()
endif()

mark_as_advanced(VORBIS_INCLUDE_DIR VORBIS_LIBRARY)
mark_as_advanced(VORBISFILE_INCLUDE_DIR VORBISFILE_LIBRARY)
mark_as_advanced(VORBISENC_INCLUDE_DIR VORBISENC_LIBRARY)
