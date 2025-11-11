#ifndef TILE_H
#define TILE_H

// 🔹 Structure de base : une seule tuile
typedef struct {
    int type;             // ID de la tuile (ex : herbe, mur, etc.)
    int textureID;        // si tu veux un atlas complet, garde-le dans TileMap (inutile ici)
    int x, y;             // position en tuiles (dans le layer)
    int width, height;    // taille d'une tile (généralement fixe -> peut venir de TileMap)
    
    // --- Collisions ---
    bool collision;       // ou "isSolid" suffit, pas besoin des deux
    bool isSolid;

    // --- Animation ---
    bool isAnimated;
    int animationFrame;
    int animationSpeed;
    int animationTimer;
    int animationMaxFrames;
} Tile;

// 🔹 Une couche (sol, objets, collisions, etc.)
typedef struct {
    Tile* tiles;          // tableau width * height
    int width;
    int height;
} TileLayer;

// 🔹 Un chunk (portion du monde)
typedef struct {
    TileLayer* layers;    // tableau de couches (ex: sol, décor, collisions)
    int layerCount;
    int width;            // en nombre de tiles
    int height;
    int chunkX;           // position dans la map
    int chunkY;
    bool loaded;          // utile pour le streaming
} Chunk;

// 🔹 La map complète
typedef struct {
    Chunk* chunks;        // tableau [chunkWidth * chunkHeight]
    int chunkWidth;       // nombre de chunks horizontaux
    int chunkHeight;      // nombre de chunks verticaux
    int tileWidth;        // taille d'une tuile en pixels
    int tileHeight;
    int tileCount;        // nombre total de types de tiles (dans la texture)
    
    unsigned int textureID;   // OpenGL texture (atlas)
} TileMap;

class TileManager {
public:
    TileManager();
    ~TileManager();

    void createTileMap(int chunkWidth, int chunkHeight, int tileWidth, int tileHeight, int tileCount);
    void loadTileMap(const char* path);
    void unloadTileMap();

    void drawTileMap();
    void updateTileMap(float dt);

    void setTile(int x, int y, int layer, int tile);
    void setTile(int x, int y, int tile);

    void setTileMapIndex(int index);
    void removeTileMap(int index);
    void addTileMap(TileMap* tileMap);

    TileMap* getTileMap();
private:
    TileMap* tileMap;
    int tileMapIndex;
    int tileMapCount;


}

#endif // TILE_H
