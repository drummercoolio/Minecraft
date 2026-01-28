#pragma once
#include "block.h"
#include <array>
#include <cstdint>

constexpr int CHUNK_W = 16;
constexpr int CHUNK_H = 256;
constexpr int CHUNK_D = 16;
constexpr int CHUNK_VOLUME = CHUNK_W * CHUNK_H * CHUNK_D;

class Chunk {
public:
    int cx, cz; // chunk coordinates
    std::array<BlockType, CHUNK_VOLUME> blocks;
    bool dirty = true;

    Chunk(int cx, int cz);

    BlockType getBlock(int x, int y, int z) const;
    void setBlock(int x, int y, int z, BlockType type);

    static int index(int x, int y, int z) {
        return y * (CHUNK_W * CHUNK_D) + z * CHUNK_W + x;
    }

    static bool inBounds(int x, int y, int z) {
        return x >= 0 && x < CHUNK_W && y >= 0 && y < CHUNK_H && z >= 0 && z < CHUNK_D;
    }
};
