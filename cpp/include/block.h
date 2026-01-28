#pragma once
#include <cstdint>

enum class BlockType : uint8_t {
    AIR = 0,
    STONE,
    DIRT,
    GRASS,
    SAND,
    WATER,
    WOOD,
    LEAVES,
    BEDROCK,
    GRAVEL,
    IRON_ORE,
    COAL_ORE,
    GOLD_ORE,
    DIAMOND_ORE,
    SNOW,
    SANDSTONE,
    CLAY,
    COBBLESTONE,
    PLANKS,
    GLASS,
    LAVA,
    CACTUS,
    COUNT
};

struct BlockDef {
    bool solid;
    bool transparent;
    // Texture atlas coordinates (column, row) for each face
    // Order: +X, -X, +Y, -Y, +Z, -Z
    int texX[6];
    int texY[6];
};

const BlockDef& getBlockDef(BlockType type);
bool blockIsSolid(BlockType type);
bool blockIsTransparent(BlockType type);
