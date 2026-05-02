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
BUILD_LINUX="build-linux"
BUILD_WINDOWS="build-windows"
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
    echo "  -w               Also build for Windows (cross-compile)"
    echo "  -c               Clean build directory before building"
    echo "  -u               Set upstream on push (first push)"
    echo "  -h               Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 -m 'Fix texture loading'"
    echo "  $0 -t Debug -b"
    echo "  $0 -c -m 'Clean rebuild'"
    echo "  $0 -p -u"
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
# Generate toolchain if missing
# =============================
generate_toolchain() {
    if [ ! -f "$TOOLCHAIN_FILE" ]; then
        echo -e "${YELLOW}[TOOLCHAIN] toolchain-windows.cmake not found, generating...${NC}"
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
        echo -e "${GREEN}[TOOLCHAIN] toolchain-windows.cmake generated OK${NC}"
    else
        echo -e "${GREEN}[TOOLCHAIN] toolchain-windows.cmake found${NC}"
    fi
}
# =============================
# BUILD LINUX
# =============================
if [ "$DO_BUILD" = true ]; then
    echo -e "${CYAN}[LINUX] Starting Linux build ($BUILD_TYPE)...${NC}"
    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${YELLOW}[LINUX] Cleaning $BUILD_LINUX...${NC}"
        rm -rf "$BUILD_LINUX"
    fi
    mkdir -p "$BUILD_LINUX"
    echo -e "${CYAN}[LINUX] Running CMake configure...${NC}"
    cmake -B "$BUILD_LINUX" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    echo -e "${CYAN}[LINUX] Running CMake build...${NC}"
    cmake --build "$BUILD_LINUX" -j$(nproc)
    echo -e "${CYAN}[LINUX] Copying assets...${NC}"
    rm -rf "$BUILD_LINUX/assets"
    cp -r assets "$BUILD_LINUX/assets"
    echo -e "${GREEN}[LINUX] Build OK${NC}"
    # =============================
    # BUILD WINDOWS (optional)
    # =============================
    if [ "$DO_WINDOWS" = true ]; then
        generate_toolchain
        echo -e "${CYAN}[WINDOWS] Starting Windows build ($BUILD_TYPE)...${NC}"
        if [ "$CLEAN_BUILD" = true ]; then
            echo -e "${YELLOW}[WINDOWS] Cleaning $BUILD_WINDOWS...${NC}"
            rm -rf "$BUILD_WINDOWS"
        fi
        mkdir -p "$BUILD_WINDOWS"
        echo -e "${CYAN}[WINDOWS] Running CMake configure with MinGW toolchain...${NC}"
        cmake -B "$BUILD_WINDOWS" \
            -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
            -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
        echo -e "${CYAN}[WINDOWS] Running CMake build...${NC}"
        cmake --build "$BUILD_WINDOWS" -j$(nproc)
        echo -e "${CYAN}[WINDOWS] Copying assets...${NC}"
        rm -rf "$BUILD_WINDOWS/assets"
        cp -r assets "$BUILD_WINDOWS/assets"
        # =============================
        # Copy MinGW DLLs
        # =============================
        echo -e "${CYAN}[WINDOWS] Copying MinGW DLLs...${NC}"
        DLLS=(
            "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/libstdc++-6.dll"
            "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/libgcc_s_seh-1.dll"
            "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
        )
        for dll in "${DLLS[@]}"; do
            if [ -f "$dll" ]; then
                cp "$dll" "$BUILD_WINDOWS/"
                echo -e "${GREEN}[WINDOWS] DLL copied: $(basename $dll)${NC}"
            else
                echo -e "${RED}[WINDOWS] DLL not found: $dll${NC}"
            fi
        done
        echo -e "${GREEN}[WINDOWS] Build OK${NC}"
    fi
fi
# =============================
# Git Push
# =============================
if [ "$DO_PUSH" = true ]; then
    echo -e "${CYAN}[GIT] Staging all changes...${NC}"
    git add .
    if git diff --cached --quiet; then
        echo -e "${YELLOW}[GIT] Nothing to commit, skipping commit${NC}"
    else
        echo -e "${CYAN}[GIT] Committing: $COMMIT_MSG${NC}"
        git commit -m "$COMMIT_MSG"
    fi
    if [ "$PUSH_UPSTREAM" = true ]; then
        echo -e "${CYAN}[GIT] Pushing with --set-upstream origin main...${NC}"
        git push --set-upstream origin main
    else
        echo -e "${CYAN}[GIT] Pushing...${NC}"
        git push
    fi
    echo -e "${GREEN}[GIT] Push done${NC}"
fi
echo -e "${GREEN}[DONE] All done!${NC}"