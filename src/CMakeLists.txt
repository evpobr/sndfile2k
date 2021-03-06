set(SNDFILE2K_SOVERSION_MAJOR 1)
set(SNDFILE2K_SOVERSION_MINOR 2)
set(SNDFILE2K_SOVERSION_PATCH 0)
set(SNDFILE2K_SOVERSION "${SNDFILE2K_SOVERSION_MAJOR}.${SNDFILE2K_SOVERSION_MINOR}.${SNDFILE2K_SOVERSION_PATCH}")

# Public headers
set(libsndfile2k_PUBLIC_HEADERS
  ${PROJECT_SOURCE_DIR}/include/sndfile2k/sndfile2k.hpp
  ${PROJECT_SOURCE_DIR}/include/sndfile2k/sndfile2k.h)

# Private headers
set(noinst_HEADERS
  common.h
  config.h
  sfendian.h
  wavlike.h
  sf_unistd.h
  ogg.h
  chanmap.h
  shift.h
)

# Common sources
set(COMMON
  common.cpp
  command.cpp
  pcm.cpp
  ulaw.cpp
  alaw.cpp
  float32.cpp
  double64.cpp
  ima_adpcm.cpp
  ms_adpcm.cpp
  gsm610.cpp
  dwvw.cpp
  vox_adpcm.cpp
  interleave.cpp
  strings.cpp
  dither.cpp
  audio_detect.cpp
  ima_oki_adpcm.cpp
  ima_oki_adpcm.h
  alac.cpp
  chunk.cpp
  ogg.cpp
  chanmap.cpp
  file_io.cpp
  sndfile_error.h
  ref_ptr.cpp
  ref_ptr.h
)
if(WIN32 OR CYGWIN)
  list(APPEND COMMON windows.cpp file_io_win32.cpp)
endif()

# File specific sources
set(FILESPECIFIC
  sndfile2k.cpp
  aiff.cpp
  au.cpp
  avr.cpp
  caf.cpp
  dwd.cpp
  flac.cpp
  g72x.cpp
  htk.cpp
  ircam.cpp
  mat4.cpp
  mat5.cpp
  nist.cpp
  paf.cpp
  pvf.cpp
  raw.cpp
  rx2.cpp
  sds.cpp
  svx.cpp
  txw.cpp
  voc.cpp
  wve.cpp
  w64.cpp
  wavlike.cpp
  wav.cpp
  xi.cpp
  mpc2k.cpp
  rf64.cpp
  ogg_vorbis.cpp
  ogg_speex.cpp
  ogg_pcm.cpp
  ogg_opus.cpp
  nms_adpcm.cpp
)

# GSM610 sources
set(libgsm_SOURCES
  GSM610/gsm.h
  GSM610/gsm610_priv.h
  GSM610/add.c
  GSM610/code.c
  GSM610/decode.c
  GSM610/gsm_create.c
  GSM610/gsm_decode.c
  GSM610/gsm_destroy.c
  GSM610/gsm_encode.c
  GSM610/gsm_option.c
  GSM610/long_term.c
  GSM610/lpc.c
  GSM610/preprocess.c
  GSM610/rpe.c
  GSM610/short_term.c
  GSM610/table.c)
source_group("GSM610" FILES ${libgsm_SOURCES})

# G72x sources
set(libg72x_SOURCES
  G72x/g72x.h
  G72x/g72x_priv.h
  G72x/g721.c
  G72x/g723_16.c
  G72x/g723_24.c
  G72x/g723_40.c
  G72x/g72x.c)
source_group("G72x" FILES ${libg72x_SOURCES})

# ALAC sources
set(libalac_SOURCES
  ALAC/ALACAudioTypes.h
  ALAC/ALACBitUtilities.h
  ALAC/EndianPortable.h
  ALAC/aglib.h
  ALAC/dplib.h
  ALAC/matrixlib.h
  ALAC/alac_codec.h
  ${PROJECT_SOURCE_DIR}/src/shift.h
  ALAC/ALACBitUtilities.c
  ALAC/ag_dec.c
  ALAC/ag_enc.c
  ALAC/dp_dec.c
  ALAC/dp_enc.c
  ALAC/matrix_dec.c
  ALAC/matrix_enc.c
  ALAC/alac_decoder.c
  ALAC/alac_encoder.c)
source_group("ALAC" FILES ${libalac_SOURCES})

# Headers to be installed on target machine
set(libsndfile2k_SOURCES ${COMMON} ${FILESPECIFIC} ${noinst_HEADERS} 
  ${libgsm_SOURCES} ${libg72x_SOURCES} ${libalac_SOURCES}
  ${libsndfile2k_PUBLIC_HEADERS})

#
# sndfile2k library
#

add_library(sndfile2k ${libsndfile2k_SOURCES})
target_include_directories(sndfile2k
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      $<$<AND:$<BOOL:${HAVE_XIPH_CODECS}>,$<BOOL:${ENABLE_EXPERIMENTAL}>>:${SPEEX_INCLUDE_DIRS}>
)

target_sources(sndfile2k PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/sndfile2k_export.h)
list(APPEND libsndfile2k_PUBLIC_HEADERS ${CMAKE_CURRENT_BINARY_DIR}/sndfile2k_export.h)
target_link_libraries(sndfile2k PRIVATE
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>
  $<$<BOOL:${HAVE_XIPH_CODECS}>:VorbisEnc>
  $<$<BOOL:${HAVE_XIPH_CODECS}>:FLAC>
  $<$<AND:$<BOOL:${HAVE_XIPH_CODECS}>,$<BOOL:${ENABLE_EXPERIMENTAL}>>:${SPEEX_LIBRARIES}>
)

