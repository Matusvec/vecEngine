# vecEngine

A from-scratch C++17 / OpenGL 3.3 game engine: no Unity, no Unreal, no scene-graph or asset library, just GLFW, GLAD and GLM. Two apps are built on it:

- **`flight`**: an arcade flight game over streamed procedural alpine terrain. You fly through a ring course, shoot down balloons, and blow craters in the ground.
- **`blackbox`**: a flight scene without the game. It has a flat airfield with a runway, taxiway and apron, six aircraft on the ramp, and one aircraft you fly with a ground model, takeoff roll, stall and landing. It streams 5 Hz UDP telemetry, can inject an aileron actuator failure, and reads a maintenance-holds file that grounds specific tail numbers.

## Features

- **Procedural terrain:** value-noise FBM, ridged multifractal peaks and domain warping. Biomes blend by altitude (sand → grass → forest → rock → snow).
- **Chunk streaming:** a fixed `(2r+1)²` ring of chunks uses modular slot indexing, so crossing a chunk boundary regenerates only one row or column.
- **Frustum and fog-distance culling** with bounding spheres, using Gribb–Hartmann plane extraction.
- **Lighting:** Phong shading from the sun and a 2048² shadow map with 3×3 PCF. The shadow camera is texel-snapped to reduce shimmering.
- **GPU-instanced grass and shrubs** with wind animation in the vertex shader. Instance buffers are rebuilt only when the camera moves to a new cell.
- **Scene details:** animated translucent water, rain, and a procedural skybox with clouds.
- **OBJ loader** with MTL textures. It drives the textured aircraft and a spinning propeller.
- **Arcade flight model:** throttle sets a target speed, roll induces yaw, and the plane stalls below its minimum speed. On the ground it has a takeoff roll, a rotate speed and a wingtip clamp.
- **Game layer:** a state machine (menu, playing, won, lost), ring detection, and missile-vs-balloon collisions with swap-and-pop removal. Craters work as a pure data effect on `heightAt`.
- **Headless unit tests** that need no GL context (`make test`).

## Quick start

Arch Linux:

```bash
sudo pacman -S cmake glfw-x11 glm   # GLAD is vendored, no package needed
git clone https://github.com/Matusvec/vecEngine.git
cd vecEngine
make run        # build + launch the flight game
make blackbox   # build + launch the BLACKBOX airfield scene
make test       # build + run the unit tests
```

Other distros need the same three packages: CMake ≥ 3.20, GLFW 3 and GLM. Run from a terminal, because the control reference and event log print to `stderr`.

| Make target      | Does                                            |
|------------------|-------------------------------------------------|
| `make all`       | `cmake -B build && cmake --build build`         |
| `make run` / `demo` | build, then `./build/flight`                 |
| `make blackbox`  | build, then `./build/blackbox`                  |
| `make test`      | build and run `./build/flight_tests`            |
| `make clean`     | `rm -rf build`                                  |

## Controls

### `flight` (the game)

| Key            | Action                                  |
|----------------|-----------------------------------------|
| `SPACE`        | start (from the menu)                   |
| `W` / `S`      | throttle up / down                      |
| `1` – `9`      | throttle preset (1 = idle, 9 = max)     |
| `Shift`        | quick burn                              |
| `↑` / `↓`      | pitch (yoke convention: ↑ = nose down)  |
| `←` / `→`      | bank / turn                             |
| Mouse          | fine yaw and pitch trim                 |
| `LMB`          | fire missile                            |
| `V`            | toggle cockpit / third-person view      |
| `R`            | restart                                 |
| `ESC`          | quit                                    |

To win, fly through every ring (+200 each) and pop every balloon (+100 each). You lose if you hit the ground. The score shows in the window title.

### `blackbox` (the airfield)

| Key                  | Action                                                  |
|----------------------|---------------------------------------------------------|
| `SPACE`              | release the brakes and creep forward. Add throttle to take off. Refused while the tail is held. |
| `W` / `S`, `1` – `9` | throttle                                                |
| `↑` / `↓`            | pitch                                                   |
| `←` / `→`            | bank in the air, steer on the ground                    |
| Mouse                | orbit the camera around the aircraft                    |
| `V` / right-click    | snap the camera back behind the aircraft                |
| `C`                  | fleet overview camera                                   |
| `F`                  | toggle aileron actuator failure                         |
| `R`                  | back to the ramp                                        |
| `ESC`                | quit                                                    |

To land, line up, go to idle and let the aircraft settle. Once it stops at idle it counts as parked again.

Set these environment variables to configure it:

| Variable               | Default                              | Meaning                                  |
|------------------------|--------------------------------------|------------------------------------------|
| `BLACKBOX_TAIL`        | `N101`                               | which of N101–N106 you fly               |
| `BLACKBOX_UDP_PORT`    | `5005`                               | telemetry goes to `127.0.0.1:<port>` at 5 Hz |
| `BLACKBOX_HOLDS`       | `../../palantir/bridge/holds.json`   | maintenance holds; a held tail can't take off |
| `BLACKBOX_FAILING_TAU` | `0.6`                                | aileron lag (s) while the actuator is failing |
| `BLACKBOX_MODEL`       | *(box plane)*                        | e.g. `assets/plane.obj` for the textured model |
| `BLACKBOX_MODEL_YAW`   | `90`                                 | rotate the model so its nose faces -Z    |
| `BLACKBOX_PROP`        | *(none)*                             | propeller OBJ, e.g. `assets/propeller.obj` |

