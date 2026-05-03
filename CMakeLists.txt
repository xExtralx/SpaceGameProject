cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES C CXX)

# =========================
# Global config
# =========================
set(CMAKE_CXX_STANDARD          20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# =========================
# GLAD (local)
# =========================
add_library(glad STATIC include/glad/src/glad.c)
target_include_directories(glad PUBLIC include/glad/include)

# =========================
# GLFW
# =========================
foreach(opt GLFW_BUILD_EXAMPLES GLFW_BUILD_TESTS GLFW_BUILD_DOCS GLFW_INSTALL)
    set(${opt} OFF CACHE BOOL "" FORCE)
endforeach()

FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.9
    GIT_SHALLOW    TRUE
)

# =========================
# TinyGLTF
# =========================
FetchContent_Declare(tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG        v2.8.21
    GIT_SHALLOW    TRUE
)

# =========================
# EnTT
# =========================
FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG        v3.13.0
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(entt)

# =========================
# FreeType
# =========================
foreach(opt FT_DISABLE_ZLIB FT_DISABLE_BZIP2 FT_DISABLE_PNG FT_DISABLE_HARFBUZZ FT_DISABLE_BROTLI)
    set(${opt} ON CACHE BOOL "" FORCE)
endforeach()

FetchContent_Declare(freetype
    GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
    GIT_TAG        VER-2-13-2
    GIT_SHALLOW    TRUE
)

# Fetch all at once (parallel when possible)
FetchContent_MakeAvailable(glfw tinygltf entt freetype)

# =========================
# FastNoise2 (submodule)
# =========================
add_subdirectory(external/FastNoise2)

# =========================
# OpenGL
# =========================
find_package(OpenGL REQUIRED)

# =========================
# Sources & target
# =========================
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)
add_executable(MyGame ${SRC_FILES})

# =========================
# Includes
# =========================
target_include_directories(MyGame PRIVATE
    src
    include
    include/glad/include
    external/FastNoise2/include
    ${tinygltf_SOURCE_DIR}
    ${entt_SOURCE_DIR}/single_include
)

# =========================
# Link
# =========================
target_link_libraries(MyGame PRIVATE
    glad
    glfw
    FastNoise
    OpenGL::GL
    freetype
)

message(STATUS "[OK] ${PROJECT_NAME} configured")