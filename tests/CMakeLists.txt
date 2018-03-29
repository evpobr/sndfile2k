### test_main

add_executable(test_main
  ../src/test_main.c
  ../src/test_main.h
  ../src/test_conversions.c
  ../src/test_float.c
  ../src/test_endswap.c
  ../src/test_audio_detect.c
  ../src/test_log_printf.c
  ../src/test_file_io.c
  ../src/test_ima_oki_adpcm.c
  ../src/test_strncpy_crlf.c
  ../src/test_cart_var.c
  ../src/test_binheader_writef.c
  ../src/test_nms_adpcm.c)
target_link_libraries(test_main PUBLIC sndfile2k)
if(MSVC)
  target_compile_definitions(test_main PRIVATE _USE_MATH_DEFINES)
endif()
target_link_libraries(test_main PRIVATE $<$<BOOL:${LIBM_REQUIRED}>:m>)
if(BUILD_SHARED_LIBS)
  target_include_directories(test_main
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
      ${PROJECT_BINARY_DIR}/src)
  target_sources(test_main
    PRIVATE
      ../src/common.c
      ../src/float32.c
      ../src/double64.c
      ../src/file_io.c
      ../src/audio_detect.c
      ../src/cart.c)
endif()
add_test(NAME test_main COMMAND $<TARGET_FILE:test_main>)

### sfversion_test

add_executable(sfversion sfversion.c)
target_link_libraries(sfversion sndfile2k)
add_test(NAME sfversion COMMAND $<TARGET_FILE:sfversion>)
set_tests_properties(sfversion PROPERTIES
  PASS_REGULAR_EXPRESSION "sndfile2k-${PROJECT_VERSION}")

### error_test

