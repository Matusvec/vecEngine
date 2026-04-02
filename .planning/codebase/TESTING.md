# Testing Patterns

**Analysis Date:** 2026-04-02

## Test Framework

**Runner:**
- Not configured
- CMake build system in place (`CMakeLists.txt`) but no test target defined
- No gtest, Catch2, or similar C++ testing framework integrated

**Assertion Library:**
- None configured

**Run Commands:**
```bash
make all                # Build project
make run                # Build and run
make clean              # Clean build artifacts
cmake --build build     # Direct cmake build
```

**Current State:**
- Testing infrastructure not yet implemented
- Manual testing only (visual inspection of 3D engine output)

## Test File Organization

**Location:**
- No test files exist (`*.test.cpp`, `*.spec.cpp`, etc.)
- No `tests/` or `test/` directory

**Naming:**
- Not applicable (no tests configured)

**Structure:**
- Would follow convention of separate test directory or co-located with source if implemented

## Test Structure

**No tests currently exist.**

**Would follow pattern:**
```cpp
#include <gtest/gtest.h>
#include <gmock/gmock.h>

TEST(ShaderTests, CompileVertexShader) {
    unsigned int shader = compileShader(GL_VERTEX_SHADER, validSource);
    EXPECT_NE(shader, 0);
}
```

## Mocking

**Framework:** Not configured

**Current Testing Approach:**
- Manual visual testing of rendered output
- OpenGL calls validated indirectly through rendering success
- Input handling tested by interacting with rendered window (arrow keys, ESC)

**Testable Components (if unit tests added):**
- Shader compilation: `compileShader()` at `main.cpp:116`
- Shader program creation: `createShaderProgram()` at `main.cpp:131`
- Input processing: `processInput()` at `main.cpp:101`
- Matrix calculations: GLM operations could be tested (lines 213-232)

## Fixtures and Factories

**Test Data:**
- Vertex data hardcoded in `main.cpp:36-72`
- Indices data hardcoded in `main.cpp:74-87`
- Shader source strings hardcoded in `main.cpp:8-32`

**Would be extracted to fixtures if testing added:**
```cpp
class ShaderTestFixture : public ::testing::Test {
protected:
    const char* validVertexShader = R"(...)";
    float testVertices[] = { ... };
};
```

## Coverage

**Requirements:** None enforced

**Current State:**
- No coverage measurement tools configured
- Manual testing only
- Core engine loop tested through interactive use

**Components Testable:**
- Shader compilation and linking: both success and error paths
- Input handling: keyboard event processing
- Matrix transformations: GLM operations
- Buffer setup: VAO/VBO/EBO initialization

**Components Difficult to Unit Test (currently):**
- Window creation: GLFW dependency, would need fixture or mock
- OpenGL state: requires valid GL context
- Frame rendering: would need OpenGL context and validation

## Test Types

**Unit Tests:**
- Not implemented
- Would test: shader compilation, input processing, math operations
- Scope: Individual functions with dependencies mocked/stubbed

**Integration Tests:**
- Not implemented
- Would test: shader compilation → program creation → attribute setup
- Scope: Component interactions

**E2E Tests:**
- Not implemented
- Would test: full render pipeline from initialization through frame render
- Framework: Would need GLFW context fixture and visual output validation

**Current Testing:**
- Manual interactive testing: run `make run`, verify cube renders and responds to input
- Visual verification: colored cube rotates, arrow keys move it, ESC closes window

## Common Patterns

**No automated tests exist yet.**

**Recommended patterns if tests added:**

**Shader Compilation Test:**
```cpp
TEST(ShaderCompilation, CompileValidVertex) {
    unsigned int shader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    EXPECT_NE(shader, 0);
    glDeleteShader(shader);
}

TEST(ShaderCompilation, FailOnInvalidSource) {
    const char* invalid = "invalid syntax here";
    unsigned int shader = compileShader(GL_VERTEX_SHADER, invalid);
    // Check error log instead of shader ID (shader=0 on failure)
}
```

**Input Processing Test:**
```cpp
TEST(InputHandling, UpArrowMovesUp) {
    GLFWwindow* window = createTestWindow();
    glm::vec3 initialPos = cubePos;

    glfwSetKey(window, GLFW_KEY_UP, GLFW_PRESS);
    processInput(window);
    deltaTime = 0.016f; // ~16ms frame

    EXPECT_GT(cubePos.y, initialPos.y);
}
```

## Build and Test Integration

**CMake Configuration:**
- Current: `/home/mvecera/Projects/vecEngine/CMakeLists.txt` only builds executable
- Would need: Enable testing with `enable_testing()` and `add_test()`

**Example Test Target (if added):**
```cmake
add_executable(engine_tests tests/shader_test.cpp vendor/src/glad.c)
target_include_directories(engine_tests PRIVATE ...)
target_link_libraries(engine_tests gtest gtest_main glfw OpenGL::GL glm::glm)

enable_testing()
add_test(NAME ShaderTests COMMAND engine_tests)
```

---

*Testing analysis: 2026-04-02*
