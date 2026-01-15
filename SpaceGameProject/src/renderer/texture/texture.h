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

    // Load texture synchronously or asynchronously
    GLuint loadTexture(const std::string& path, bool async = false) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = textures.find(path);
        if (it != textures.end()) {
            it->second->addRef();
            return it->second->id;
        }

        if (async) {
            // Launch background thread to load texture
            std::thread(&TextureManager::loadTextureAsync, this, path).detach();
            // Return a placeholder or invalid handle until loaded
            return 0;
        } else {
            return loadTextureSync(path);
        }
    }

    // Unload a texture manually
    void releaseTexture(const std::string& path) {
        std::lock_guard<std::mutex> lock(mutex_);
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
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : textures) {
            delete pair.second;
        }
        textures.clear();
    }

private:
    std::unordered_map<std::string, Texture*> textures;
    std::mutex mutex_;

    // Async load queue
    std::queue<std::string> loadQueue;
    std::thread loaderThread;
    bool stopThread = false;

    TextureManager() {
        loaderThread = std::thread(&TextureManager::loaderLoop, this);
    }
    ~TextureManager() {
        stopThread = true;
        if (loaderThread.joinable()) loaderThread.join();
        cleanup();
    }

    GLuint loadTextureSync(const std::string& path) {
        std::vector<unsigned char> imageData;
        int width, height, channels;
        FileManager fm;
        if (!fm.LoadPNG(path, imageData, width, height, channels)) {
            std::cerr << "[TextureManager] Failed to load " << path << std::endl;
            return 0;
        }

        GLuint texID;
        glGenTextures(1, &texID);
        glBindTexture(GL_TEXTURE_2D, texID);
        GLenum format = (channels == 4) ? GL_RGBA : (channels == 3) ? GL_RGB : GL_RED;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, imageData.data());
        glGenerateMipmap(GL_TEXTURE_2D);

        Texture* tex = new Texture(texID, width, height, channels);
        textures.emplace(path, tex);
        return texID;
    }

    void loadTextureAsync(const std::string& path) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            loadQueue.push(path);
        }
        // The loader thread will process the queue
    }

    void loaderLoop() {
        while (!stopThread) {
            std::string path;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                if (loadQueue.empty()) {
                    lock.unlock();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                path = loadQueue.front();
                loadQueue.pop();
            }
            loadTextureSync(path);
        }
    }
};