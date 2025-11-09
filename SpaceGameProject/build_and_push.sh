!/bin/bash

# =============================
# 🚀 Script d'automatisation :
# Compile ton projet C++ (via CMake)
# et pousse les changements sur GitHub
# =============================

# ⚙️ 1. Paramètres du build
BUILD_DIR="build"
EXEC_NAME="MyGame"

# ⚙️ 2. Couleurs (pour affichage joli)
GREEN="\033[1;32m"
RED="\033[1;31m"
CYAN="\033[1;36m"
NC="\033[0m" # No Color

# =============================
echo -e "${CYAN}🧹 Nettoyage et préparation du dossier de build...${NC}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit 1

# ⚙️ 3. Génération et compilation
echo -e "${CYAN}⚙️ Compilation en cours...${NC}"
cmake .. && make -j$(nproc)

# Vérifie si la compilation a réussi
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Erreur de compilation.${NC}"
    exit 1
fi

# =============================
# ✅ Compilation réussie
echo -e "${GREEN}✅ Compilation réussie : ${EXEC_NAME}${NC}"

cd ..

# ⚙️ 4. Ajout du binaire dans Git (forcer même s’il est ignoré)
if [ -f "${BUILD_DIR}/${EXEC_NAME}" ]; then
    echo -e "${CYAN}📦 Ajout de l'exécutable dans Git...${NC}"
    git add -f "${BUILD_DIR}/${EXEC_NAME}"
else
    echo -e "${RED}❌ L'exécutable ${EXEC_NAME} est introuvable.${NC}"
    exit 1
fi

# ⚙️ 5. Commit + push
COMMIT_MSG="${1:-Auto build $(date '+%Y-%m-%d %H:%M:%S')}"
echo -e "${CYAN}💬 Commit message :${RED} ${COMMIT_MSG}"

git add .
git commit -m "$COMMIT_MSG"

echo -e "${CYAN}📤 Push vers GitHub...${NC}"
git push

echo -e "${GREEN}✅ Build + Push terminés avec succès !${NC}"
