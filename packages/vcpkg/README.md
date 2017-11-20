# Vcpkg package

This directory contains unofficial [Vcpkg](https://github.com/Microsoft/vcpkg) port of **SndFile2K**.

This is the easiest way to intergate SndFile2K to your **CMake** or **MSBuild**-based project under Windows.

## Installation

Install **Vcpkg** if you don't have (follow **Vcpkg's** `README.md` instructions).

Copy `ports/` directory to **Vcpkg** install directory.

## How to use

Read **Vcpkg's** `docs\examples\using-sqlite.md` for help.

## Additional notes

* **Vcpkg** installs full version of **SndFile2K** library with **Vorbis** and **FLAC** support
* To save disk space it's recommended to update your **Git** and **CMake** to the latest versions or **Vcpkg** will download and install its own copies
* **CMake's** `find_package` will work in package mode. It means you don't need to write your own find module, just add `find_package(sndfile2k)` to your `CMakeLists.txt` and you have `sndfile2k` library target properly configured. Just add it with `add_library(sndfile2k)` to your target.
* If you build dynamic version of **SndFile2K** (e.g. with `x64-windows` triplet) `sndfile2k.dll` depends on `ogg.dll`, `vorbis.dll`, `vorbis.dll` and `flac.dll` libraries. They will be copied automatically to build output directory.
