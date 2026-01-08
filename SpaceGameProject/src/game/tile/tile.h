#ifndef TILE_H
#define TILE_H

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>
#include <random>
#include <cstring>      // memcpy
#include <glad/glad.h>  // assure que glad est chargé avant d'utiliser OpenGL

#include "../../renderer/renderer.h" // doit définir : Vertex { pos,color }, Vec2, Vec4, Renderer::Shader etc.

// -------------------- bit masks (layout clair) --------------------
constexpr uint32_t TILE_MASK_FLAGS     = 0x000000FFu; // bits 0..7
constexpr uint32_t TILE_MASK_TYPE      = 0x0000FF00u; // bits 8..15
constexpr uint32_t TILE_MASK_ANIMFR    = 0x00FF0000u; // bits 16..23
constexpr uint32_t TILE_MASK_ANIMMAX   = 0xFF000000u; // bits 24..31

// Flags
#define TILE_SOLID      0x01u
#define TILE_ANIMATED   0x02u
#define TILE_VISIBLE    0x04u
#define TILE_FIRE       0x08u

// -------------------- Tile (32 bits compact) --------------------
struct Tile {
    uint32_t data = 0u;

    inline void setFlag(uint8_t flag)   { data |= (uint32_t)flag; }
    inline void clearFlag(uint8_t flag) { data &= ~((uint32_t)flag); }
    inline bool checkFlag(uint8_t flag) const { return (data & (uint32_t)flag) != 0u; }

    inline void setType(uint8_t t) {
        data = (data & ~TILE_MASK_TYPE) | (static_cast<uint32_t>(t) << 8);
    }
    inline uint8_t getType() const {
        return static_cast<uint8_t>((data & TILE_MASK_TYPE) >> 8);
    }

    inline void setAnimFrame(uint8_t f) {
        data = (data & ~TILE_MASK_ANIMFR) | (static_cast<uint32_t>(f) << 16);
    }
    inline uint8_t getAnimFrame() const {
        return static_cast<uint8_t>((data & TILE_MASK_ANIMFR) >> 16);
    }

    inline void setAnimMax(uint8_t f) {
        data = (data & ~TILE_MASK_ANIMMAX) | (static_cast<uint32_t>(f) << 24);
    }
    inline uint8_t getAnimMax() const {
        return static_cast<uint8_t>((data & TILE_MASK_ANIMMAX) >> 24);
    }
};

// -------------------- TileLayer --------------------
struct TileLayer {
    std::vector<Tile> tiles;
    int width = 0;
    int height = 0;

    TileLayer(int w = 0, int h = 0) : tiles(static_cast<size_t>(w)*static_cast<size_t>(h)), width(w), height(h) {}

    inline Tile& getTile(int x, int y) { return tiles[y * width + x]; }
    inline const Tile& getTile(int x, int y) const { return tiles[y * width + x]; }
};

// -------------------- Chunk --------------------
struct Chunk {
    std::vector<std::unique_ptr<TileLayer>> layers;
    int width = 0;
    int height = 0;
    int chunkX = 0;
    int chunkY = 0;
    bool active = false;

    // GPU mesh
    std::vector<Vertex> vertices;
    GLuint VAO = 0;
    GLuint VBO = 0;

    Chunk(int w = 0, int h = 0, int layerCount = 1, int cx = 0, int cy = 0)
        : width(w), height(h), chunkX(cx), chunkY(cy)
    {
        layers.reserve(layerCount);
        for (int i = 0; i < layerCount; ++i)
            layers.emplace_back(std::make_unique<TileLayer>(w, h));
    }

    ~Chunk() {
        if (VBO) { glDeleteBuffers(1, &VBO); VBO = 0; }
        if (VAO) { glDeleteVertexArrays(1, &VAO); VAO = 0; }
    }

    inline void addTileToMesh(
        const Vec2& tilePos,   // (x, y) en grille
        const Vec4& color,
        float layerZ
    )
    {
        float z = layerZ;

        // quad logique (2 triangles)
        vertices.push_back(Vertex{z, {0,0}, { tilePos[0],     tilePos[1]     }, color });
        vertices.push_back(Vertex{z, {1,0}, { tilePos[0] + 1, tilePos[1]     }, color });
        vertices.push_back(Vertex{z, {1,1}, { tilePos[0] + 1, tilePos[1] + 1 }, color });

        vertices.push_back(Vertex{z, {0,0}, { tilePos[0],     tilePos[1]     }, color });
        vertices.push_back(Vertex{z, {1,1}, { tilePos[0] + 1, tilePos[1] + 1 }, color });
        vertices.push_back(Vertex{z, {0,1}, { tilePos[0],     tilePos[1] + 1 }, color });
    }


