#include "block.h"
#include <array>
#include <cstddef>

// Texture atlas is 16x16 tiles. Each block face maps to (col, row).
// We'll generate a procedural texture atlas, so these are logical IDs.
static const std::array<BlockDef, (size_t)BlockType::COUNT> BLOCK_DEFS = [] {
    std::array<BlockDef, (size_t)BlockType::COUNT> defs{};

    auto set = [&](BlockType t, bool solid, bool transparent,
                    int tx, int ty) {
        auto& d = defs[(size_t)t];
        d.solid = solid;
        d.transparent = transparent;
        for (int i = 0; i < 6; i++) { d.texX[i] = tx; d.texY[i] = ty; }
    };

    auto setFaces = [&](BlockType t, bool solid, bool transparent,
                         int sideX, int sideY, int topX, int topY, int botX, int botY) {
        auto& d = defs[(size_t)t];
        d.solid = solid;
        d.transparent = transparent;
        // +X, -X, +Z, -Z = sides; +Y = top; -Y = bottom
        d.texX[0] = sideX; d.texY[0] = sideY;
        d.texX[1] = sideX; d.texY[1] = sideY;
        d.texX[2] = topX;  d.texY[2] = topY;
        d.texX[3] = botX;  d.texY[3] = botY;
        d.texX[4] = sideX; d.texY[4] = sideY;
        d.texX[5] = sideX; d.texY[5] = sideY;
    };

    set(BlockType::AIR,         false, true,  0, 0);
    setFaces(BlockType::GRASS,  true, false,  1, 0,  2, 0,  3, 0); // side, top(grass), bot(dirt)
    set(BlockType::DIRT,        true, false,  3, 0);
    set(BlockType::STONE,       true, false,  4, 0);
    set(BlockType::SAND,        true, false,  5, 0);
    set(BlockType::WATER,       false, true,  6, 0);
    setFaces(BlockType::WOOD,   true, false,  7, 0,  8, 0,  8, 0); // side(bark), top/bot(rings)
    set(BlockType::LEAVES,      true, true,   9, 0);
    set(BlockType::BEDROCK,     true, false, 10, 0);
    set(BlockType::GRAVEL,      true, false, 11, 0);
    set(BlockType::IRON_ORE,    true, false, 12, 0);
    set(BlockType::COAL_ORE,    true, false, 13, 0);
    set(BlockType::GOLD_ORE,    true, false, 14, 0);
    set(BlockType::DIAMOND_ORE, true, false, 15, 0);
    setFaces(BlockType::SNOW,   true, false,  0, 1,  1, 1,  3, 0); // side(snow+dirt), top(snow), bot(dirt)
    set(BlockType::SANDSTONE,   true, false,  2, 1);
    set(BlockType::CLAY,        true, false,  3, 1);
    set(BlockType::COBBLESTONE, true, false,  4, 1);
    set(BlockType::PLANKS,      true, false,  5, 1);
    set(BlockType::GLASS,       true, true,   6, 1);
    set(BlockType::LAVA,        false, true,  7, 1);
    setFaces(BlockType::CACTUS, true, true,   8, 1,  9, 1,  9, 1);

    return defs;
}();

const BlockDef& getBlockDef(BlockType type) {
    return BLOCK_DEFS[(size_t)type];
}

bool blockIsSolid(BlockType type) {
    return BLOCK_DEFS[(size_t)type].solid;
}

bool blockIsTransparent(BlockType type) {
    return BLOCK_DEFS[(size_t)type].transparent;
}
