#include "world.h"
#include <FastNoiseLite.h>
#include <cmath>
#include <algorithm>

World::World(int seed) : seed(seed) {}

Chunk* World::getChunk(int cx, int cz) {
    auto it = chunks_.find({cx, cz});
    return it != chunks_.end() ? it->second.get() : nullptr;
}

const Chunk* World::getChunk(int cx, int cz) const {
    auto it = chunks_.find({cx, cz});
    return it != chunks_.end() ? it->second.get() : nullptr;
}

BlockType World::getBlock(int x, int y, int z) const {
    if (y < 0 || y >= CHUNK_H) return BlockType::AIR;
    int cx = (x >= 0) ? x / CHUNK_W : (x - CHUNK_W + 1) / CHUNK_W;
    int cz = (z >= 0) ? z / CHUNK_D : (z - CHUNK_D + 1) / CHUNK_D;
    int lx = ((x % CHUNK_W) + CHUNK_W) % CHUNK_W;
    int lz = ((z % CHUNK_D) + CHUNK_D) % CHUNK_D;
    const Chunk* c = getChunk(cx, cz);
    return c ? c->getBlock(lx, y, lz) : BlockType::AIR;
}

void World::setBlock(int x, int y, int z, BlockType type) {
    if (y < 0 || y >= CHUNK_H) return;
    int cx = (x >= 0) ? x / CHUNK_W : (x - CHUNK_W + 1) / CHUNK_W;
    int cz = (z >= 0) ? z / CHUNK_D : (z - CHUNK_D + 1) / CHUNK_D;
    int lx = ((x % CHUNK_W) + CHUNK_W) % CHUNK_W;
    int lz = ((z % CHUNK_D) + CHUNK_D) % CHUNK_D;
    Chunk* c = getChunk(cx, cz);
    if (c) {
        c->setBlock(lx, y, lz, type);
    }
}

int World::getHeight(int x, int z) const {
    for (int y = CHUNK_H - 1; y >= 0; y--) {
        BlockType b = getBlock(x, y, z);
        if (b != BlockType::AIR && b != BlockType::WATER)
            return y;
    }
    return 0;
}

