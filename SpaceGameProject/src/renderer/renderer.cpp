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

    glBindVertexArray(VAO); 
    glDrawArrays(GL_TRIANGLES, 0,
        static_cast<GLsizei>(vertices.size() / 5));
    glBindVertexArray(0);
}

float quadVertices[] = {
    // localPos.x, localPos.y, uv.x, uv.y, tilePos.x, tilePos.y, tilePos.z
    -0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f, 0.0f, // top-left
    -0.5f, -0.5f,  0.0f, 0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
     0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f, 0.0f, // bottom-right

    -0.5f,  0.5f,  0.0f, 1.0f,  0.0f, 0.0f, 0.0f, // top-left
     0.5f, -0.5f,  1.0f, 0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
     0.5f,  0.5f,  1.0f, 1.0f,  0.0f, 0.0f, 0.0f  // top-right
};

void drawImage(const std::string& filePath, Shader& shader) {
    static GLuint VAO = 0, VBO = 0;

    // Initialize VAO/VBO once
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        // localPos attribute (location = 0)
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // uv attribute (location = 1)
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // tilePos attribute (location = 2)
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(4 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }

    // Load texture
    GLuint textureID = TextureManager::getInstance().loadTexture(filePath);
    if (textureID == 0) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        return;
    }

    // Use shader
    shader.use();

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Make sure your shader has a uniform like: uniform sampler2D texture1;
    // and set it with: shader.setInt("texture1", 0);

    // Draw the quad
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
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

void Renderer::uploadGeometry(
    const void* vertexData,
    size_t vertexCount,
    const VertexLayout& layout
)
{
    if (VAO == 0)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        glBufferData(
            GL_ARRAY_BUFFER,
            vertexCount * layout.stride,
            vertexData,
            GL_DYNAMIC_DRAW
        );

        for (const auto& attrib : layout.attributes)
        {
            glVertexAttribPointer(
                attrib.index,
                attrib.count,
                GL_FLOAT,
                GL_FALSE,
                layout.stride,
                (void*)attrib.offset
            );
            glEnableVertexAttribArray(attrib.index);
        }

        glBindVertexArray(0);
    }

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertexCount * layout.stride,
        vertexData,
        GL_DYNAMIC_DRAW
    );
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


