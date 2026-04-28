#!/bin/bash

set -e

# =============================
# ⚙️ Paramètres
# =============================
PROJECT_NAME="SpaceExplorationGame"

BUILD_LINUX="build-linux"
BUILD_WINDOWS="build-windows"

COMMIT_MSG="${1:-Auto build $(date '+%Y-%m-%d %H:%M:%S')}"

GREEN="\033[1;32m"
CYAN="\033[1;36m"
RED="\033[1;31m"
NC="\033[0m"

# =============================
# 🐧 BUILD LINUX
# =============================
echo -e "${CYAN}🐧 Build Linux...${NC}"

mkdir -p $BUILD_LINUX
cd $BUILD_LINUX

cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

cd ..

echo -e "${GREEN}✅ Linux OK${NC}"

# =============================
# 📦 Git
# =============================
echo -e "${CYAN}📦 Ajout des binaires...${NC}"

git add .

git commit -m "$COMMIT_MSG"
git push

echo -e "${GREEN}🚀 Build + Push terminé${NC}"
