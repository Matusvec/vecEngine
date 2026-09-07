# vecEngine

A from-scratch C++/OpenGL 3D game engine driven by an arcade flight game with destructible terrain, scoring rings, and shootable balloon targets.

---

## Project Overview

vecEngine is a single-binary game engine + flight game built in C++17 against raw OpenGL 3.3 (no engine, no scene graph, no asset library). The world is a streamed procedural terrain (Tatra-mountain alpine biome with sand → grass → forest → rock → snow), populated with instanced grass, shrubbery, animated water, rain, a Phong-lit + shadow-mapped sun, and a thin gameplay layer on top — fly the plane, fly through rings (+200 each), shoot down balloons (+100 each), don't crash.

## Relevant Data Structure Concepts

The project leans on several data-structure ideas at once, but the load-bearing ones are:

1. **Spatially indexed dynamic array (chunked grid)** — terrain is held in a `std::vector<Chunk>` of fixed size `(2r+1)²`, addressed by **modular slot indexing** so the visible ring around the player is bounded in memory and only the trailing-edge chunks regenerate when the player crosses a chunk boundary.
2. **Instance buffers + GPU instancing** — grass blades and shrubs live as a `std::vector<Instance>` uploaded once to a GPU vertex buffer, then drawn N-up via `glDrawElementsInstanced`. Per-instance state (position, rotation, scale, tint) is packed and pre-baked on the CPU.
3. **Vector + swap-and-pop for transient objects** — missiles and per-collision sweeps use `std::vector` with O(1) removal by swapping the dead element to the back and popping. No allocation churn.
4. **Hash-indexed cache** — `Shader` uses an `std::unordered_map<string, int>` to memoize `glGetUniformLocation` results (driver lookups are surprisingly expensive when called per frame per uniform).
5. **Sequential array with active index** — the ring course is a `std::vector<Ring>` with a single `active` cursor, advanced when the player enters the next ring's detection sphere.
6. **Append-only event list** — terrain craters live as a static `std::vector<Crater>` that `heightAt(x, z)` reads on every sample. Each crater contributes a smoothstep depression to the noise output, so destruction is a pure data effect.

### 2.1 Application of DS

- **Streaming terrain** (`src/terrain.cpp`): the chunk vector is sized `(2*chunkRadius+1)²`. When the player moves, the new desired chunk-coord per slot is `slot = ((wx % side) + side) % side`. If the slot's stored coord doesn't match the desired coord, that chunk is regenerated on the spot. Crossing one chunk boundary regenerates exactly one row/column, not the whole ring.
- **Frustum + distance culling** (`src/frustum.cpp`, `src/terrain.cpp`): each chunk stores a bounding sphere (center + radius, both computed when the chunk is built). The render loop tests sphere-vs-frustum (Gribb-Hartmann plane extraction) and sphere distance vs fog-end, drawing only what's potentially visible.
- **Grass / shrub instancing** (`src/grass.cpp`, `src/shrubs.cpp`): when the camera crosses a unit cell, the CPU rebuilds the visible instance vector (skipping cells outside the radius / wrong altitude band / below a smooth-noise patch mask) and uploads once via `glBufferData`. Per-frame work is one bind + one instanced draw.
- **Missile-vs-balloon collision** (`src/missile.cpp`, `src/balloon.cpp`): each frame, every missile is tested against every alive balloon with a sphere-overlap check (squared distance vs squared sum-of-radii). Hits remove both via swap-and-pop.
- **Game-state machine** (`src/game.cpp`): four states (`MENU`, `PLAYING`, `WON`, `LOST`) drive screen overlay color + window-title HUD. Transitions are event-driven (SPACE → PLAYING, crash → LOST, all rings + balloons cleared → WON).

## Project Workflow

**Inputs**
- Keyboard: `W/S` throttle, `↑/↓` pitch (yoke convention — push UP to dive), `←/→` bank, `1-9` throttle preset, `Shift` quick burn, `LMB` fire missile, `V` toggle cockpit/3rd-person, `R` restart, `SPACE` start, `ESC` quit.
- Mouse: subtle yaw/pitch trim.

