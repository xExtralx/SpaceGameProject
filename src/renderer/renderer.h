//
// Created by raphael on 02/11/2025.
//

#ifndef MYGAME_RENDERER_H
#define MYGAME_RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include "shader/shader.h"
#include "../utils/utils.h"
#include "texture/texture.h"

struct Vertex {
    Vec2 localPos;   // coin du quad : (-0.5,-0.5) → (0.5,0.5)
    Vec2 uv;         // UV texture
    Vec3 tilePos;    // (tileX, tileY, height)
};


enum class VertexAttribType {
    Float
};

struct VertexAttrib {
    GLuint index;       // layout(location = X)
    GLint  count;       // nombre de composantes (1,2,3,4)
    VertexAttribType type;
    size_t offset;      // offsetof(...)
};

struct VertexLayout {
    GLsizei stride;
    std::vector<VertexAttrib> attributes;
};


class Renderer {
public:

    Renderer(int w,int h);
    ~Renderer();

    // Pixel perfect
    static const int RENDER_WIDTH  = 320;
    static const int RENDER_HEIGHT = 180;
    GLuint pixelFBO     = 0;
    GLuint pixelTexture = 0;
    Shader* upscaleShader = nullptr;
    GLuint screenVAO = 0, screenVBO = 0;

    int init();
    void initPixelFBO();
    void update();
    void uploadGeometry(
        const void* vertexData,
        size_t vertexCount,
        const VertexLayout& layout);
    void draw();
    void clear();
    void present() const;

    // Draw Methods

    void addTriangle(
        const Vec2& v1,
        const Vec2& v2,
        const Vec2& v3,
        const Vec4& color,
        float z);
    void drawImage(const std::string& filePath, Shader& shader);

    bool shouldClose() const;

    int getWidth() { return this->width; }

    int getHeight() { return this->height; }

    GLFWwindow* getWindow() { return this->window; }
    Shader* getShader() { return this->shader; }

    static void key_callback(GLFWwindow* window,int key,int scancode,int action,int mods);
    static void mouse_button_callback(GLFWwindow* window,int button,int action,int mods);

    bool fullscreen = false;

private:

    int width = 0;
    int height = 0;

    GLFWwindow* window;

    FileManager fileManager;

    GLFWmonitor* monitor;
    const GLFWvidmode* mode;

    Shader* shader;
    Shader* imageShader;
    unsigned int VAO,VBO;

    std::vector<Vertex> vertices;
};


#endif //MYGAME_RENDERER_H