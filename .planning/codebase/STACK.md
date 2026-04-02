# Technology Stack

**Analysis Date:** 2026-04-02

## Languages

**Primary:**
- C++ 17 - Graphics engine, main application logic in `main.cpp`

**Secondary:**
- C - GLAD loader generated from glad 0.1.36, included as `vendor/src/glad.c`
- GLSL - Vertex and fragment shaders embedded in `main.cpp`

## Runtime

**Environment:**
- Cross-platform (Linux/Windows/macOS support via CMake)
- Compiled to native binary: `build/flight`

**Build System:**
- CMake 3.20+ (required by `CMakeLists.txt`)
- GNU Make (convenience wrapper in `Makefile`)

## Frameworks

**Graphics:**
- OpenGL 4.6 - Core graphics API (GL_CONTEXT_VERSION_MAJOR 3, GL_CONTEXT_VERSION_MINOR 3 in `main.cpp:155-157`)
- GLAD 0.1.36 - OpenGL function loader, generated for GL 4.6 compatibility

**Window Management:**
- GLFW3 - Cross-platform window creation and input handling (`main.cpp:2, CMakeLists.txt:8`)

**Math:**
- GLM (OpenGL Mathematics) - Vector and matrix operations for transforms (`main.cpp:3-5`)

## Key Dependencies

**Critical:**
- `glfw3` - Required for window management and user input processing
- `OpenGL::GL` - Required for graphics rendering
- `glm::glm` - Required for mathematical operations (model, view, projection matrices)
- `glad` - Required for OpenGL function loading (compiled from `vendor/src/glad.c`)

**Build-time:**
- CMake 3.20+
- C++ compiler with C++17 support
- Make (optional, for convenience)

## Configuration

**Environment:**
- No environment variables detected
- Configuration is compile-time via CMake and C++ code

**Build:**
- CMakeLists.txt: Root build configuration
- Makefile: Convenience targets for `make all`, `make run`, `make clean`
- Build output directory: `build/` (created by CMake)

**Runtime Configuration:**
- Window size: 2560x1600 pixels (hardcoded in `main.cpp:159`)
- OpenGL profile: Core profile with version 3.3
- Cube movement speed: 2.0 units/second (configurable in `main.cpp:91`)
- Camera: Fixed at (0, 0, 3) looking at origin

## Platform Requirements

**Development:**
- CMake 3.20 or later
- C++ compiler (GCC, Clang, or MSVC) with C++17 support
- GLFW3 development libraries
- OpenGL development libraries
- GLM header-only library
- Arch Linux specific: Available via `pacman` (glfw glm)

**Runtime:**
- OpenGL 3.3+ compatible graphics hardware
- GLFW3 runtime library
- OpenGL drivers (typically included with GPU drivers)

## Build Commands

```bash
make all          # Build the engine
make run          # Build and run (executes ./build/flight)
make clean        # Remove build artifacts
cmake -B build    # Configure build directory
cmake --build build  # Build using CMake
```

**Output:** Executable at `./build/flight` (named "flight" internally, displays "vecEngine" in window title)

---

*Stack analysis: 2026-04-02*
