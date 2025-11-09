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
    Vec3 pos;
    Vec4 color;
};

class Renderer {
public:

    Renderer(int w,int h);
    ~Renderer();

    int init();
    void update();
    void uploadGeometry();
    void draw() const;
    void clear();
    void present() const;

    // Draw Methods

    void addTriangle(Vec2 v1, Vec2 v2, Vec2 v3, Vec4 color, float depth);
    void addQuad(Vec2 pos,Vec2 size, Vec4 color, float depth);

    bool shouldClose() const;

    int getWidth() { return this->width; }

    int getHeight() { return this->height; }

    GLFWwindow* getWindow() { return this->window; }

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