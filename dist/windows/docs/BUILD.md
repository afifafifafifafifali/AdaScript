# Build Guide

This guide covers building AdaScript on Windows and Linux/WSL using CMake and Ninja.

## Prerequisites

- CMake >= 3.15
- Ninja
- C++17 compiler
- Windows: WinHTTP is part of the OS, linked automatically.
- Linux/WSL: libcurl development package (Ubuntu: `sudo apt-get install libcurl4-openssl-dev`)

## Configure and Build

Windows (PowerShell):

```
cmake -S . -B build -G Ninja
cmake --build build --config Release
```

Linux/WSL:

```
cmake -S . -B build -G Ninja
cmake --build build --config Release
```

## Artifacts

Binaries are built for your current platform. You can also build Linux binaries from Windows using WSL:

Windows -> Linux (via WSL):
- If libcurl dev is not installed, the build will still succeed but HTTP will be disabled (requests.* will throw at runtime). Install libcurl dev to enable HTTP.
- Ensure WSL with a distro (e.g., Ubuntu) is installed and has Ninja and a C++17 toolchain and libcurl dev.
- From PowerShell (repo root):
  - wsl -e bash -lc 'cmake -S /mnt/c/Users/AFIF/Desktop/AdaScript -B /mnt/c/Users/AFIF/Desktop/AdaScript/build-linux -G Ninja && cmake --build /mnt/c/Users/AFIF/Desktop/AdaScript/build-linux --config Release'
- Linux artifacts will be in build-linux/ (adascript, libadascript_core.so)

- Shared library (for embedding):
  - Windows: build/adascript_core.dll (+ import lib)
  - Linux: build/libadascript_core.so
- CLI:
  - Windows: build/adascript.exe
  - Linux: build/adascript

## Run

- Windows: `.\build\adascript.exe path\to\script.ad`
- Linux/WSL: `./build/adascript path/to/script.ad`

## Embed (C)

- Header: `include/AdaScript.h`
- Link against `adascript_core` (see docs/C_API.md)
