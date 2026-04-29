#ifndef TILE_H
#define TILE_H

#include <cstdint>
#include <cstddef>
#include <functional>

static const int8_t CHUNK_SIZE = 32;

// =====================
// TILE FLAGS
// =====================
static const int8_t TILE_SOLID      = 0x01; // 00000001 — blocks movement
static const int8_t TILE_WALKABLE   = 0x02; // 00000010 — can walk on
static const int8_t TILE_RESOURCE   = 0x04; // 00000100 — has mineable resource
static const int8_t TILE_WATER      = 0x08; // 00001000 — is water
static const int8_t TILE_BUILDING   = 0x10; // 00010000 — has a building on it
static const int8_t TILE_RESERVED1  = 0x20; // 00100000 — free for future use
static const int8_t TILE_RESERVED2  = 0x40; // 01000000 — free for future use
static const int8_t TILE_RESERVED3  = 0x80; // 10000000 — free for future use

// =====================
// TILE TYPES
// =====================
enum class TileType : int8_t {
    NONE     = 0,
    GRASS    = 1,
    STONE    = 2,
    WATER    = 3,
    SAND     = 4,
    ORE_IRON = 5,
    ORE_COAL = 6,
    ORE_COPPER = 7,
};

// =====================
// TILE
// =====================
struct Tile {
    TileType type     = TileType::NONE;
    int8_t   flags    = 0;
    int8_t   resource = 0; // resource amount 0-127

    // --- Flag helpers ---
    void   setFlag(int8_t flag)           { flags |=  flag; }
    void   unsetFlag(int8_t flag)         { flags &= ~flag; }
    void   toggleFlag(int8_t flag)        { flags ^=  flag; }
    bool   hasFlag(int8_t flag)     const { return flags & flag; }

    // --- Convenience ---
    bool isSolid()      const { return hasFlag(TILE_SOLID);    }
    bool isWalkable()   const { return hasFlag(TILE_WALKABLE); }
    bool hasResource()  const { return hasFlag(TILE_RESOURCE); }
    bool isWater()      const { return hasFlag(TILE_WATER);    }
    bool hasBuilding()  const { return hasFlag(TILE_BUILDING); }
};

// =====================
// POSITIONS
// =====================

// Position of a tile WITHIN a chunk (0 to CHUNK_SIZE-1)
struct TilePos {
    int8_t x = 0;
    int8_t y = 0;

    TilePos() = default;
    TilePos(int8_t x, int8_t y) : x(x), y(y) {}

    bool isValid() const {
        return x >= 0 && x < CHUNK_SIZE &&
               y >= 0 && y < CHUNK_SIZE;
    }

    bool operator==(const TilePos& o) const { return x == o.x && y == o.y; }
    bool operator!=(const TilePos& o) const { return !(*this == o); }
};

// Position of a chunk IN the world
struct ChunkPos {
    int32_t x = 0;
    int32_t y = 0;

    ChunkPos() = default;
    ChunkPos(int32_t x, int32_t y) : x(x), y(y) {}

    bool operator==(const ChunkPos& o) const { return x == o.x && y == o.y; }
    bool operator!=(const ChunkPos& o) const { return !(*this == o); }

    // Neighbors
    ChunkPos north() const { return { x,     y + 1 }; }
    ChunkPos south() const { return { x,     y - 1 }; }
    ChunkPos east()  const { return { x + 1, y     }; }
    ChunkPos west()  const { return { x - 1, y     }; }
};

// Hash for ChunkPos so it can be used in unordered_map
struct ChunkPosHash {
    size_t operator()(const ChunkPos& p) const { // <-- const here
        return std::hash<int64_t>()(((int64_t)p.x << 32) | (uint32_t)p.y);
    }
};

// Full world position = chunk + tile
struct WorldPos {
    ChunkPos chunk;
    TilePos  tile;

    WorldPos() = default;
    WorldPos(ChunkPos chunk, TilePos tile) : chunk(chunk), tile(tile) {}

    // Absolute tile coordinates in the world
    int32_t absX() const { return chunk.x * CHUNK_SIZE + tile.x; }
    int32_t absY() const { return chunk.y * CHUNK_SIZE + tile.y; }

    // World position in pixels
    float worldX(float tileSize) const { return absX() * tileSize; }
    float worldY(float tileSize) const { return absY() * tileSize; }

    // Build from absolute tile coordinates
    static WorldPos fromAbs(int32_t ax, int32_t ay) {
        WorldPos wp;
        // Arithmetic right shift for negative numbers
        wp.chunk.x = ax >> 5; // divide by CHUNK_SIZE (32)
        wp.chunk.y = ay >> 5;
        wp.tile.x  = ax & (CHUNK_SIZE - 1); // modulo 32
        wp.tile.y  = ay & (CHUNK_SIZE - 1);
        return wp;
    }

    bool operator==(const WorldPos& o) const {
        return chunk == o.chunk && tile == o.tile;
    }
};

// =====================
// CHUNK
// =====================
struct Chunk {
    ChunkPos pos;
    Tile     tiles[CHUNK_SIZE][CHUNK_SIZE];
    bool     dirty    = true;  // needs re-upload to GPU
    bool     generated = false; // has been world-gen'd

    Chunk() = default;
    explicit Chunk(ChunkPos pos) : pos(pos) {}

    // --- Tile access ---
    Tile& getTile(TilePos p)             { return tiles[p.y][p.x]; }
    const Tile& getTile(TilePos p) const { return tiles[p.y][p.x]; }

    Tile& getTile(int8_t x, int8_t y)             { return tiles[y][x]; }
    const Tile& getTile(int8_t x, int8_t y) const { return tiles[y][x]; }

    // --- Fill entire chunk with a tile type ---
    void fill(TileType type, int8_t flags = 0) {
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                tiles[y][x].type  = type;
                tiles[y][x].flags = flags;
            }
    }
};

#endif //TILE_H