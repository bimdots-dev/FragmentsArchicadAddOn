# Archicad Fragments Add-On

[![Build](https://github.com/bimdots-dev/FragmentsArchicadAddOn/actions/workflows/build.yml/badge.svg)](https://github.com/bimdots-dev/FragmentsArchicadAddOn/actions/workflows/build.yml)

Archicad Add-On to export [Fragments (.frag)](https://github.com/ThatOpen/engine_fragment) files.

## Build instructions

### Install prerequisites

#### Windows

- [Visual Studio](https://visualstudio.microsoft.com/downloads) (any version)
  - Platform toolset v142.
  - Platform toolset v143.

#### macOS

- [Xcode](https://developer.apple.com/xcode/)
  - Version 14.2: `/Applications/Xcode_14.2.app`.
  - Version 15.2: `/Applications/Xcode_15.2.app`.

#### Common

- [CMake](https://cmake.org) (3.16+)
- [Python](https://www.python.org) (3.8+)
- [7-Zip](https://www.7-zip.org) (22+)
  - The executable must be in the PATH.

### Build

Run the build script.

```
python Tools\BuildAddOn.py --configFile config.json --acVersion 28
```
