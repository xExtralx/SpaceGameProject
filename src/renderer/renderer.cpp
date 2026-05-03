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
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glfwSwapInterval(1);
    glfwSetMouseButtonCallback(window, Renderer::mouse_button_callback);
    glfwSetKeyCallback(window, Renderer::key_callback);

    glGenTextures(1, &fallbackWhiteTexture);
    glBindTexture(GL_TEXTURE_2D, fallbackWhiteTexture);
    unsigned char white[4] = {255, 255, 255, 255};
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glBindTexture(GL_TEXTURE_2D, 0);

    monitor = glfwGetPrimaryMonitor();
    mode    = glfwGetVideoMode(monitor);

    colorShader = new Shader(
        FileManager::LoadTextFile("shader/color.vert"),
        FileManager::LoadTextFile("shader/color.frag")
    );
    imageShader = new Shader(
        FileManager::LoadTextFile("shader/image.vert"),
        FileManager::LoadTextFile("shader/image.frag")
    );

    tileShader = new Shader(
        FileManager::LoadTextFile("shader/tile.vert"),
        FileManager::LoadTextFile("shader/tile.frag")
    );

    meshShader = new Shader(
        FileManager::LoadTextFile("shader/mesh.vert"),
        FileManager::LoadTextFile("shader/mesh.frag")
    );

    tilesetTexture = TextureManager::getInstance().loadTexture("textures/tileset/atlas.png");

    initTileQuad();

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

    GLuint depthRBO;
    glGenRenderbuffers(1, &depthRBO);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, RENDER_WIDTH, RENDER_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRBO);

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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    tileVertices.clear();
    colorVertices.clear();
}

void Renderer::draw() {
    flushColorGeometry();
    flushTileGeometry();
}

void Renderer::present() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST); // ← AJOUTEZ ÇA

    upscaleShader->use();
    upscaleShader->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pixelTexture);
    glBindVertexArray(screenVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

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
    glBufferData(GL_ARRAY_BUFFER,
        colorVertices.size() * sizeof(ColorVertex),
        colorVertices.data(),
        GL_DYNAMIC_DRAW);

    colorShader->use();
    Mat4 viewProj = camera.getViewProj(RENDER_WIDTH, RENDER_HEIGHT);
    colorShader->setMat4("uViewProj", viewProj);

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

    // Camera matrix
    Mat4 viewProj = camera.getViewProj(RENDER_WIDTH, RENDER_HEIGHT);
    tileShader->setMat4("uViewProj", viewProj);

    // Tile size in world units (pixels per tile)
    tileShader->setVec2("uTileSize", 32.0f, 16.0f); // isometric tile width/height
    tileShader->setFloat("uHeightStep", 8.0f);        // pixels per height unit

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

void Renderer::drawImage(const std::string& filePath, float x, float y, float scale) {
    GLuint textureID = TextureManager::getInstance().loadTexture(filePath);
    if (textureID == 0) return;

    // Get texture dimensions
    int texW, texH;
    glBindTexture(GL_TEXTURE_2D, textureID);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH,  &texW);
    glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texH);

    // Convert pixel size to NDC size accounting for aspect ratio
    float aspectX = (float)texW / RENDER_WIDTH  * scale;
    float aspectY = (float)texH / RENDER_HEIGHT * scale;

    // Build quad with correct proportions centered at (x, y) in NDC
    float quad[] = {
        x - aspectX,  y + aspectY,  0.0f, 0.0f,
        x - aspectX,  y - aspectY,  0.0f, 1.0f,
        x + aspectX,  y - aspectY,  1.0f, 1.0f,

        x - aspectX,  y + aspectY,  0.0f, 0.0f,
        x + aspectX,  y - aspectY,  1.0f, 1.0f,
        x + aspectX,  y + aspectY,  1.0f, 0.0f
    };

    static GLuint imgVAO = 0, imgVBO = 0;
    if (imgVAO == 0) {
        glGenVertexArrays(1, &imgVAO);
        glGenBuffers(1, &imgVBO);
        glBindVertexArray(imgVAO);
        glBindBuffer(GL_ARRAY_BUFFER, imgVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quad), nullptr, GL_DYNAMIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);
    }

    // Update quad data every call
    glBindVertexArray(imgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, imgVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);

    imageShader->use();
    imageShader->setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureID);
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

// =====================
// Tile Rendering
// =====================

// Quad shared by all tile instances
static float tileQuad[] = {
    // localPos      uv
    -0.5f,  0.5f,   0.0f, 0.0f, // top-left
    -0.5f, -0.5f,   0.0f, 1.0f, // bottom-left
     0.5f, -0.5f,   1.0f, 1.0f, // bottom-right

    -0.5f,  0.5f,   0.0f, 0.0f, // top-left
     0.5f, -0.5f,   1.0f, 1.0f, // bottom-right
     0.5f,  0.5f,   1.0f, 0.0f  // top-right
};

