cmake_minimum_required(VERSION 3.2)

project(sndfile2k VERSION 1.1.1)

#
# Variables
#

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED TRUE)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR})

set(PACKAGE_NAME ${PROJECT_NAME})
set(PACKAGE_VERSION ${PROJECT_VERSION})

set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

#
# System-wide includes
#

include(GNUInstallDirs)
include(FeatureSummary)
include(CMakeDependentOption)
include(CPack)

#
# Options
#

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_PROGRAMS "Build programs" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(BUILD_TESTING "Build tests" ON)
option(ENABLE_CPU_CLIP "Enable tricky cpu specific clipper" ON)
if(MSVC OR MINGW)
  option(ENABLE_STATIC_RUNTIME "Enable static runtime" OFF)
endif(MSVC OR MINGW)
option(ENABLE_CMAKE_PACKAGE_CONFIG "Generate and install CMake package configuration files" ON)
option(ENABLE_PKGCONFIG_FILE "Generate and install pkg-config file (sndfile2k.pc)" ON)
option(ENABLE_EXPERIMENTAL "Enable experimental code" OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")

#
# Setup definitions
#

include(${CMAKE_MODULE_PATH}/SndFileChecks.cmake)

configure_file(src/config.h.cmake src/config.h)

cmake_dependent_option(HAVE_XIPH_CODECS "Enable use of Xiph.org Vorbis and FLAC codecs" ON "HAVE_EXTERNAL_XIPH_LIBS" OFF)
cmake_dependent_option(HAVE_SPEEX "Enable use of Xiph.org Speex codec" ON "ENABLE_EXPERIMENTAL AND SPEEX_FOUND" OFF)
cmake_dependent_option(BUILD_DOCUMENTATION "Enable use of Doxygen documentation generator" ON "Doxygen_FOUND;CMAKE_DOXYGEN_SUPPORTS_TARGETS" OFF)

add_feature_info(BuildSharedLibs BUILD_SHARED_LIBS "build shared libraries")
add_feature_info(WithVorbis HAVE_XIPH_CODECS "Xiph Vorbis codec support")
add_feature_info(WithFLAC HAVE_XIPH_CODECS "Xiph FLAC codec support")
add_feature_info(WithSpeex HAVE_SPEEX "Xiph Speex codec support")
add_feature_info(WithDoxygen HAVE_DOXYGEN "Doxygen documentation generator")
add_feature_info(ExperimentalCode ENABLE_EXPERIMENTAL "enable experimental code")
add_feature_info(BuildTesting BUILD_TESTING "build tests")
add_feature_info(CPUClip ENABLE_CPU_CLIP "Enable tricky cpu specific clipper")
add_feature_info(CMakePackageConfig ENABLE_CMAKE_PACKAGE_CONFIG "Generate and install CMake package configuration files")
add_feature_info(PkgConfigFile ENABLE_PKGCONFIG_FILE "Generate and install pkg-config file (sndfile2k.pc)")

if(MSVC OR MINGW)
  add_feature_info(ENABLE_STATIC_RUNTIME ENABLE_STATIC_RUNTIME "Enable static runtime")
endif()

set_package_properties(Ogg PROPERTIES TYPE RECOMMENDED
  URL "www.xiph.org/ogg/"
  DESCRIPTION "library for manipulating ogg bitstreams"
  PURPOSE "Enables Xiph.org Vorbis and FLAC codecs support")
set_package_properties(Vorbis PROPERTIES TYPE RECOMMENDED
  URL "www.vorbis.com/"
  DESCRIPTION "open source lossy audio codec"
  PURPOSE "Enables Xiph.org Vorbis support")
set_package_properties(FLAC PROPERTIES TYPE RECOMMENDED
  URL "www.xiph.org/flac/"
  DESCRIPTION "Free Lossless Audio Codec Library"
  PURPOSE "Enables Xiph.org FLAC codec support")
set_package_properties(Speex PROPERTIES TYPE OPTIONAL
  URL "www.speex.org/"
  DESCRIPTION "an audio codec tuned for speech"
  PURPOSE "Enables experemental Speex support")
set_package_properties(Doxygen PROPERTIES TYPE OPTIONAL
  URL "www.doxygen.org/"
  DESCRIPTION "generate documentation from source code"
  PURPOSE "Enables documentation generation")

feature_summary(WHAT ALL)

#
# sndfile2k
#

  add_subdirectory(src)

#
# Programs
#

if(BUILD_PROGRAMS)

  add_subdirectory(programs)

endif()

#
# Examples
#

if(BUILD_EXAMPLES)

  add_subdirectory(examples)

endif()

# Doxygen documentation

if(BUILD_DOCUMENTATION)
  add_subdirectory(docs)
endif()

#
# Testing
#

if(BUILD_TESTING)

  include(CTest)

  enable_testing()

  add_subdirectory(tests)

endif()
