#include <unordered_map>
#include <string>
#include <iostream>
#include <vector>
#include <filesystem>
#include <glad/glad.h>
#include <thread>
#include <mutex>
#include <queue>
#include "../../utils/utils.h"

class Texture {
public:
    GLuint id = 0;
    int width = 0, height = 0, channels = 0;
    int refCount = 0;

    Texture() = default;
    Texture(GLuint texID, int w, int h, int ch)
        : id(texID), width(w), height(h), channels(ch), refCount(1) {}

    void addRef() { ++refCount; }
    void release() { if (--refCount == 0) { glDeleteTextures(1, &id); id = 0; } }
};

class TextureManager {
public:
    static TextureManager& getInstance() {
        static TextureManager instance;
        return instance;
    }

    GLuint loadTexture(const std::string& path) {
        auto it = textures.find(path);
        if (it != textures.end()) {
            it->second->addRef();
            return it->second->id;
        }
        return loadTextureSync(path);
    }

    void releaseTexture(const std::string& path) {
        auto it = textures.find(path);
        if (it != textures.end()) {
            it->second->release();
            if (it->second->refCount == 0) {
                delete it->second;
                textures.erase(it);
            }
        }
    }

    void cleanup() {
        for (auto& pair : textures) {
            delete pair.second;
        }
        textures.clear();
    }

private:
    std::unordered_map<std::string, Texture*> textures;

    TextureManager() {} // no thread!
    ~TextureManager() { cleanup(); }

    GLuint loadTextureSync(const std::string& path) {
        std::vector<unsigned char> imageData;
        int width, height, channels;

        if (!FileManager::LoadPNG(path, imageData, width, height, channels)) {
            std::cerr << "[TextureManager] Failed to load " << path << std::endl;
            return 0;
        }

        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);

        GLenum format = (channels == 4) ? GL_RGBA : (channels == 3) ? GL_RGB : GL_RED;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Texture* tex = new Texture(texID, width, height, channels);
        textures.emplace(path, tex);
        return texID;
    }
};