void World::generateTerrain(Chunk& chunk) {
    FastNoiseLite noise;
    noise.SetSeed(seed);
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(6);
    noise.SetFrequency(0.005f);

    FastNoiseLite biomeNoise;
    biomeNoise.SetSeed(seed + 1);
    biomeNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    biomeNoise.SetFrequency(0.002f);

    FastNoiseLite caveNoise;
    caveNoise.SetSeed(seed + 2);
    caveNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    caveNoise.SetFrequency(0.03f);

    FastNoiseLite oreNoise;
    oreNoise.SetSeed(seed + 3);
    oreNoise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
    oreNoise.SetFrequency(0.1f);

    int baseX = chunk.cx * CHUNK_W;
    int baseZ = chunk.cz * CHUNK_D;

    for (int lz = 0; lz < CHUNK_D; lz++) {
        for (int lx = 0; lx < CHUNK_W; lx++) {
            int wx = baseX + lx;
            int wz = baseZ + lz;

            float h = noise.GetNoise((float)wx, (float)wz);       // -1..1
            float biome = biomeNoise.GetNoise((float)wx, (float)wz); // -1..1

            // Determine biome type and height
            int seaLevel = 62;
            int baseHeight;
            BlockType surface, subsurface;

            if (biome < -0.2f) {
                // Plains
                baseHeight = 64 + (int)((h + 1.0f) * 4);
                surface = BlockType::GRASS;
                subsurface = BlockType::DIRT;
            } else if (biome < 0.15f) {
                // Forest
                baseHeight = 65 + (int)((h + 1.0f) * 5);
                surface = BlockType::GRASS;
                subsurface = BlockType::DIRT;
            } else if (biome < 0.4f) {
                // Desert
                baseHeight = 66 + (int)((h + 1.0f) * 3);
                surface = BlockType::SAND;
                subsurface = BlockType::SANDSTONE;
            } else {
                // Tundra
                baseHeight = 63 + (int)((h + 1.0f) * 3);
                surface = BlockType::SNOW;
                subsurface = BlockType::DIRT;
            }

            for (int y = 0; y < CHUNK_H; y++) {
                BlockType block = BlockType::AIR;

                if (y == 0) {
                    block = BlockType::BEDROCK;
                } else if (y < baseHeight - 4) {
                    block = BlockType::STONE;

                    // Caves
                    float cave = caveNoise.GetNoise((float)wx, (float)y, (float)wz);
                    if (cave > 0.6f && y > 5) {
                        block = (y < 11) ? BlockType::LAVA : BlockType::AIR;
                    } else {
                        // Ores
                        float ore = oreNoise.GetNoise((float)wx, (float)y, (float)wz);
                        if (ore > 0.85f) {
                            if (y < 16) block = BlockType::DIAMOND_ORE;
                            else if (y < 32) block = BlockType::GOLD_ORE;
                            else if (y < 64) block = BlockType::IRON_ORE;
                            else block = BlockType::COAL_ORE;
                        }
                    }
                } else if (y < baseHeight) {
                    block = subsurface;
                } else if (y == baseHeight) {
                    block = surface;
                } else if (y <= seaLevel && baseHeight < seaLevel) {
                    block = BlockType::WATER;
                }

                chunk.setBlock(lx, y, lz, block);
            }

            // Trees in forest biome
            if (biome >= -0.1f && biome < 0.15f && surface == BlockType::GRASS) {
                if (((wx * 13 + wz * 7 + seed) % 37) == 0 && lx > 1 && lx < 14 && lz > 1 && lz < 14) {
                    int treeBase = baseHeight + 1;
                    int treeHeight = 5 + ((wx * 3 + wz * 5 + seed) % 3);
                    for (int ty = treeBase; ty < treeBase + treeHeight; ty++) {
                        chunk.setBlock(lx, ty, lz, BlockType::WOOD);
                    }
                    // Leaves
                    int leafStart = treeBase + treeHeight - 3;
                    for (int ly = leafStart; ly <= treeBase + treeHeight; ly++) {
                        int r = (ly < treeBase + treeHeight) ? 2 : 1;
                        for (int dx = -r; dx <= r; dx++) {
                            for (int dz = -r; dz <= r; dz++) {
                                if (dx == 0 && dz == 0 && ly < treeBase + treeHeight) continue;
                                int nx = lx + dx, nz = lz + dz;
                                if (nx >= 0 && nx < CHUNK_W && nz >= 0 && nz < CHUNK_D) {
                                    if (chunk.getBlock(nx, ly, nz) == BlockType::AIR)
                                        chunk.setBlock(nx, ly, nz, BlockType::LEAVES);
                                }
                            }
                        }
                    }
                }
            }

            // Cacti in desert
            if (biome >= 0.15f && biome < 0.4f && surface == BlockType::SAND) {
                if (((wx * 17 + wz * 11 + seed) % 61) == 0) {
                    int cactusH = 2 + ((wx + wz + seed) % 2);
                    for (int cy = baseHeight + 1; cy <= baseHeight + cactusH; cy++) {
                        chunk.setBlock(lx, cy, lz, BlockType::CACTUS);
                    }
                }
            }
        }
    }
}

void World::generateChunk(int cx, int cz) {
    auto chunk = std::make_unique<Chunk>(cx, cz);
    generateTerrain(*chunk);
    chunks_[{cx, cz}] = std::move(chunk);
}

void World::rebuildMesh(int cx, int cz) {
    Chunk* chunk = getChunk(cx, cz);
    if (!chunk) return;

    auto lookup = [this](int x, int y, int z) -> BlockType {
        return this->getBlock(x, y, z);
    };

    std::vector<float> vertices;
    buildChunkMesh(*chunk, lookup, vertices);

    auto& mesh = meshes_[{cx, cz}];
    mesh.upload(vertices);
    chunk->dirty = false;
}

