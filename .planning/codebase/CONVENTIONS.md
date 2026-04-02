# Coding Conventions

**Analysis Date:** 2026-04-02

## Naming Patterns

**Files:**
- Source files use lowercase with `.cpp` extension: `main.cpp`
- Header files use `.h` extension for C headers: `glad.h`
- No consistent subdirectory structure yet — all source in root

**Functions:**
- camelCase for public functions: `processInput()`, `compileShader()`, `createShaderProgram()`
- Descriptive verb-noun pattern: `framebufferSizeCallback()`, `glGetShaderiv()`
- Callback functions explicitly named with `Callback` suffix: `framebufferSizeCallback()`

**Variables:**
- camelCase for local variables: `deltaTime`, `moveSpeed`, `cubePos`, `currentFrame`
- snake_case for global state variables: Not observed in codebase
- Abbreviated names for standard GL objects: `VAO`, `VBO`, `EBO` (Vertex Array Object, Vertex Buffer Object, Element Buffer Object)
- Descriptive names for mathematical values: `vertexColor`, `projection`, `view`, `model`

**Types:**
- CamelCase for custom types (none in main.cpp)
- Using standard library types: `unsigned int`, `float`, `int`
- GLM math types used directly: `glm::vec3`, `glm::mat4`
- GLFW types: `GLFWwindow*`

## Code Style

**Formatting:**
- No automated formatter configured (no `.clang-format` or similar)
- Manual indentation observed: tabs used for indentation in vertex array setup
- Brace style: Opening braces on same line for function definitions
- Line length: No strict limit observed — pragmatic to content (see shader strings, vertex data)

**Linting:**
- No clang-tidy or similar configured
- Relies on manual code review
- No static analysis configured

## Import Organization

**Order:**
1. External libraries first: `<glad/glad.h>`, `<GLFW/glfw3.h>`
2. GLM math library: `<glm/glm.hpp>`, `<glm/gtc/matrix_transform.hpp>`
3. Standard library: `<iostream>`

**Path Style:**
- Angle brackets for external headers: `#include <library>`
- No local header includes in `main.cpp`
- Vendor dependencies in separate `vendor/` directory structure

## Error Handling

**Patterns:**
- Early exit checks with immediate return: Lines 160-164 (window creation failure)
- Logging to stderr on errors: `std::cerr << "Failed to create GLFW window" << std::endl;`
- Shader compilation errors logged with `glGetShaderInfoLog()`: Lines 123-127
- Error messages include context: "Shader compilation failed:" with info log
- No exception handling observed (C-style error checking)
- Silent failures not acceptable — all major operations checked

**Error Recovery:**
- Graceful shutdown on init failure: calls `glfwTerminate()` before returning
- Resource cleanup on error: see lines 241-244 (cleanup on exit)

## Logging

**Framework:** `std::cerr` for error messages, no dedicated logging library

**Patterns:**
- Use `std::cerr` for error conditions only: shader compilation failures, initialization failures
- Always include context in error messages: "Shader compilation failed:\n" + infoLog
- Use `std::endl` for manual newlines
- No debug logging infrastructure

## Comments

**When to Comment:**
- Structural comments for complex data: "Each face has 4 vertices..." (line 34)
- Grouped comments for logical blocks of code: "Front face (orange)" for vertex groups
- Comments explain the "why", not the "what": "// camera pulled back" (line 217)
- Purpose comments for non-obvious parameters: "// moved by arrow keys" (line 89)

**JSDoc/TSDoc:**
- Not applicable (C++ project, no JSDoc standard observed)
- No function documentation headers

## Function Design

**Size:**
- Small focused functions: `processInput()` is 13 lines, `compileShader()` is 13 lines
- Largest function is `main()` at ~95 lines — contains initialization, main loop, and cleanup
- No functions exceed 150 lines

**Parameters:**
- Explicit pointer parameters for GL callbacks: `GLFWwindow* window`, `GLADloadproc` function pointers
- Pass by value for primitives: `float source`, `unsigned int type`
- No default parameters observed

**Return Values:**
- Return handles/IDs for GL resources: `compileShader()` returns `unsigned int`
- Return status implicitly via side effects: `glfwCreateWindow()` returns pointer or null
- Main returns exit codes: 0 for success, -1 for initialization failure

## Module Design

**Exports:**
- Single entry point: `main()` function
- No header files or modules in codebase yet
- Global state variables at top level: `cubePos`, `moveSpeed`, `deltaTime`, `lastFrame`

**Global State:**
- Minimal global state: only animation/input-related: `cubePos`, `moveSpeed`, `deltaTime`, `lastFrame`
- Globals used because OpenGL callbacks require global context
- No global manager or singleton patterns yet

---

*Convention analysis: 2026-04-02*
