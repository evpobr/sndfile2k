# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/en/1.0.0/)
and this project adheres to [Semantic Versioning](http://semver.org/spec/v2.0.0.html).

## [Unreleased]

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

[Unreleased]: https://github.com/evpobr/sndfile2k/compare/v1.0.0-beta1...HEAD