void World::update(const glm::vec3& playerPos) {
    int pcx = (int)floor(playerPos.x / CHUNK_W);
    int pcz = (int)floor(playerPos.z / CHUNK_D);

    // Generate missing chunks
    for (int dx = -renderDistance; dx <= renderDistance; dx++) {
        for (int dz = -renderDistance; dz <= renderDistance; dz++) {
            int cx = pcx + dx, cz = pcz + dz;
            if (!getChunk(cx, cz)) {
                generateChunk(cx, cz);
            }
        }
    }

    // Rebuild dirty meshes (limit per frame)
    int rebuilt = 0;
    for (int dx = -renderDistance; dx <= renderDistance; dx++) {
        for (int dz = -renderDistance; dz <= renderDistance; dz++) {
            int cx = pcx + dx, cz = pcz + dz;
            Chunk* c = getChunk(cx, cz);
            if (c && c->dirty) {
                rebuildMesh(cx, cz);
                if (++rebuilt >= 4) return; // limit rebuilds per frame
            }
        }
    }
}

void World::render() {
    for (auto& [key, mesh] : meshes_) {
        mesh.draw();
    }
}

// --- Structure helpers ---

void World::fillRect(int x1, int y, int z1, int x2, int z2, BlockType type) {
    for (int x = x1; x <= x2; x++)
        for (int z = z1; z <= z2; z++)
            setBlock(x, y, z, type);
}

void World::fillBox(int x1, int y1, int z1, int x2, int y2, int z2, BlockType type) {
    for (int y = y1; y <= y2; y++)
        fillRect(x1, y, z1, x2, z2, type);
}

void World::hollowBox(int x1, int y1, int z1, int x2, int y2, int z2,
                      BlockType wall, BlockType inside) {
    for (int y = y1; y <= y2; y++) {
        for (int x = x1; x <= x2; x++) {
            for (int z = z1; z <= z2; z++) {
                bool edge = (x == x1 || x == x2 || z == z1 || z == z2 ||
                             y == y1 || y == y2);
                setBlock(x, y, z, edge ? wall : inside);
            }
        }
    }
}

void World::placeTree(int x, int y, int z, int height) {
    // Trunk
    for (int ty = y; ty < y + height; ty++)
        setBlock(x, ty, z, BlockType::WOOD);
    // Leaf canopy
    int leafBase = y + height - 3;
    for (int ly = leafBase; ly <= y + height + 1; ly++) {
        int r = (ly <= y + height - 1) ? 3 : (ly == y + height) ? 2 : 1;
        for (int dx = -r; dx <= r; dx++) {
            for (int dz = -r; dz <= r; dz++) {
                if (dx == 0 && dz == 0 && ly < y + height) continue;
                if (abs(dx) == r && abs(dz) == r) continue; // round corners
                if (getBlock(x + dx, ly, z + dz) == BlockType::AIR)
                    setBlock(x + dx, ly, z + dz, BlockType::LEAVES);
            }
        }
    }
}

