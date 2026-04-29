#ifndef MYGAME_RENDERER_H
#define MYGAME_RENDERER_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "shader/shader.h"
#include "../utils/utils.h"
#include "texture/texture.h"
#include "../game/world/worldgen.h"

struct ChunkRenderData {
    GLuint VAO = 0;
    GLuint instanceVBO = 0; // per instance data
    int    instanceCount = 0;
    bool   uploaded = false;
};

struct TileInstance {
    Vec3  tilePos;
    Vec2  uvOffset;
    Vec2  uvSize;   // add this
    float ao;
};

struct Camera {
    Vec2  position = Vec2(0.0f, 0.0f);
    float zoom     = 1.0f;

    Mat4 getViewProj(int renderW, int renderH) const {
        float halfW = (renderW * 0.5f) / zoom;
        float halfH = (renderH * 0.5f) / zoom;

        float left   = position[0] - halfW;
        float right  = position[0] + halfW;
        float bottom = position[1] - halfH;
        float top    = position[1] + halfH;

        return Mat4::ortho(left, right, bottom, top, -1.0f, 1.0f);
    }
};

// Textured tile vertex
struct TileVertex {
    Vec2 localPos;
    Vec2 uv;
    Vec3 tilePos;
};

// Colored primitive vertex
struct ColorVertex {
    Vec3 pos;
    Vec4 color;
};

class Renderer {
public:
    Renderer(int w, int h);
    ~Renderer();

    Camera camera;

    // Pixel perfect internal resolution
    static const int RENDER_WIDTH  = 320;
    static const int RENDER_HEIGHT = 180;

    int  init();
    void clear();
    void draw();
    void present() const;
    void update();

    // Draw calls — accumulate geometry, flushed in draw()
    void addTriangle(const Vec2& v1, const Vec2& v2, const Vec2& v3, const Vec4& color, float z);
    void addTile(const TileVertex verts[4]); // for textured tiles later
    void drawImage(const std::string& filePath, float x, float y, float scale);

    bool shouldClose() const;

    int        getWidth()  { return width; }
    int        getHeight() { return height; }
    GLFWwindow* getWindow() { return window; }

    static void key_callback(GLFWwindow*, int, int, int, int);
    static void mouse_button_callback(GLFWwindow*, int, int, int);

    bool fullscreen = false;

    void initTileQuad();
    void uploadChunk(const Chunk& chunk);
    void renderChunks(const ChunkManager& chunkManager);
    Vec2 tileTypeToUV(TileType type) const;
private:
    int width  = 0;
    int height = 0;

    GLFWwindow*      window   = nullptr;
    GLFWmonitor*     monitor  = nullptr;
    const GLFWvidmode* mode   = nullptr;

    // Shaders
    Shader* tileShader    = nullptr;
    Shader* colorShader   = nullptr;
    Shader* imageShader   = nullptr;
    Shader* upscaleShader = nullptr;

    // Tile geometry (textured)
    std::vector<TileVertex>  tileVertices;
    GLuint tileVAO = 0, tileVBO = 0;

    // Color geometry (primitives)
    std::vector<ColorVertex> colorVertices;
    GLuint colorVAO = 0, colorVBO = 0;

    // Pixel perfect FBO
    GLuint pixelFBO     = 0;
    GLuint pixelTexture = 0;
    GLuint screenVAO    = 0;
    GLuint screenVBO    = 0;

    void initPixelFBO();
    void initColorBuffer();
    void initTileBuffer();
    void flushColorGeometry();
    void flushTileGeometry();

    // Tile rendering
    GLuint               tileQuadVAO = 0;
    GLuint               tileQuadVBO = 0;
    GLuint               tilesetTexture = 0;

    std::unordered_map<ChunkPos, ChunkRenderData, ChunkPosHash> chunkRenderData;
};

#endif