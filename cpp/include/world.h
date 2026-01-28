#pragma once
#include "chunk.h"
#include "chunk_mesh.h"
#include <unordered_map>
#include <memory>
#include <glm/glm.hpp>

struct PairHash {
    size_t operator()(const std::pair<int,int>& p) const {
        return std::hash<long long>()(((long long)p.first << 32) | (unsigned int)p.second);
    }
};

class World {
public:
    int seed;
    int renderDistance = 8;

    World(int seed = 42);

    void update(const glm::vec3& playerPos);
    void render();

    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);

    Chunk* getChunk(int cx, int cz);
    const Chunk* getChunk(int cx, int cz) const;

    int getHeight(int x, int z) const;
    void placeStructures();

private:
    bool structuresPlaced_ = false;
    void placeTree(int x, int y, int z, int height);
    void fillRect(int x1, int y, int z1, int x2, int z2, BlockType type);
    void fillBox(int x1, int y1, int z1, int x2, int y2, int z2, BlockType type);
    void hollowBox(int x1, int y1, int z1, int x2, int y2, int z2, BlockType wall, BlockType inside);
    std::unordered_map<std::pair<int,int>, std::unique_ptr<Chunk>, PairHash> chunks_;
    std::unordered_map<std::pair<int,int>, ChunkMesh, PairHash> meshes_;

    void generateChunk(int cx, int cz);
    void generateTerrain(Chunk& chunk);
    void rebuildMesh(int cx, int cz);
};