## Project layout

```
main.cpp              flight game entry point
apps/blackbox_main.cpp BLACKBOX airfield entry point
src/, include/        engine modules
  include/core/       input
  include/renderer/   shader, mesh, camera, frustum, light, shadow map, skybox, texture, OBJ loader, overlay
  include/world/      terrain, grass, shrubs, water, rain, transform
  include/game/       flight controller, plane model, rings, balloons, missiles, game state
  include/blackbox/   UDP telemetry, holds.json parsing
shaders/              GLSL (.vert / .frag)
assets/               aircraft + propeller OBJ/MTL/PNG
tests/test_main.cpp   headless unit tests
vendor/               GLAD loader
roadmap.md            phase-by-phase build plan
docs/ENGINE_BACKLOG.md rendering backlog + rigid-body aerodynamics plan
```

## Tests

`make test` builds `flight_tests`, which runs pure-logic checks with no window and no GL context. It covers:

- deterministic and varying terrain height
- frustum culling
- camera basis vectors
- `holds.json` parsing
- OBJ parsing
- the flight ground model: takeoff under throttle, the wingtip clamp while banking low, and stall at idle

It exits with a nonzero status on any failure.

## Design notes

### Data structures

1. **Chunked grid with modular slot indexing:** terrain is a `std::vector<Chunk>` of size `(2r+1)²`. For a world chunk coordinate `w`, the slot is `((w % side) + side) % side`. A slot is regenerated only when the coordinate it holds no longer matches the one it should hold, so memory stays bounded while the world looks infinite.
2. **Instance buffers:** grass blades and shrubs are a `std::vector<Instance>`. When the camera changes cell, the vector is uploaded with a single `glBufferData` call. Each frame then needs only one bind and one `glDrawElementsInstanced`.
3. **Swap-and-pop vectors:** missiles and balloons are removed in O(1) with no allocation churn.
4. **Hash-cached uniforms:** `Shader` memoizes `glGetUniformLocation` in an `std::unordered_map<std::string, int>`.
5. **Cursor over a sequence:** the ring course is a `std::vector<Ring>` with one `active` index.
6. **Append-only crater list:** `heightAt(x, z)` applies a smoothstep depression for each crater, so destroying terrain is a data change rather than a geometry edit.

### Frame loop (`flight`)

1. Poll input. The flight controller integrates the plane.
2. Update the streaming systems: terrain, grass and shrubs.
3. Update missiles (terrain and balloon hits) and rings (sphere-crossing test).
4. Game tick: crash check, win check, HUD title.
5. Shadow pass: depth-only into a 2048² FBO from the sun's orthographic view.
6. Main pass, in this order: skybox, terrain, grass, shrubs, water, missiles, balloons, rings, plane, rain, then the state overlay.
7. Swap buffers.

### Performance

With C = visible chunks, V = visible grass and shrub instances, B = balloons, R = rings and M = missiles, the per-frame cost is:

```
O( C·verts_per_chunk + V + B + R + M·(B + 1) + shadow_pass )
```

Frustum and fog culling skip chunks beyond about 1600 units. Instance buffers are uploaded once per rebuild, not once per frame.

On an Arch dev box with a modest GPU at 2560×1600, the game stays comfortably above 60 fps with `chunkRadius = 14` (841 chunks, about 150 visible), 30 balloons, 8 rings and several thousand grass blades. The shadow pass is the dominant cost.

### Challenges

- **Shadow shimmer:** the light-space matrix follows the camera, so its anchor has to snap to whole shadow texels.
- **Wind sped up with flight speed:** noise with a high spatial frequency changed phase quickly as the plane moved. Using longer wavelengths fixed it.
- **Uniform lookups per frame were expensive:** solved with the hash cache described above.
- **Rings were missed:** detection originally counted only one crossing direction. It now uses a generous sphere check, plus a segment-vs-sphere fallback for very fast frames.
- **GL resource lifetime:** destructors could run after `glfwTerminate`. Every resource owner now lives in an inner scope inside `main`.

### What's next

- **Rigid-body aerodynamics** to replace the arcade model: per-surface lift and drag, plus an inertia tensor. See `docs/ENGINE_BACKLOG.md`.
- **Cascaded shadow maps** for sharper near shadows and wider far coverage.
- **A spatial hash** for collisions.
- **An in-game text HUD** instead of the window title.
- **Audio.**

## Credits

- Airplane model: "Airplane" by Poly by Google (Pushilin), [CC BY 3.0](https://creativecommons.org/licenses/by/3.0/).
- [GLAD](https://github.com/Dav1dde/glad), [GLFW](https://www.glfw.org/), [GLM](https://github.com/g-truc/glm).
- Built with help from Claude (Anthropic) via Claude Code for brainstorming, boilerplate drafts, debugging and docs. Every feature was verified and tuned by hand in the running binary.
