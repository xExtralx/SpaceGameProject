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
    NONE            = 0,

    // Water
    WARM_WATER      = 1,
    WATER           = 2,
    DEEP_WATER      = 3,

    // Ground
    GRASSY_ROCKS    = 4,
    GRASS           = 5,
    GRASS_ALT       = 6,

    // Desert
    DESERT_GRASS    = 7,
    DESERT_GRASS_2  = 8,
    DESERT_GRASS_3  = 9,
    DESERT_GRASS_4  = 10,

    // Ores
    COPPER_ORE      = 11,
    IRON_ORE        = 12,
    AMETHYST        = 13,
};

// =====================
// TILE
// =====================
struct Tile {
    TileType type      = TileType::NONE;
    int8_t   flags     = 0;
    int8_t   resource  = 0;     // resource amount 0-127
    float    elevation = 0.0f;  // raw elevation from noise

    // --- Flag helpers ---
    void   setFlag(int8_t flag)        { flags |=  flag; }
    void   unsetFlag(int8_t flag)      { flags &= ~flag; }
    void   toggleFlag(int8_t flag)     { flags ^=  flag; }
    bool   hasFlag(int8_t flag) const  { return flags & flag; }

    // --- Convenience ---
    bool isSolid()     const { return hasFlag(TILE_SOLID);    }
    bool isWalkable()  const { return hasFlag(TILE_WALKABLE); }
    bool hasResource() const { return hasFlag(TILE_RESOURCE); }
    bool isWater()     const { return hasFlag(TILE_WATER);    }
    bool hasBuilding() const { return hasFlag(TILE_BUILDING); }

    // --- Default flags for type ---
    static int8_t defaultFlags(TileType t) {
        switch (t) {
            case TileType::WARM_WATER:
            case TileType::WATER:
            case TileType::DEEP_WATER:
                return TILE_WATER;

            case TileType::GRASS:
            case TileType::GRASS_ALT:
            case TileType::GRASSY_ROCKS:
            case TileType::DESERT_GRASS:
            case TileType::DESERT_GRASS_2:
            case TileType::DESERT_GRASS_3:
            case TileType::DESERT_GRASS_4:
                return TILE_WALKABLE;

            case TileType::COPPER_ORE:
            case TileType::IRON_ORE:
            case TileType::AMETHYST:
                return TILE_SOLID | TILE_RESOURCE;

            default:
                return 0;
        }
    }
};

// =====================
// POSITIONS
// =====================
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

struct ChunkPos {
    int32_t x = 0;
    int32_t y = 0;

    ChunkPos() = default;
    ChunkPos(int32_t x, int32_t y) : x(x), y(y) {}

    bool operator==(const ChunkPos& o) const { return x == o.x && y == o.y; }
    bool operator!=(const ChunkPos& o) const { return !(*this == o); }

    ChunkPos north() const { return { x,     y + 1 }; }
    ChunkPos south() const { return { x,     y - 1 }; }
    ChunkPos east()  const { return { x + 1, y     }; }
    ChunkPos west()  const { return { x - 1, y     }; }
};

struct ChunkPosHash {
    size_t operator()(const ChunkPos& p) const {
        return std::hash<int64_t>()(((int64_t)p.x << 32) | (uint32_t)p.y);
    }
};

struct WorldPos {
    ChunkPos chunk;
    TilePos  tile;

    WorldPos() = default;
    WorldPos(ChunkPos chunk, TilePos tile) : chunk(chunk), tile(tile) {}

    int32_t absX() const { return chunk.x * CHUNK_SIZE + tile.x; }
    int32_t absY() const { return chunk.y * CHUNK_SIZE + tile.y; }

    float worldX(float tileSize) const { return absX() * tileSize; }
    float worldY(float tileSize) const { return absY() * tileSize; }

    static WorldPos fromAbs(int32_t ax, int32_t ay) {
        WorldPos wp;
        wp.chunk.x = ax >> 5;
        wp.chunk.y = ay >> 5;
        wp.tile.x  = static_cast<int8_t>(ax & (CHUNK_SIZE - 1));
        wp.tile.y  = static_cast<int8_t>(ay & (CHUNK_SIZE - 1));
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
    bool     dirty     = true;
    bool     generated = false;

    Chunk() = default;
    explicit Chunk(ChunkPos pos) : pos(pos) {}

    Tile&       getTile(TilePos p)             { return tiles[p.y][p.x]; }
    const Tile& getTile(TilePos p)       const { return tiles[p.y][p.x]; }
    Tile&       getTile(int8_t x, int8_t y)             { return tiles[y][x]; }
    const Tile& getTile(int8_t x, int8_t y)       const { return tiles[y][x]; }

    void fill(TileType type, int8_t flags = 0) {
        for (int y = 0; y < CHUNK_SIZE; y++)
            for (int x = 0; x < CHUNK_SIZE; x++) {
                tiles[y][x].type  = type;
                tiles[y][x].flags = flags == 0
                    ? Tile::defaultFlags(type)
                    : flags;
            }
    }
};

// =====================
// TILESET UV
// =====================

static constexpr float ATLAS_W    = 768.0f;
static constexpr float ATLAS_H    = 3840.0f; // 128 * 30 rows
static constexpr float TILE_UV_W  = 256.0f;
static constexpr float TILE_UV_H  = 128.0f;
static constexpr int   ATLAS_COLS = 3;
static constexpr int   ATLAS_ROWS = 30;

struct TileUV {
    float u, v; // top-left
    float w, h; // size
};

// x = column from left (1-based), y = row from bottom (1-based)
inline TileUV tileUVFromGrid(int x, int y) {
    int row = ATLAS_ROWS - y; // convert bottom-based to top-based
    int col = x - 1;

    return {
        (col * TILE_UV_W) / ATLAS_W,
        (row * TILE_UV_H) / ATLAS_H,
        TILE_UV_W / ATLAS_W,
        TILE_UV_H / ATLAS_H
    };
}

inline TileUV getUVForType(TileType type) {
    switch (type) {
        case TileType::WARM_WATER:     return tileUVFromGrid(3, 1);
        case TileType::WATER:          return tileUVFromGrid(1, 6);
        case TileType::DEEP_WATER:     return tileUVFromGrid(2, 6);
        case TileType::GRASSY_ROCKS:   return tileUVFromGrid(1, 7);
        case TileType::GRASS:          return tileUVFromGrid(1, 21);
        case TileType::GRASS_ALT:      return tileUVFromGrid(2, 21);
        case TileType::DESERT_GRASS:   return tileUVFromGrid(1, 22);
        case TileType::DESERT_GRASS_2: return tileUVFromGrid(2, 22);
        case TileType::DESERT_GRASS_3: return tileUVFromGrid(1, 23);
        case TileType::DESERT_GRASS_4: return tileUVFromGrid(3, 23);
        case TileType::COPPER_ORE:     return tileUVFromGrid(1, 8);
        case TileType::IRON_ORE:       return tileUVFromGrid(2, 8);
        case TileType::AMETHYST:       return tileUVFromGrid(3, 8);
        default:                       return tileUVFromGrid(1, 21);
    }
}

#endif // TILE_H