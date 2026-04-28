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
    glfwSetMouseButtonCallback(window, Renderer::mouse_button_callback);
    glfwSetKeyCallback(window, Renderer::key_callback);

    monitor = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(monitor);

    shader = new Shader(
        FileManager::LoadTextFile("shader/default.vert"),
        FileManager::LoadTextFile("shader/default.frag")
    );

    imageShader = new Shader(
        FileManager::LoadTextFile("shader/image.vert"),
        FileManager::LoadTextFile("shader/image.frag")
    );

    initPixelFBO();

    return true;
}

void Renderer::initPixelFBO() {
    // Create FBO
    glGenFramebuffers(1, &pixelFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pixelFBO);

    // Create low res texture
    glGenTextures(1, &pixelTexture);
    glBindTexture(GL_TEXTURE_2D, pixelTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // critical!
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // critical!
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pixelTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[FBO] Pixel FBO not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Fullscreen quad for upscale pass
    float screenQuad[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };

    glGenVertexArrays(1, &screenVAO);
    glGenBuffers(1, &screenVBO);
    glBindVertexArray(screenVAO);
    glBindBuffer(GL_ARRAY_BUFFER, screenVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(screenQuad), screenQuad, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // Load upscale shader
    upscaleShader = new Shader(
        FileManager::LoadTextFile("shader/upscale.vert"),
        FileManager::LoadTextFile("shader/upscale.frag")
    );
}

void Renderer::clear() {
    // Render INTO low res FBO
    glBindFramebuffer(GL_FRAMEBUFFER, pixelFBO);
    glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    vertices.clear();
}

float quadVertices[] = {
    // localPos.x, localPos.y, uv.x, uv.y
    -0.5f,  0.5f,  0.0f, 0.0f, // top-left
    -0.5f, -0.5f,  0.0f, 1.0f, // bottom-left
     0.5f, -0.5f,  1.0f, 1.0f, // bottom-right

    -0.5f,  0.5f,  0.0f, 0.0f, // top-left
     0.5f, -0.5f,  1.0f, 1.0f, // bottom-right
     0.5f,  0.5f,  1.0f, 0.0f  // top-right
};

void Renderer::drawImage(const std::string& filePath, Shader& shader) {
    static GLuint VAO = 0, VBO = 0;

    // Initialize VAO/VBO once
    if (VAO == 0) {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        // remove tilePos attrib entirely for image shader
    }

    // Load texture
    GLuint textureID = TextureManager::getInstance().loadTexture(filePath);
    if (textureID == 0) {
        std::cerr << "Failed to load texture: " << filePath << std::endl;
        return;
    }

    // Use shader
    shader.use();

    GLint program = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &program);
    std::cerr << "[drawImage] program id: " << program << std::endl;
    std::cerr << "[drawImage] textureID: " << textureID << std::endl;

    GLint texLoc = glGetUniformLocation(program, "uTexture");
    std::cerr << "[drawImage] uTexture loc: " << texLoc << std::endl; // -1 means not found

    GLenum err = glGetError();
    std::cerr << "[drawImage] GL error: " << err << std::endl;

    shader.setInt("uTexture", 0); // add this line
    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Make sure your shader has a uniform like: uniform sampler2D texture1;
    // and set it with: shader.setInt("texture1", 0);

    // Draw the quad
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Renderer::draw() {
    // Everything draws into pixelFBO at low res
    drawImage("debug/debug.png", *imageShader);
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
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClearColor(1.0f, 0.0f, 0.0f, 1.0f); // RED background to confirm upscale pass works
    glClear(GL_COLOR_BUFFER_BIT);

    upscaleShader->use();
    upscaleShader->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pixelTexture);

    GLint texLoc = glGetUniformLocation(upscaleShader->ID, "uTexture");
    std::cerr << "[present] uTexture loc: " << texLoc << std::endl;
    std::cerr << "[present] pixelTexture: " << pixelTexture << std::endl;
    std::cerr << "[present] screenVAO: " << screenVAO << std::endl;

    GLenum err = glGetError();
    std::cerr << "[present] GL error before draw: " << err << std::endl;

    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    err = glGetError();
    std::cerr << "[present] GL error after draw: " << err << std::endl;

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


