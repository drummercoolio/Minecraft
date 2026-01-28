#include "chunk.h"

Chunk::Chunk(int cx, int cz) : cx(cx), cz(cz) {
    blocks.fill(BlockType::AIR);
}

BlockType Chunk::getBlock(int x, int y, int z) const {
    if (!inBounds(x, y, z)) return BlockType::AIR;
    return blocks[index(x, y, z)];
}

void Chunk::setBlock(int x, int y, int z, BlockType type) {
    if (inBounds(x, y, z)) {
        blocks[index(x, y, z)] = type;
        dirty = true;
    }
}
