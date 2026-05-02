#!/bin/bash
set -e
# =============================
# ?? Colors
# =============================
GREEN="\033[1;32m"
CYAN="\033[1;36m"
RED="\033[1;31m"
YELLOW="\033[1;33m"
NC="\033[0m"
# =============================
# ?? Defaults
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
# ?? Usage
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
# ?? Parse Arguments
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
        *) echo -e "${RED}Unknown option: -$OPTARG${NC}"; usage ;;
    esac
done
# =============================
# ??? Generate toolchain if missing
# =============================
generate_toolchain() {
    if [ ! -f "$TOOLCHAIN_FILE" ]; then
        echo -e "${YELLOW}??  toolchain-windows.cmake introuvable, génération automatique...${NC}"
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
        echo -e "${GREEN}? toolchain-windows.cmake généré${NC}"
    fi
}
# =============================
# ?? BUILD LINUX
# =============================
if [ "$DO_BUILD" = true ]; then
    echo -e "${CYAN}?? Build Linux ($BUILD_TYPE)...${NC}"
    if [ "$CLEAN_BUILD" = true ]; then
        echo -e "${YELLOW}?? Cleaning $BUILD_LINUX...${NC}"
        rm -rf "$BUILD_LINUX"
    fi
    mkdir -p "$BUILD_LINUX"
    cmake -B "$BUILD_LINUX" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
    cmake --build "$BUILD_LINUX" -j$(nproc)
    echo -e "${CYAN}?? Copying assets...${NC}"
    rm -rf "$BUILD_LINUX/assets"
    cp -r assets "$BUILD_LINUX/assets"
    echo -e "${GREEN}? Linux build OK${NC}"
    # =============================
    # ?? BUILD WINDOWS (optional)
    # =============================
    if [ "$DO_WINDOWS" = true ]; then
        generate_toolchain
        echo -e "${CYAN}?? Build Windows...${NC}"
        if [ "$CLEAN_BUILD" = true ]; then
            echo -e "${YELLOW}?? Cleaning $BUILD_WINDOWS...${NC}"
            rm -rf "$BUILD_WINDOWS"
        fi
        mkdir -p "$BUILD_WINDOWS"
        cmake -B "$BUILD_WINDOWS" \
            -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
            -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
        cmake --build "$BUILD_WINDOWS" -j$(nproc)
        echo -e "${CYAN}?? Copying assets...${NC}"
        rm -rf "$BUILD_WINDOWS/assets"
        cp -r assets "$BUILD_WINDOWS/assets"
        # =============================
        # ?? Copy MinGW DLLs
        # =============================
        echo -e "${CYAN}?? Copying MinGW DLLs...${NC}"
        MINGW_GCC_VER=$(x86_64-w64-mingw32-gcc -dumpversion | cut -d. -f1)
        MINGW_DLL_PATH="/usr/lib/gcc/x86_64-w64-mingw32/${MINGW_GCC_VER}-win32"
        if [ ! -d "$MINGW_DLL_PATH" ]; then
            MINGW_DLL_PATH=$(dirname $(x86_64-w64-mingw32-gcc --print-libgcc-file-name))
        fi
        MINGW_SYSROOT="/usr/x86_64-w64-mingw32/lib"
        DLLS=(
            "$MINGW_DLL_PATH/libstdc++-6.dll"
            "$MINGW_DLL_PATH/libgcc_s_seh-1.dll"
            "$MINGW_SYSROOT/libwinpthread-1.dll"
        )
        for dll in "${DLLS[@]}"; do
            if [ -f "$dll" ]; then
                cp "$dll" "$BUILD_WINDOWS/"
                echo -e "${GREEN}  ? $(basename $dll)${NC}"
            else
                echo -e "${YELLOW}  ??  Introuvable : $dll${NC}"
            fi
        done
        echo -e "${GREEN}? Windows build OK${NC}"
    fi
fi
# =============================
# ?? Git Push
# =============================
if [ "$DO_PUSH" = true ]; then
    echo -e "${CYAN}?? Committing and pushing...${NC}"
    git add .
    if git diff --cached --quiet; then
        echo -e "${YELLOW}??  Nothing to commit, skipping commit${NC}"
    else
        git commit -m "$COMMIT_MSG"
    fi
    if [ "$PUSH_UPSTREAM" = true ]; then
        git push --set-upstream origin main
    else
        git push
    fi
    echo -e "${GREEN}?? Push done${NC}"
fi
echo -e "${GREEN}? All done!${NC}"