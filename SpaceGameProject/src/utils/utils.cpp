#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "utils.h"

// Load PNG file and get image data, width, height, channels
bool FileManager::LoadPNG(const std::string& relativePath, std::vector<unsigned char>& imageData, int& width, int& height, int& channels) {
    namespace fs = std::filesystem;
    fs::path fullPath = fs::path(basePath) / "assets" / relativePath;
    unsigned char* data = stbi_load(fullPath.string().c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "[FileManager] Failed to load PNG: " << fullPath << std::endl;
        return false;
    }
    size_t dataSize = width * height * channels;
    imageData.assign(data, data + dataSize);
    stbi_image_free(data);
    return true;
}