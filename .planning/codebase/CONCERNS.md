# Codebase Concerns

**Analysis Date:** 2026-04-02

## Tech Debt

**Monolithic main function:**
- Issue: All rendering, input handling, initialization, and cleanup logic is tightly coupled in a single file (`main.cpp`). No separation of concerns or modular architecture.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp`
- Impact: Difficult to extend with new features (additional geometry, materials, lighting), test individual components, or reuse code. Adding a mesh loader, animation system, or post-processing requires modifying this single file.
- Fix approach: Refactor into separate modules: `Renderer` (GPU state management), `InputManager` (keyboard/mouse), `Transform` (model matrices), `ShaderProgram` (shader compilation/linking), `Mesh` (geometry storage). Use header files in `include/` to expose public interfaces.

**Hardcoded geometry:**
- Issue: Cube vertex and index data (lines 36-87 in `main.cpp`) are hardcoded as C arrays. Colors are baked into vertex data.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 36-87)
- Impact: Cannot load external models, cannot change colors at runtime, cannot easily swap geometries. Scaling the engine to support multiple objects or complex scenes is blocked.
- Fix approach: Create a `Mesh` class to encapsulate geometry data. Implement a mesh loader (OBJ parser or custom format) to load from files. Store colors separately or use a material system.

**Global state variables:**
- Issue: Cube position, movement speed, and timing are global variables (`cubePos`, `moveSpeed`, `deltaTime`, `lastFrame` at lines 90-95).
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 90-95)
- Impact: Impossible to have multiple objects or scenes. Cannot be tested independently. Makes multithreading or state management dangerous. Future camera system, UI, or scripting integration will be complicated.
- Fix approach: Encapsulate in a `Scene` or `GameObject` class. Use dependency injection to pass state to functions that need it. Store frame timing in a `Timer` utility.

**Shader compilation errors not fatal:**
- Issue: The `compileShader()` function logs compilation/linking errors to stderr but returns the shader regardless (lines 116-129, 131-151). If shader compilation fails, the program continues with an invalid shader.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 116-129, 131-151)
- Impact: Silent failures. If a shader doesn't compile, rendering will fail or produce garbage. Difficult to debug. No user-facing error indication.
- Fix approach: Make compilation/linking failures throw exceptions or return error codes. Exit early with a meaningful error message if shaders fail. Consider implementing a debug callback using `glDebugMessageCallback()` (available in GLAD).

**No error handling for window/OpenGL initialization:**
- Issue: Window creation and GLAD initialization check for errors (lines 160-164, 168-171) but subsequent GL setup doesn't. `glGenVertexArrays()`, `glBindVertexArray()`, `glBufferData()` can silently fail.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 160-171, 177-198)
- Impact: If GPU memory is exhausted or VAO/VBO creation fails, the application continues with invalid state and crashes later. No diagnostics.
- Fix approach: Use `glGetError()` to check for GL errors after critical operations. Implement a debug callback or error logging function. Consider a wrapper class that validates GL operations.

**Fixed window resolution:**
- Issue: Window size is hardcoded to 2560x1600 (line 159). Aspect ratio handling is done dynamically (line 227) but there's no DPI awareness or fullscreen option.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (line 159)
- Impact: Non-responsive to different monitor sizes. On high-DPI displays, the window may be unreadable or too large. No easy way to toggle fullscreen or resize the window from code.
- Fix approach: Read monitor size at startup and use a percentage (e.g., 80% of monitor) or make window resizable with `glfwWindowHint(GLFW_RESIZABLE, GL_TRUE)`.

**Magic numbers throughout code:**
- Issue: Vertex stride (6 floats), cube size (±0.5), camera distance (3.0f), rotation axis (0.5f, 1.0f, 0.0f), field of view (45.0f), near/far planes (0.1f, 100.0f) are hardcoded.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (multiple lines)
- Impact: Difficult to adjust appearance or camera behavior. No centralized configuration. Hard to understand intent of numeric constants.
- Fix approach: Define named constants at the top of the file or in a config struct (e.g., `struct EngineConfig { float cameraDist, float fov; }`).

**No cleanup on error paths:**
- Issue: If initialization fails after some GL objects are created (e.g., shader fails after VAO is created), the cleanup code (lines 241-244) is never reached.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 160-198, 241-244)
- Impact: GPU memory leaks on initialization failure. Not critical for a one-shot program, but bad practice that will cause issues in a persistent application.
- Fix approach: Use RAII pattern or ensure all resource creation is exception-safe. Wrap GL objects in classes with destructors.

## Known Bugs

**No frame rate limiting:**
- Symptoms: The render loop runs as fast as the GPU can push frames (potentially hundreds of FPS). No vsync or frame rate cap.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 200-239)
- Trigger: Run on high-end GPU or reduce scene complexity. Observe excessive power consumption and frame pacing issues.
- Workaround: Add `glfwSwapInterval(1)` after `glfwMakeContextCurrent()` to enable vsync.

**Division by zero risk in aspect ratio:**
- Symptoms: If window height becomes 0 (edge case), the fallback value of 1.0f prevents division by zero, but the window would be invisible anyway. The check is there but fragile.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (line 227)
- Trigger: Minimize the window or trigger a framebuffer size of 0x0.
- Workaround: The existing check mitigates this. However, rendering with height=0 should skip rendering entirely.

**Hard-coded cube rotation doesn't match object center:**
- Symptoms: Cube rotates around global origin (0,0,0), not its own center, even though it can translate via arrow keys.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 213-215)
- Trigger: Move the cube with arrow keys. Observe it rotates in an orbit pattern rather than spinning in place.
- Workaround: Not a critical bug for a demo, but unexpected behavior. Fix: Apply rotation before translation in the model matrix.

## Security Considerations

**No input validation:**
- Risk: Keyboard input is checked but no bounds on movement. Cube can translate infinitely far (no viewport clipping). No protection against untrusted input if the engine ever loads external data.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 101-114)
- Current mitigation: Single-user application, local input only. No network exposure.
- Recommendations: If loading meshes from files, validate file format and size. Implement viewport bounds checks. Add range limits to transformations.

**No buffer overflow protection:**
- Risk: Shader info logs are read into fixed-size 512-byte buffers (lines 124, 143). Long error messages could overflow.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 124, 143)
- Current mitigation: OpenGL limits message length, but not guaranteed. GLAD handles bounds safely.
- Recommendations: Use `glGetShaderInfoLog()` with a length parameter to prevent overflow. Consider using a vector or string class for dynamic sizing.

## Performance Bottlenecks

**No vertex buffer object updates:**
- Problem: Vertices are static (uploaded once with `GL_STATIC_DRAW`), which is correct. However, if the engine scales to thousands of objects, each with its own VBO, there will be a CPU→GPU bandwidth bottleneck. No instancing, no batching.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 184-189)
- Cause: Single object architecture. No mesh batching or GPU-side instancing.
- Improvement path: Implement GPU instancing using instanced vertex attributes or compute shaders. Add a render queue that batches similar materials together.

**Uniform lookup via string every frame:**
- Problem: `glGetUniformLocation()` is called every frame (lines 230-232) to find the uniform location by name string. This is O(1) but unnecessary overhead.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 230-232)
- Cause: Uniforms are looked up by string every iteration instead of being cached once during initialization.
- Improvement path: Cache uniform locations in a `ShaderProgram` class during initialization. Lookup the string-to-location mapping once, then use the cached ID.

**No frustum culling or visibility optimization:**
- Problem: The entire scene (one cube) is rendered every frame without any culling. For a complex scene, invisible objects would still be submitted to the GPU.
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 234-235)
- Cause: Monolithic design with no scene graph. No spatial partitioning.
- Improvement path: Implement a scene graph with visibility culling. Use bounding volume hierarchies (BVH) or spatial hashing for large scenes.

## Fragile Areas

**Shader source as embedded C strings:**
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 8-32)
- Why fragile: Shaders are C++ string literals. If you want to add more complex shaders (normal mapping, parallax mapping, etc.), the C++ file becomes unmaintainable. No syntax highlighting or shader debugging in C++ editor.
- Safe modification: Move shaders to separate `.glsl` files. Load at runtime or embed via a build system (e.g., `xxd` to generate hex dumps).
- Test coverage: No test coverage for shader compilation. If a shader has a subtle syntax error, it won't be caught until runtime.

**Input handling hardcoded to specific keys:**
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 101-114)
- Why fragile: Arrow keys are hard-coded. Changing controls requires modifying the main loop. No input mapping system. If you want to support multiple input schemes or rebinding, this breaks.
- Safe modification: Create an `InputMap` class that abstracts key bindings from actions. E.g., `inputMap.isActionPressed("move_up")` instead of checking `GLFW_KEY_UP` directly.
- Test coverage: Input handling cannot be tested independently. No mock input system.

**Renderer state not encapsulated:**
- Files: `/home/mvecera/Projects/vecEngine/main.cpp` (lines 175-198)
- Why fragile: VAO, VBO, EBO, and shader program IDs are raw OpenGL handles. If you add a second mesh, you need multiple sets of these, but there's no data structure to manage them.
- Safe modification: Create a `Mesh` class to encapsulate VAO/VBO/EBO. Create a `RenderPass` or `Scene` class to manage multiple meshes.
- Test coverage: No way to unit test rendering state without spinning up OpenGL context.

## Scaling Limits

**Single mesh architecture:**
- Current capacity: 1 cube (24 vertices, 36 indices). Theoretically can handle thousands of objects on modern GPUs, but the code structure is hardcoded for one object.
- Limit: Adding a second cube requires rewriting a significant portion of the code. No scene graph or entity system.
- Scaling path: Implement a scene graph. Store multiple meshes in a vector. Iterate and render each. Use a material/shader system to batch by shader. Eventually move to GPU-driven rendering with compute shaders.

**No threaded loading or LOD system:**
- Current capacity: Instant loading of hardcoded geometry.
- Limit: If you add mesh loading from disk, the main thread blocks. No level-of-detail (LOD) or streaming system for large scenes.
- Scaling path: Implement async mesh loading on a background thread. Implement LOD by precomputing simplified meshes. Use a thread pool or async/await pattern.

**Fixed camera system:**
- Current capacity: Static camera at (0, 0, 3), looking at origin.
- Limit: Cannot support multiple cameras, cinematic views, or interactive camera control. Orbit camera or first-person camera would require major refactoring.
- Scaling path: Implement a `Camera` class with position, rotation, and projection parameters. Support multiple camera types (perspective, orthographic, orbital).

## Dependencies at Risk

**GLM version not pinned:**
- Risk: `find_package(glm REQUIRED)` in CMakeLists.txt doesn't specify a version. If the system GLM is upgraded with breaking changes, the build may fail silently or produce incorrect math.
- Impact: Matrix math could change. Quaternion behavior could differ.
- Migration plan: Pin GLM to a specific version in CMakeLists.txt: `find_package(glm 0.9.9.5 REQUIRED)`. Alternatively, vendor GLM source into the project (similar to how GLAD is vendored).

**GLFW window creation fragile to system configuration:**
- Risk: `find_package(glfw3 REQUIRED)` assumes GLFW3 is installed system-wide. On some systems (particularly headless servers), GLFW3 may not be available or may be built without necessary backends.
- Impact: Build fails on unsupported systems. No fallback or detection.
- Migration plan: Vendor GLFW as a CMake subdirectory or use Conan/vcpkg for dependency management.

**GLAD generated code mismatch:**
- Risk: GLAD headers and the C source (`vendor/src/glad.c`) are generated for a specific OpenGL version. If regenerated without updating C++ includes, the code may call functions that aren't defined.
- Impact: Link errors or runtime crashes if GL function pointers are null.
- Migration plan: Document the OpenGL version (3.3 core) used to generate GLAD. Store GLAD generation parameters (API, profile, version) in a README. If updating, regenerate both header and source.

## Missing Critical Features

**No mesh loading:**
- Problem: Cannot import 3D models. Engine is limited to hardcoded geometry.
- Blocks: Cannot build interesting scenes, cannot use the engine for real projects, no content pipeline.

**No lighting system:**
- Problem: Shaders use flat per-vertex colors. No per-pixel lighting, shadows, reflections, or physically-based materials.
- Blocks: Cannot achieve realistic visuals. Limited to stylized/flat-shaded rendering.

**No animation system:**
- Problem: Cube rotation is hard-coded and continuous. No skeletal animation, no keyframe animation, no animation state machine.
- Blocks: Cannot animate characters, objects, or cameras programmatically.

**No UI/HUD rendering:**
- Problem: No 2D overlay system. Cannot display text, buttons, menus, or debug overlays.
- Blocks: Cannot build user-facing applications. No debug UI for visualization.

**No asset management:**
- Problem: All resources (shaders, meshes, textures) are embedded or hardcoded. No asset loading, caching, or unloading system.
- Blocks: Cannot build large projects with thousands of assets. Memory management will be chaotic.

## Test Coverage Gaps

**No unit tests:**
- What's not tested: Math utilities (transforms, matrices), shader compilation, mesh loading (when implemented), input handling.
- Files: None (no test directory exists)
- Risk: Bugs in core systems go unnoticed until they manifest visually at runtime. No regression detection.
- Priority: High - A graphics engine should have test coverage for matrix math at minimum.

**No integration tests:**
- What's not tested: Shader program linking, VAO/VBO creation and binding, rendering pipeline.
- Files: None (no test framework integrated)
- Risk: Shader compatibility issues, GL state bugs, and rendering pipeline errors only discovered through visual inspection.
- Priority: Medium - Visual verification is possible during development, but automated tests would catch regressions earlier.

**No performance benchmarks:**
- What's not tested: Frame rate, GPU memory usage, CPU/GPU synchronization, rendering time per object.
- Files: None (no profiling or benchmarking infrastructure)
- Risk: Performance regressions introduced silently. Scalability limits unknown until they're hit.
- Priority: Medium - Not critical for a small engine, but important as features scale up.

---

*Concerns audit: 2026-04-02*
