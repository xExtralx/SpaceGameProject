#include <string>
#include "renderer.h"
#include "../game/game.h"

// =====================
// Constructor / Destructor
// =====================

Renderer::Renderer(int w, int h) 
    : width(w), height(h) {}

Renderer::~Renderer() {
    // Cleanup shaders
    delete tileShader;
    delete colorShader;
    delete imageShader;
    delete upscaleShader;

    // Cleanup GL buffers
    if (tileVAO)   glDeleteVertexArrays(1, &tileVAO);
    if (tileVBO)   glDeleteBuffers(1, &tileVBO);
    if (colorVAO)  glDeleteVertexArrays(1, &colorVAO);
    if (colorVBO)  glDeleteBuffers(1, &colorVBO);
    if (screenVAO) glDeleteVertexArrays(1, &screenVAO);
    if (screenVBO) glDeleteBuffers(1, &screenVBO);
    if (pixelFBO)  glDeleteFramebuffers(1, &pixelFBO);
    if (pixelTexture) glDeleteTextures(1, &pixelTexture);

    glfwTerminate();
}

// =====================
// Init
// =====================

int Renderer::init() {
    if (!glfwInit()) {
        std::cerr << "failed to Init GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "My Game", nullptr, nullptr);
    if (!window) {
        std::cerr << "failed to Create window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "failed to Load Glad" << std::endl;
        return false;
    }

    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glfwSwapInterval(1);
    glfwSetMouseButtonCallback(window, Renderer::mouse_button_callback);
    glfwSetKeyCallback(window, Renderer::key_callback);

    monitor = glfwGetPrimaryMonitor();
    mode    = glfwGetVideoMode(monitor);

    // Load shaders
    tileShader = new Shader(
        FileManager::LoadTextFile("shader/default.vert"),
        FileManager::LoadTextFile("shader/default.frag")
    );
    colorShader = new Shader(
        FileManager::LoadTextFile("shader/color.vert"),
        FileManager::LoadTextFile("shader/color.frag")
    );
    imageShader = new Shader(
        FileManager::LoadTextFile("shader/image.vert"),
        FileManager::LoadTextFile("shader/image.frag")
    );

    initPixelFBO();
    initColorBuffer();
    initTileBuffer();

    return true;
}

// =====================
// Buffer Init
// =====================

void Renderer::initPixelFBO() {
    glGenFramebuffers(1, &pixelFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, pixelFBO);

    glGenTextures(1, &pixelTexture);
    glBindTexture(GL_TEXTURE_2D, pixelTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, RENDER_WIDTH, RENDER_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pixelTexture, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "[FBO] Pixel FBO not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Fullscreen quad (NDC, covers entire screen)
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

    upscaleShader = new Shader(
        FileManager::LoadTextFile("shader/upscale.vert"),
        FileManager::LoadTextFile("shader/upscale.frag")
    );
}

void Renderer::initColorBuffer() {
    glGenVertexArrays(1, &colorVAO);
    glGenBuffers(1, &colorVBO);
    glBindVertexArray(colorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);

    // Reserve space, will be updated dynamically
    glBufferData(GL_ARRAY_BUFFER, sizeof(ColorVertex) * 3000, nullptr, GL_DYNAMIC_DRAW);

    // pos (location = 0) — vec3
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)offsetof(ColorVertex, pos));
    glEnableVertexAttribArray(0);

    // color (location = 1) — vec4
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(ColorVertex), (void*)offsetof(ColorVertex, color));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Renderer::initTileBuffer() {
    glGenVertexArrays(1, &tileVAO);
    glGenBuffers(1, &tileVBO);
    glBindVertexArray(tileVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tileVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(TileVertex) * 6000, nullptr, GL_DYNAMIC_DRAW);

    // localPos (location = 0) — vec2
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, localPos));
    glEnableVertexAttribArray(0);

    // uv (location = 1) — vec2
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, uv));
    glEnableVertexAttribArray(1);

    // tilePos (location = 2) — vec3
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(TileVertex), (void*)offsetof(TileVertex, tilePos));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

// =====================
// Frame
// =====================

void Renderer::clear() {
    glBindFramebuffer(GL_FRAMEBUFFER, pixelFBO);
    glViewport(0, 0, RENDER_WIDTH, RENDER_HEIGHT);
    glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    tileVertices.clear();
    colorVertices.clear();
}

void Renderer::draw() {
    flushColorGeometry();
    flushTileGeometry();
}

void Renderer::present() const {
    // Upscale low res FBO to screen with nearest neighbor
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    upscaleShader->use();
    upscaleShader->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pixelTexture);
    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

// =====================
// Geometry flush
// =====================

void Renderer::flushColorGeometry() {
    if (colorVertices.empty()) return;

    glBindVertexArray(colorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, colorVBO);

    // Grow buffer if needed
    glBufferData(GL_ARRAY_BUFFER, 
        colorVertices.size() * sizeof(ColorVertex), 
        colorVertices.data(), 
        GL_DYNAMIC_DRAW);

    colorShader->use();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(colorVertices.size()));
    glBindVertexArray(0);
}

void Renderer::flushTileGeometry() {
    if (tileVertices.empty()) return;

    glBindVertexArray(tileVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tileVBO);

    glBufferData(GL_ARRAY_BUFFER,
        tileVertices.size() * sizeof(TileVertex),
        tileVertices.data(),
        GL_DYNAMIC_DRAW);

    tileShader->use();
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(tileVertices.size()));
    glBindVertexArray(0);
}

// =====================
// Draw calls
// =====================

void Renderer::addTriangle(const Vec2& v1, const Vec2& v2, const Vec2& v3, const Vec4& color, float z) {
    colorVertices.push_back({ Vec3(v1[0], v1[1], z), color });
    colorVertices.push_back({ Vec3(v2[0], v2[1], z), color });
    colorVertices.push_back({ Vec3(v3[0], v3[1], z), color });
}

void Renderer::drawImage(const std::string& filePath, Shader& shader) {
    static GLuint imgVAO = 0, imgVBO = 0;

    if (imgVAO == 0) {
        float quad[] = {
            -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f,  0.0f, 1.0f,
             0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.0f, 0.0f,
             0.5f, -0.5f,  1.0f, 1.0f,
             0.5f,  0.5f,  1.0f, 0.0f
        };

        glGenVertexArrays(1, &imgVAO);
        glGenBuffers(1, &imgVBO);
        glBindVertexArray(imgVAO);
        glBindBuffer(GL_ARRAY_BUFFER, imgVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    GLuint textureID = TextureManager::getInstance().loadTexture(filePath);
    if (textureID == 0) {
        std::cerr << "[drawImage] Failed to load: " << filePath << std::endl;
        return;
    }

    shader.use();
    shader.setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glBindVertexArray(imgVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

// =====================
// Callbacks / Utils
// =====================

void Renderer::update() {
    const auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    game->moveEvent(static_cast<float>(xpos), static_cast<float>(getHeight() - ypos));
}

bool Renderer::shouldClose() const {
    return glfwWindowShouldClose(window);
}

void Renderer::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    const auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    const float worldX = static_cast<float>(xpos);
    const float worldY = static_cast<float>(game->getRenderer().getHeight() - ypos);

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        game->clickEvent(static_cast<int>(worldX), static_cast<int>(worldY));
}

void Renderer::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    const auto game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (!game) return;

    if (action == GLFW_PRESS)
        game->keyPressEvent(key);
    else if (action == GLFW_RELEASE)
        game->keyReleaseEvent(key);
}