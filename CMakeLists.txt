cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES C CXX)

# =========================
# Configuration globale
# =========================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# =========================
# Dépendances
# =========================

# GLAD
add_library(glad STATIC include/glad/src/glad.c)
target_include_directories(glad PUBLIC include/glad/include)

# GLFW
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.3.9
)
FetchContent_MakeAvailable(glfw)

# FreeType
find_package(Freetype REQUIRED)

# =========================
# Sources
# =========================
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

# 🔴 CIBLE CRÉÉE ICI
add_executable(MyGame ${SRC_FILES})

# =========================
# Includes
# =========================
target_include_directories(MyGame PRIVATE
    src
    include
    external/glad/include
    ${FREETYPE_INCLUDE_DIRS}
)

# =========================
# Link libraries (APRÈS add_executable)
# =========================

find_package(OpenGL REQUIRED)
        target_link_libraries(MyGame PRIVATE
        glfw
        glad
        ${FREETYPE_LIBRARIES}
        OpenGL::GL
)

# =========================
# Assets
# =========================
add_custom_command(
    TARGET MyGame POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:MyGame>/assets
)

message(STATUS "✅ Configuration OK pour ${PROJECT_NAME}")
