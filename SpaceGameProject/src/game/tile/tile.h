#ifndef TILE_H
#define TILE_H

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "../../renderer/renderer.h"

// 🔹 Flags pour les tiles (1 bit chacun dans uint8_t)
#define TILE_SOLID      0x01
#define TILE_ANIMATED   0x02
#define TILE_VISIBLE    0x04
#define TILE_FIRE       0x08
// jusqu'à 8 flags dans un uint8_t, peut étendre si besoin

// 🔹 Structure ultra-compacte pour 1 tile (32 bits)
struct Tile {
    uint32_t data; 
    /*
        bits 0-7   : flags
        bits 8-15  : type (0-255 types)
        bits 16-23 : animationFrame
        bits 24-31 : animationMaxFrames
    */

    inline void setFlag(uint8_t flag)   { data |= flag; }
    inline void clearFlag(uint8_t flag) { data &= ~flag; }
    inline bool checkFlag(uint8_t flag) const { return (data & flag) != 0; }

    inline void setType(uint8_t t) { data = (data & 0xFF00FFFF) | (t << 8); }
    inline uint8_t getType() const { return (data >> 8) & 0xFF; }

    inline void setAnimFrame(uint8_t f) { data = (data & 0xFF00FFFF) | (f << 16); }
    inline uint8_t getAnimFrame() const { return (data >> 16) & 0xFF; }

    inline void setAnimMax(uint8_t f) { data = (data & 0x00FFFFFF) | (f << 24); }
    inline uint8_t getAnimMax() const { return (data >> 24) & 0xFF; }
};

// 🔹 Layer de tiles
struct TileLayer {
    Tile* tiles;  
    int width;
    int height;

    TileLayer(int w, int h) : width(w), height(h) {
        tiles = new Tile[w*h](); // contigu et zero-init
    }

    ~TileLayer() { delete[] tiles; }

    inline Tile& getTile(int x, int y) { return tiles[y*width + x]; }
};

// 🔹 Chunk de la map
struct Chunk {
    std::vector<TileLayer*> layers; // plusieurs layers (sol, déco, collisions)
    int width, height;              // tiles par chunk
    int chunkX, chunkY;             // position dans la map
    bool loaded;                    // alloué en mémoire
    bool active;                    // dans le rayon du joueur, à updater/rendre

    Chunk(int w, int h, int layerCount, int cx, int cy)
        : width(w), height(h), chunkX(cx), chunkY(cy), loaded(true), active(false) {
        for(int i=0; i<layerCount; i++)
            layers.push_back(new TileLayer(w,h));
    }

    ~Chunk() {
        for(auto l : layers) delete l;
    }
};

// 🔹 Map complète avec chunks persistants
struct TileMap {
    std::unordered_map<uint64_t, Chunk*> chunks; // clé = (x<<32)|y
    int chunkWidth, chunkHeight; // taille des chunks en tiles
    int tileWidth, tileHeight;   // pixels
    int tileCount;               // nombre de types de tiles dans atlas
    unsigned int textureID;      // OpenGL atlas texture

    TileMap(int cW, int cH, int tW, int tH, int tCount, unsigned int texID)
        : chunkWidth(cW), chunkHeight(cH), tileWidth(tW), tileHeight(tH),
          tileCount(tCount), textureID(texID) {}

    inline uint64_t hashChunkPos(int x, int y) const {
        return (static_cast<uint64_t>(x) << 32) | static_cast<uint32_t>(y);
    }

    void addChunk(int cx, int cy, int layers=1) {
        uint64_t key = hashChunkPos(cx, cy);
        if(chunks.find(key)==chunks.end())
            chunks[key] = new Chunk(chunkWidth, chunkHeight, layers, cx, cy);
    }

    Chunk* getChunk(int cx, int cy) {
        uint64_t key = hashChunkPos(cx, cy);
        auto it = chunks.find(key);
        return (it != chunks.end()) ? it->second : nullptr;
    }

    // Décharge pas, juste désactive
    void deactivateChunksOutside(int minCx, int maxCx, int minCy, int maxCy) {
        for(auto& kv : chunks) {
            Chunk* c = kv.second;
            c->active = !(c->chunkX < minCx || c->chunkX > maxCx || c->chunkY < minCy || c->chunkY > maxCy) ? true : false;
        }
    }
};

// 🔹 TileManager ultra-optimisé
class TileManager {
public:
    TileManager() {}
    ~TileManager() {
        for(auto& kv : tileMaps) delete kv.second;
    }

    void addTileMap(int id, TileMap* map) { tileMaps[id] = map; }
    TileMap* getTileMap(int id) {
        auto it = tileMaps.find(id);
        return (it != tileMaps.end()) ? it->second : nullptr;
    }

    // Streaming : active les chunks autour du joueur
    void streamChunks(TileMap* map, int playerX, int playerY, int radius) {
        int minCx = playerX - radius;
        int maxCx = playerX + radius;
        int minCy = playerY - radius;
        int maxCy = playerY + radius;

        // Ajouter chunks inexistants et activer ceux dans le rayon
        for(int cx=minCx; cx<=maxCx; cx++) {
            for(int cy=minCy; cy<=maxCy; cy++) {
                Chunk* c = map->getChunk(cx, cy);
                if(!c) {
                    map->addChunk(cx, cy, 2);
                    c = map->getChunk(cx, cy);
                }
                c->active = true;
            }
        }

        // Désactive les chunks hors rayon
        map->deactivateChunksOutside(minCx, maxCx, minCy, maxCy);
    }

    // Update GPU-ready (SSBO / compute shader)
    void updateTileMap(float dt) {
        // Envoi des tiles dynamiques au GPU et dispatch compute shader
        // Par exemple via glBindBufferBase + glDispatchCompute
    }

    void drawTileMap(TileMap* map) {
        for (auto& [key, chunk] : map->chunks) {
            if (!chunk->active) continue;
            for (auto& layer : chunk->layers) {
                for (int y = 0; y < layer->height; ++y) {
                    for (int x = 0; x < layer->width; ++x) {
                        Tile& tile = layer->getTile(x, y);
                        uint8_t type = tile.getType();
                        if (tile.checkFlag(TILE_VISIBLE)) {
                            printf("[%d]", type); // affichage console
                        } else {
                            printf(" . ");
                        }
                    }
                    printf("\n");
                }
                printf("\n--- Layer suivant ---\n");
            }
        }
    }

    void generateBlankTileMap() {
        // Crée la tilemap : (chunks de 16x16 tiles, tiles de 16x16 pixels, 1 type, texture ID = 0)
        auto* map = new TileMap(16, 16, 16, 16, 1, 0);

        // Ajoute un chunk en (0,0) avec 2 layers (sol et décor)
        uint64_t key = map->hashChunkPos(0, 0);
        map->chunks[key] = new Chunk(16, 16, 2, 0, 0);

        // (Optionnel) remplir les tiles du premier layer avec un type spécifique :
        auto* layer = map->chunks[key]->layers[0];
        for (int y = 0; y < layer->height; ++y) {
            for (int x = 0; x < layer->width; ++x) {
                Tile& tile = layer->getTile(x, y);
                tile.setType(1);                // type 1
                tile.setFlag(TILE_VISIBLE);     // visible
            }
        }

        tileMaps[0] = map;
    }



private:
    std::unordered_map<int, TileMap*> tileMaps;

    int tileSize;
};

#endif // TILE_H
