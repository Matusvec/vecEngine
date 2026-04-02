# Architecture

**Analysis Date:** 2026-04-02

## Pattern Overview

**Overall:** Monolithic procedural rendering engine

**Key Characteristics:**
- Single-threaded immediate-mode OpenGL rendering loop
- Shader-driven geometry rendering with matrix transformations
- Input-driven state updates within render loop
- Vertex Array Objects (VAO) for vertex data management
- Fixed transformation pipeline (Model-View-Projection)

## Layers

**Presentation (Graphics API):**
- Purpose: Render colored 3D geometry to screen
- Location: `main.cpp` (lines 175-235)
- Contains: OpenGL API calls, VAO/VBO/EBO management, shader program linking
- Depends on: GLFW (windowing), GLAD (OpenGL function loading), GLM (math)
- Used by: Render loop (main loop)

**Shader System:**
- Purpose: Compile and manage GPU programs for vertex and fragment processing
- Location: `main.cpp` (lines 8-32, 116-151)
- Contains: Inline GLSL source, shader compilation, program linking with error handling
- Depends on: OpenGL API
- Used by: Presentation layer for rendering

**Input Handler:**
- Purpose: Process keyboard input and update entity state
- Location: `main.cpp` (lines 101-114)
- Contains: Key polling via GLFW, position updates with frame timing
- Depends on: GLFW, deltaTime calculation
- Used by: Main loop before rendering

**Window Management:**
- Purpose: Initialize and manage GLFW window, OpenGL context
- Location: `main.cpp` (lines 153-173, 200-239)
- Contains: GLFW initialization, window creation, context setup, event polling
- Depends on: GLFW library
- Used by: Entry point (main function)

**Entity State:**
- Purpose: Store mutable scene state (cube position)
- Location: `main.cpp` (lines 89-95)
- Contains: Global variables: cubePos, moveSpeed, deltaTime, lastFrame
- Depends on: None (pure state)
- Used by: Input handler, transformation pipeline

**Geometry Definition:**
- Purpose: Store static vertex and index data
- Location: `main.cpp` (lines 34-87)
- Contains: Vertex position + color interleaved data (24 vertices × 6 floats), index array (36 indices for 6 cube faces)
- Depends on: None (data only)
- Used by: Presentation layer (VAO/VBO setup and rendering)

## Data Flow

**Application Startup:**

1. GLFW initialization with OpenGL 3.3 core profile
2. Window creation (2560×1600) with framebuffer size callback
3. GLAD loader initializes OpenGL function pointers
4. Depth test enabled for 3D rendering
5. Shader program compiled and linked from embedded GLSL
6. VAO created with VBO (vertex data) and EBO (index data)
7. Vertex attributes configured (position, color with stride 24 bytes)

**Per-Frame Render Loop:**

1. Calculate deltaTime from glfwGetTime() since last frame
2. Process keyboard input via glfwGetKey() polling
3. Update cubePos based on arrow keys × velocity × deltaTime
4. Clear color and depth buffers
5. Build transformation matrices:
   - Model: translation (cubePos) + rotation (time-based)
   - View: camera at (0, 0, 3) looking at origin
   - Projection: 45° perspective with aspect ratio correction
6. Upload matrices as uniform variables to shader
7. Bind VAO and render 36 indices (6 triangles × 2 per face)
8. Swap buffers and poll events

**State Management:**

- Global mutable state: `cubePos` (position), `cubePos.y/x` (updated via input)
- Timing state: `deltaTime` (frame interval), `lastFrame` (previous timestamp)
- All state changes occur in input handler before each render
- No persistence or deferred updates

## Key Abstractions

**Shader Program:**
- Purpose: Encapsulate compiled GLSL vertex and fragment shaders
- Examples: `main.cpp` (lines 116-151 compileShader, createShaderProgram functions)
- Pattern: Function-based factory (createShaderProgram()) returns unsigned int handle to GPU resource

**Vertex Data:**
- Purpose: Define shape, colors, and rendering order
- Examples: `vertices` array (line 36), `indices` array (line 74)
- Pattern: Interleaved vertex attributes (position + color) with separate index buffer

**Transformation Pipeline:**
- Purpose: Convert local cube coordinates → world → camera → clip space
- Examples: `main.cpp` (lines 213-232)
- Pattern: Matrix composition using GLM: Model × View × Projection

## Entry Points

**main():**
- Location: `main.cpp` (lines 153-247)
- Triggers: Program execution
- Responsibilities:
  - GLFW/OpenGL initialization
  - Shader and geometry setup
  - Main render loop execution
  - Resource cleanup on exit

**Render Loop (lines 200-239):**
- Location: Main loop in main()
- Triggers: Each frame while window open
- Responsibilities:
  - Input processing
  - Matrix calculations
  - Uniform updates
  - Draw call execution

## Error Handling

**Strategy:** Immediate console logging with error codes

**Patterns:**
- Shader compilation failures: logged to std::cerr with infoLog (line 126)
- Shader linking failures: logged to std::cerr with infoLog (line 145)
- GLFW window creation failure: logged to std::cerr with return -1 (line 161)
- GLAD initialization failure: logged to std::cerr with return -1 (line 170)
- No recovery mechanism—errors cause premature exit

## Cross-Cutting Concerns

**Logging:**
- Tool: std::cerr for errors
- Pattern: Only error conditions logged, no debug output

**Timing:**
- Approach: GLFW clock-based deltaTime calculation
- Pattern: Store currentFrame and lastFrame, compute difference per iteration

**Input Processing:**
- Approach: Synchronous GLFW key polling in processInput()
- Pattern: Check each relevant key per frame, apply velocity scaled by deltaTime

**Camera & Projection:**
- Approach: Fixed camera position, dynamic aspect ratio handling
- Pattern: Recalculate aspect ratio and projection matrix every frame to handle window resize

---

*Architecture analysis: 2026-04-02*
