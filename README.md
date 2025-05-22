# Archicad Fragments Add-On

[![Build](https://github.com/bimdots-dev/FragmentsArchicadAddOn/actions/workflows/build.yml/badge.svg)](https://github.com/bimdots-dev/FragmentsArchicadAddOn/actions/workflows/build.yml)

Archicad Add-On developed by [Bimdots](https://www.bimdots.com) to export [Fragments (.frag)](https://github.com/ThatOpen/engine_fragment) files.

## Installation

You can download the Add-On from the [Bimdots store](https://bimdots.com/product/fragments-exporter). Please read the [installation guide](https://bimdots.com/help-center/add-on-installation-guide) for more instructions.

## Example

[Click here](https://bimdots-dev.github.io/FragmentsArchicadExample) for a live example using an exported file. The example model is provided to Bimdots by [Enzyme APD](https://www.weareenzyme.com).

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
