# A miniature game engine created by gagoa

## Cross-platform build and run

This project uses CMake for a single, portable build across macOS, Linux, and Windows. A thin Makefile is provided for Unix-like systems.

### Prerequisites

- macOS: Xcode Command Line Tools or Homebrew `llvm`, plus `cmake`. SDL2 frameworks are bundled under `SDL2/`, `SDL_image/`, `SDL_mixer/`, `SDL2_ttf/`.
- Linux: `g++` or `clang`, plus `cmake` and `make`. Install SDL2 dev packages: `libsdl2-dev`, `libsdl2-image-dev`, `libsdl2-mixer-dev`, `libsdl2-ttf-dev` (Debian/Ubuntu) or the equivalent on your distro.
- Windows (MSVC): Visual Studio 2022 (Desktop C++ workload) or Build Tools, plus `cmake`. **SDL2 and add-ons are required** (see Windows section below).
- Windows (MinGW): MSYS2/MinGW toolchain and `cmake`.

GLM and RapidJSON are vendored under `third_party/` and `rapidjson-1.1.0/`. SDL2 is bundled on macOS only; on Windows you must install SDL2 (e.g. via vcpkg).

### macOS / Linux (CMake)

```bash
cd game_engine_gagoa
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/game_engine          # macOS
./build/game_engine_linux    # Linux
```

Debug build: use `-DCMAKE_BUILD_TYPE=Debug` and run the same executable from `build/`.

### Linux only (Makefile)

On Linux you can also build with `make` (no CMake). This produces `game_engine_linux` in the project root and requires the SDL2 dev packages above:

```bash
make
./game_engine_linux
```

### Windows (MSVC)

Install SDL2 and add-ons via **vcpkg** (recommended), then build with CMake:

```powershell
# One-time: install vcpkg, then install SDL2 packages (x64-windows or x64-windows-static)
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
C:\vcpkg\vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf --triplet x64-windows

# Build the game engine (use vcpkg toolchain so CMake finds SDL2)
cd game_engine_gagoa
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
.\build\Release\game_engine.exe
```

To open in Visual Studio: generate the solution as above, then open `build\game_engine_gagoa.sln` and build/run the `game_engine` target. You can also open the existing `game_engine.sln` and add vcpkg integration (`vcpkg integrate install`) so that SDL2 libs and includes are found when building the `.vcxproj` directly.

**Running the game:** If you use dynamic vcpkg libraries (`x64-windows`), copy the DLLs from `C:\vcpkg\installed\x64-windows\bin\` (e.g. `SDL2.dll`, `SDL2_image.dll`, `SDL2_mixer.dll`, `SDL2_ttf.dll` and their dependencies) into the same folder as `game_engine.exe`, or run from a shell that has that path in `PATH`.

### Windows (MinGW)

From an MSYS2/MinGW shell:

```bash
cd game_engine_gagoa
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/game_engine.exe
```

### Notes

- C++17 is required and enforced by the build.
- Warnings are enabled (`/W4` on MSVC, `-Wall -Wextra -Wpedantic` elsewhere).
- The Makefile here only wraps CMake; Windows users should prefer the CMake commands above.