add_executable(error_test error_test.c utils.c utils.h)
target_include_directories(error_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(error_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:m>
)
add_test(NAME error_test COMMAND $<TARGET_FILE:error_test>)

### ulaw_test

add_executable(ulaw_test ulaw_test.c utils.c utils.h)
target_include_directories(ulaw_test
  PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src)
target_link_libraries(ulaw_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:m>
)
add_test(NAME ulaw_test COMMAND $<TARGET_FILE:ulaw_test>)

### alaw_test

add_executable(alaw_test alaw_test.c utils.c utils.h)
target_include_directories(alaw_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(alaw_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>
)
add_test(NAME alaw_test COMMAND $<TARGET_FILE:alaw_test>)

### dwvw_test

add_executable(dwvw_test dwvw_test.c utils.c utils.h)
target_include_directories(dwvw_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(dwvw_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>
)
add_test(NAME dwvw_test COMMAND $<TARGET_FILE:dwvw_test>)

### command_test

add_executable(command_test command_test.c utils.c utils.h)
target_include_directories(command_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(command_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME command_test COMMAND $<TARGET_FILE:command_test> all)

### floating_point_test

add_executable(floating_point_test dft_cmp.c floating_point_test.c
  utils.c utils.h)
target_include_directories(floating_point_test
PRIVATE
  ${PROJECT_SOURCE_DIR}/src
  ${PROJECT_BINARY_DIR}/src)
target_link_libraries(floating_point_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME floating_point_test COMMAND $<TARGET_FILE:floating_point_test>)

### checksum_test

add_executable(checksum_test checksum_test.c utils.c utils.h)
target_include_directories(checksum_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(checksum_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME checksum_test COMMAND $<TARGET_FILE:checksum_test>)

### scale_clip_test

add_executable(scale_clip_test scale_clip_test.c utils.c utils.h)
target_include_directories(scale_clip_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(scale_clip_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME scale_clip_test COMMAND $<TARGET_FILE:scale_clip_test>)

### headerless_test

add_executable(headerless_test headerless_test.c utils.c utils.h)
target_include_directories(headerless_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(headerless_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME headerless_test COMMAND $<TARGET_FILE:headerless_test>)

### rdwr_test

add_executable(rdwr_test rdwr_test.c utils.c utils.h)
target_include_directories(rdwr_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(rdwr_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME rdwr_test COMMAND $<TARGET_FILE:rdwr_test>)

### locale_test

add_executable(locale_test locale_test.c utils.c utils.h)
target_include_directories(locale_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(locale_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME locale_test COMMAND $<TARGET_FILE:locale_test>)

### cpp_test

add_executable(cpp_test cpp_test.cc utils.c utils.h)
target_include_directories(cpp_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(cpp_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME cpp_test COMMAND $<TARGET_FILE:cpp_test>)

### external_libs_test

add_executable(external_libs_test external_libs_test.c utils.c utils.h)
target_include_directories(external_libs_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(external_libs_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME external_libs_test COMMAND $<TARGET_FILE:external_libs_test>)

### format_check_test

add_executable(format_check_test format_check_test.c utils.c utils.h)
target_include_directories(format_check_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(format_check_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME format_check_test COMMAND $<TARGET_FILE:format_check_test>)

### channel_test

add_executable(channel_test channel_test.c utils.c utils.h)
target_link_libraries(channel_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
target_include_directories(channel_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
add_test(NAME channel_test COMMAND $<TARGET_FILE:channel_test>)

### pcm_test

add_executable(pcm_test pcm_test.c utils.c utils.h)
target_include_directories(pcm_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(pcm_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME pcm_test COMMAND $<TARGET_FILE:pcm_test>)

### common test executables

add_executable(write_read_test generate.c write_read_test.c utils.c utils.h)
target_include_directories(write_read_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(write_read_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
target_include_directories(write_read_test PRIVATE tests)

add_executable(lossy_comp_test lossy_comp_test.c utils.c utils.h)
target_include_directories(lossy_comp_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(lossy_comp_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(peak_chunk_test peak_chunk_test.c utils.c utils.h)
target_include_directories(peak_chunk_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(peak_chunk_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(header_test header_test.c utils.c utils.h)
target_include_directories(header_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(header_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(misc_test misc_test.c utils.c utils.h)
target_include_directories(misc_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(misc_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(string_test string_test.c utils.c utils.h)
target_include_directories(string_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(string_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(multi_file_test multi_file_test.c utils.c utils.h)
target_include_directories(multi_file_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(multi_file_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(aiff_rw_test aiff_rw_test.c utils.c utils.h)
target_include_directories(aiff_rw_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(aiff_rw_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(chunk_test chunk_test.c utils.c utils.h)
target_include_directories(chunk_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(chunk_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(long_read_write_test long_read_write_test.c utils.c utils.h)
target_include_directories(long_read_write_test
  PRIVATE
  ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(long_read_write_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(raw_test raw_test.c utils.c utils.h)
target_include_directories(raw_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(raw_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(compression_size_test compression_size_test.c utils.c utils.h)
target_include_directories(compression_size_test
  PRIVATE
  ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(compression_size_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(ogg_test ogg_test.c utils.c utils.h)
target_include_directories(ogg_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(ogg_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(stdin_test stdin_test.c utils.c utils.h)
target_include_directories(stdin_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(stdin_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
set_target_properties(stdin_test
  PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "tests")

add_executable(stdout_test stdout_test.c utils.c utils.h)
target_include_directories(stdout_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(stdout_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
set_target_properties(stdout_test
PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "tests")

add_executable(stdio_test stdio_test.c utils.c utils.h)
target_include_directories(stdio_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(stdio_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(pipe_test pipe_test.c utils.c utils.h)
target_include_directories(pipe_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(pipe_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

add_executable(virtual_io_test virtual_io_test.c utils.c utils.h)
target_include_directories(virtual_io_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(virtual_io_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)

### g72x_test

add_executable(g72x_test
  ${PROJECT_SOURCE_DIR}/src/G72x/g72x_test.c
  ${PROJECT_SOURCE_DIR}/src/G72x/g72x.h
  ${PROJECT_SOURCE_DIR}/src/G72x/g72x_priv.h
  ${PROJECT_SOURCE_DIR}/src/G72x/g721.c
  ${PROJECT_SOURCE_DIR}/src/G72x/g723_16.c
  ${PROJECT_SOURCE_DIR}/src/G72x/g723_24.c
  ${PROJECT_SOURCE_DIR}/src/G72x/g723_40.c
  ${PROJECT_SOURCE_DIR}/src/G72x/g72x.c)
target_include_directories(g72x_test
  PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_BINARY_DIR}/src)
target_link_libraries(g72x_test PRIVATE
  sndfile2k
  $<$<BOOL:${LIBM_REQUIRED}>:${M_LIBRARY}>)
add_test(NAME g72x_test COMMAND $<TARGET_FILE:g72x_test> all)

### aiff-tests

add_test(NAME write_read_test_aiff COMMAND $<TARGET_FILE:write_read_test> aiff)
add_test(NAME lossy_comp_test_aiff_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> aiff_ulaw)
add_test(NAME lossy_comp_test_aiff_alaw COMMAND $<TARGET_FILE:lossy_comp_test> aiff_alaw)
add_test(NAME lossy_comp_test_aiff_gsm610 COMMAND $<TARGET_FILE:lossy_comp_test> aiff_gsm610)
add_test(NAME peak_chunk_test_aiff COMMAND $<TARGET_FILE:peak_chunk_test> aiff)
add_test(NAME header_test_aiff COMMAND $<TARGET_FILE:header_test> aiff)
add_test(NAME misc_test_aiff COMMAND $<TARGET_FILE:misc_test> aiff)
add_test(NAME string_test_aiff COMMAND $<TARGET_FILE:string_test> aiff)
add_test(NAME multi_file_test_aiff COMMAND $<TARGET_FILE:multi_file_test> aiff)
add_test(NAME aiff_rw_test COMMAND $<TARGET_FILE:aiff_rw_test>)

### au-tests

add_test(NAME write_read_test_au COMMAND $<TARGET_FILE:write_read_test> au)
add_test(NAME lossy_comp_test_au_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> au_ulaw)
add_test(NAME lossy_comp_test_au_alaw COMMAND $<TARGET_FILE:lossy_comp_test> au_alaw)
add_test(NAME lossy_comp_test_au_g721 COMMAND $<TARGET_FILE:lossy_comp_test> au_g721)
add_test(NAME lossy_comp_test_au_g723 COMMAND $<TARGET_FILE:lossy_comp_test> au_g723)
add_test(NAME header_test_au COMMAND $<TARGET_FILE:header_test> au)
add_test(NAME misc_test_au COMMAND $<TARGET_FILE:misc_test> au)
add_test(NAME multi_file_test_au COMMAND $<TARGET_FILE:multi_file_test> au)

### caf-tests

add_test(NAME write_read_test_caf COMMAND $<TARGET_FILE:write_read_test> caf)
add_test(NAME lossy_comp_test_caf_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> caf_ulaw)
add_test(NAME lossy_comp_test_caf_alaw COMMAND $<TARGET_FILE:lossy_comp_test> caf_alaw)
add_test(NAME header_test_caf COMMAND $<TARGET_FILE:header_test> caf)
add_test(NAME peak_chunk_test_caf COMMAND $<TARGET_FILE:peak_chunk_test> caf)
add_test(NAME misc_test_caf COMMAND $<TARGET_FILE:misc_test> caf)
add_test(NAME chunk_test_caf COMMAND $<TARGET_FILE:chunk_test> caf)
add_test(NAME string_test_caf COMMAND $<TARGET_FILE:string_test> caf)
add_test(NAME long_read_write_test_alac COMMAND $<TARGET_FILE:long_read_write_test> alac)

### wav-tests

add_test(NAME write_read_test_wav COMMAND $<TARGET_FILE:write_read_test> wav)
add_test(NAME lossy_comp_test_wav_pcm COMMAND $<TARGET_FILE:lossy_comp_test> wav_pcm)
add_test(NAME lossy_comp_test_wav_ima COMMAND $<TARGET_FILE:lossy_comp_test> wav_ima)
add_test(NAME lossy_comp_test_wav_msadpcm COMMAND $<TARGET_FILE:lossy_comp_test> wav_msadpcm)
add_test(NAME lossy_comp_test_wav_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> wav_ulaw)
add_test(NAME lossy_comp_test_wav_alaw COMMAND $<TARGET_FILE:lossy_comp_test> wav_alaw)
add_test(NAME lossy_comp_test_wav_gsm610 COMMAND $<TARGET_FILE:lossy_comp_test> wav_gsm610)
add_test(NAME lossy_comp_test_wav_g721 COMMAND $<TARGET_FILE:lossy_comp_test> wav_g721)
add_test(NAME lossy_comp_test_wav_nmsadpcm COMMAND $<TARGET_FILE:lossy_comp_test> wav_nmsadpcm)
add_test(NAME peak_chunk_test_wav COMMAND $<TARGET_FILE:peak_chunk_test> wav)
add_test(NAME header_test_wav COMMAND $<TARGET_FILE:header_test> wav)
add_test(NAME misc_test_wav COMMAND $<TARGET_FILE:misc_test> wav)
add_test(NAME string_test_wav COMMAND $<TARGET_FILE:string_test> wav)
add_test(NAME multi_file_test_wav COMMAND $<TARGET_FILE:multi_file_test> wav)
add_test(NAME chunk_test_wav COMMAND $<TARGET_FILE:chunk_test> wav)

### w64-tests

add_test(NAME write_read_test_w64 COMMAND $<TARGET_FILE:write_read_test> w64)
add_test(NAME lossy_comp_test_w64_ima COMMAND $<TARGET_FILE:lossy_comp_test> w64_ima)
add_test(NAME lossy_comp_test_w64_msadpcm COMMAND $<TARGET_FILE:lossy_comp_test> w64_msadpcm)
add_test(NAME lossy_comp_test_w64_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> w64_ulaw)
add_test(NAME lossy_comp_test_w64_alaw COMMAND $<TARGET_FILE:lossy_comp_test> w64_alaw)
add_test(NAME lossy_comp_test_w64_gsm610 COMMAND $<TARGET_FILE:lossy_comp_test> w64_gsm610)
add_test(NAME header_test_w64 COMMAND $<TARGET_FILE:header_test> w64)
add_test(NAME misc_test_w64 COMMAND $<TARGET_FILE:misc_test> w64)

### rf64-tests

add_test(NAME write_read_test_rf64 COMMAND $<TARGET_FILE:write_read_test> rf64)
add_test(NAME header_test_rf64 COMMAND $<TARGET_FILE:header_test> rf64)
add_test(NAME misc_test_rf64 COMMAND $<TARGET_FILE:misc_test> rf64)
add_test(NAME string_test_rf64 COMMAND $<TARGET_FILE:string_test> rf64)
add_test(NAME peak_chunk_test_rf64 COMMAND $<TARGET_FILE:peak_chunk_test> rf64)
add_test(NAME chunk_test_rf64 COMMAND $<TARGET_FILE:chunk_test> rf64)

### raw-tests

add_test(NAME write_read_test_raw COMMAND $<TARGET_FILE:write_read_test> raw)
add_test(NAME lossy_comp_test_raw_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> raw_ulaw)
add_test(NAME lossy_comp_test_raw_alaw COMMAND $<TARGET_FILE:lossy_comp_test> raw_alaw)
add_test(NAME lossy_comp_test_raw_gsm610 COMMAND $<TARGET_FILE:lossy_comp_test> raw_gsm610)
add_test(NAME lossy_comp_test_vox_adpcm COMMAND $<TARGET_FILE:lossy_comp_test> vox_adpcm)
add_test(NAME raw_test COMMAND $<TARGET_FILE:raw_test>)

### paf-tests

add_test(NAME write_read_test_paf COMMAND $<TARGET_FILE:write_read_test> paf)
add_test(NAME header_test_paf COMMAND $<TARGET_FILE:header_test> paf)
add_test(NAME misc_test_paf COMMAND $<TARGET_FILE:misc_test> paf)

### svx-tests

add_test(NAME write_read_test_svx COMMAND $<TARGET_FILE:write_read_test> svx)
add_test(NAME header_test_svx COMMAND $<TARGET_FILE:header_test> svx)
add_test(NAME misc_test_svx COMMAND $<TARGET_FILE:misc_test> svx)

### nist-tests

add_test(NAME write_read_test_nist COMMAND $<TARGET_FILE:write_read_test> nist)
add_test(NAME lossy_comp_test_nist_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> nist_ulaw)
add_test(NAME lossy_comp_test_nist_alaw COMMAND $<TARGET_FILE:lossy_comp_test> nist_alaw)
add_test(NAME header_test_nist COMMAND $<TARGET_FILE:header_test> nist)
add_test(NAME misc_test_nist COMMAND $<TARGET_FILE:misc_test> nist)

### ircam-tests

add_test(NAME write_read_test_ircam COMMAND $<TARGET_FILE:write_read_test> ircam)
add_test(NAME lossy_comp_test_ircam_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> ircam_ulaw)
add_test(NAME lossy_comp_test_ircam_alaw COMMAND $<TARGET_FILE:lossy_comp_test> ircam_alaw)
add_test(NAME header_test_ircam COMMAND $<TARGET_FILE:header_test> ircam)
add_test(NAME misc_test_ircam COMMAND $<TARGET_FILE:misc_test> ircam)

### voc-tests

add_test(NAME write_read_test_voc COMMAND $<TARGET_FILE:write_read_test> voc)
add_test(NAME lossy_comp_test_voc_ulaw COMMAND $<TARGET_FILE:lossy_comp_test> voc_ulaw)
add_test(NAME lossy_comp_test_voc_alaw COMMAND $<TARGET_FILE:lossy_comp_test> voc_alaw)
add_test(NAME header_test_voc COMMAND $<TARGET_FILE:header_test> voc)
add_test(NAME misc_test_voc COMMAND $<TARGET_FILE:misc_test> voc)

### mat4-tests

add_test(NAME write_read_test_mat4 COMMAND $<TARGET_FILE:write_read_test> mat4)
add_test(NAME header_test_mat4 COMMAND $<TARGET_FILE:header_test> mat4)
add_test(NAME misc_test_mat4 COMMAND $<TARGET_FILE:misc_test> mat4)

### mat5-tests

add_test(NAME write_read_test_mat5 COMMAND $<TARGET_FILE:write_read_test> mat5)
add_test(NAME header_test_mat5 COMMAND $<TARGET_FILE:header_test> mat5)
add_test(NAME misc_test_mat5 COMMAND $<TARGET_FILE:misc_test> mat5)

### pvf-tests

add_test(NAME write_read_test_pvf COMMAND $<TARGET_FILE:write_read_test> pvf)
add_test(NAME header_test_pvf COMMAND $<TARGET_FILE:header_test> pvf)
add_test(NAME misc_test_pvf COMMAND $<TARGET_FILE:misc_test> pvf)

### xi-tests

add_test(NAME lossy_comp_test_xi_dpcm COMMAND $<TARGET_FILE:lossy_comp_test> xi_dpcm)

### htk-tests

add_test(NAME write_read_test_htk COMMAND $<TARGET_FILE:write_read_test> htk)
add_test(NAME header_test_htk COMMAND $<TARGET_FILE:header_test> htk)
add_test(NAME misc_test_htk COMMAND $<TARGET_FILE:misc_test> htk)

### avr-tests

add_test(NAME write_read_test_avr COMMAND $<TARGET_FILE:write_read_test> avr)
add_test(NAME header_test_avr COMMAND $<TARGET_FILE:header_test> avr)
add_test(NAME misc_test_avr COMMAND $<TARGET_FILE:misc_test> avr)

### sds-tests

add_test(NAME write_read_test_sds COMMAND $<TARGET_FILE:write_read_test> sds)
add_test(NAME header_test_sds COMMAND $<TARGET_FILE:header_test> sds)
add_test(NAME misc_test_sds COMMAND $<TARGET_FILE:misc_test> sds)

### sd2-tests

add_test(NAME write_read_test_sd2 COMMAND $<TARGET_FILE:write_read_test> sd2)

### wve-tests

add_test(NAME lossy_comp_test_wve COMMAND $<TARGET_FILE:lossy_comp_test> wve)

### mpc2k-tests

add_test(NAME write_read_test_mpc2k COMMAND $<TARGET_FILE:write_read_test> mpc2k)
add_test(NAME header_test_mpc2k COMMAND $<TARGET_FILE:header_test> mpc2k)
add_test(NAME misc_test_mpc2k COMMAND $<TARGET_FILE:misc_test> mpc2k)

### flac-tests

add_test(NAME write_read_test_flac COMMAND $<TARGET_FILE:write_read_test> flac)
add_test(NAME compression_size_test_flac COMMAND $<TARGET_FILE:compression_size_test> flac)
add_test(NAME string_test_flac COMMAND $<TARGET_FILE:string_test> flac)

### vorbis-tests
add_test(NAME ogg_test COMMAND $<TARGET_FILE:ogg_test>)
add_test(NAME compression_size_test_vorbis COMMAND $<TARGET_FILE:compression_size_test> vorbis)
add_test(NAME lossy_comp_test_ogg_vorbis COMMAND $<TARGET_FILE:lossy_comp_test> ogg_vorbis)
add_test(NAME string_test_ogg COMMAND $<TARGET_FILE:string_test> ogg)
add_test(NAME misc_test_ogg COMMAND $<TARGET_FILE:misc_test> ogg)

### io-tests

add_test(NAME stdio_test COMMAND $<TARGET_FILE:stdio_test>)
add_test(NAME pipe_test COMMAND $<TARGET_FILE:pipe_test>)
add_test(NAME virtual_io_test COMMAND $<TARGET_FILE:virtual_io_test>)

set(SNDFILE_TEST_TARGETS
  test_main
  sfversion
  error_test
  ulaw_test
  alaw_test
  dwvw_test
  command_test
  floating_point_test
  checksum_test
  scale_clip_test
  headerless_test
  rdwr_test
  locale_test
  cpp_test
  external_libs_test
  format_check_test
  channel_test
  pcm_test
  write_read_test
  lossy_comp_test
  peak_chunk_test
  header_test
  misc_test
  string_test
  multi_file_test
  aiff_rw_test
  chunk_test
  long_read_write_test
  raw_test
  compression_size_test
  ogg_test
  stdin_test
  stdout_test
  stdio_test
  pipe_test
  virtual_io_test
  g72x_test)

set_target_properties(${SNDFILE_TEST_TARGETS} PROPERTIES FOLDER Tests)
