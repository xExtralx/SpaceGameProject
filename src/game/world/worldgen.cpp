#include "worldgen.h"
#include <cmath>
#include <iostream>

// =====================
// WORLDGEN
// =====================

WorldGen::WorldGen(int32_t seed) : seed(seed) {
    auto elevSimplex = FastNoise::New<FastNoise::Simplex>();
    elevationNoise   = FastNoise::New<FastNoise::FractalFBm>();
    elevationNoise->SetSource(elevSimplex);
    elevationNoise->SetOctaveCount(6);
    elevationNoise->SetGain(0.5f);
    elevationNoise->SetLacunarity(2.0f);

    auto resSimplex = FastNoise::New<FastNoise::Simplex>();
    resourceNoise   = FastNoise::New<FastNoise::FractalFBm>();
    resourceNoise->SetSource(resSimplex);
    resourceNoise->SetOctaveCount(3);
    resourceNoise->SetGain(0.6f);
    resourceNoise->SetLacunarity(2.5f);

    auto forSimplex = FastNoise::New<FastNoise::Simplex>();
    forestNoise     = FastNoise::New<FastNoise::FractalFBm>();
    forestNoise->SetSource(forSimplex);
    forestNoise->SetOctaveCount(4);
    forestNoise->SetGain(0.5f);
    forestNoise->SetLacunarity(2.0f);
}

void WorldGen::generateChunk(Chunk& chunk) const {
    const int     size         = CHUNK_SIZE;
    const int     area         = size * size;
    const int32_t worldOffsetX = chunk.pos.x * size;
    const int32_t worldOffsetY = chunk.pos.y * size;

    std::vector<float> elevationMap(area);
    std::vector<float> resourceMap(area);
    std::vector<float> forestMap(area);

    elevationNoise->GenUniformGrid2D(
        elevationMap.data(),
        worldOffsetX, worldOffsetY,
        size, size, 0.008f, 0.008f, seed
    );
    resourceNoise->GenUniformGrid2D(
        resourceMap.data(),
        worldOffsetX, worldOffsetY,
        size, size, 0.05f, 0.05f, seed + 1
    );
    forestNoise->GenUniformGrid2D(
        forestMap.data(),
        worldOffsetX, worldOffsetY,
        size, size, 0.03f, 0.03f, seed + 2
    );

    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int   idx      = y * size + x;
            float elev     = elevationMap[idx];
            float resource = resourceMap[idx];
            float forest   = forestMap[idx];

            Tile& tile = chunk.getTile(x, y);

            // Store raw elevation for height rendering
            tile.elevation = elev;

            // Base type from elevation
            tile.type  = elevationToType(elev);
            tile.flags = Tile::defaultFlags(tile.type);

            // Skip resource placement on water
            if (tile.type == TileType::WARM_WATER ||
                tile.type == TileType::WATER       ||
                tile.type == TileType::DEEP_WATER)
                continue;

            // Ore placement
            if (resource > IRON_THRESHOLD) {
                tile.type  = TileType::IRON_ORE;
                tile.flags = Tile::defaultFlags(tile.type);
                tile.resource = static_cast<int8_t>(
                    (resource - IRON_THRESHOLD) / (1.0f - IRON_THRESHOLD) * 127.0f
                );
            } else if (resource < -COPPER_THRESHOLD) {
                tile.type  = TileType::COPPER_ORE;
                tile.flags = Tile::defaultFlags(tile.type);
                tile.resource = static_cast<int8_t>(
                    (-resource - COPPER_THRESHOLD) / (1.0f - COPPER_THRESHOLD) * 127.0f
                );
            } else if (resource > AMETHYST_THRESHOLD * 0.8f &&
                       forest   > AMETHYST_THRESHOLD * 0.6f) {
                tile.type  = TileType::AMETHYST;
                tile.flags = Tile::defaultFlags(tile.type);
                tile.resource = static_cast<int8_t>(resource * 60.0f + 60.0f);
            }
        }
    }

    chunk.generated = true;
    chunk.dirty     = true;
}

TileType WorldGen::elevationToType(float e) const {
    if (e < -0.5f) return TileType::DEEP_WATER;
    if (e < -0.2f) return TileType::WATER;
    if (e < -0.0f) return TileType::WARM_WATER;
    if (e <  0.3f) return TileType::GRASS;
    if (e <  0.5f) return TileType::GRASS_ALT;
    if (e <  0.7f) return TileType::GRASSY_ROCKS;
    return TileType::GRASSY_ROCKS;
}

// =====================
// CHUNK MANAGER
// =====================

ChunkManager::ChunkManager(int32_t seed) : worldGen(seed) {}

Chunk& ChunkManager::getChunk(ChunkPos pos) {
    auto it = chunks.find(pos);
    if (it != chunks.end())
        return it->second;

    Chunk chunk(pos);
    worldGen.generateChunk(chunk);
    chunks.emplace(pos, std::move(chunk));
    return chunks.at(pos);
}

bool ChunkManager::hasChunk(ChunkPos pos) const {
    return chunks.find(pos) != chunks.end();
}

void ChunkManager::updateLoadedChunks(float worldX, float worldY,
                                       float tileSize, int renderDistance) {
    int32_t camChunkX = static_cast<int32_t>(
        std::floor(worldX / (tileSize * CHUNK_SIZE)));
    int32_t camChunkY = static_cast<int32_t>(
        std::floor(worldY / (tileSize * CHUNK_SIZE)));

    for (int dy = -renderDistance; dy <= renderDistance; dy++)
        for (int dx = -renderDistance; dx <= renderDistance; dx++) {
            ChunkPos pos(camChunkX + dx, camChunkY + dy);
            if (!hasChunk(pos))
                getChunk(pos);
        }

    unloadDistantChunks(ChunkPos(camChunkX, camChunkY), renderDistance + 2);
}

void ChunkManager::unloadDistantChunks(ChunkPos center, int renderDistance) {
    // Also cleanup GPU data for unloaded chunks
    auto it = chunks.begin();
    while (it != chunks.end()) {
        int32_t dx = std::abs(it->first.x - center.x);
        int32_t dy = std::abs(it->first.y - center.y);
        if (dx > renderDistance || dy > renderDistance)
            it = chunks.erase(it);
        else
            ++it;
    }
}