void World::placeStructures() {
    if (structuresPlaced_) return;
    structuresPlaced_ = true;

    // Ground level
    const int G = 64;

    // === Flatten the property area: roughly -50..50 x -50..50 ===
    for (int x = -55; x <= 55; x++) {
        for (int z = -55; z <= 55; z++) {
            // Clear everything above ground
            for (int y = G + 1; y < G + 40; y++)
                setBlock(x, y, z, BlockType::AIR);
            // Set ground
            setBlock(x, G, z, BlockType::GRASS);
            for (int y = G - 1; y >= G - 4; y--)
                setBlock(x, y, z, BlockType::DIRT);
        }
    }

    // ============================================================
    // HOUSE - L-shaped, based on aerial view
    // Main wing runs east-west (X axis), narrower wing extends south (+Z)
    // House center roughly at (5, G+1, -5)
    // ============================================================

    // Main wing: 20 wide (X: -5 to 14), 12 deep (Z: -15 to -4), 6 tall
    int hx1 = -5, hx2 = 14, hz1 = -15, hz2 = -4;
    int hy = G + 1;
    int wallH = 5;

    // Foundation
    fillRect(hx1 - 1, G, hz1 - 1, hx2 + 1, hz2 + 1, BlockType::STONE);

    // Walls (cobblestone) with air inside
    hollowBox(hx1, hy, hz1, hx2, hy + wallH - 1, hz2,
              BlockType::COBBLESTONE, BlockType::AIR);

    // Floor
    fillRect(hx1 + 1, hy, hz1 + 1, hx2 - 1, hz2 - 1, BlockType::PLANKS);

    // South wing (the L extension): 10 wide (X: -5 to 4), 12 deep (Z: -4 to 7)
    int sx1 = -5, sx2 = 4, sz1 = -4, sz2 = 7;
    hollowBox(sx1, hy, sz1, sx2, hy + wallH - 1, sz2,
              BlockType::COBBLESTONE, BlockType::AIR);
    fillRect(sx1 + 1, hy, sz1 + 1, sx2 - 1, sz2 - 1, BlockType::PLANKS);

    // Remove interior wall between wings
    for (int x = sx1 + 1; x <= sx2 - 1; x++)
        for (int y = hy + 1; y < hy + wallH - 1; y++)
            setBlock(x, y, hz2, BlockType::AIR);

    // Windows - main wing (north and south walls)
    for (int x = hx1 + 2; x <= hx2 - 2; x += 3) {
        for (int y = hy + 2; y <= hy + 3; y++) {
            setBlock(x, y, hz1, BlockType::GLASS); // north
            if (x > sx2) // south wall only where no south wing
                setBlock(x, y, hz2, BlockType::GLASS);
        }
    }
    // Windows - south wing (east and west walls)
    for (int z = sz1 + 2; z <= sz2 - 2; z += 3) {
        for (int y = hy + 2; y <= hy + 3; y++) {
            setBlock(sx1, y, z, BlockType::GLASS); // west
            setBlock(sx2, y, z, BlockType::GLASS); // east
        }
    }

    // Door - south wing south wall, centered
    int doorX = (sx1 + sx2) / 2;
    setBlock(doorX, hy + 1, sz2, BlockType::AIR);
    setBlock(doorX, hy + 2, sz2, BlockType::AIR);

    // Roof - main wing (peaked, running east-west)
    for (int x = hx1 - 1; x <= hx2 + 1; x++) {
        for (int layer = 0; layer <= 3; layer++) {
            int rz1 = hz1 - 1 + layer;
            int rz2 = hz2 + 1 - layer;
            int ry = hy + wallH + layer;
            if (rz1 <= rz2) {
                for (int z = rz1; z <= rz2; z++)
                    setBlock(x, ry, z, BlockType::CLAY); // brown-ish roof
            }
        }
    }

    // Roof - south wing (peaked, running north-south)
    for (int z = sz1; z <= sz2 + 1; z++) {
        for (int layer = 0; layer <= 2; layer++) {
            int rx1 = sx1 - 1 + layer;
            int rx2 = sx2 + 1 - layer;
            int ry = hy + wallH + layer;
            if (rx1 <= rx2) {
                for (int x = rx1; x <= rx2; x++) {
                    if (getBlock(x, ry, z) == BlockType::AIR)
                        setBlock(x, ry, z, BlockType::CLAY);
                }
            }
        }
    }

    // ============================================================
    // PATIO / DECK - between house and pool (west side)
    // ============================================================
    int px1 = -18, px2 = -6, pz1 = -10, pz2 = 4;
    fillRect(px1, G, pz1, px2, pz2, BlockType::PLANKS);

    // ============================================================
    // POOL - rectangular, west of patio
    // ============================================================
    int poolX1 = -30, poolX2 = -19, poolZ1 = -8, poolZ2 = 2;
    // Pool walls and floor (stone)
    fillBox(poolX1, G - 3, poolZ1, poolX2, G, poolZ2, BlockType::STONE);
    // Pool interior (water)
    fillBox(poolX1 + 1, G - 2, poolZ1 + 1, poolX2 - 1, G, poolZ2 - 1, BlockType::WATER);
    // Pool edge (sandstone border)
    for (int x = poolX1 - 1; x <= poolX2 + 1; x++) {
        setBlock(x, G, poolZ1 - 1, BlockType::SANDSTONE);
        setBlock(x, G, poolZ2 + 1, BlockType::SANDSTONE);
    }
    for (int z = poolZ1 - 1; z <= poolZ2 + 1; z++) {
        setBlock(poolX1 - 1, G, z, BlockType::SANDSTONE);
        setBlock(poolX2 + 1, G, z, BlockType::SANDSTONE);
    }

    // ============================================================
    // DRIVEWAY - coming from the south, gravel path
    // ============================================================
    for (int z = sz2 + 2; z <= 55; z++) {
        for (int x = -2; x <= 4; x++) {
            setBlock(x, G, z, BlockType::GRAVEL);
        }
    }

    // ============================================================
    // POND - northwest of the house
    // ============================================================
    int pondCX = -25, pondCZ = -30;
    // Irregular pond shape
    for (int dx = -6; dx <= 6; dx++) {
        for (int dz = -5; dz <= 5; dz++) {
            float dist = sqrtf((float)(dx*dx) + (float)(dz*dz) * 1.2f);
            if (dist < 5.5f) {
                int px = pondCX + dx, pz = pondCZ + dz;
                setBlock(px, G, pz, BlockType::WATER);
                setBlock(px, G - 1, pz, BlockType::CLAY);
                setBlock(px, G - 2, pz, BlockType::CLAY);
                // Sandy edges
                if (dist > 4.0f)
                    setBlock(px, G, pz, BlockType::WATER);
            }
            // Sand border
            if (dist >= 5.0f && dist < 6.5f) {
                setBlock(pondCX + dx, G, pondCZ + dz, BlockType::SAND);
            }
        }
    }

    // ============================================================
    // TREES - dense around the property perimeter
    // ============================================================
    // Tree positions forming a border, matching the aerial view
    struct TreePos { int x, z, h; };
    TreePos trees[] = {
        // North tree line
        {-20, -25, 7}, {-12, -28, 8}, {-5, -26, 6}, {3, -25, 7},
        {10, -27, 8}, {18, -24, 7}, {25, -26, 6},
        // Northeast
        {22, -18, 7}, {25, -12, 8}, {28, -6, 7}, {24, -2, 6},
        // East tree line
        {22, 3, 7}, {25, 8, 8}, {23, 14, 7}, {20, 20, 6},
        {22, 26, 7},
        // South tree line (along driveway edges)
        {-10, 15, 6}, {-15, 18, 7}, {-12, 25, 8}, {-8, 30, 7},
        {10, 15, 7}, {14, 20, 6}, {12, 28, 8},
        // West tree line
        {-35, -20, 7}, {-38, -12, 8}, {-36, -5, 7}, {-35, 3, 6},
        {-38, 10, 7}, {-35, 18, 8}, {-36, 25, 7},
        // Northwest (around pond)
        {-35, -28, 6}, {-32, -35, 7}, {-20, -38, 8}, {-15, -35, 7},
        // Fill more around the house
        {-18, -20, 6}, {-20, -15, 7}, {16, -20, 8}, {18, -10, 7},
        {-20, 8, 6}, {-22, 14, 7}, {15, 10, 6}, {18, 15, 7},
        // Extra trees for density
        {-28, -22, 7}, {-30, -8, 6}, {-32, 5, 7}, {-30, 15, 8},
        {28, -20, 6}, {30, -10, 7}, {28, 5, 6}, {26, 15, 7},
        {-8, -35, 7}, {5, -33, 8}, {15, -32, 6},
        {-25, 20, 7}, {-20, 28, 6}, {18, 25, 7},
    };

    for (auto& t : trees) {
        placeTree(t.x, G + 1, t.z, t.h);
    }

    // Mark all chunks as dirty so meshes rebuild
    for (auto& [key, chunk] : chunks_) {
        chunk->dirty = true;
    }
}
