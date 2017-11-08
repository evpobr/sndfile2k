# - Find WinMM
# Find the native winmm includes and libraries
#
#  WINMM_INCLUDE_DIRS - where to find mmsystem.h, etc.
#  WINMM_LIBRARIES    - List of libraries when using winmm.
#  WINMM_FOUND        - True if winmm found.

if(WINMM_INCLUDE_DIR)
    # Already in cache, be silent
    set(WINMM_FIND_QUIETLY TRUE)
endif(WINMM_INCLUDE_DIR)

if(WIN32)

  find_path(WINMM_INCLUDE_DIR "")
  set(WINMM_LIBRARY winmm)

else()

  find_path(WINMM_INCLUDE_DIR mmsystem.h windows.h)
  find_library(WINMM_LIBRARY winmm)

endif()

  # Handle the QUIETLY and REQUIRED arguments and set WINMM_FOUND
  # to TRUE if all listed variables are TRUE.
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(WinMM DEFAULT_MSG WINMM_INCLUDE_DIR WINMM_LIBRARY)

if (WINMM_FOUND)
	set (WINMM_LIBRARIES ${WINMM_LIBRARY})
	set (WINMM_INCLUDE_DIRS ${WINMM_INCLUDE_DIR})
endif (WINMM_FOUND)

mark_as_advanced(WINMM_INCLUDE_DIR WINMM_LIBRARY)