**Per-frame processing**
1. Poll input → flight controller integrates plane physics (throttle → target speed; roll → yaw; pitch; stall under min speed).
2. Update streaming systems (`terrain.update`, `grass.update`, `shrubs.update`).
3. Update transient game state (`missiles.update` → terrain/balloon collisions; `rings.update` → sphere-cross detection).
4. Game tick — crash check, win check, HUD title refresh.
5. **Shadow pass** — render terrain depth into a 2048² depth-only FBO from the sun's orthographic POV (texel-snapped to keep shadow edges stable as the player moves).
6. **Main pass** — clear, draw skybox, terrain (Phong + shadow sample), grass, shrubs, translucent water, missiles, balloons, rings, optional plane mesh (3rd-person), rain, then state-color overlay if not playing.
7. Swap buffers.

**Outputs**
- The framebuffer (the game window).
- Window title bar (live HUD: score, rings, balloons, speed, throttle).
- `stderr` — control reference at startup, diagnostic ring distance every ~30 frames, "RING N PASSED" / "CRASHED" / "WIN" event lines.

## Performance

Per-frame complexity (where C = visible chunks, B = balloons, R = ring count, M = active missiles, V = visible grass blades + shrubs):

```
O( C·verts_per_chunk        // terrain draws
  + V                          // grass + shrub instances
  + B                          // balloon draws
  + R                          // ring draws
  + M·(B + 1)                  // missile collision: sphere vs every balloon, plus terrain
  + 1·shadow_pass              // C chunks again at depth-only into 2048²
)
```

Constants are kept low by frustum + fog-distance culling on terrain (chunks past ~1600 units are skipped) and by uploading instance buffers exactly once per regen instead of per frame.

Observed on an Arch Linux dev box (modest GPU, 2560×1600): comfortably above 60 fps with chunkRadius=14 (=841 chunks total, ~150 visible), 30 balloons, 8 rings, several thousand grass blades. Single-threaded CPU; the dominant cost is the shadow pass at high resolution.

## Challenges

- **Shadow mapping math.** Constructing a stable orthographic light-space matrix that follows the camera without shimmering required snapping the anchor point to whole shadow-texel boundaries — easy to overlook, immediately visible if you don't.
- **Wind animation speeding up at high flight speed.** Initial wind/wave shaders sampled noise at a high spatial frequency; flying through the field made phase change rapidly, creating an unintended "faster wind" effect. Fix: lower spatial frequency (longer wavelengths) so player movement contributes less per-frame phase change.
- **Per-frame uniform queries cost more than expected.** `glGetUniformLocation` per uniform per frame visibly hurt frame time; replaced with a hash-cached lookup in `Shader`.
- **Ring detection.** First implementation only counted one direction of plane crossing → user could fly through a ring and have nothing register. Replaced with a generous 2.5×-radius sphere check + per-frame segment-vs-sphere test as a fallback for ultra-fast frames.
- **GL resource lifetime.** Mesh/Shader destructors invoke `glDelete*` on a context that may already be torn down by `glfwTerminate`. Fixed by wrapping all resource owners in an inner `{}` scope inside `main`, so they destruct before terminate.
- **Crash detection without visible feedback.** The crash check fired correctly but the player just saw a frozen scene with no signal. Added a state-driven screen-color overlay (red on crash, green on win, dark blue on menu) to make the result unmistakable.

## Improvements

If I were to redo this:

- **Spatial hash for collision.** Missile-vs-balloon is currently O(B) per missile per frame. With thousands of targets it would matter; a uniform spatial grid would make it amortized O(1).
- **Cascaded shadow maps.** Single-cascade shadows at 2048² either look pixelated up close or coarse far away. CSM (3-4 frusta) is the standard fix.
- **In-game text.** Status currently goes to the window title and the console — fine for development, weak for a real player. A bitmap-font or stb_truetype text renderer would give proper HUD.
- **Decouple lighting plumbing.** Every renderer's `draw()` signature ended up taking `LightingSystem`, `lightSpaceMatrix`, `shadowMapUnit`. A "frame context" struct would clean the interfaces up.
- **Sound.** No audio at all currently. miniaudio + a few WAVs would meaningfully improve game feel.

