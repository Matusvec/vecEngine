cmake_minimum_required(VERSION 3.20)
project(FlightEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# glm is header-only and needed by every target. Try the cmake config first;
# fall back to looking for the header on the include path so this builds on
# environments (Nix, etc.) where glm doesn't ship a cmake config.
find_package(glm QUIET)
if(NOT glm_FOUND)
    find_path(GLM_INCLUDE_DIR glm/glm.hpp)
    if(GLM_INCLUDE_DIR)
        message(STATUS "Found glm headers at: ${GLM_INCLUDE_DIR}")
    else()
        message(FATAL_ERROR "glm not found. Install glm (Arch: glm, Nix: nix-shell -p glm).")
    endif()
endif()

# OpenGL + GLFW are only needed for the runnable game, NOT for the unit tests.
# Look for them quietly so `make test` still works on a machine without them.
find_package(glfw3 QUIET)
find_package(OpenGL QUIET)

# Build the main game executable only if we have OpenGL + GLFW.
if(glfw3_FOUND AND OpenGL_FOUND)
    set(BUILD_FLIGHT_GAME TRUE)
else()
    set(BUILD_FLIGHT_GAME FALSE)
    message(WARNING
        "glfw3 / OpenGL not found — skipping `flight` target.\n"
        "  `make test` will still work (tests are headless).\n"
        "  Install on Arch: sudo pacman -S glfw-x11\n"
        "  Install on Nix:  nix-shell -p glfw glm")
endif()

if(BUILD_FLIGHT_GAME)
# Build the executable — includes glad.c since it's C, not C++
add_executable(flight
    main.cpp
    src/balloon.cpp
    src/camera.cpp
    src/flight_controller.cpp
    src/frustum.cpp
    src/game.cpp
    src/grass.cpp
    src/holds.cpp
    src/input.cpp
    src/light.cpp
    src/mesh.cpp
    src/missile.cpp
    src/obj_loader.cpp
    src/overlay.cpp
    src/plane_model.cpp
    src/rain.cpp
    src/ring.cpp
    src/shader.cpp
    src/shadow_map.cpp
    src/shrubs.cpp
    src/skybox.cpp
    src/telemetry.cpp
    src/terrain.cpp
    src/texture.cpp
    src/transform.cpp
    src/water.cpp
    vendor/src/glad.c
)

# Tell the compiler where to find headers
target_include_directories(flight PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/include
    ${CMAKE_SOURCE_DIR}/include
)

# Link against the libraries
target_link_libraries(flight glfw OpenGL::GL)
if(glm_FOUND)
    target_link_libraries(flight glm::glm)
else()
    target_include_directories(flight PRIVATE ${GLM_INCLUDE_DIR})
endif()
endif()  # BUILD_FLIGHT_GAME

# -----------------------------------------------------------------------------
# BLACKBOX: the flight scene without the game (apps/blackbox_main.cpp)
add_executable(blackbox
    apps/blackbox_main.cpp
    src/camera.cpp
    src/flight_controller.cpp
    src/frustum.cpp
    src/grass.cpp
    src/holds.cpp
    src/input.cpp
    src/light.cpp
    src/mesh.cpp
    src/obj_loader.cpp
    src/overlay.cpp
    src/plane_model.cpp
    src/shader.cpp
    src/shadow_map.cpp
    src/skybox.cpp
    src/telemetry.cpp
    src/terrain.cpp
    src/texture.cpp
    src/transform.cpp
    vendor/src/glad.c
)
target_include_directories(blackbox PRIVATE ${CMAKE_SOURCE_DIR}/vendor/include ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(blackbox PRIVATE glfw OpenGL::GL)
if(TARGET glm::glm)
    target_link_libraries(blackbox PRIVATE glm::glm)
elseif(GLM_INCLUDE_DIR)
    target_include_directories(blackbox PRIVATE ${GLM_INCLUDE_DIR})
endif()

# Unit tests — pure-logic only (no GL context needed at runtime). Links glad.c
# so GL function-pointer symbols resolve, but never calls the loader so all the
# pointers stay null and harmless.
# -----------------------------------------------------------------------------
add_executable(flight_tests
    tests/test_main.cpp
    src/camera.cpp
    src/holds.cpp
    src/obj_loader.cpp
    src/frustum.cpp
    src/mesh.cpp
    src/shader.cpp
    src/terrain.cpp
    vendor/src/glad.c
)
target_include_directories(flight_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/include
    ${CMAKE_SOURCE_DIR}/include
)
if(glm_FOUND)
    target_link_libraries(flight_tests glm::glm)
else()
    target_include_directories(flight_tests PRIVATE ${GLM_INCLUDE_DIR})
endif()
