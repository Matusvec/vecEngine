cmake_minimum_required(VERSION 3.20)
project(FlightEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find system packages
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)

# Build the executable — includes glad.c since it's C, not C++
add_executable(flight main.cpp vendor/src/glad.c)

# Tell the compiler where to find headers
target_include_directories(flight PRIVATE
    ${CMAKE_SOURCE_DIR}/vendor/include
)

# Link against the libraries
target_link_libraries(flight glfw OpenGL::GL glm::glm)
