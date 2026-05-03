#!/bin/bash
set -e
# =============================
# Colors
# =============================
GREEN="\033[1;32m"
CYAN="\033[1;36m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
NC="\033[0m"
# =============================
# Defaults
# =============================
PROJECT_NAME="SpaceExplorationGame"
BUILD_TYPE="Release"
COMMIT_MSG="Auto build $(date '+%Y-%m-%d %H:%M:%S')"
DO_BUILD=true
DO_PUSH=true
DO_WINDOWS=false
PUSH_UPSTREAM=false
CLEAN_BUILD=false
TOOLCHAIN_FILE="$(pwd)/toolchain-windows.cmake"
# =============================
# Usage
# =============================
usage() {
    echo -e "${CYAN}Usage: $0 [options]${NC}"
    echo ""
    echo "Options:"
    echo "  -m <message>     Commit message (default: auto timestamp)"
    echo "  -t <type>        Build type: Release|Debug|RelWithDebInfo (default: Release)"
    echo "  -b               Build only, skip git push"
    echo "  -p               Push only, skip build"
    echo "  -w               Build for Windows (default: Linux)"
    echo "  -c               Clean build directory before building"
    echo "  -u               Set upstream on push (first push)"
    echo "  -h               Show this help"
    echo ""
    echo "Examples:"
    echo "  $0               Linux build + push"
    echo "  $0 -w            Windows build + push"
    echo "  $0 -w -b         Windows build only"
    echo "  $0 -c -w         Clean Windows build + push"
    exit 0
}
# =============================
# Parse Arguments
# =============================
while getopts "m:t:bpwcuh" opt; do
    case $opt in
        m) COMMIT_MSG="$OPTARG" ;;
        t) BUILD_TYPE="$OPTARG" ;;
        b) DO_PUSH=false ;;
        p) DO_BUILD=false ;;
        w) DO_WINDOWS=true ;;
        c) CLEAN_BUILD=true ;;
        u) PUSH_UPSTREAM=true ;;
        h) usage ;;
        *) echo -e "${RED}[ERROR] Unknown option: -$OPTARG${NC}"; usage ;;
    esac
done
# =============================
# Setup target platform
# =============================
if [ "$DO_WINDOWS" = true ]; then
    BUILD_DIR="build-windows"
    EXE_NAME="MyGame.exe"
else
    BUILD_DIR="build-linux"
    EXE_NAME="MyGame"
fi
# =============================
# Generate .gitignore
# =============================
generate_gitignore() {
    cat > .gitignore << 'EOF'
# Build artifacts
build-linux/*
build-windows/*

# Keep exe, dlls and assets in build dirs
!build-linux/MyGame
!build-linux/assets/
!build-linux/assets/**
!build-windows/MyGame.exe
!build-windows/*.dll
!build-windows/assets/
!build-windows/assets/**

# CMake
CMakeCache.txt
CMakeFiles/
cmake_install.cmake
compile_commands.json
Makefile

# Deps cache (heavy, no need to push)
_deps/

# Compiled objects
*.o
*.a
*.obj
*.lib

# IDE
.vscode/
.idea/
*.user

# OS
.DS_Store
Thumbs.db
EOF
    echo -e "${GREEN}[GIT] .gitignore generated${NC}"
}
# =============================
# Generate toolchain (Windows only)
# =============================
generate_toolchain() {
    if [ ! -f "$TOOLCHAIN_FILE" ]; then
        echo -e "${YELLOW}[TOOLCHAIN] Generating toolchain-windows.cmake...${NC}"
        cat > "$TOOLCHAIN_FILE" << 'EOF'
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)

set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
EOF
        echo -e "${GREEN}[TOOLCHAIN] Generated OK${NC}"
    else
        echo -e "${GREEN}[TOOLCHAIN] Found${NC}"
    fi
}
# =============================
# BUILD
# =============================
if [ "$DO_BUILD" = true ]; then
    if [ "$DO_WINDOWS" = true ]; then
        echo -e "${CYAN}[BUILD] Target: Windows ($BUILD_TYPE)${NC}"
        generate_toolchain
        CMAKE_EXTRA="-DCMAKE_TOOLCHAIN_FILE=$TOOLCHAIN_FILE"
    else
        echo -e "${CYAN}[BUILD] Target: Linux ($BUILD_TYPE)${NC}"
        CMAKE_EXTRA=""
    fi

    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${YELLOW}[BUILD] Cleaning $BUILD_DIR...${NC}"
        rm -rf "$BUILD_DIR"
    fi

    mkdir -p "$BUILD_DIR"

    echo -e "${CYAN}[BUILD] CMake configure...${NC}"
    cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" $CMAKE_EXTRA

    echo -e "${CYAN}[BUILD] CMake build...${NC}"
    cmake --build "$BUILD_DIR" -j$(nproc)

    # Copy MinGW DLLs (Windows only)
    if [ "$DO_WINDOWS" = true ]; then
        echo -e "${CYAN}[BUILD] Copying MinGW DLLs...${NC}"
        DLLS=(
            "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/libstdc++-6.dll"
            "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/libgcc_s_seh-1.dll"
            "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
        )
        for dll in "${DLLS[@]}"; do
            if [ -f "$dll" ]; then
                cp "$dll" "$BUILD_DIR/"
                echo -e "${GREEN}[BUILD] DLL copied: $(basename $dll)${NC}"
            else
                echo -e "${RED}[BUILD] DLL not found: $dll${NC}"
            fi
        done
    fi

    # Copy assets
    echo -e "${CYAN}[BUILD] Copying assets...${NC}"
    rm -rf "$BUILD_DIR/assets"
    cp -r assets "$BUILD_DIR/assets"

    echo -e "${GREEN}[BUILD] Done — $BUILD_DIR/$EXE_NAME ready${NC}"
fi

# =============================
# Git Push
# =============================
if [ "$DO_PUSH" = true ]; then
    echo -e "${CYAN}[GIT] Updating .gitignore...${NC}"
    generate_gitignore

    echo -e "${CYAN}[GIT] Staging...${NC}"
    git add .

    if git diff --cached --quiet; then
        echo -e "${YELLOW}[GIT] Nothing to commit${NC}"
    else
        echo -e "${CYAN}[GIT] Committing: $COMMIT_MSG${NC}"
        git commit -m "$COMMIT_MSG"
    fi

    if [ "$PUSH_UPSTREAM" = true ]; then
        git push --set-upstream origin main
    else
        git push
    fi
    echo -e "${GREEN}[GIT] Push done${NC}"
fi

echo -e "${GREEN}[DONE] All done!${NC}"