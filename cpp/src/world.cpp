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

    // Property is a long rectangle running west(-X) to east(+X)
    // Layout west to east: POND | open green | POOL | HOUSE | DRIVEWAY -->
    // Property bounds: X: -80 to 80, Z: -25 to 25
    int propX1 = -80, propX2 = 80, propZ1 = -25, propZ2 = 25;

    // === Flatten the property ===
    for (int x = propX1 - 5; x <= propX2 + 5; x++) {
        for (int z = propZ1 - 5; z <= propZ2 + 5; z++) {
            for (int y = G + 1; y < G + 40; y++)
                setBlock(x, y, z, BlockType::AIR);
            setBlock(x, G, z, BlockType::GRASS);
            for (int y = G - 1; y >= G - 4; y--)
                setBlock(x, y, z, BlockType::DIRT);
        }
    }

    // ============================================================
    // POND - spans full width (Z) at the west end
    // X: -75 to -55, Z: -22 to 22
    // ============================================================
    int pondX1 = -75, pondX2 = -55, pondZ1 = -22, pondZ2 = 22;
    // Irregular pond with organic edges
    for (int x = pondX1 - 2; x <= pondX2 + 2; x++) {
        for (int z = pondZ1 - 2; z <= pondZ2 + 2; z++) {
            // Normalize to ellipse
            float nx = (float)(x - (pondX1 + pondX2) / 2) / ((pondX2 - pondX1) / 2.0f);
            float nz = (float)(z - (pondZ1 + pondZ2) / 2) / ((pondZ2 - pondZ1) / 2.0f);
            float dist = sqrtf(nx * nx + nz * nz);
            if (dist < 0.9f) {
                setBlock(x, G, z, BlockType::WATER);
                setBlock(x, G - 1, z, BlockType::CLAY);
                setBlock(x, G - 2, z, BlockType::CLAY);
                setBlock(x, G - 3, z, BlockType::CLAY);
            } else if (dist < 1.05f) {
                setBlock(x, G, z, BlockType::SAND);
            }
        }
    }

    // ============================================================
    // POOL - between open green and house
    // X: -15 to 0, Z: -8 to 8
    // ============================================================
    int poolX1 = -15, poolX2 = 0, poolZ1 = -8, poolZ2 = 8;
    fillBox(poolX1, G - 3, poolZ1, poolX2, G, poolZ2, BlockType::STONE);
    fillBox(poolX1 + 1, G - 2, poolZ1 + 1, poolX2 - 1, G, poolZ2 - 1, BlockType::WATER);
    // Sandstone border
    for (int x = poolX1 - 1; x <= poolX2 + 1; x++) {
        setBlock(x, G, poolZ1 - 1, BlockType::SANDSTONE);
        setBlock(x, G, poolZ2 + 1, BlockType::SANDSTONE);
    }
    for (int z = poolZ1 - 1; z <= poolZ2 + 1; z++) {
        setBlock(poolX1 - 1, G, z, BlockType::SANDSTONE);
        setBlock(poolX2 + 1, G, z, BlockType::SANDSTONE);
    }

    // Patio/deck between pool and house
    fillRect(1, G, -10, 8, 10, BlockType::PLANKS);

    // ============================================================
    // HOUSE - T-shaped, east of pool (from close-up aerial)
    // Main body runs north-south, cross wing runs east-west
    // Dark asphalt shingle roof
    //
    // Main body (north-south): X: 12 to 28, Z: -14 to 8
    // Cross wing (east-west):  X: 8 to 35, Z: -6 to 2
    // ============================================================
    int hy = G + 1;
    int wallH = 6;
    BlockType roofBlock = BlockType::COAL_ORE; // dark asphalt shingles

    // --- Main body (north-south rectangle) ---
    int mx1 = 12, mx2 = 28, mz1 = -14, mz2 = 8;
    fillRect(mx1 - 1, G, mz1 - 1, mx2 + 1, mz2 + 1, BlockType::STONE);
    hollowBox(mx1, hy, mz1, mx2, hy + wallH - 1, mz2,
              BlockType::COBBLESTONE, BlockType::AIR);
    fillRect(mx1 + 1, hy, mz1 + 1, mx2 - 1, mz2 - 1, BlockType::PLANKS);

    // --- Cross wing (east-west rectangle) ---
    int cwx1 = 8, cwx2 = 35, cwz1 = -6, cwz2 = 2;
    fillRect(cwx1 - 1, G, cwz1 - 1, cwx2 + 1, cwz2 + 1, BlockType::STONE);
    hollowBox(cwx1, hy, cwz1, cwx2, hy + wallH - 1, cwz2,
              BlockType::COBBLESTONE, BlockType::AIR);
    fillRect(cwx1 + 1, hy, cwz1 + 1, cwx2 - 1, cwz2 - 1, BlockType::PLANKS);

    // Remove interior walls at intersection
    for (int x = std::max(mx1, cwx1); x <= std::min(mx2, cwx2); x++) {
        for (int y = hy; y < hy + wallH; y++) {
            // Clear main body walls inside cross wing zone
            if (x > mx1 && x < mx2) {
                setBlock(x, y, cwz1, BlockType::AIR);
                setBlock(x, y, cwz2, BlockType::AIR);
            }
        }
    }
    for (int z = std::max(mz1, cwz1); z <= std::min(mz2, cwz2); z++) {
        for (int y = hy; y < hy + wallH; y++) {
            if (z > cwz1 && z < cwz2) {
                setBlock(mx1, y, z, BlockType::AIR);
                setBlock(mx2, y, z, BlockType::AIR);
            }
        }
    }
    // Re-place floors at intersection
    for (int x = std::max(mx1, cwx1); x <= std::min(mx2, cwx2); x++)
        for (int z = std::max(mz1, cwz1); z <= std::min(mz2, cwz2); z++)
            setBlock(x, hy, z, BlockType::PLANKS);

    // --- Windows: all exterior walls ---
    // Main body north wall
    for (int x = mx1 + 2; x <= mx2 - 2; x += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(x, y, mz1, BlockType::GLASS);
    // Main body south wall
    for (int x = mx1 + 2; x <= mx2 - 2; x += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(x, y, mz2, BlockType::GLASS);
    // Main body east/west walls (skip cross wing zone)
    for (int z = mz1 + 2; z <= mz2 - 2; z += 3) {
        if (z < cwz1 || z > cwz2) {
            for (int y = hy + 2; y <= hy + 3; y++) {
                setBlock(mx1, y, z, BlockType::GLASS);
                setBlock(mx2, y, z, BlockType::GLASS);
            }
        }
    }
    // Cross wing west wall (pool side)
    for (int z = cwz1 + 1; z <= cwz2 - 1; z += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(cwx1, y, z, BlockType::GLASS);
    // Cross wing east wall (driveway side)
    for (int z = cwz1 + 1; z <= cwz2 - 1; z += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(cwx2, y, z, BlockType::GLASS);
    // Cross wing north/south walls (outside main body)
    for (int x = cwx1 + 2; x <= mx1 - 1; x += 3)
        for (int y = hy + 2; y <= hy + 3; y++) {
            setBlock(x, y, cwz1, BlockType::GLASS);
            setBlock(x, y, cwz2, BlockType::GLASS);
        }
    for (int x = mx2 + 1; x <= cwx2 - 2; x += 3)
        for (int y = hy + 2; y <= hy + 3; y++) {
            setBlock(x, y, cwz1, BlockType::GLASS);
            setBlock(x, y, cwz2, BlockType::GLASS);
        }

    // --- Doors ---
    int doorZ = (cwz1 + cwz2) / 2;
    // East door (front, facing driveway)
    setBlock(cwx2, hy + 1, doorZ, BlockType::AIR);
    setBlock(cwx2, hy + 2, doorZ, BlockType::AIR);
    // West door (back, facing pool/patio)
    setBlock(cwx1, hy + 1, doorZ, BlockType::AIR);
    setBlock(cwx1, hy + 2, doorZ, BlockType::AIR);

    // --- Roof: dark asphalt shingles ---
    // Main body roof: ridge runs north-south, peaked east-west
    for (int z = mz1 - 1; z <= mz2 + 1; z++) {
        for (int layer = 0; layer <= 4; layer++) {
            int rx1 = mx1 - 1 + layer;
            int rx2 = mx2 + 1 - layer;
            int ry = hy + wallH + layer;
            if (rx1 <= rx2)
                for (int x = rx1; x <= rx2; x++)
                    setBlock(x, ry, z, roofBlock);
        }
    }
    // Cross wing roof: ridge runs east-west, peaked north-south
    for (int x = cwx1 - 1; x <= cwx2 + 1; x++) {
        for (int layer = 0; layer <= 2; layer++) {
            int rz1 = cwz1 - 1 + layer;
            int rz2 = cwz2 + 1 - layer;
            int ry = hy + wallH + layer;
            if (rz1 <= rz2)
                for (int z = rz1; z <= rz2; z++)
                    setBlock(x, ry, z, roofBlock);
        }
    }

    // ============================================================
    // DRIVEWAY - exits east from the house front door
    // ============================================================
    for (int x = cwx2 + 1; x <= propX2; x++) {
        for (int z = doorZ - 3; z <= doorZ + 3; z++) {
            setBlock(x, G, z, BlockType::GRAVEL);
        }
    }

    // ============================================================
    // TREES - border the property perimeter
    // ============================================================
    struct TreePos { int x, z, h; };
    TreePos trees[] = {
        // North border
        {-70, -24, 7}, {-60, -24, 8}, {-50, -24, 7}, {-40, -24, 6},
        {-30, -24, 8}, {-20, -24, 7}, {-10, -24, 6}, {0, -24, 7},
        {10, -24, 8}, {20, -24, 7}, {30, -24, 6}, {40, -24, 8},
        {50, -24, 7}, {60, -24, 6}, {70, -24, 7},
        // South border
        {-70, 24, 8}, {-60, 24, 7}, {-50, 24, 6}, {-40, 24, 8},
        {-30, 24, 7}, {-20, 24, 6}, {-10, 24, 7}, {0, 24, 8},
        {10, 24, 7}, {20, 24, 6}, {30, 24, 8}, {40, 24, 7},
        {50, 24, 6}, {60, 24, 8}, {70, 24, 7},
        // West border (around pond)
        {-78, -15, 7}, {-78, -5, 8}, {-78, 5, 7}, {-78, 15, 6},
        // Between pond and pool (open green area with scattered trees)
        {-45, -18, 7}, {-38, 16, 8}, {-48, 10, 6}, {-35, -10, 7},
        {-42, 5, 8}, {-30, -16, 6}, {-25, 14, 7},
        // Around house
        {8, -18, 7}, {8, 16, 8}, {25, 16, 7}, {32, 16, 6},
        {32, -18, 8},
        // Along driveway (east)
        {45, -15, 7}, {55, -15, 8}, {65, -15, 7},
        {45, 15, 6}, {55, 15, 7}, {65, 15, 8},
    };

    for (auto& t : trees) {
        placeTree(t.x, G + 1, t.z, t.h);
    }

    // Mark all chunks as dirty so meshes rebuild
    for (auto& [key, chunk] : chunks_) {
        chunk->dirty = true;
    }
}
