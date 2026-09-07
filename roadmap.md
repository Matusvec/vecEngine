# Flight Engine — Full Build Roadmap

> A phase-by-phase guide to building a C++ GPU-powered flight engine from scratch.
> Designed for learning — each phase builds on the last, with concepts explained before implementation.
> Reference your old `project.c` for game logic values (speeds, spawn rates, collision radii, terrain formula).

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Phase 1: Project Setup & First Triangle](#phase-1-project-setup--first-triangle)
- [Phase 2: Shader System & Camera](#phase-2-shader-system--camera)
- [Phase 3: 3D Meshes & Transforms](#phase-3-3d-meshes--transforms)
- [Phase 4: Terrain System](#phase-4-terrain-system)
- [Phase 5: Flight Controller & Input](#phase-5-flight-controller--input)
- [Phase 6: Game Objects & Entity System](#phase-6-game-objects--entity-system)
- [Phase 7: Textures & Materials](#phase-7-textures--materials)
- [Phase 8: Lighting](#phase-8-lighting)
- [Phase 9: Skybox & Atmosphere](#phase-9-skybox--atmosphere)
- [Phase 10: Collision System](#phase-10-collision-system)
- [Phase 11: Projectile System](#phase-11-projectile-system)
- [Phase 12: HUD & Text Rendering](#phase-12-hud--text-rendering)
- [Phase 13: Audio](#phase-13-audio)
- [Phase 14: Particle Effects](#phase-14-particle-effects)
- [Phase 15: Performance & Polish](#phase-15-performance--polish)
- [Phase 16: Advanced Features (Optional)](#phase-16-advanced-features-optional)
- [Appendix A: Project Structure](#appendix-a-project-structure)
- [Appendix B: Key Math Concepts](#appendix-b-key-math-concepts)
- [Appendix C: Common Pitfalls](#appendix-c-common-pitfalls)
- [Appendix D: Resources](#appendix-d-resources)

---

## Prerequisites

### Install These (Arch Linux)

```bash
sudo pacman -S cmake glfw-x11 glew glm freetype2 assimp
yay -S glad          # or generate from https://glad.dav1d.de/
```

If you prefer generating GLAD yourself (recommended to understand it):
1. Go to https://glad.dav1d.de/
2. Language: C/C++, Specification: OpenGL, Profile: Core, Version: 4.6 (or 3.3 minimum)
3. Generate → download → extract `glad.c` and `glad/` headers into your project

### Concepts You Should Understand Before Starting

- **What a GPU is**: A massively parallel processor. Your CPU runs 1 thing fast. The GPU runs 10,000 things simultaneously. Every vertex and every pixel gets its own tiny program (shader) running in parallel.
- **What a shader is**: A small program written in GLSL (GL Shading Language) that runs on the GPU. There are two you'll use constantly:
  - **Vertex shader**: runs once per vertex. Input: 3D position. Output: screen position.
  - **Fragment shader**: runs once per pixel (fragment). Input: interpolated data from vertex shader. Output: color.
- **What a buffer is**: A chunk of GPU memory. You upload your vertex data to a buffer (VBO), and the GPU reads from it directly — no CPU involvement per frame.
- **What a matrix is in this context**: A 4×4 grid of numbers that encodes a transformation (move, rotate, scale, project). Multiplying a position by a matrix transforms it. The GPU does this multiplication for every vertex, in parallel.

### C++ Knowledge You'll Need

- Classes and structs (constructors, destructors, methods)
- `std::vector`, `std::string`, `std::unordered_map`
- Smart pointers (`std::unique_ptr`, `std::shared_ptr`) — or raw pointers if you prefer
- Basic file I/O (`std::ifstream`)
- Header/source file separation
- No need for templates, metaprogramming, or advanced C++ — keep it simple

---

## Phase 1: Project Setup & First Triangle

### Goal
Get a window open with a colored triangle rendered by the GPU. This proves your entire toolchain works.

### Concepts to Learn
- **GLFW**: Library that creates a window and an OpenGL context. Without a context, OpenGL calls do nothing.
- **GLAD/GLEW**: OpenGL is just a specification. The actual function pointers need to be loaded at runtime. GLAD/GLEW does this for you.
- **The OpenGL state machine**: OpenGL is global state. When you call `glBindBuffer(...)`, you're saying "all future buffer operations apply to THIS buffer." It's like setting a current selection.

### Steps

1. **Set up CMake project**

```
flight-engine/
├── CMakeLists.txt
├── src/
│   └── main.cpp
├── shaders/
│   ├── basic.vert
│   └── basic.frag
└── vendor/          # third-party headers (glad, stb, etc.)
```

2. **Write `CMakeLists.txt`**

```cmake
cmake_minimum_required(VERSION 3.20)
project(FlightEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

# Your source files (add more as you create them)
file(GLOB_RECURSE SOURCES src/*.cpp)

add_executable(flight ${SOURCES} vendor/glad/glad.c)  # if using GLAD

target_include_directories(flight PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(flight glfw OpenGL::GL glm::glm)
```

3. **Write `main.cpp`** — do the following in order:
   - `glfwInit()` → initialize GLFW
   - `glfwCreateWindow()` → create an 800×600 window (match your old resolution to start)
   - `glfwMakeContextCurrent()` → bind the OpenGL context
   - `gladLoadGLLoader()` or `glewInit()` → load OpenGL functions
   - `glViewport(0, 0, 800, 600)` → tell OpenGL the render area
   - **Main loop**: `while (!glfwWindowShouldClose(window))`
     - `glClear(GL_COLOR_BUFFER_BIT)` → clear screen
     - `glfwSwapBuffers(window)` → show frame (replaces your `gfx_flush()`)
     - `glfwPollEvents()` → handle input (replaces your `gfx_event_waiting()` loop)

4. **Define a triangle** (3 vertices, x/y/z each):

```cpp
float vertices[] = {
    -0.5f, -0.5f, 0.0f,
     0.5f, -0.5f, 0.0f,
     0.0f,  0.5f, 0.0f
};
```

5. **Upload to GPU**:
   - Create a VAO (Vertex Array Object) — stores the vertex format configuration
   - Create a VBO (Vertex Buffer Object) — stores the actual vertex data
   - `glVertexAttribPointer()` — tell OpenGL how to interpret the data (3 floats per vertex, tightly packed)

6. **Write basic shaders**:

```glsl
// basic.vert
#version 330 core
layout (location = 0) in vec3 aPos;
void main() {
    gl_Position = vec4(aPos, 1.0);
}
```

```glsl
// basic.frag
#version 330 core
out vec4 FragColor;
void main() {
    FragColor = vec4(0.4, 1.0, 0.4, 1.0);  // green like your terrain
}
```

7. **Compile shaders at runtime**: read the files, call `glCreateShader()`, `glShaderSource()`, `glCompileShader()`, `glCreateProgram()`, `glLinkProgram()`

8. **Draw**: `glUseProgram(shaderProgram)` → `glBindVertexArray(VAO)` → `glDrawArrays(GL_TRIANGLES, 0, 3)`

### Checkpoint
You should see a green triangle on a black background. If you do, your entire build pipeline, OpenGL context, shader compilation, and GPU rendering all work.

### Key Differences from Your Old Code
| Old (`gfx.h`) | New (OpenGL) |
|---|---|
| `gfx_open(800, 600, "title")` | `glfwCreateWindow(800, 600, "title", NULL, NULL)` |
| `gfx_clear()` | `glClear(GL_COLOR_BUFFER_BIT \| GL_DEPTH_BUFFER_BIT)` |
| `gfx_flush()` | `glfwSwapBuffers(window)` |
| `gfx_line(x1,y1,x2,y2)` | Upload vertices to VBO, `glDrawArrays(GL_LINES, ...)` |
| `gfx_color(r,g,b)` | Set color as a uniform in your shader |
| `gfx_event_waiting()` | `glfwPollEvents()` + callbacks or state queries |

---

## Phase 2: Shader System & Camera

### Goal
Build a reusable `Shader` class and implement a 3D camera with perspective projection.

### Concepts to Learn
- **Uniforms**: Variables you send from C++ to a shader. The MVP matrix is a uniform.
- **Model-View-Projection (MVP)**: The three matrices that transform a 3D point to a screen pixel:
  - **Model**: object space → world space (where is this object in the world?)
  - **View**: world space → camera space (where is everything relative to the camera?)
  - **Projection**: camera space → clip space (apply perspective distortion)
  - The GPU multiplies: `gl_Position = Projection * View * Model * vec4(position, 1.0)`
  - This single line replaces your entire `project_point()` function.
- **GLM**: The math library. Drop-in replacement for all your manual trig.

### Steps

1. **Create `Shader` class** (`src/renderer/Shader.h` and `.cpp`):
   - Constructor takes vertex and fragment shader file paths
   - Reads shader source from files (`std::ifstream`)
   - Compiles and links (with error checking — `glGetShaderiv(GL_COMPILE_STATUS)`)
   - Methods: `use()`, `setMat4(name, matrix)`, `setVec3(name, vec)`, `setFloat(name, val)`
   - Destructor calls `glDeleteProgram()`

2. **Create `Camera` class** (`src/renderer/Camera.h` and `.cpp`):
   - Members: `position` (glm::vec3), `front`, `up`, `right`, `yaw`, `pitch`
   - Method: `getViewMatrix()` → returns `glm::lookAt(position, position + front, up)`
   - Method: `getProjectionMatrix(fov, aspect, near, far)` → returns `glm::perspective(...)`
   - Method: `updateVectors()` → recalculates `front`, `right`, `up` from yaw/pitch
     - This is what `update_camera_trig()` did, but GLM computes the direction vector for you

3. **Update your shaders** to accept a MVP uniform:

```glsl
// basic.vert
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 MVP;

void main() {
    gl_Position = MVP * vec4(aPos, 1.0);
}
```

4. **In your render loop**:

```cpp
glm::mat4 model = glm::mat4(1.0f);  // identity — no transform yet
glm::mat4 view = camera.getViewMatrix();
glm::mat4 proj = glm::perspective(glm::radians(70.0f), 800.0f/600.0f, 0.1f, 2000.0f);
glm::mat4 mvp = proj * view * model;

shader.use();
shader.setMat4("MVP", mvp);
```

5. **Enable depth testing**: `glEnable(GL_DEPTH_TEST)` — this makes closer objects hide farther ones. Without it, things draw in the order you submit them (painter's algorithm), which breaks for 3D. Clear with `GL_DEPTH_BUFFER_BIT`.

### Checkpoint
Move the triangle away from the origin (translate the model matrix). Rotate it. You should see it in perspective — smaller when far, larger when close. Try moving the camera position to confirm the view matrix works.

### Understanding: Your Old Projection vs. New

Your old code (CPU, per point, every frame):
```c
dx = p.x - cam->position.x;          // translate
rx = dx * cos_yaw - dz * sin_yaw;    // rotate yaw
ry = ty * cos_pitch - tz * sin_pitch; // rotate pitch
scale = 300.0 / rz;                  // perspective divide
*sx = rx * scale + 400;              // to screen coords
```

New code (GPU, all points simultaneously, every frame):
```glsl
gl_Position = Projection * View * Model * vec4(aPos, 1.0);
// that's it. the GPU does ALL the math above in one matrix multiply.
```

---

## Phase 3: 3D Meshes & Transforms

### Goal
Build a `Mesh` class that can load and render any geometry. Draw a cube (like your obstacles).

### Concepts to Learn
- **Index buffers (EBO)**: Instead of repeating shared vertices, list each vertex once and use indices to form triangles. A cube has 8 unique vertices but 36 indices (12 triangles × 3 vertices each).
- **Vertex attributes**: Each vertex can carry more than just position — also normals, texture coordinates, colors. You configure this with `glVertexAttribPointer()`.
- **Normals**: A vector perpendicular to a surface, pointing outward. Used for lighting later. For a cube face pointing toward +Z, the normal is (0, 0, 1).

### Steps

1. **Define a `Vertex` struct**:

```cpp
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};
```

2. **Create `Mesh` class** (`src/renderer/Mesh.h` and `.cpp`):
   - Constructor takes `std::vector<Vertex>` and `std::vector<unsigned int>` (indices)
   - Creates VAO, VBO, EBO in constructor
   - Method: `draw()` → binds VAO, calls `glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, 0)`
   - Destructor cleans up GPU resources

3. **Write a cube generator function**:
   - 24 vertices (4 per face × 6 faces — duplicated at edges so each face has its own normal)
   - 36 indices
   - Reference: your `draw_wireframe_cube()` already defines the 8 corners. Now you're making triangulated faces instead of 12 edge lines.

4. **Implement transforms**:
   - Each renderable object gets a model matrix
   - Position → `glm::translate()`
   - Rotation → `glm::rotate()` (replaces your `cos(rot)/sin(rot)` corner rotation)
   - Scale → `glm::scale()` (replaces your `size * 0.5` half-size math)

5. **Draw multiple cubes** at different positions/rotations by changing the model matrix before each draw call.

### Checkpoint
Render 5-10 cubes scattered in 3D space, each rotating at a different speed (like your obstacles). They should be solid-colored (no textures yet), with correct depth ordering.

---

## Phase 4: Terrain System

### Goal
Replace your `draw_terrain()` wireframe grid with a real mesh that the GPU renders.

### Concepts to Learn
- **Heightmap mesh generation**: Create a grid of vertices where the Y coordinate comes from a height function. Connect vertices with triangles.
- **Triangle strips vs. indexed triangles**: Two ways to define a grid mesh. Indexed is more flexible.
- **Terrain chunking**: For large terrains, break into chunks and only render nearby ones.

### Steps

1. **Create `Terrain` class** (`src/world/Terrain.h` and `.cpp`):
   - Constructor takes grid size, spacing, and a reference to a height function
   - Generates a mesh:
     - For each grid point (i, j): create a vertex at `(i * spacing, height(x, z), j * spacing)`
     - For each grid cell: create 2 triangles (6 indices or 2 triangle strip entries)
   - The height function is your existing formula:

```cpp
float getHeight(float x, float z) {
    return 30.0f * sin(x * 0.01f) * sin(z * 0.01f)
         + 15.0f * sin(x * 0.03f + z * 0.02f);
}
```

2. **Calculate normals** for lighting later:
   - For each vertex, sample height at 4 neighboring points
   - Cross product of the tangent vectors gives the surface normal
   - This makes hills shade correctly when you add lighting

```cpp
glm::vec3 calcNormal(float x, float z) {
    float h = 0.5f;  // sample offset
    float hL = getHeight(x - h, z);
    float hR = getHeight(x + h, z);
    float hD = getHeight(x, z - h);
    float hU = getHeight(x, z + h);
    return glm::normalize(glm::vec3(hL - hR, 2.0f * h, hD - hU));
}
```

3. **Implement terrain following** — the terrain should move with the camera:
   - Option A: Generate one huge mesh (simple, limited world size)
   - Option B: Generate chunks around the camera and load/unload as you move (your old code did this with the `baseX/baseZ` grid snapping — same concept, but now with mesh chunks)

4. **Add a wireframe toggle** for debugging: `glPolygonMode(GL_FRONT_AND_BACK, GL_LINE)` draws everything as wireframes — useful to verify your mesh topology before adding textures.

### Checkpoint
You should see rolling green hills extending in all directions, moving under you as the camera flies forward. Switch between wireframe and solid to verify the mesh.

---

## Phase 5: Flight Controller & Input

### Goal
Implement your flight controls so you can fly around the terrain.

### Concepts to Learn
- **Delta time**: Instead of `usleep(12000)`, calculate the actual time between frames. Multiply all movement by delta time so the game runs at the same speed regardless of framerate.
- **GLFW input**: Two approaches:
  - **Polling**: `glfwGetKey(window, GLFW_KEY_Q)` — check every frame
  - **Callbacks**: `glfwSetKeyCallback(window, func)` — called when key state changes
  - Use polling for continuous input (movement), callbacks for one-shot input (quit, shoot)

### Steps

1. **Create `Input` class** (`src/core/Input.h` and `.cpp`):
   - Wraps GLFW input functions
   - Tracks mouse position, mouse delta, key states
   - Method: `isKeyPressed(int key)` → `glfwGetKey(window, key) == GLFW_PRESS`
   - Method: `getMouseDelta()` → returns how much the mouse moved since last frame
   - Call `glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED)` to capture the mouse (FPS-style)

2. **Create `FlightController` class** (`src/game/FlightController.h` and `.cpp`):
   - Owns a `Camera` reference
   - `update(float dt, Input& input)`:
     - Get mouse delta
     - Convert to yaw/pitch changes (same math as your `target_yaw * steer_speed`)
     - Clamp pitch (your old limits: -1.2 to +0.8 radians)
     - Move camera forward along its front vector: `position += front * speed * dt`
     - Handle speed changes (+/- keys)
   - This is a direct port of your main loop steering code, just cleaner

3. **Implement delta time**:

```cpp
float lastFrame = 0.0f;
while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    float dt = currentFrame - lastFrame;
    lastFrame = currentFrame;

    flightController.update(dt, input);
    // ... render ...
}
```

4. **Add roll** (new feature your old game didn't have):
   - When turning left/right, the plane should bank
   - Interpolate a roll angle toward `targetRoll` based on yaw rate
   - Apply roll to the camera's up vector using a rotation around the front vector

### Checkpoint
You should be able to fly freely over the terrain with mouse steering. Speed up/slow down with +/-. The terrain should stream infinitely as you move. Press Q to quit.

### Porting Reference
| Old code | New code |
|---|---|
| `game.camera.yaw += target_yaw * steer_speed` | Same idea, use mouse delta * sensitivity * dt |
| `position.x += speed * sin_yaw * cos_pitch` | `position += front * speed * dt` (GLM does the trig) |
| `if (pitch < -1.2) pitch = -1.2` | `pitch = glm::clamp(pitch, -1.2f, 0.8f)` |
| `usleep(12000)` | Delta time (no sleeping, just measure elapsed time) |

---

## Phase 6: Game Objects & Entity System

### Goal
Build a system to manage game objects (obstacles, bullets, player) in a structured way.

### Concepts to Learn
- **Entity-Component pattern** (simplified): Each game object has a transform (position/rotation/scale) and optional components (renderable, collider, etc.)
- **Object pooling**: Pre-allocate a pool of objects and activate/deactivate them instead of allocating/freeing memory every frame. Your old code already does this with `active` flags — same concept.

### Steps

1. **Create `Transform` struct**:

```cpp
struct Transform {
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};  // euler angles (or use glm::quat)
    glm::vec3 scale{1.0f};

    glm::mat4 getModelMatrix() const;
};
```

2. **Create `Entity` base class** (`src/world/Entity.h`):

```cpp
class Entity {
public:
    Transform transform;
    bool active = false;

    virtual void update(float dt) {}
    virtual void render(Shader& shader) {}
};
```

3. **Create `Obstacle` class** inheriting from Entity:
   - Has a `Mesh*` pointer (shared cube mesh — don't duplicate geometry)
   - Has a rotation speed
   - `update()`: rotate over time
   - `render()`: set model matrix uniform, draw mesh
   - Spawning logic from your `update_obstacles()` — spawn in front of camera, random angle within FOV

4. **Create `ObjectPool<T>` template** (optional but clean):

```cpp
template<typename T, int MAX>
class ObjectPool {
    T objects[MAX];
public:
    T* spawn();          // find inactive, activate, return pointer
    void updateAll(float dt);
    void renderAll(Shader& shader);
    // iterator support for collision checking
};
```

### Checkpoint
Cubes should spawn ahead of you as you fly, rotate in place, and despawn when far away. Same behavior as your old game, but structured.

---

## Phase 7: Textures & Materials

### Goal
Add textures to your terrain and objects. Replace flat colors with actual images.

### Concepts to Learn
- **Texture mapping**: Each vertex has a UV coordinate (0-1 range) that maps to a point on an image. The GPU interpolates UVs across triangles and samples the image in the fragment shader.
- **Texture units**: The GPU has slots (GL_TEXTURE0, GL_TEXTURE1, etc.) where you bind textures. Shaders reference them by slot number.
- **Mipmaps**: Pre-scaled versions of a texture for when it's far away. Prevents shimmering/aliasing. Generate with `glGenerateMipmap()`.

### Steps

1. **Add stb_image** (header-only image loader):
   - Download from https://github.com/nothings/stb
   - Put `stb_image.h` in `vendor/`
   - In one `.cpp` file: `#define STB_IMAGE_IMPLEMENTATION` then `#include "stb_image.h"`

2. **Create `Texture` class** (`src/renderer/Texture.h` and `.cpp`):
   - Constructor takes a file path
   - Loads image with `stbi_load()`
   - Creates OpenGL texture: `glGenTextures()`, `glBindTexture()`, `glTexImage2D()`
   - Sets filtering: `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR)`
   - Generates mipmaps
   - Method: `bind(int unit)` → `glActiveTexture(GL_TEXTURE0 + unit)` then `glBindTexture()`

3. **Update fragment shader** to sample a texture:

```glsl
uniform sampler2D textureSampler;
in vec2 TexCoords;

void main() {
    FragColor = texture(textureSampler, TexCoords);
}
```

4. **Generate terrain UVs**: For each terrain vertex at world position (x, z):
   - `u = x * tileScale` and `v = z * tileScale`
   - The texture repeats across the terrain (tiling)

5. **Find or create textures**:
   - Grass texture for terrain
   - Metal/stone texture for cubes
   - Free textures: textures.com, polyhaven.com, ambientcg.com

### Checkpoint
The terrain should have a grassy look. Cubes should have a metallic or stone texture. Everything is now textured instead of flat-colored.

### Note: This Replaces Your PPM System
Your old `draw_ppm_scaled()` loaded PPMs manually pixel-by-pixel. Now `stbi_load()` handles PNG/JPG/BMP/PPM in one call, and the GPU renders the texture — no per-pixel CPU loops.

---

## Phase 8: Lighting

### Goal
Add directional lighting (a sun) so objects have shading and depth.

### Concepts to Learn
- **Phong lighting model**: Three components:
  - **Ambient**: base light everywhere (prevents pure black shadows)
  - **Diffuse**: light hitting a surface based on angle (uses the normal vector)
  - **Specular**: shiny highlights (uses view direction)
- **Normal vectors**: You already computed these for terrain in Phase 4. Cubes have simple normals (each face points outward along an axis).
- **Fragment shader lighting**: All lighting math happens per-pixel on the GPU.

### Steps

1. **Define a directional light** (like the sun):

```cpp
struct DirectionalLight {
    glm::vec3 direction{-0.5f, -1.0f, -0.3f};  // pointing down and to the side
    glm::vec3 color{1.0f, 0.95f, 0.8f};         // warm white
    float ambientStrength = 0.2f;
};
```

2. **Write a lit fragment shader**:

```glsl
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform sampler2D textureSampler;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 viewPos;

void main() {
    vec3 texColor = texture(textureSampler, TexCoords).rgb;

    // Ambient
    vec3 ambient = 0.2 * lightColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 dir = normalize(-lightDir);
    float diff = max(dot(norm, dir), 0.0);
    vec3 diffuse = diff * lightColor;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);
    vec3 specular = 0.5 * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * texColor;
    FragColor = vec4(result, 1.0);
}
```

3. **Update vertex shader** to pass normals and world position to fragment shader:

```glsl
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;  // transpose(inverse(model)) — transforms normals correctly

out vec3 FragPos;
out vec3 Normal;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = normalMatrix * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
```

4. **Pass uniforms from C++**:

```cpp
shader.setVec3("lightDir", light.direction);
shader.setVec3("lightColor", light.color);
shader.setVec3("viewPos", camera.position);
shader.setMat3("normalMatrix", glm::transpose(glm::inverse(glm::mat3(model))));
```

### Checkpoint
Terrain hills should be lighter on sun-facing slopes and darker in valleys. Cubes should have visible shading — you can tell which face points toward the light. The scene suddenly looks 3D instead of flat.

---

## Phase 9: Skybox & Atmosphere

### Goal
Replace your `draw_sky()` (wireframe horizon lines + circle sun) with a proper sky.

### Concepts to Learn
- **Cubemap**: 6 images forming the inside of a cube. The camera sits at the center. Render it behind everything else.
- **Depth buffer trick**: Render the skybox with depth writing disabled, and set its vertex shader to always output maximum depth. It draws behind everything.

### Steps

1. **Option A: Cubemap skybox** (image-based)
   - Find 6 sky images (top, bottom, left, right, front, back) — polyhaven.com has free ones
   - Load into a `GL_TEXTURE_CUBE_MAP`
   - Render a cube with a special shader that samples the cubemap using the view direction
   - Strip translation from the view matrix so the skybox doesn't move with the camera (it's infinitely far away)

2. **Option B: Procedural sky shader** (no images needed, more impressive)
   - Write a fullscreen quad shader that computes sky color based on direction
   - Mix blue (zenith) to orange (horizon) based on Y component of view direction
   - Add a sun disc using `dot(viewDir, sunDir)`
   - Add atmospheric scattering for sunset colors

3. **Add fog** to blend terrain into the sky at distance:

```glsl
// In terrain fragment shader
float dist = length(FragPos - viewPos);
float fogFactor = exp(-dist * 0.001);  // exponential fog
FragColor = mix(fogColor, objectColor, fogFactor);
```

### Checkpoint
The sky should look like a sky. The terrain should fade into haze at the render distance rather than abruptly popping in/out.

---

## Phase 10: Collision System

### Goal
Port your collision detection from distance-squared checks to a proper collision system.

### Concepts to Learn
- **AABB (Axis-Aligned Bounding Box)**: A box aligned to world axes. Cheapest collision check. Good enough for most cases.
- **OBB (Oriented Bounding Box)**: A box that rotates with the object. More accurate for rotated cubes.
- **Spatial partitioning**: Don't check every object against every other object. Divide space into cells and only check objects in the same/neighboring cells.

### Steps

1. **Create `Collider` struct**:

```cpp
struct AABB {
    glm::vec3 min, max;

    bool intersects(const AABB& other) const {
        return (min.x <= other.max.x && max.x >= other.min.x) &&
               (min.y <= other.max.y && max.y >= other.min.y) &&
               (min.z <= other.max.z && max.z >= other.min.z);
    }
};

struct SphereCollider {
    glm::vec3 center;
    float radius;

    bool intersects(const SphereCollider& other) const {
        float distSq = glm::length2(center - other.center);  // length squared — no sqrt!
        float radSum = radius + other.radius;
        return distSq < radSum * radSum;
    }
};
```

2. **Create `CollisionSystem`** (`src/game/CollisionSystem.h`):
   - `checkBulletObstacle(bullets, obstacles)` — your existing logic
   - `checkPlayerObstacle(player, obstacles)` — your existing logic
   - `checkPlayerTerrain(player, terrain)` — ground collision
   - Returns collision events that the game logic handles (score, lives, game over)

3. **Port your collision radii** from the old code:
   - Bullet-obstacle hit distance: `obstacle.size` (from `check_collisions()`)
   - Player-obstacle hit distance: `obstacle.size + 20` (from `check_collisions()`)
   - Ground collision: `terrain_height + 15` (from your ground check)

### Checkpoint
Flying into cubes should register hits. Flying into the ground should trigger death. Bullets should destroy cubes. Same gameplay as before, just cleaner code.

---

## Phase 11: Projectile System

### Goal
Implement bullet firing and rendering.

### Steps

1. **Create `Projectile` class**:
   - Position, velocity, lifetime
   - `update(float dt)`: `position += velocity * dt` (your old code, but with delta time)
   - Render as either:
     - A small glowing sphere mesh
     - A billboard quad (always faces camera) with a glow texture
     - A point sprite (simplest — `glPointSize()` + glow in fragment shader)

2. **Create `ProjectilePool`**:
   - Pre-allocate array (like your `Bullet bullets[MAX_BULLETS]`)
   - `fire(position, direction, speed)` — finds inactive slot, activates
   - `updateAll(dt)` — move all active projectiles, deactivate expired ones

3. **Firing logic** (port from `fire_bullet()`):

```cpp
void fire(const Camera& cam) {
    pool.fire(cam.position, cam.front * BULLET_SPEED);
}
```

This replaces your manual `sin_yaw * cos_pitch` velocity calculation — `cam.front` already encodes the direction.

4. **Optional: Add a tracer/trail effect** — store the last N positions and render as a line strip.

### Checkpoint
Click to shoot. Bullets travel forward. They hit cubes and destroy them. Score increases. Same gameplay as old project.

---

## Phase 12: HUD & Text Rendering

### Goal
Display score, lives, timer, and crosshair on screen.

### Concepts to Learn
- **Orthographic projection**: For 2D overlay, use `glm::ortho(0, width, 0, height)` instead of perspective. Coordinates map directly to pixels.
- **Text rendering**: This is genuinely one of the harder parts of OpenGL. The GPU doesn't know about fonts — you have to manually convert characters to textured quads.

### Steps

1. **Crosshair** (simplest — render first):
   - Draw 4 short lines using `GL_LINES` in screen space with orthographic projection
   - Or render a crosshair texture on a centered quad

2. **Text rendering with FreeType**:
   - Load a font (`.ttf`) with FreeType
   - For each character: render the glyph to a bitmap, upload as a texture
   - Store character metrics (width, height, bearing, advance)
   - To render a string: for each character, draw a textured quad at the right position

3. **Alternative: Bitmap font atlas** (simpler):
   - Create/download a texture with all ASCII characters in a grid
   - Map each character to UV coordinates in the atlas
   - Render text as a series of textured quads
   - Faster to implement but less flexible

4. **HUD layout** (port from `draw_hud()`):
   - Top-left: score bar (a colored quad, width based on score/WIN_SCORE)
   - Top-left: timer text
   - Top-right: lives (render as icons or text)
   - Center: crosshair
   - Disable depth testing for HUD rendering (`glDisable(GL_DEPTH_TEST)`)

### Checkpoint
You should see score, timer, lives overlaid on the 3D scene. Text is readable at any camera angle.

---

## Phase 13: Audio

### Goal
Replace `printf("\a")` terminal beeps with actual sound effects.

### Steps

1. **Choose a library**:
   - **miniaudio** (recommended) — single header, zero dependencies, works on Linux
   - Download from https://github.com/mackron/miniaudio
   - Put `miniaudio.h` in `vendor/`

2. **Create `AudioSystem`** (`src/core/Audio.h`):
   - Initialize audio engine on startup
   - Load sound files (`.wav` or `.ogg`) into memory
   - `playSound(name)` — play a sound effect (fire-and-forget)
   - `playMusic(name)` — looping background track

3. **Sound events**:
   - Shoot: quick laser/pew sound
   - Hit obstacle: explosion
   - Collision (damage): crunch/impact
   - Ground crash: big explosion
   - Win: victory jingle
   - Background: engine hum or ambient wind (loop)

4. **Find free sounds**: freesound.org, opengameart.org, kenney.nl/assets

### Checkpoint
Shooting, hitting, and crashing all play distinct sounds. Optional: engine hum gets louder with speed.

---

## Phase 14: Particle Effects

### Goal
Add visual effects for explosions, bullet trails, and engine exhaust.

### Concepts to Learn
- **GPU particles**: Each particle has position, velocity, lifetime, color. Update on CPU (or in a compute shader), render as point sprites or billboard quads.
- **Billboarding**: A quad that always faces the camera. Extract the right/up vectors from the view matrix.
- **Additive blending**: `glBlendFunc(GL_SRC_ALPHA, GL_ONE)` — overlapping particles get brighter instead of occluding.

### Steps

1. **Create `ParticleSystem`**:
   - Pool of particles (position, velocity, life, color, size)
   - `emit(position, count, settings)` — burst of particles
   - `update(dt)` — move particles, fade out, remove dead ones
   - `render()` — draw as point sprites with a glow texture

2. **Effects to implement**:
   - **Obstacle destruction**: burst of red/orange particles outward
   - **Bullet trail**: small yellow particles spawned along bullet path
   - **Engine exhaust**: continuous stream of particles behind the camera (visible in 3rd person if you ever add that)
   - **Ground hit**: dirt/dust burst

3. **Use instanced rendering** for particles — upload all positions to a buffer, draw with `glDrawArraysInstanced()`. One draw call for all particles.

### Checkpoint
Destroying a cube creates a satisfying explosion. Bullets leave faint trails. The scene feels alive.

---

## Phase 15: Performance & Polish

### Goal
Make it run smoothly and look finished.

### Steps

1. **Frustum culling**:
   - Extract the 6 frustum planes from the VP matrix
   - Before drawing any object, check if its bounding sphere is inside the frustum
   - Skip the draw call if it's outside — saves GPU work
   - Your old code did this with `if (distSq > RENDER_DIST_SQ) continue` — same idea, better shape

2. **Frame rate counter**:
   - Track frames per second, display in HUD or window title
   - `glfwSetWindowTitle(window, ("Flight Engine — " + std::to_string(fps) + " FPS").c_str())`

3. **Instanced rendering for obstacles**:
   - Instead of setting a new model matrix and drawing for each cube, upload ALL model matrices to a buffer and draw them all in one call
   - `glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, obstacleCount)`

4. **Game state management**:
   - Create a simple state machine: `PLAYING`, `WIN_SCREEN`, `LOSE_SCREEN`, `PAUSED`
   - Render different things based on state
   - Port your win/lose screen logic

5. **Window resizing**: handle `glfwSetFramebufferSizeCallback()` to update viewport and projection matrix aspect ratio.

6. **Gamma correction**: `glEnable(GL_FRAMEBUFFER_SRGB)` or do it manually in the fragment shader. Makes colors look correct on modern monitors.

7. **Anti-aliasing**: Request MSAA samples when creating the GLFW window: `glfwWindowHint(GLFW_SAMPLES, 4)` then `glEnable(GL_MULTISAMPLE)`.

### Checkpoint
The game runs at 60+ FPS even with many obstacles, particles, and terrain. No visual popping, smooth frame pacing.

---

## Phase 16: Advanced Features (Optional)

These are stretch goals. Each one is a project in itself.

### Shadow Mapping
- Render the scene from the sun's point of view into a depth-only framebuffer (shadow map)
- In the main render pass, compare each fragment's depth (from the sun's perspective) to the shadow map
- If the shadow map depth is less than the fragment's depth → the fragment is in shadow
- Makes the scene dramatically more realistic

### Post-Processing (Framebuffer Effects)
- Render the entire scene to an off-screen framebuffer (FBO) instead of the screen
- Draw a fullscreen quad with the FBO as a texture
- Apply effects in the fragment shader:
  - **Bloom**: bright areas glow
  - **HDR tone mapping**: wider range of brightness
  - **Vignette**: darken edges
  - **Motion blur**: blur in the direction of movement
  - **Color grading**: adjustable look/mood

### 3D Model Loading
- Use Assimp to load `.obj`, `.gltf`, or `.fbx` files
- Replace procedural cubes with real aircraft and obstacle models
- Free models: sketchfab.com, kenney.nl, opengameart.org

### Terrain Improvements
- **Multi-texture splatting**: blend grass, rock, sand based on height and slope
- **LOD (Level of Detail)**: fewer triangles far away, more close up
- **Heightmap from image**: load a grayscale PNG where white = high, black = low

### Physics Engine Integration
- **Bullet Physics** (`sudo pacman -S bullet`): rigid body dynamics, realistic collisions
- Adds weight, inertia, bounce, and forces to all objects
- Far more realistic flight model

### Deferred Rendering
- Advanced rendering technique that enables many lights efficiently
- Render geometry data (position, normal, albedo) to multiple textures (G-buffer)
- Calculate lighting in screen space — cost is per-pixel, not per-pixel-per-light
- Enables hundreds of lights (explosions, bullet impacts, etc.)

---

## Appendix A: Project Structure

```
flight-engine/
├── CMakeLists.txt
├── assets/
│   ├── textures/         # .png/.jpg images
│   ├── models/           # .obj/.gltf 3D models
│   ├── sounds/           # .wav/.ogg audio
│   └── fonts/            # .ttf fonts
├── shaders/
│   ├── basic.vert
│   ├── basic.frag
│   ├── terrain.vert
│   ├── terrain.frag
│   ├── skybox.vert
│   ├── skybox.frag
│   ├── particle.vert
│   ├── particle.frag
│   ├── hud.vert
│   └── hud.frag
├── src/
│   ├── main.cpp
│   ├── core/
│   │   ├── Engine.h / .cpp       # Main loop, init, shutdown
│   │   ├── Window.h / .cpp       # GLFW wrapper
│   │   ├── Input.h / .cpp        # Keyboard/mouse
│   │   └── Audio.h / .cpp        # Sound system
│   ├── renderer/
│   │   ├── Renderer.h / .cpp     # OpenGL state management
│   │   ├── Shader.h / .cpp       # Shader loading/uniforms
│   │   ├── Mesh.h / .cpp         # VAO/VBO/EBO management
│   │   ├── Texture.h / .cpp      # Image loading to GPU
│   │   └── Camera.h / .cpp       # View/projection matrices
│   ├── world/
│   │   ├── Terrain.h / .cpp      # Heightmap mesh generation
│   │   ├── Skybox.h / .cpp       # Sky rendering
│   │   └── Entity.h / .cpp       # Base game object
│   └── game/
│       ├── FlightController.h / .cpp  # Player flight physics
│       ├── Projectile.h / .cpp        # Bullet system
│       ├── Obstacle.h / .cpp          # Cube obstacle system
│       ├── CollisionSystem.h / .cpp   # Collision checks
│       ├── ParticleSystem.h / .cpp    # Visual effects
│       ├── HUD.h / .cpp               # Score/lives/timer overlay
│       └── GameState.h / .cpp         # State machine (play/win/lose)
└── vendor/
    ├── glad/                  # OpenGL loader
    ├── stb_image.h            # Image loading
    └── miniaudio.h            # Audio
```

---

## Appendix B: Key Math Concepts

### Matrices You'll Use Constantly

| Matrix | What it does | GLM function |
|---|---|---|
| Model | Positions/rotates/scales an object in the world | `glm::translate() * glm::rotate() * glm::scale()` |
| View | Moves the world relative to the camera | `glm::lookAt(eye, center, up)` |
| Projection | Applies perspective distortion | `glm::perspective(fov, aspect, near, far)` |
| MVP | All three combined — sent to vertex shader | `projection * view * model` |
| Normal matrix | Transforms normals correctly under non-uniform scale | `glm::transpose(glm::inverse(glm::mat3(model)))` |

### Vectors You'll Use Constantly

| Vector | What it represents |
|---|---|
| `camera.front` | Direction the camera faces (unit vector) |
| `camera.up` | Which way is "up" for the camera |
| `camera.right` | Perpendicular to front and up |
| `surface normal` | Direction a surface faces (for lighting) |
| `light direction` | Direction light travels (toward the surface) |

### Quaternions (Phase 5+)
- Represent rotations without gimbal lock
- `glm::quat q = glm::angleAxis(angle, axis)`
- `glm::mat4 rotMatrix = glm::mat4_cast(q)`
- Use for the flight controller once Euler angles cause problems

---

## Appendix C: Common Pitfalls

1. **Black screen, no errors**: You forgot `glEnable(GL_DEPTH_TEST)`, or your near/far planes are wrong (near=0 breaks the depth buffer — use 0.1 minimum), or your model is behind the camera.

2. **Shader won't compile**: Check the error log with `glGetShaderInfoLog()`. Usually a typo or version mismatch. Always use `#version 330 core` or higher.

3. **Objects render inside-out**: Enable face culling: `glEnable(GL_CULL_FACE)`. Make sure your triangles wind counter-clockwise when viewed from the front.

4. **Texture is all white or black**: You forgot to bind the texture before drawing, or the UV coordinates are wrong (all 0 = samples one pixel forever), or the texture didn't load (check `stbi_load` return value).

5. **Lighting looks wrong**: Your normals aren't normalized, or you forgot the normal matrix (objects scaled non-uniformly have broken normals without it).

6. **Movement speed changes with framerate**: You forgot to multiply by delta time. All movement should be `speed * dt`, not just `speed`.

7. **Terrain has gaps or cracks**: Adjacent chunks have different LOD and edges don't match. Fix by stitching edge vertices.

8. **Z-fighting (flickering surfaces)**: Two surfaces are nearly coplanar. Increase the near plane distance, or use `glPolygonOffset()`.

---

## Appendix D: Resources

### Tutorials
- **LearnOpenGL** (https://learnopengl.com) — the single best OpenGL tutorial. Covers everything from a triangle to PBR. Follow this alongside this roadmap.
- **The Cherno's OpenGL series** (YouTube) — video format, good explanations
- **ogldev.org** — advanced topics (shadow mapping, deferred rendering, skeletal animation)

### References
- **OpenGL Reference** (https://docs.gl) — quick function reference
- **GLM Documentation** (https://glm.g-truc.net) — math library docs
- **GLFW Documentation** (https://www.glfw.org/docs/latest/) — window/input reference
- **Khronos OpenGL Wiki** (https://www.khronos.org/opengl/wiki/) — official spec reference

### Books (Optional)
- *"OpenGL Programming Guide"* (Red Book) — comprehensive reference
- *"Real-Time Rendering"* by Akenine-Möller — theory bible for graphics programmers
- *"Game Engine Architecture"* by Jason Gregory — how AAA engines are structured
- *"Mathematics for 3D Game Programming"* by Eric Lengyel — the math behind everything

### Free Assets
- **Poly Haven** (https://polyhaven.com) — free HDRIs, textures, models (CC0)
- **Kenney** (https://kenney.nl) — free game assets (CC0)
- **Freesound** (https://freesound.org) — free sound effects
- **Google Fonts** (https://fonts.google.com) — free fonts for text rendering

---

## Progress Tracker

Use this to track which phases you've completed:

- [ ] Phase 1: Project Setup & First Triangle
- [ ] Phase 2: Shader System & Camera
- [ ] Phase 3: 3D Meshes & Transforms
- [ ] Phase 4: Terrain System
- [ ] Phase 5: Flight Controller & Input
- [ ] Phase 6: Game Objects & Entity System
- [ ] Phase 7: Textures & Materials
- [ ] Phase 8: Lighting
- [ ] Phase 9: Skybox & Atmosphere
- [ ] Phase 10: Collision System
- [ ] Phase 11: Projectile System
- [ ] Phase 12: HUD & Text Rendering
- [ ] Phase 13: Audio
- [ ] Phase 14: Particle Effects
- [ ] Phase 15: Performance & Polish
- [ ] Phase 16: Advanced Features (Optional)

---

*Built as an upgrade path from the FundComp wireframe flight shooter to a full GPU-powered engine.*
*Keep the old project — it's a great reference and a completed piece of work.*
