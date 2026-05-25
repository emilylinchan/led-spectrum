# Contributing to `spectrum`

First off, thank you for considering contributing to `spectrum`! This project aims to be the fastest and most accurate terminal visualizer for Windows.

## Prerequisites
To build and run this project locally, you will need:
* **Windows 10/11**
* **CMake** (v3.10 or higher)
* **A C++17 compatible compiler** (MSVC or MinGW-w64)
* **FFTW3** binaries (included in `third_party/`)
* **OpenMP** (Usually included with your compiler)

## Building from Source

### Option 1: g++ (MinGW-w64)
Best for those using MSYS2 or a standalone MinGW installation.
```bash
# Generate build files
cmake -B build_mingw -G "MinGW Makefiles"

# Compile
cmake --build build_mingw

# Run!
.\build_mingw\spectrum.exe
```

### Option 2: cl (MSVC Command Line)
Best for those who prefer the Microsoft C++ compiler but want to stay in the terminal.
```bash
# Generate build files (Ensure x64 architecture)
cmake -S . -B build_msvc -G "Visual Studio 17 2022" -A x64

# Compile
cmake --build build_msvc --config Release

# Run!!
.\build_msvc\Release\spectrum.exe
```

### Option 3: Visual Studio (MSVC IDE)
1. Open Visual Studio.
2. Select **Open a local folder** and choose the `spectrum` directory.
3. Visual Studio will automatically detect `CMakeLists.txt` and configure the project.
4. Select `spectrum.exe` in the "Select Startup Item" dropdown.
5. Press **F5** to build and run.

## Architectural Guidelines
* **Zero-Dependency Preference:** `spectrum` is designed to be highly optimized and lightweight. Native C++ implementations are preferred to keep the footprint small.
* **Compiler Agnostic:** Ensure your code compiles cleanly on both MSVC and MinGW. Use C++17 standard features only (avoid C++20 specific syntax like designated initializers for now to maintain compatibility).
* **Low-Latency focus:** The DSP thread and the Render thread must stay decoupled via the shared state to prevent audio capture from lagging during complex frame renders.

## Pull Request Process
1. Fork the repo and create your branch from `main`.
2. Test your changes locally to ensure no performance degradation or divide-by-zero crashes.
3. If you added a new theme, ensure it follows the schema in `THEMES.md`.
4. Open a PR with a clear description of the improvements.
