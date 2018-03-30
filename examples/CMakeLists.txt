# sndfile-to-text

add_executable(sndfile-to-text sndfile-to-text.cpp)
target_link_libraries(sndfile-to-text PUBLIC sndfile2k)

# sndfile-loopify

add_executable(sndfile-loopify sndfile-loopify.cpp)
target_link_libraries(sndfile-loopify PUBLIC sndfile2k)

# make_sine

add_executable(make_sine make_sine.cpp)
target_link_libraries(make_sine PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:m>)

# sfprocess

add_executable(sfprocess sfprocess.cpp)
target_link_libraries(sfprocess PUBLIC sndfile2k)

# list_formats

add_executable(list_formats list_formats.cpp)
target_link_libraries(list_formats PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:m>)

# sndfilehandle

add_executable(sndfilehandle sndfilehandle.cpp)
target_link_libraries(sndfilehandle PUBLIC sndfile2k)

set(SNDFILE_EXAMPLE_TARGETS
  sndfile-to-text
  sndfile-loopify
  make_sine
  sfprocess
  list_formats
  sndfilehandle)

set_target_properties(${SNDFILE_EXAMPLE_TARGETS} PROPERTIES FOLDER Examples)
