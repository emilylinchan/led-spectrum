# Contributing to `spectrum`

First off, thank you for considering contributing to `spectrum`! 

## Prerequisites
To build and run this project locally, you will need:
* A Windows environment (Windows 10/11)
* A C++17 compatible compiler (MSVC or Clang-CL)
* CMake (v3.10 or higher)
* FFTW3 binaries (included in source code of `spectrum`)

## Local Development Setup
We use CMake to keep the build process compiler-agnostic. To get started:
1. Clone the repo: `git clone https://github.com/majockbim/spectrum.git`
2. Create a build directory: `mkdir build && cd build`
3. Generate the build files: `cmake -S . -B output -G "MinGW Makefiles"`
4. Compile: `cmake --build output`

## Architectural Guidelines
* **Zero-Dependency Preference:** `spectrum` is designed to be highly optimized and lightweight. Before introducing a new heavy third-party library, please open an issue to discuss it. Native C++ implementations are preferred (like custom parsers) to keep the footprint small.
* **Compiler Agnostic:** Ensure your code compiles cleanly on standard C++ compilers. Do not embed compiler-specific linker pragmas directly in the source files; handle all library linking via the `CMakeLists.txt`.

## Pull Request Process
1. Fork the repo and create your branch from `main` (e.g., `feature/custom-config-parser`).
2. Test your changes locally to ensure no performance degradation, memory leaks, or divide-by-zero crashes.
3. Open a PR with a clear description of the problem solved or feature added.
