# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project

vecEngine is a from-scratch C++ OpenGL game engine following the phase-by-phase build plan in `roadmap.md`. Current state sits between Phase 3 (3D Meshes & Transforms — there's a colored cube with MVP transforms) and Phase 5 (Flight Controller — currently just arrow-key translation, no flight model yet). Future phases (terrain, flight controller, entity system, lighting, etc.) will progressively modularize the codebase.

This project is run hands-on without GSD tooling — don't propose `/gsd-*` workflows or write into `.planning/`. The existing `.planning/codebase/` docs are background only.

Note the naming inconsistency: the CMake project is `FlightEngine`, the binary is `flight`, but the repo and window title are `vecEngine`. Don't "fix" this without asking — the roadmap was written around the `flight` name.

## Build & Run

```bash
make all      # cmake -B build && cmake --build build
make run      # build, then ./build/flight
make clean    # rm -rf build
```

System deps (Arch): `sudo pacman -S cmake glfw-x11 glm` (GLAD is vendored under `vendor/`, no system package needed).

`build/` is gitignored. There are no lints or formatters wired up, and no tests yet — verification is manual: run the binary and visually confirm rendering/input. Note: `project_requirements.md` (class assignment) requires a README, `make test` target, and ≥3 unit tests before submission, so a test target will need to land eventually.

## Architecture (current state)

Everything lives in `main.cpp` (~250 lines) — a single-threaded immediate-mode OpenGL render loop. Top-to-bottom:

- **Embedded GLSL**: `vertexShaderSource` and `fragmentShaderSource` as C++ raw string literals. Will move to `shaders/*.vert|.frag` files in a later phase.
- **Hardcoded geometry**: interleaved position+color `vertices[]` (24 verts, stride 6 floats) and `indices[]` (36 indices) for a cube.
- **Globals**: `cubePos`, `moveSpeed`, `deltaTime`, `lastFrame`. These exist because GLFW callbacks need global access; they will be encapsulated when the scene/entity system lands.
- **Helpers**: `framebufferSizeCallback`, `processInput`, `compileShader`, `createShaderProgram`.
- **`main()`**: GLFW init → GLAD load → shader compile → VAO/VBO/EBO setup → render loop (input → MVP matrices → uniform upload → draw) → cleanup.

MVP pipeline: `projection * view * model * vec4(aPos, 1.0)` in the vertex shader. The model matrix is rebuilt every frame from `cubePos` (translate) and `glfwGetTime()` (rotate). The view matrix is fixed at `(0,0,3)` looking at origin; projection is rebuilt every frame from the current framebuffer aspect ratio.

Context profile: `glfwWindowHint` requests OpenGL 3.3 core even though GLAD was generated for 4.6 — keep the 3.3 baseline unless a phase explicitly requires bumping it.

No vsync: `glfwSwapInterval(1)` is intentionally not called (the loop runs as fast as the GPU allows). Don't add it unless the user asks.

## Directory layout

```
main.cpp           — all current application code
CMakeLists.txt     — single executable target `flight`, links glfw + OpenGL::GL + glm::glm
Makefile           — convenience wrapper around cmake
shaders/           — empty, reserved for extracted GLSL files
src/               — empty, reserved for split-out modules
include/           — project headers (e.g. include/renderer/vertex.h); also still contains a stale duplicate of vendor/include/ (glad/, KHR/) — only the project subdirs are live
vendor/include/    — GLAD + KHR headers (the path actually on the include search path)
vendor/src/glad.c  — GLAD loader, compiled into the executable
.planning/codebase/— deeper analysis docs (ARCHITECTURE, CONCERNS, etc.)
roadmap.md         — phase-by-phase build plan; authoritative source for game-logic values
project_requirements.md — class assignment spec (README + make test + unit tests + demo video)
```

When extracting code from `main.cpp` into modules, put `.cpp` files in `src/`, headers in `include/`, and add the new sources to the `add_executable(flight ...)` line in `CMakeLists.txt`.

## Repo-specific conventions

- C++17, tabs for indentation, opening braces on the same line.
- camelCase for functions and locals; ALLCAPS for raw GL handles (`VAO`, `VBO`, `EBO`).
- Errors go to `std::cerr` and return early; there is no exception handling and no logging library.
- Shader compile/link failures currently log but do not abort — preserve this behavior unless a phase explicitly fixes it (`CONCERNS.md` flags it as known debt).
- Window size is hardcoded to 2560×1600 in `glfwCreateWindow`; this will look wrong on smaller displays — don't change it casually, prefer making it configurable if asked.

## Working with the roadmap

`roadmap.md` is the source of truth for *what* to build next and the numeric constants (speeds, spawn rates, collision radii, terrain formulas) referenced from the user's older `project.c`. Before implementing a feature, check whether the roadmap already specifies the approach or values — match it rather than inventing new numbers.

The deeper analysis under `.planning/codebase/` (`ARCHITECTURE.md`, `CONCERNS.md`, `CONVENTIONS.md`, `STACK.md`, `STRUCTURE.md`, `TESTING.md`, `INTEGRATIONS.md`) was generated 2026-04-02 — useful for context, but verify against the current `main.cpp` before relying on line numbers.
