# sndfile-info

add_executable(sndfile2k-info
  sndfile-info.c
  common.c
  common.h)
target_link_libraries(sndfile2k-info PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:m>)

# sndfile-play

add_executable(sndfile2k-play
  sndfile-play.c
  common.c
  common.h)
target_include_directories(sndfile2k-play
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src
    $<$<BOOL:${ALSA_FOUND}>:${ALSA_INCLUDE_DIRS}>
    $<$<BOOL:${SNDIO_FOUND}>:${SNDIO_INCLUDE_DIRS}>
)
target_link_libraries(sndfile2k-play
  PRIVATE
    sndfile2k
    $<$<OR:$<BOOL:${WIN32}>,$<BOOL:${CYGWIN}>>:${WINMM_LIBRARY}>
    $<$<BOOL:${ALSA_FOUND}>:${ALSA_LIBRARIES}>
    $<$<BOOL:${SNDIO_FOUND}>:${SNDIO_LIBRARIES}>
)

# sndfile-convert

add_executable(sndfile2k-convert
  sndfile-convert.c
  common.c
  common.h)
target_link_libraries(sndfile2k-convert PUBLIC sndfile2k)

# sndfile-cmp

add_executable(sndfile2k-cmp
  sndfile-cmp.c
  common.c
  common.h)
target_link_libraries(sndfile2k-cmp PUBLIC sndfile2k)

# sndfile-metadata-set

add_executable(sndfile2k-metadata-set
  sndfile-metadata-set.c
  common.c
  common.h)
target_link_libraries(sndfile2k-metadata-set PUBLIC sndfile2k)

# sndfile-metadata-get

add_executable(sndfile2k-metadata-get
  sndfile-metadata-get.c
  common.c
  common.h)
target_link_libraries(sndfile2k-metadata-get PUBLIC sndfile2k)

# sndfile-interleave

add_executable(sndfile2k-interleave
  sndfile-interleave.c
  common.c
  common.h)
target_link_libraries(sndfile2k-interleave PUBLIC sndfile2k)

# sndfile-deinterleave

add_executable(sndfile2k-deinterleave
  sndfile-deinterleave.c
  common.c
  common.h)
target_link_libraries(sndfile2k-deinterleave PUBLIC sndfile2k)

# sndfile-concat

add_executable(sndfile2k-concat
  sndfile-concat.c
  common.c
  common.h)
target_link_libraries(sndfile2k-concat PUBLIC sndfile2k)

# sndfile-salvage

add_executable(sndfile2k-salvage
  sndfile-salvage.c
  common.c
  common.h)
target_include_directories(sndfile2k-salvage
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(sndfile2k-salvage PUBLIC sndfile2k)

set(SNDFILE_PROGRAM_TARGETS
  sndfile2k-info
  sndfile2k-play
  sndfile2k-convert
  sndfile2k-cmp
  sndfile2k-metadata-set
  sndfile2k-metadata-get
  sndfile2k-interleave
  sndfile2k-deinterleave
  sndfile2k-concat
  sndfile2k-salvage)

set_target_properties(${SNDFILE_PROGRAM_TARGETS} PROPERTIES FOLDER Programs)


install(TARGETS ${SNDFILE_PROGRAM_TARGETS}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT BIN
)