    // build mesh (iso) et upload GPU - appeler quand chunk changé
    void buildMeshIsometric(int tileSize) {
        vertices.clear();
        // parcourir layers (ordre important: draw layers bottom->top)
        for (size_t layerIndex = 0; layerIndex < layers.size(); ++layerIndex) {
            const TileLayer& layer = *layers[layerIndex];

            for (int y = 0; y < layer.height; ++y) {
                for (int x = 0; x < layer.width; ++x) {
                    const Tile& tile = layer.getTile(x, y);
                    if (!tile.checkFlag(TILE_VISIBLE)) continue;

                    int gx = chunkX * width + x;
                    int gy = chunkY * height + y;

                    uint8_t ttype = tile.getType();
                    Vec4 color = {
                        (ttype % 3 == 0) ? 1.0f : 0.35f,
                        (ttype % 3 == 1) ? 1.0f : 0.35f,
                        (ttype % 3 == 2) ? 1.0f : 0.35f,
                        1.0f
                    };

                    float baseZ = layerIndex * 100000.0f;
                    float topZ  = baseZ;

                    addTileToMesh({ static_cast<float>(gx), static_cast<float>(gy) }, color, topZ);
                }
            }
        }



        // upload to GPU (single upload)
        if (VAO == 0) glGenVertexArrays(1, &VAO);
        if (VBO == 0) glGenBuffers(1, &VBO);

        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        if (!vertices.empty()) {
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
        } else {
            // upload empty small buffer to avoid undefined bindings
            glBufferData(GL_ARRAY_BUFFER, 1, nullptr, GL_STATIC_DRAW);
        }

        // attribute layout must match renderer::Vertex
        glEnableVertexAttribArray(0); // aZ
        glVertexAttribPointer(0, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, z));

        glEnableVertexAttribArray(1); // aUV
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

        glEnableVertexAttribArray(2); // aPos
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

        glEnableVertexAttribArray(3); // aColor
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));


        glBindVertexArray(0);
    }
};

// -------------------- TileMap --------------------
struct TileMap {
    std::unordered_map<uint64_t, std::unique_ptr<Chunk>> chunks;
    int chunkWidth = 16;
    int chunkHeight = 16;
    int tileWidth = 16;
    int tileHeight = 16;

    TileMap(int cW = 16, int cH = 16, int tW = 16, int tH = 16)
        : chunkWidth(cW), chunkHeight(cH), tileWidth(tW), tileHeight(tH) {}

    inline uint64_t hashChunkPos(int x, int y) const {
        return (static_cast<uint64_t>(static_cast<uint32_t>(x)) << 32) | static_cast<uint32_t>(y);
    }

    Chunk* getOrCreateChunk(int cx, int cy, int layers = 2) {
        uint64_t key = hashChunkPos(cx, cy);
        auto it = chunks.find(key);
        if (it == chunks.end()) {
            chunks[key] = std::make_unique<Chunk>(chunkWidth, chunkHeight, layers, cx, cy);
            return chunks[key].get();
        }
        return it->second.get();
    }

    Chunk* getChunk(int cx, int cy) {
        uint64_t key = hashChunkPos(cx, cy);
        auto it = chunks.find(key);
        return (it != chunks.end()) ? it->second.get() : nullptr;
    }
};

// -------------------- TileManager --------------------
class TileManager {
public:
    std::unordered_map<int, std::unique_ptr<TileMap>> tileMaps;

    TileManager() = default;
    ~TileManager() = default;

    bool debug = false;

    TileMap* createTileMap(int id, int cW, int cH, int tW, int tH) {
        auto map = std::make_unique<TileMap>(cW, cH, tW, tH);
        TileMap* ptr = map.get();
        tileMaps[id] = std::move(map);
        return ptr;
    }

    TileMap* getTileMap(int id) {
        auto it = tileMaps.find(id);
        return (it != tileMaps.end()) ? it->second.get() : nullptr;
    }