void Renderer::initTileQuad() {
    glGenVertexArrays(1, &tileQuadVAO);
    glGenBuffers(1, &tileQuadVBO);
    glBindVertexArray(tileQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, tileQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(tileQuad), tileQuad, GL_STATIC_DRAW);

    // aLocalPos (location = 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // aUV (location = 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Renderer::uploadChunk(const Chunk& chunk) {
    // Build instance data for every tile in chunk
    std::vector<TileInstance> instances;
    instances.reserve(CHUNK_SIZE * CHUNK_SIZE);

    for (int y = 0; y < CHUNK_SIZE; y++) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            const Tile& tile = chunk.getTile(x, y);
            if (tile.type == TileType::NONE) continue;

            float worldX = chunk.pos.x * CHUNK_SIZE + x;
            float worldY = chunk.pos.y * CHUNK_SIZE + y;

            // UV from atlas
            TileUV uv = getUVForType(tile.type);

            TileInstance inst;
            inst.tilePos  = Vec3(worldX, worldY, 0.0f);
            inst.uvOffset = Vec2(uv.u, uv.v);
            inst.uvSize   = Vec2(uv.w, uv.h);
            inst.ao       = 0.0f;

            instances.push_back(inst);
        }
    }

    // Get or create render data for this chunk
    ChunkRenderData& rd = chunkRenderData[chunk.pos];

    if (rd.VAO == 0) {
        glGenVertexArrays(1, &rd.VAO);
        glGenBuffers(1, &rd.instanceVBO);
    }

    // Bind the shared quad VAO and add instance buffer
    glBindVertexArray(rd.VAO);

    // First bind the shared quad VBO for vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, tileQuadVBO);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Now bind instance VBO
    glBindBuffer(GL_ARRAY_BUFFER, rd.instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
        instances.size() * sizeof(TileInstance),
        instances.data(),
        GL_DYNAMIC_DRAW);

    // iTilePos (location = 2) — vec3
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(TileInstance),
        (void*)offsetof(TileInstance, tilePos));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1); // advance per instance

    // iUVOffset (location = 3)
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance),
        (void*)offsetof(TileInstance, uvOffset));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // iUVSize (location = 4)
    glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(TileInstance),
        (void*)offsetof(TileInstance, uvSize));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    // iAO (location = 5) — bumped from 4 to 5
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(TileInstance),
        (void*)offsetof(TileInstance, ao));
    glEnableVertexAttribArray(5);
    glVertexAttribDivisor(5, 1);

    glBindVertexArray(0);

    rd.instanceCount = static_cast<int>(instances.size());
    rd.uploaded = true;
}

void Renderer::renderChunks(const ChunkManager& chunkManager) {
    tileShader->use();

    // Camera
    Mat4 viewProj = camera.getViewProj(RENDER_WIDTH, RENDER_HEIGHT);
    tileShader->setMat4("uViewProj", viewProj);
    tileShader->setVec2("uTileSize", 32.0f, 16.0f);
    tileShader->setFloat("uHeightStep", 8.0f);
    tileShader->setVec3("uAmbientColor", 1.0f, 1.0f, 1.0f);
    tileShader->setFloat("uAmbientStr", 1.0f);
    tileShader->setFloat("uTime", (float)glfwGetTime());
    tileShader->setInt("uTileset", 0);

    // Bind tileset
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tilesetTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Draw each loaded chunk
    for (auto& [pos, chunk] : chunkManager.getChunks()) {
        // Upload if dirty
        if (chunk.dirty) {
            uploadChunk(chunk);
            const_cast<Chunk&>(chunk).dirty = false;
        }

        auto it = chunkRenderData.find(pos);
        if (it == chunkRenderData.end()) continue;

        ChunkRenderData& rd = it->second;
        if (!rd.uploaded || rd.instanceCount == 0) continue;

        glBindVertexArray(rd.VAO);
        glDrawArraysInstanced(GL_TRIANGLES, 0, 6, rd.instanceCount);
        glBindVertexArray(0);
    }
}

// =====================
// Meshes
// =====================