## Learning

- The OpenGL pipeline end-to-end: GLAD loading, VAO/VBO/EBO layout, shader compile/link, uniform binding, texture units, FBOs, depth testing/blending state.
- GLSL — value noise, FBM, ridged-multifractal terrain, domain warping, smoothstep biome blending, vertex-shader animation (wind/waves), Phong lighting, PCF shadow sampling.
- Procedural content techniques — chunked streaming, modular slot indexing, instanced rendering, distance-snapped shadow camera.
- A simple but real game-state architecture (MENU/PLAYING/WON/LOST) and event-driven scoring.
- An arcade flight model: throttle → target speed, roll → induced yaw, stall behavior, exponential velocity smoothing for momentum.

## Real-World Relevance

The same techniques scale directly to:

- **Flight simulators / drone training** — same physics shape (banked turns, stall, momentum), same procedural-terrain pattern, same shadow + atmospheric pipeline.
- **Open-world games** — the chunked-streaming pattern here is functionally identical to how voxel and terrain games (Minecraft, No Man's Sky) bound memory while presenting an "infinite" world.
- **GIS / scientific visualization** — terrain LOD, frustum culling, biome-by-altitude shading are exactly what a city/landscape viewer needs.
- **Game-engine fundamentals education** — the codebase intentionally avoids high-level engines so each concept (mesh, shader, framebuffer, instance buffer, shadow map) is exposed in isolation.

## Use of AI Tools

This project was developed in close collaboration with **Claude (Anthropic)** running inside Claude Code:

- **Brainstorming.** Architectural decisions (modular chunk indexing, when to upload instance buffers, shadow-mapping vs screen-space ambient occlusion) were talked through interactively before implementation.
- **Code generation.** Most of the rendering boilerplate (FBO setup, torus mesh generator, sphere mesh generator, value-noise + FBM, MVP/lookAt math) was AI-drafted, then visually verified and tuned by hand.
- **Debugging.** Two non-obvious bugs (wind animation accelerating with player speed, ring detection only firing in one direction) were diagnosed conversationally — the AI proposed hypotheses based on the code and the user's reproduction steps.
- **Documentation.** Inline comments throughout the source explain the *why* behind non-obvious choices (texel-snapping the shadow camera, stalling under low speed, swap-and-pop for missile cleanup). This README itself was drafted by the AI from the actual code state and reviewed.
- **Library reference.** The Context7 MCP server was used to pull current documentation when picking GL/GLM idioms.

The AI did not invent anything that wasn't visually verified in the running game. Every feature was tested in the binary, and every numeric tunable (sun direction, fog distances, biome thresholds, throttle ramp, ring detection radius) was iterated by running the game and adjusting based on what looked right.

---

## Build & Run

System dependencies (Arch Linux): `sudo pacman -S cmake glfw-x11 glm`. GLAD is vendored.

```
make all      # cmake -B build && cmake --build build
make run      # build + run ./build/flight
make clean    # rm -rf build
```

Run from a terminal so you can see the startup control banner and ring-distance diagnostic lines.

## Controls

| Key            | Action                                  |
|----------------|-----------------------------------------|
| `W` / `S`      | throttle up / down                      |
| `1` – `9`      | snap throttle to preset                 |
| `Shift`        | quick burn                              |
| `↑` / `↓`      | pitch (yoke convention: UP = dive)      |
| `←` / `→`      | bank (LEFT = turn left)                 |
| Mouse          | fine yaw/pitch trim                     |
| `LMB`          | fire missile                            |
| `V`            | toggle cockpit / 3rd-person             |
| `R`            | restart                                 |
| `SPACE`        | start (from menu)                       |
| `ESC`          | quit                                    |
