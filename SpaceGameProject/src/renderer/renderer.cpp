//
// Created by raphael on 02/11/2025.
//

#include <string>
#include "renderer.h"
#include "../game/game.h"

Renderer::Renderer(int w, int h) : width(w), height(h), window(nullptr), shader(nullptr), VAO(0), VBO(0) {}

Renderer::~Renderer() {
    glfwTerminate();
}

int Renderer::init() {
    if (!glfwInit()) {
        std::cout << "failed to Init GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width,height,"My Game",nullptr,nullptr);
    if (!window) {
        std::cout << "failed to Create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "failed to Load Glad" << std::endl;
        return false;
    }

    glViewport(0,0,width,height);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glfwSwapInterval(1);
    glfwSetWindowUserPointer(window,this);
    glfwSetMouseButtonCallback(window, Renderer::mouse_button_callback);
    glfwSetKeyCallback(window, Renderer::key_callback);

    monitor = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(monitor);

    shader = new Shader(
        FileManager::LoadTextFile("shader/default.vert"),
        FileManager::LoadTextFile("shader/default.frag")
    );

    return true;
}

void Renderer::clear() {
    glClearColor(0.05f,0.05f,0.08f,1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    vertices.clear();
}

void Renderer::draw() {
    shader->use();

    shader->setFloat("uTime", (float)glfwGetTime());
    shader->setFloat("uScale", 100.0f);
    shader->setVec2("uResolution", (float)width, (float)height);

    glBindVertexArray(VAO); 
    glDrawArrays(GL_TRIANGLES, 0,
        static_cast<GLsizei>(vertices.size() / 5));
    glBindVertexArray(0);
}


void Renderer::update() {
    const auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    const float worldX = xpos;
    const float worldY = game->getRenderer().getHeight() - ypos;

    game->moveEvent(worldX,worldY);
}

void Renderer::present() const {
    glfwSwapBuffers(window);
    glfwPollEvents();
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Renderer::uploadGeometry() {
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            vertices.size() * sizeof(Vertex),
            vertices.data(),
            GL_DYNAMIC_DRAW
        );

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, pos));
        glEnableVertexAttribArray(0);

        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex),
                          (void*)offsetof(Vertex, color));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER,VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(),GL_DYNAMIC_DRAW);
}

void Renderer::addTriangle(Vec2 v1, Vec2 v2, Vec2 v3, Vec4 color,float depth) {
    vertices.push_back({{v1[0],v1[1],depth},color});
    vertices.push_back({{v2[0],v2[1],depth},color});
    vertices.push_back({{v3[0],v3[1],depth},color});
}

void Renderer::addQuad(Vec2 pos,Vec2 size,Vec4 color,float depth) {
    addTriangle(
        pos,
        {pos[0] + size[0],pos[1]},
        {pos[0],pos[1] + size[1]},
        color,depth );

    addTriangle(
        {pos[0] + size[0],pos[1] + size[1]},
        {pos[0] + size[0],pos[1]},
        {pos[0],pos[1] + size[1]},
        color,depth );
}

void Renderer::addLine(Vec2 start, Vec2 end, float thickness, Vec4 color, float depth) {
    // Calcul du vecteur perpendiculaire normalisé
    Vec2 dir = { end[0] - start[0], end[1] - start[1] };
    float length = std::sqrt(dir[0]*dir[0] + dir[1]*dir[1]);
    if(length == 0.0f) return; // ligne nulle

    dir[0] /= length;
    dir[1] /= length;

    // vecteur perpendiculaire
    Vec2 perp = {-dir[1] * thickness * 0.5f, dir[0] * thickness * 0.5f};

    // 4 coins du rectangle
    Vec2 v0 = { start[0] + perp[0], start[1] + perp[1] };
    Vec2 v1 = { start[0] - perp[0], start[1] - perp[1] };
    Vec2 v2 = { end[0] + perp[0], end[1] + perp[1] };
    Vec2 v3 = { end[0] - perp[0], end[1] - perp[1] };

    // deux triangles
    addTriangle(v0, v1, v2, color, depth);
    addTriangle(v2, v1, v3, color, depth);
}


void Renderer::addTile(Vec2 pos,Vec2 size,Vec4 color,float depth) {
    Vec2 b = {pos[0],(pos[1] - (size[1] / 2))};
    Vec2 t = {pos[0],(pos[1] + (size[1] / 2))};
    Vec2 l = {(pos[0] - (size[0] / 2)),pos[1]};
    Vec2 r = {(pos[0] + (size[0] / 2)),pos[1]};

    addTriangle(b,l,t,color,depth);
    addTriangle(b,r,t,color,depth);
}



void Renderer::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    const auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    const float worldX = xpos;
    const float worldY = game->getRenderer().getHeight() - ypos;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        game->clickEvent(static_cast<int>(worldX),static_cast<int>(worldY));
    }
}

void Renderer::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    const auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    if (action == GLFW_PRESS) {
        game->keyPressEvent(key);
    }
    else if (action == GLFW_RELEASE) {
        game->keyReleaseEvent(key);
    }
}


