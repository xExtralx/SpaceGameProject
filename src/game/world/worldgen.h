#ifndef WORLDGEN_H
#define WORLDGEN_H

#include "tile.h"
#include <FastNoise/FastNoise.h>
#include <unordered_map>
#include <memory>

class WorldGen {
public:
    explicit WorldGen(int32_t seed = 1337);

    // Generate a chunk at the given position
    void generateChunk(Chunk& chunk) const;

private:
    int32_t seed;

    // Noise nodes
    FastNoise::SmartNode<FastNoise::FractalFBm> elevationNoise;
    FastNoise::SmartNode<FastNoise::FractalFBm> resourceNoise;
    FastNoise::SmartNode<FastNoise::FractalFBm> forestNoise;

    // Thresholds
    static constexpr float WATER_LEVEL    = -0.2f;
    static constexpr float SAND_LEVEL     =  0.0f;
    static constexpr float GRASS_LEVEL    =  0.4f;
    static constexpr float STONE_LEVEL    =  0.7f;

    static constexpr float IRON_THRESHOLD  = 0.6f;
    static constexpr float COAL_THRESHOLD  = 0.5f;
    static constexpr float COPPER_THRESHOLD= 0.7f;

    // Internal helpers
    TileType   elevationToType(float elevation)                    const;
    void       applyResources(Chunk& chunk,
                              const std::vector<float>& resourceMap,
                              const std::vector<float>& elevationMap) const;
    int8_t     flagsForType(TileType type)                         const;
};

// =====================
// CHUNK MANAGER
// =====================
class ChunkManager {
public:
    explicit ChunkManager(int32_t seed = 1337);

    // Get or generate a chunk
    Chunk& getChunk(ChunkPos pos);
    bool   hasChunk(ChunkPos pos) const;

    // Load chunks around a world position (camera)
    void updateLoadedChunks(float worldX, float worldY,
                            float tileSize, int renderDistance);

    // Unload chunks far from camera
    void unloadDistantChunks(ChunkPos center, int renderDistance);

    const std::unordered_map<ChunkPos, Chunk, ChunkPosHash>& getChunks() const {
        return chunks;
    }

private:
    WorldGen worldGen;
    std::unordered_map<ChunkPos, Chunk, ChunkPosHash> chunks;
};

#endif // WORLDGEN_H