    // streaming : active chunks dans le carré [playerCx-radius .. playerCx+radius]
    void streamChunks(TileMap* map, int playerCx, int playerCy, int radius) {
        int minCx = playerCx - radius, maxCx = playerCx + radius;
        int minCy = playerCy - radius, maxCy = playerCy + radius;

        // ensure chunks exist and mark active
        for (int cx = minCx; cx <= maxCx; ++cx) {
            for (int cy = minCy; cy <= maxCy; ++cy) {
                Chunk* c = map->getOrCreateChunk(cx, cy, 2);
                c->active = true;
            }
        }

        // deactivate others
        for (auto& kv : map->chunks) {
            Chunk* c = kv.second.get();
            if (c->chunkX < minCx || c->chunkX > maxCx || c->chunkY < minCy || c->chunkY > maxCy)
                c->active = false;
        }
    }

    // build mesh for all active chunks (call after you modify tiles)
    void buildActiveChunkMeshes(TileMap* map, int tileSize) {
        for (auto& kv : map->chunks) {
            Chunk* c = kv.second.get();
            if (c->active) c->buildMeshIsometric(tileSize);
        }
    }

    // draw (renderer must have shader already set-up)
    void drawTileMap(TileMap* map, Renderer& renderer) {
        // renderer should have shader bound and projection set
        for (auto& kv : map->chunks) {
            Chunk* c = kv.second.get();
            if (!c->active) continue;
            if (c->vertices.empty()) continue;
            if(debug) {
                Vec4 color = {0.7f,0.7f,0.0f,1.0f};

                // start et end en Vec2
                Vec2 tl = { c->chunkX, c->chunkY };
                Vec2 tr = { (int) c->chunkX + c->width,(int) c->chunkY };
                Vec2 br = { (int) c->chunkX + c->width,(int) c->chunkY + c->height };
                Vec2 bl = { (int) c->chunkX,(int) c->chunkY + c->height };

                renderer.addLine(tl, tr, 5.0f, color, 0.0f);
                renderer.addLine(tr, br, 5.0f, color, 0.0f);
                renderer.addLine(bl, bl, 5.0f, color, 0.0f);
                renderer.addLine(bl, tl, 5.0f, color, 0.0f);

            }
            glBindVertexArray(c->VAO);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(c->vertices.size()));
        }
        glBindVertexArray(0);
    }

    // Exemple : génère une tilemap pour test
    TileMap* generateDebugMap(int id = 0)
    {
        // paramètres de base
        const int chunkW = 16;
        const int chunkH = 16;
        const int tileSize = 16;
        const int layers = 2;

        // création de la TileMap
        TileMap* map = createTileMap(id, chunkW, chunkH, tileSize, tileSize);

        // générateur pseudo-aléatoire
        std::mt19937 rng(12345); // seed fixe pour test
        std::uniform_int_distribution<int> typeDist(0, 3);
        std::uniform_real_distribution<float> visibleChance(0.0f, 1.0f);

        // on va créer un carré de 3x3 chunks autour du centre
        const int radius = 1;
        for (int cx = -radius; cx <= radius; ++cx)
        {
            for (int cy = -radius; cy <= radius; ++cy)
            {
                Chunk* chunk = map->getOrCreateChunk(cx, cy, layers);
                chunk->active = true;

                // remplir chaque layer
                for (int l = 0; l < layers; ++l)
                {
                    TileLayer& layer = *chunk->layers[l];
                    for (int y = 0; y < layer.height; ++y)
                    {
                        for (int x = 0; x < layer.width; ++x)
                        {
                            Tile& tile = layer.getTile(x, y);
                            tile.setType(l);
                            tile.clearFlag(0xFF); // reset flags

                            // visibilité aléatoire
                            if (visibleChance(rng) > 0.25f)
                                tile.setFlag(TILE_VISIBLE);

                            // aléatoirement solide ou feu
                            if (visibleChance(rng) > 0.85f)
                                tile.setFlag(TILE_SOLID);
                            if (visibleChance(rng) > 0.95f)
                                tile.setFlag(TILE_FIRE);
                        }
                    }
                }

                // construire le mesh GPU pour ce chunk
                chunk->buildMeshIsometric(tileSize);
            }
        }

        return map;
    }

};

#endif // TILE_H
