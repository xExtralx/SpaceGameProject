cmake_minimum_required(VERSION 3.20)
project(MyGame LANGUAGES C CXX)

# === Configuration globale ===
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# === Dépendances externes ===

## --- GLAD ---
add_library(glad STATIC external/glad/src/glad.c)
target_include_directories(glad PUBLIC external/glad/include)

## --- GLFW (via FetchContent) ---
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        glfw
        GIT_REPOSITORY https://github.com/glfw/glfw.git
        GIT_TAG 3.3.9  # ou 3.4 si tu veux la dernière version stable
)
FetchContent_MakeAvailable(glfw)

## --- FreeType ---
find_package(Freetype REQUIRED)

## --- OpenGL ---
find_package(OpenGL REQUIRED)

# === Fichiers source du projet ===
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS src/*.cpp)

add_executable(MyGame ${SRC_FILES})

# === Dossiers d'inclusion ===
target_include_directories(MyGame
        PRIVATE
        src
        include
        external/glad/include
        ${FREETYPE_INCLUDE_DIRS}
)

# === Liens des bibliothèques ===
target_link_libraries(MyGame
        PRIVATE
        glfw
        glad
        ${FREETYPE_LIBRARIES}
        OpenGL::GL
        ${CMAKE_DL_LIBS}
)

# === Liens des assets ===

# === Copie automatique du dossier assets dans le dossier de build ===
add_custom_command(
        TARGET MyGame POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:MyGame>/assets
)

# === Compilation Debug ===
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MyGame PRIVATE DEBUG)
endif()

message(STATUS "✅ Configuration OK pour ${CMAKE_PROJECT_NAME}")
