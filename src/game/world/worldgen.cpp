#include "worldgen.h"
#include <cmath>
#include <iostream>

// =====================
// WORLDGEN
// =====================

WorldGen::WorldGen(int32_t seed) : seed(seed) {
    // --- Elevation noise ---
    auto elevSimplex = FastNoise::New<FastNoise::Simplex>();
    elevationNoise   = FastNoise::New<FastNoise::FractalFBm>();
    elevationNoise->SetSource(elevSimplex);
    elevationNoise->SetOctaveCount(6);
    elevationNoise->SetGain(0.5f);
    elevationNoise->SetLacunarity(2.0f);

    // --- Resource noise (separate frequency) ---
    auto resSimplex = FastNoise::New<FastNoise::Simplex>();
    resourceNoise   = FastNoise::New<FastNoise::FractalFBm>();
    resourceNoise->SetSource(resSimplex);
    resourceNoise->SetOctaveCount(3);
    resourceNoise->SetGain(0.6f);
    resourceNoise->SetLacunarity(2.5f);

    // --- Forest noise ---
    auto forSimplex = FastNoise::New<FastNoise::Simplex>();
    forestNoise     = FastNoise::New<FastNoise::FractalFBm>();
    forestNoise->SetSource(forSimplex);
    forestNoise->SetOctaveCount(4);
    forestNoise->SetGain(0.5f);
    forestNoise->SetLacunarity(2.0f);
}

void WorldGen::generateChunk(Chunk& chunk) const {
    const int size = CHUNK_SIZE;
    const int area = size * size;

    // World offset for this chunk
    const int32_t worldOffsetX = chunk.pos.x * size;
    const int32_t worldOffsetY = chunk.pos.y * size;

    // Generate noise maps
    std::vector<float> elevationMap(area);
    std::vector<float> resourceMap(area);
    std::vector<float> forestMap(area);

    elevationNoise->GenUniformGrid2D(
        elevationMap.data(),
        worldOffsetX, worldOffsetY,
        size, size,
        0.008f, 0.008f, // xScale, yScale
        seed
    );

    resourceNoise->GenUniformGrid2D(
        resourceMap.data(),
        worldOffsetX, worldOffsetY,
        size, size,
        0.05f, 0.05f, // xScale, yScale
        seed + 1
    );

    forestNoise->GenUniformGrid2D(
        forestMap.data(),
        worldOffsetX, worldOffsetY,
        size, size,
        0.03f, 0.03f, // xScale, yScale
        seed + 2
    );

    // Fill tiles from noise
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            int idx = y * size + x;

            float elev     = elevationMap[idx];
            float resource = resourceMap[idx];
            float forest   = forestMap[idx];

            Tile& tile = chunk.getTile(x, y);

            // Base type from elevation
            tile.type  = elevationToType(elev);
            tile.flags = flagsForType(tile.type);

            // Skip resource/forest placement on water/sand
            if (tile.type == TileType::WATER ||
                tile.type == TileType::SAND)
                continue;

            // Ore placement — offset resource noise per ore type
            if (resource > IRON_THRESHOLD) {
                tile.type = TileType::ORE_IRON;
                tile.setFlag(TILE_RESOURCE | TILE_SOLID);
                tile.resource = static_cast<int8_t>(
                    (resource - IRON_THRESHOLD) / (1.0f - IRON_THRESHOLD) * 127.0f
                );
            } else if (resource < -COAL_THRESHOLD) {
                tile.type = TileType::ORE_COAL;
                tile.setFlag(TILE_RESOURCE | TILE_SOLID);
                tile.resource = static_cast<int8_t>(
                    (-resource - COAL_THRESHOLD) / (1.0f - COAL_THRESHOLD) * 127.0f
                );
            } else if (resource > COPPER_THRESHOLD * 0.8f &&
                       forest   > COPPER_THRESHOLD * 0.6f) {
                // Copper uses combination of two noise maps
                tile.type = TileType::ORE_COPPER;
                tile.setFlag(TILE_RESOURCE | TILE_SOLID);
                tile.resource = static_cast<int8_t>(resource * 60.0f + 60.0f);
            }
        }
    }

    chunk.generated = true;
    chunk.dirty     = true;
}

TileType WorldGen::elevationToType(float e) const {
    if (e < WATER_LEVEL) return TileType::WATER;
    if (e < SAND_LEVEL)  return TileType::SAND;
    if (e < GRASS_LEVEL) return TileType::GRASS;
    if (e < STONE_LEVEL) return TileType::STONE;
    return TileType::STONE;
}

int8_t WorldGen::flagsForType(TileType type) const {
    switch (type) {
        case TileType::GRASS:      return TILE_WALKABLE;
        case TileType::SAND:       return TILE_WALKABLE;
        case TileType::STONE:      return TILE_WALKABLE | TILE_SOLID;
        case TileType::WATER:      return TILE_WATER;
        case TileType::ORE_IRON:   return TILE_SOLID | TILE_RESOURCE;
        case TileType::ORE_COAL:   return TILE_SOLID | TILE_RESOURCE;
        case TileType::ORE_COPPER: return TILE_SOLID | TILE_RESOURCE;
        default:                   return 0;
    }
}

// =====================
// CHUNK MANAGER
// =====================

ChunkManager::ChunkManager(int32_t seed) : worldGen(seed) {}

Chunk& ChunkManager::getChunk(ChunkPos pos) {
    auto it = chunks.find(pos);
    if (it != chunks.end())
        return it->second;

    // Generate new chunk
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
    // Convert world pixel position to chunk position
    int32_t camChunkX = static_cast<int32_t>(std::floor(worldX / (tileSize * CHUNK_SIZE)));
    int32_t camChunkY = static_cast<int32_t>(std::floor(worldY / (tileSize * CHUNK_SIZE)));

    ChunkPos center(camChunkX, camChunkY);

    // Load chunks in render distance
    for (int dy = -renderDistance; dy <= renderDistance; dy++) {
        for (int dx = -renderDistance; dx <= renderDistance; dx++) {
            ChunkPos pos(camChunkX + dx, camChunkY + dy);
            if (!hasChunk(pos)) {
                getChunk(pos); // generates and caches
            }
        }
    }

    unloadDistantChunks(center, renderDistance + 2); // unload with margin
}

void ChunkManager::unloadDistantChunks(ChunkPos center, int renderDistance) {
    auto it = chunks.begin();
    while (it != chunks.end()) {
        int32_t dx = std::abs(it->first.x - center.x);
        int32_t dy = std::abs(it->first.y - center.y);
        if (dx > renderDistance || dy > renderDistance) {
            it = chunks.erase(it);
        } else {
            ++it;
        }
    }
}