//
// Created by raphael on 02/11/2025.
//

#ifndef MYGAME_RENDERER_H
#define MYGAME_RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <vector>
#include "shader/shader.h"
#include "../utils.h"

struct Vertex {
    float z;
    Vec2 uv;
    Vec2 pos;
    Vec4 color;
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

    int init();
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
    void addQuad(Vec2 pos,Vec2 size, Vec4 color, float depth);
    void addLine(Vec2 start, Vec2 end, float thickness, Vec4 color, float depth);
    void addTile(Vec2 pos, Vec2 size,Vec4 color ,float depth);

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
    unsigned int VAO,VBO;

    std::vector<Vertex> vertices;
};


#endif //MYGAME_RENDERER_H