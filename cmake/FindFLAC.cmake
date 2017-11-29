# - Find FLAC
# Find the native FLAC includes and libraries
#
#  FLAC_INCLUDE_DIRS - where to find FLAC headers.
#  FLAC_LIBRARIES    - List of libraries when using libFLAC.
#  FLAC_FOUND        - True if libFLAC found.
#  FLAC_DEFINITIONS  - FLAC compile definitons 

if(FLAC_INCLUDE_DIR)
  # Already in cache, be silent
  set(FLAC_FIND_QUIETLY TRUE)
endif(FLAC_INCLUDE_DIR)

if(FLAC_FIND_QUIETLY)
  set(_FIND_OGG_ARG QUIET)
endif()

find_package(Ogg QUIET)

find_package(PkgConfig QUIET)
pkg_check_modules(PC_FLAC QUIET flac)

find_path(FLAC_INCLUDE_DIR FLAC/stream_decoder.h
  HINTS ${PC_FLAC_INCLUDEDIR} ${PC_FLAC_INCLUDE_DIRS} ${FLAC_ROOT}
  PATH_SUFFIXES include)

# MSVC built libraries can name them *_static, which is good as it
# distinguishes import libraries from static libraries with the same extension.
find_library(FLAC_LIBRARY NAMES FLAC libFLAC libFLAC_dynamic libFLAC_static
  HINTS ${PC_FLAC_LIBDIR} ${PC_FLAC_LIBRARY_DIRS} ${FLAC_ROOT}
  PATH_SUFFIXES lib)

# Handle the QUIETLY and REQUIRED arguments and set FLAC_FOUND to TRUE if
# all listed variables are TRUE.
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FLAC DEFAULT_MSG
  FLAC_INCLUDE_DIR FLAC_LIBRARY Ogg_FOUND)

if(FLAC_FOUND)
  set(FLAC_INCLUDE_DIRS ${FLAC_INCLUDE_DIR} ${OGG_INCLUDE_DIRS})
  set(FLAC_LIBRARIES ${FLAC_LIBRARY} ${OGG_LIBRARIES})
  set(FLAC_DEFINITIONS "")
  if(WIN32)
    set(FLAC_LIBRARIES ${FLAC_LIBRARIES} wsock32)
    get_filename_component(FLAC_LIBRARY_FILENAME ${FLAC_LIBRARY} NAME_WE)
    if(FLAC_LIBRARY_FILENAME MATCHES "libFLAC_static")
      set(FLAC_IS_STATIC 1)
      set(FLAC_DEFINITIONS -DFLAC__NO_DLL)
    endif()
  endif(WIN32)

  if(NOT TARGET FLAC)
    if(FLAC_IS_STATIC)
      set(FLAC_TARGET_TYPE "STATIC")
    else()
      set(FLAC_TARGET_TYPE "UNKNOWN")
    endif()
    add_library(FLAC ${FLAC_TARGET_TYPE} IMPORTED)
    unset(FLAC_IS_STATIC)    
    unset(FLAC_TARGET_TYPE)
    set_target_properties(FLAC PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES ${FLAC_INCLUDE_DIR}
      INTERFACE_COMPILE_OPTIONS "${FLAC_DEFINITIONS}"
      IMPORTED_LOCATION ${FLAC_LIBRARY}
      INTERFACE_LINK_LIBRARIES Ogg)
  endif()
endif(FLAC_FOUND)

mark_as_advanced(FLAC_INCLUDE_DIR FLAC_LIBRARY)
