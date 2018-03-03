# sndfile-to-text

add_executable(sndfile-to-text sndfile-to-text.c)
target_link_libraries(sndfile-to-text PUBLIC sndfile2k)

# sndfile-loopify

add_executable(sndfile-loopify sndfile-loopify.c)
target_link_libraries(sndfile-loopify PUBLIC sndfile2k)

# make_sine

add_executable(make_sine make_sine.c)
target_link_libraries(make_sine PUBLIC sndfile2k)
if(BUILD_SHARED_LIBS AND LIBM_REQUIRED)
  target_link_libraries(make_sine PRIVATE ${M_LIBRARY})
endif()

# sfprocess

add_executable(sfprocess sfprocess.c)
target_link_libraries(sfprocess PUBLIC sndfile2k)

# list_formats

add_executable(list_formats list_formats.c)
target_link_libraries(list_formats PUBLIC sndfile2k)
if(BUILD_SHARED_LIBS AND LIBM_REQUIRED)
  target_link_libraries(make_sine PRIVATE ${M_LIBRARY})
endif()

# sndfilehandle

add_executable(sndfilehandle sndfilehandle.cc)
target_link_libraries(sndfilehandle PUBLIC sndfile2k)

set(SNDFILE_EXAMPLE_TARGETS
  sndfile-to-text
  sndfile-loopify
  make_sine
  sfprocess
  list_formats
  sndfilehandle)

set_target_properties(${SNDFILE_EXAMPLE_TARGETS} PROPERTIES FOLDER Examples)