set_target_properties(sndfile2k
  PROPERTIES
    SOVERSION ${SNDFILE2K_SOVERSION_MAJOR}
    VERSION ${SNDFILE2K_SOVERSION}
    DEFINE_SYMBOL SNDFILE2K_EXPORTS)

target_compile_definitions(sndfile2k PRIVATE _USE_MATH_DEFINES)

if(BUILD_SHARED_LIBS)

  set_target_properties(sndfile2k
    PROPERTIES
      C_VISIBILITY_PRESET hidden
      VISIBILITY_INLINES_HIDDEN 1)

  if(WIN32 OR CYGWIN)
    set(VER_FILEVERSION "${SNDFILE2K_SOVERSION_MAJOR},${SNDFILE2K_SOVERSION_MINOR},${SNDFILE2K_SOVERSION_PATCH},0")
    set(VER_PRODUCTVERSION "${PROJECT_VERSION_MAJOR},${PROJECT_VERSION_MINOR},${PROJECT_VERSION_PATCH},0")

    set(VER_FILEVERSION_STR ${SNDFILE2K_SOVERSION})
    set(VER_PRODUCTVERSION_STR ${PROJECT_VERSION})

    # No idea why CMake adds SONAME for Cygwin, but not for MinGW.
    # But OriginalFilename field is required, so we need to handle it.
    if(CYGWIN AND (NOT WIN32))
      set(VER_ORIGINALFILENAME_STR ${CMAKE_SHARED_LIBRARY_PREFIX}sndfile2k-1${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
      set(VER_ORIGINALFILENAME_STR ${CMAKE_SHARED_LIBRARY_PREFIX}sndfile2k${CMAKE_SHARED_LIBRARY_SUFFIX})
    endif()
    configure_file(version-metadata.rc.cmake version-metadata.rc @ONLY)
    list(APPEND libsndfile_SOURCES
      ${CMAKE_CURRENT_BINARY_DIR}/version-metadata.rc)
  endif()

endif()

include(GenerateExportHeader)

generate_export_header(sndfile2k)

if(UNIX)
  set(CMAKE_INSTALL_PACKAGEDIR ${CMAKE_INSTALL_LIBDIR}/cmake/sndfile2k)
elseif(WIN32)
  set(CMAKE_INSTALL_PACKAGEDIR cmake)
endif()

install(TARGETS sndfile2k
  EXPORT SndFile2KTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT BIN
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT DEV
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT BIN)

install(FILES ${libsndfile2k_PUBLIC_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/sndfile2k COMPONENT DEV)

include(CMakePackageConfigHelpers)

write_basic_package_version_file(SndFile2KConfigVersion.cmake COMPATIBILITY SameMajorVersion)
configure_package_config_file(
  ../cmake/SndFile2KConfig.in.cmake
  SndFile2KConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PACKAGEDIR})

if(ENABLE_CMAKE_PACKAGE_CONFIG)
  install(EXPORT SndFile2KTargets DESTINATION ${CMAKE_INSTALL_PACKAGEDIR} COMPONENT DEV)
  install(
    FILES
      ${CMAKE_CURRENT_BINARY_DIR}/SndFile2KConfig.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/SndFile2KConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_PACKAGEDIR}
    COMPONENT DEV)
endif()

if(ENABLE_PKGCONFIG_FILE)

  if(HAVE_XIPH_CODECS)
    set(PC_REQUIRES_PRIVATE "flac >= 1.3 vorbis >= 1.2 vorbisenc >= 1.2")
  endif()
  set(PC_PRIVATE_LIBS "-lm")

  configure_file(${PROJECT_SOURCE_DIR}/sndfile2k.pc.in ${PROJECT_BINARY_DIR}/sndfile2k.pc @ONLY)
    install(
      FILES ${PROJECT_BINARY_DIR}/sndfile2k.pc
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
      COMPONENT DEV
    )
  endif()


  ### test_main

add_executable(test_main
  test_main.cpp
  test_main.h
  test_conversions.cpp
  test_float.cpp
  test_endswap.cpp
  test_audio_detect.cpp
  test_log_printf.cpp
  test_file_io.cpp
  test_ima_oki_adpcm.cpp
  test_strncpy_crlf.cpp
  test_binheader_writef.cpp
  test_nms_adpcm.cpp
  ${libsndfile2k_SOURCES}
)
target_include_directories(test_main
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}/src>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<$<AND:$<BOOL:${HAVE_XIPH_CODECS}>,$<BOOL:${ENABLE_EXPERIMENTAL}>>:${SPEEX_INCLUDE_DIRS}>
)

target_sources(test_main PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/sndfile2k_export.h)
target_link_libraries(test_main PRIVATE
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>
  $<$<BOOL:${HAVE_XIPH_CODECS}>:VorbisEnc>
  $<$<BOOL:${HAVE_XIPH_CODECS}>:FLAC>
  $<$<AND:$<BOOL:${HAVE_XIPH_CODECS}>,$<BOOL:${ENABLE_EXPERIMENTAL}>>:${SPEEX_LIBRARIES}>
)

if(MSVC)
  target_compile_definitions(test_main PRIVATE _USE_MATH_DEFINES)
endif()
target_link_libraries(test_main PRIVATE $<$<BOOL:${LIBM_REQUIRED}>:m>)

add_test(NAME test_main COMMAND $<TARGET_FILE:test_main>)
