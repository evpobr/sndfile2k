# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [1.2.0] - 2018-03-25

### Fixed

- Skipping of erroneous subchunks
- Always return SF_ERR_UNRECOGNISED_FORMAT on unrecognised format
- Avoid unnessesary seeks when opening Ogg files

### Changed

- Coding style rules: bump column width limit of C source files to 132 chars.
  We have too long lines, so i choose 132 for readability. It is another
  historical limit. More explained [here](https://lkml.org/lkml/2009/12/17/229).
- New Posix based internal file IO. Now IO is unform across all platforms.
- `sf_open_virtual()` function deprecated, use `sf_open_virtual_ex` instead.
- `sf_open_fd()` function deprecated, use `sf_open_virtual_ex` instead.
- Broadcast info support deprecated and will be removed in next release.
- CART info support deprecated and will be removed in next release.

### Added

- New members os `SF_VIRTUAL_IO` struct: `flush`, `set_filelen`, `is_pipe`, `ref`
  and `unref`. Now you can implement fully functional IO with SF_VIRTUAL_IO.
- New `sf_open_virtual_ex()` function to use all the features of `SF_VIRTUAL_IO`.

## [1.1.1] - 2018-03-10

### Added

- NMS ADPCM codec in WAV support

### Fixed

- Max channel count bug

## [1.1.0] - 2018-03-02

### Added

- NMS ADPCM codec support

### Fixed

- `sndfile-convert` fail due to no. of channels
- `SFC_GET_LIB_VERSION` command behavior on error, now it returns 0

### Removed

- MacOSX & BeOS support from `sndfile-play`

## [1.0.0] - 2017-12-24

### Changed

- Coding style rules: bump column width limit of C source files to 100 chars

### Fixed

- Small CMake project fix

## [1.0.0-rc.2] - 2017-12-24

### Added

- CMake documentation target `docs`

### Changed

- CMake installation ia now component based

### Fixed

- Appveyor CI badge in `README.md`
- Documentation glitches

## [1.0.0-rc.1] - 2017-12-22

### Added

- Doxygen generated online documentation available at https://evpobr.github.io/sndfile2k
- CMake `ENABLE_PKGCONFIG_FILE` option

### Changed

- CMake option `DISABLE_CPU_CLIP` name to `ENABLE_CPU_CLIP` and set to `ON` by default
- CMake option `ENABLE_PACKAGE_CONFIG` name to `ENABLE_CMAKE_PACKAGE_CONFIG`
- CMake option `DISABLE_EXTERNAL_LIBS` name to `HAVE_XIPH_CODECS`
- CMake features names
- `sndfile2k.hh` header renamed to `sndfile2k.hpp`
- CMake Ogg, Vorbis and FLAC find modules improved
- `SNDFILE` is now typedef of existing `sf_private_tag` struct

### Fixed

- CMake configuration when `ENABLE_EXPERIMENTAL` is activated but Speex library is not found
- `SFC_GET_LOG_INFO` command return output. When data is NULL, 0 is returned now.
- Some compiler warnings

### Removed

- [Vcpkg](https://github.com/Microsoft/vcpkg) port (issue #7)
- `sndfile-regtest` program removed with SQLite3 dependency

## [1.0.0-beta4] - 2017-11-27

### Added

- [Vcpkg](https://github.com/Microsoft/vcpkg) port in `packages/vcpkg` directory (issue #7)

### Changed

- Improve `pkg-config` module
- Small fixes

## [1.0.0-beta3] - 2017-11-19

### Fixed
- CMake: fix public include directory (issue #6)

### Changed
- Typedef structs and enums
- Small fixes

## [1.0.0-beta2] - 2017-11-08

### Fixed
- Appveyor CI badge
- EditorConfig rules
- Small fixes
- sndfile2k-info linking to winmm library

### Changed
- Unicode open function `sf_wchar_open` is always accessible under Win32 and Cygwin
- Travis CI tests are faster now (OSX test are disabled)

## [1.0.0-beta1] - 2017-11-07

### Fixed

- Compiler errors
- [CMake warning](https://cmake.org/cmake/help/latest/policy/CMP0063.html)

## [1.0.0-alpha1] - 2017-11-07 [YANKED]

### Added

- Tests sources which were generated are now in source tree. No `autogen` stuff requred, faster configuration and compilation.

### Changed

- Version numbering restarted from `1`.
- Project renamed to SndFile2K. Library name is `sndfile2k`. Utilities are renamed in the same way.
- Main public header renamed to sndfile2k to avoid conflicts with installed `libsndfile`.
- Public headers are moved to `include/sndfile2k` directory. It is bad to keep public headers in source directory.
- Public API is now exported via [compiler visibility pragmas](https://gcc.gnu.org/wiki/Visibility), not symbol files. This was done to be maximum portable, also Python is not requred now to build shared library. Since it is different way to export public function, **ABI is not compatible with libsndfile**. You cannot just replace `libsndfile` binary with `sndfile2k` library. Symbols files allows features unsupported via current export system. But **API is compatibile** with `libsndfile`. Recompile and use. Just include `sndfile2k/sndfile2k.h` instead of `sndfile.h`.

  NOTE: You need to add `SNDFILE2K_STATIC` define if you adding **static** SndFile2K library to your project under Windows.
  
- Using reStructuredTest for documentation.
  
### Removed

- Autootools build system in favour of CMake build system.
- Documentation - will be rewritten in ReStructuredText and added later.
- Python based style checking script and git hook - coding style was changed.
- Other unportable or obsolete scripts.
- `Win32` directory. Test file inside was useless, since Windows is supported now.
- Travis CI MacOS GCC test removed

[Unreleased]: https://github.com/evpobr/sndfile2k/compare/v1.2.0...HEAD
[1.0.0-beta1]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-alpha1...v1.0.0-beta1
[1.0.0-beta2]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-beta1...v1.0.0-beta2
[1.0.0-beta3]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-beta2...v1.0.0-beta3
[1.0.0-beta4]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-beta3...v1.0.0-beta4
[1.0.0-rc.1]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-beta4...v1.0.0-rc.1
[1.0.0-rc.2]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-rc.1...v1.0.0-rc.2
[1.0.0]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-rc.2...v1.0.0
[1.1.0]: https://github.com/evpobr/sndfile2k/compare/v1.0.0...v1.1.0
[1.1.1]: https://github.com/evpobr/sndfile2k/compare/v1.1.0...v1.1.1
[1.2.0]: https://github.com/evpobr/sndfile2k/compare/v1.1.1...v1.2.0