Mesh Renderer::loadGLTF(const std::string& path) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool ok = loader.LoadASCIIFromFile(&model, &err, &warn, path);
    std::cerr << "[GLTF] path=" << path << " ok=" << ok << std::endl;
    if (!warn.empty()) std::cerr << "[GLTF] warn: " << warn << std::endl;
    if (!err.empty())  std::cerr << "[GLTF] err:  " << err  << std::endl;
    if (!ok) return {};

    std::cerr << "[GLTF] meshes=" << model.meshes.size() << std::endl;
    if (model.meshes.empty()) return {};

    auto& primitive = model.meshes[0].primitives[0];
    std::cerr << "[GLTF] has POSITION=" << primitive.attributes.count("POSITION") << std::endl;
    std::cerr << "[GLTF] indices accessor=" << primitive.indices << std::endl;

    if (!ok) {
        std::cerr << "[GLTF] Failed to load: " << path << " - " << err << std::endl;
        return {};
    }

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Positions
    auto& posAccessor = model.accessors[primitive.attributes["POSITION"]];
    auto& posView     = model.bufferViews[posAccessor.bufferView];
    auto& posBuffer   = model.buffers[posView.buffer];
    const float* positions = reinterpret_cast<const float*>(
        posBuffer.data.data() + posView.byteOffset + posAccessor.byteOffset);

    // UVs
    const float* uvs = nullptr;
    if (primitive.attributes.count("TEXCOORD_0")) {
        auto& uvAccessor = model.accessors[primitive.attributes["TEXCOORD_0"]];
        auto& uvView     = model.bufferViews[uvAccessor.bufferView];
        uvs = reinterpret_cast<const float*>(
            model.buffers[uvView.buffer].data.data() + uvView.byteOffset + uvAccessor.byteOffset);
    }

    for (size_t i = 0; i < posAccessor.count; i++) {
        vertices.push_back(positions[i * 3 + 0]);
        vertices.push_back(positions[i * 3 + 1]);
        vertices.push_back(positions[i * 3 + 2]);
        if (uvs) {
            vertices.push_back(uvs[i * 2 + 0]);
            vertices.push_back(uvs[i * 2 + 1]);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    // Indices
    auto& idxAccessor = model.accessors[primitive.indices];
    auto& idxView     = model.bufferViews[idxAccessor.bufferView];
    const unsigned short* rawIdx = reinterpret_cast<const unsigned short*>(
        model.buffers[idxView.buffer].data.data() + idxView.byteOffset + idxAccessor.byteOffset);
    for (size_t i = 0; i < idxAccessor.count; i++)
        indices.push_back(rawIdx[i]);

    // Upload GPU
    Mesh mesh;
    glGenVertexArrays(1, &mesh.VAO);
    glGenBuffers(1, &mesh.VBO);
    glGenBuffers(1, &mesh.EBO);

    glBindVertexArray(mesh.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // pos (location 0) — vec3
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // uv (location 1) — vec2
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    mesh.indexCount = static_cast<int>(indices.size());

    // Texture du modele si présente
    if (!model.textures.empty()) {
        auto& tex = model.textures[0];
        auto& img = model.images[tex.source];
        glGenTextures(1, &mesh.textureID);
        glBindTexture(GL_TEXTURE_2D, mesh.textureID);
        
        GLenum format = (img.component == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, img.width, img.height, 0, format, GL_UNSIGNED_BYTE, img.image.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }

    std::cerr << "posView.byteOffset=" << posView.byteOffset
          << " posAccessor.byteOffset=" << posAccessor.byteOffset
          << " total offset=" << (posView.byteOffset + posAccessor.byteOffset)
          << " buffer size=" << posBuffer.data.size() << std::endl;

    // Vérifiez les 3 premiers vertices
    std::cerr << "v0=(" << positions[0] << "," << positions[1] << "," << positions[2] << ")" << std::endl;
    std::cerr << "v1=(" << positions[3] << "," << positions[4] << "," << positions[5] << ")" << std::endl;

    return mesh;
}

void Renderer::renderMesh(const Mesh& mesh, const Mat4& transform) {
    // DEBUG
    GLint currentFBO;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &currentFBO);
    std::cerr << "renderMesh FBO: " << currentFBO << " (expected: " << pixelFBO << ")" << std::endl;
    std::cerr << "indexCount: " << mesh.indexCount << std::endl;
    if (mesh.VAO == 0 || mesh.indexCount == 0) return;

    if (!meshShader) {
        meshShader = new Shader(
            FileManager::LoadTextFile("shader/mesh.vert"),
            FileManager::LoadTextFile("shader/mesh.frag")
        );
    }

    meshShader->use();

    // Matrices
    Mat4 viewProj = camera.getViewProj(RENDER_WIDTH, RENDER_HEIGHT);
    meshShader->setMat4("uViewProj", viewProj);
    meshShader->setMat4("uModel",    transform);

    // Texture
    glActiveTexture(GL_TEXTURE0);
    if (mesh.textureID)
        glBindTexture(GL_TEXTURE_2D, mesh.textureID);
    else
        glBindTexture(GL_TEXTURE_2D, fallbackWhiteTexture);

    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
    std::cerr << "Active program: " << currentProgram << " meshShader ID: " << meshShader->ID << std::endl;

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    std::cerr << "Viewport: " << viewport[0] << " " << viewport[1] << " " << viewport[2] << " " << viewport[3] << std::endl;

    // Draw
    glBindVertexArray(mesh.VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3); // ← pas d'EBO, juste 3 vertices hardcodés
    glBindVertexArray(0);
}