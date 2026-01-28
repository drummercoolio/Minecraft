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

    // Property wider N-S, longer E for driveway: X: -80 to 120, Z: -45 to 45
    int propX1 = -80, propX2 = 120, propZ1 = -45, propZ2 = 45;

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
    // X: -75 to -55, Z: -40 to 40
    // ============================================================
    int pondX1 = -75, pondX2 = -55, pondZ1 = -40, pondZ2 = 40;
    for (int x = pondX1 - 2; x <= pondX2 + 2; x++) {
        for (int z = pondZ1 - 2; z <= pondZ2 + 2; z++) {
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
    // X: -15 to 0, Z: -10 to 10
    // ============================================================
    int poolX1 = -8, poolX2 = 0, poolZ1 = -10, poolZ2 = 10;
    fillBox(poolX1, G - 3, poolZ1, poolX2, G, poolZ2, BlockType::STONE);
    fillBox(poolX1 + 1, G - 2, poolZ1 + 1, poolX2 - 1, G, poolZ2 - 1, BlockType::WATER);
    for (int x = poolX1 - 1; x <= poolX2 + 1; x++) {
        setBlock(x, G, poolZ1 - 1, BlockType::SANDSTONE);
        setBlock(x, G, poolZ2 + 1, BlockType::SANDSTONE);
    }
    for (int z = poolZ1 - 1; z <= poolZ2 + 1; z++) {
        setBlock(poolX1 - 1, G, z, BlockType::SANDSTONE);
        setBlock(poolX2 + 1, G, z, BlockType::SANDSTONE);
    }

    // Patio/deck between pool and house
    fillRect(1, G, -12, 8, 12, BlockType::PLANKS);

    // ============================================================
    // HOUSE - L-shape with 3 equal segments (each 12 wide x 20 long)
    //   Seg1 (bottom of L): runs east-west (E-W)
    //   Seg2 (vertical of L): runs north-south (N-S) from west end of seg1, going north
    //   Seg3 (top of L): bends northeast at 45 degrees from top of seg2
    // Orange roof (CLAY)
    // ============================================================
    int hy = G + 1;
    int wallH = 6;
    BlockType roofBlock = BlockType::CLAY;

    int segLen = 20; // length of each segment
    int segW = 12;   // width of each segment

    // --- Seg1: bottom of L, runs east-west ---
    // X: 10 to 29, Z: -6 to 5
    int s1x1 = 10, s1x2 = s1x1 + segLen - 1; // 10 to 29
    int s1z1 = -segW / 2, s1z2 = s1z1 + segW - 1; // -6 to 5

    fillRect(s1x1 - 1, G, s1z1 - 1, s1x2 + 1, s1z2 + 1, BlockType::STONE);
    hollowBox(s1x1, hy, s1z1, s1x2, hy + wallH - 1, s1z2,
              BlockType::COBBLESTONE, BlockType::AIR);
    fillRect(s1x1 + 1, hy, s1z1 + 1, s1x2 - 1, s1z2 - 1, BlockType::PLANKS);

    // Windows on seg1
    for (int x = s1x1 + 2; x <= s1x2 - 2; x += 3) {
        for (int y = hy + 2; y <= hy + 3; y++) {
            setBlock(x, y, s1z1, BlockType::GLASS);
            setBlock(x, y, s1z2, BlockType::GLASS);
        }
    }
    for (int z = s1z1 + 2; z <= s1z2 - 2; z += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(s1x2, y, z, BlockType::GLASS);

    // East door
    int doorZ = (s1z1 + s1z2) / 2;
    setBlock(s1x2, hy + 1, doorZ, BlockType::AIR);
    setBlock(s1x2, hy + 2, doorZ, BlockType::AIR);

    // Seg1 roof: peaked N-S
    for (int x = s1x1 - 1; x <= s1x2 + 1; x++)
        for (int layer = 0; layer <= segW / 2; layer++) {
            int rz1 = s1z1 - 1 + layer;
            int rz2 = s1z2 + 1 - layer;
            int ry = hy + wallH + layer;
            if (rz1 <= rz2)
                for (int z = rz1; z <= rz2; z++)
                    setBlock(x, ry, z, roofBlock);
        }

    // --- Seg2: vertical of L, runs north from west end of seg1 ---
    // Z goes negative = north in Minecraft convention
    // X: s1x1 - segLen + segW to s1x1 + segW - 1 (overlaps seg1 by segW)
    // Actually: seg2 starts at west end of seg1 and goes north
    // X: s1x1 to s1x1 + segW - 1, Z: s1z1 - segLen + segW to s1z1 + segW - 1
    // But we want it going north (negative Z)
    int s2x1 = s1x1, s2x2 = s1x1 + segW - 1; // 10 to 21
    int s2z2 = s1z1 + segW - 1; // overlap with seg1 at the south
    int s2z1 = s2z2 - segLen + 1; // goes north
    // s2z1 = -6 + 12 - 1 - 20 + 1 = -6 + 11 - 19 = -14, s2z2 = 5

    fillRect(s2x1 - 1, G, s2z1 - 1, s2x2 + 1, s2z2 + 1, BlockType::STONE);
    hollowBox(s2x1, hy, s2z1, s2x2, hy + wallH - 1, s2z2,
              BlockType::COBBLESTONE, BlockType::AIR);
    fillRect(s2x1 + 1, hy, s2z1 + 1, s2x2 - 1, s2z2 - 1, BlockType::PLANKS);

    // Windows on seg2
    for (int z = s2z1 + 2; z <= s2z2 - 2; z += 3) {
        for (int y = hy + 2; y <= hy + 3; y++) {
            setBlock(s2x1, y, z, BlockType::GLASS);
            setBlock(s2x2, y, z, BlockType::GLASS);
        }
    }
    for (int x = s2x1 + 2; x <= s2x2 - 2; x += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(x, y, s2z1, BlockType::GLASS);

    // West door on seg2
    int backDoorX = (s2x1 + s2x2) / 2;
    setBlock(s2x1, hy + 1, (s2z1 + s2z2) / 2, BlockType::AIR);
    setBlock(s2x1, hy + 2, (s2z1 + s2z2) / 2, BlockType::AIR);

    // Seg2 roof: peaked E-W
    for (int z = s2z1 - 1; z <= s2z2 + 1; z++)
        for (int layer = 0; layer <= segW / 2; layer++) {
            int rx1 = s2x1 - 1 + layer;
            int rx2 = s2x2 + 1 - layer;
            int ry = hy + wallH + layer;
            if (rx1 <= rx2)
                for (int x = rx1; x <= rx2; x++)
                    setBlock(x, ry, z, roofBlock);
        }

    // Remove shared walls between seg1 and seg2
    {
        int ox1 = std::max(s1x1, s2x1);
        int ox2 = std::min(s1x2, s2x2);
        int oz1 = std::max(s1z1, s2z1);
        int oz2 = std::min(s1z2, s2z2);
        for (int x = ox1; x <= ox2; x++)
            for (int z = oz1; z <= oz2; z++) {
                for (int y = hy + 1; y < hy + wallH - 1; y++)
                    setBlock(x, y, z, BlockType::AIR);
                setBlock(x, hy, z, BlockType::PLANKS);
            }
    }

    // --- Seg3: bends northeast at 45 degrees from top of seg2 ---
    // Built as stair-stepped sub-segments going +X, -Z from the north end of seg2
    int numSteps = 4;
    int stepSize = segLen / numSteps; // 5 blocks per step
    // Start from north end of seg2
    int s3startX = s2x2 + 1; // east of seg2 top
    int s3startZ = s2z1;     // north end of seg2

    for (int s = 0; s < numSteps; s++) {
        int sx1 = s3startX + s * stepSize;
        int sz1 = s3startZ - s * stepSize - segW + 1;
        int sx2 = sx1 + stepSize + segW - 1; // overlap width
        int sz2 = sz1 + segW - 1;

        fillRect(sx1 - 1, G, sz1 - 1, sx2 + 1, sz2 + 1, BlockType::STONE);
        hollowBox(sx1, hy, sz1, sx2, hy + wallH - 1, sz2,
                  BlockType::COBBLESTONE, BlockType::AIR);
        fillRect(sx1 + 1, hy, sz1 + 1, sx2 - 1, sz2 - 1, BlockType::PLANKS);

        // Windows
        for (int x = sx1 + 2; x <= sx2 - 2; x += 3)
            for (int y = hy + 2; y <= hy + 3; y++) {
                setBlock(x, y, sz1, BlockType::GLASS);
                setBlock(x, y, sz2, BlockType::GLASS);
            }
        for (int z = sz1 + 2; z <= sz2 - 2; z += 3)
            for (int y = hy + 2; y <= hy + 3; y++) {
                setBlock(sx1, y, z, BlockType::GLASS);
                setBlock(sx2, y, z, BlockType::GLASS);
            }

        // Roof
        for (int x = sx1 - 1; x <= sx2 + 1; x++)
            for (int layer = 0; layer <= segW / 2; layer++) {
                int rz1 = sz1 - 1 + layer;
                int rz2 = sz2 + 1 - layer;
                int ry = hy + wallH + layer;
                if (rz1 <= rz2)
                    for (int z = rz1; z <= rz2; z++)
                        setBlock(x, ry, z, roofBlock);
            }
    }

    // Remove interior walls between seg3 sub-segments
    for (int s = 0; s < numSteps - 1; s++) {
        int sx1_a = s3startX + s * stepSize;
        int sz1_a = s3startZ - s * stepSize - segW + 1;
        int sx2_a = sx1_a + stepSize + segW - 1;
        int sz2_a = sz1_a + segW - 1;
        int sx1_b = s3startX + (s+1) * stepSize;
        int sz1_b = s3startZ - (s+1) * stepSize - segW + 1;
        int sx2_b = sx1_b + stepSize + segW - 1;
        int sz2_b = sz1_b + segW - 1;

        int ox1 = std::max(sx1_a, sx1_b);
        int ox2 = std::min(sx2_a, sx2_b);
        int oz1 = std::max(sz1_a, sz1_b);
        int oz2 = std::min(sz2_a, sz2_b);

        for (int x = ox1; x <= ox2; x++)
            for (int z = oz1; z <= oz2; z++) {
                for (int y = hy + 1; y < hy + wallH - 1; y++)
                    setBlock(x, y, z, BlockType::AIR);
                setBlock(x, hy, z, BlockType::PLANKS);
            }
    }

    // Remove walls between seg2 and first seg3 sub-segment
    {
        int sx1_b = s3startX;
        int sz1_b = s3startZ - segW + 1;
        int sx2_b = sx1_b + stepSize + segW - 1;
        int sz2_b = sz1_b + segW - 1;

        int ox1 = std::max(s2x1, sx1_b);
        int ox2 = std::min(s2x2, sx2_b);
        int oz1 = std::max(s2z1, sz1_b);
        int oz2 = std::min(s2z2, sz2_b);

        for (int x = ox1; x <= ox2; x++)
            for (int z = oz1; z <= oz2; z++) {
                for (int y = hy + 1; y < hy + wallH - 1; y++)
                    setBlock(x, y, z, BlockType::AIR);
                setBlock(x, hy, z, BlockType::PLANKS);
            }
    }

    // ============================================================
    // TWO-CAR GARAGE - attached to south side of seg1
    // ============================================================
    int gx1 = s1x1 + 6, gx2 = s1x2, gz1 = s1z2 + 1, gz2 = s1z2 + 11;
    fillRect(gx1 - 1, G, gz1 - 1, gx2 + 1, gz2 + 1, BlockType::STONE);
    hollowBox(gx1, hy, gz1, gx2, hy + wallH - 1, gz2,
              BlockType::COBBLESTONE, BlockType::AIR);
    fillRect(gx1 + 1, hy, gz1 + 1, gx2 - 1, gz2 - 1, BlockType::PLANKS);

    // Remove shared wall
    for (int x = gx1 + 1; x <= gx2 - 1; x++)
        for (int y = hy; y < hy + wallH; y++)
            setBlock(x, y, gz1, BlockType::AIR);
    for (int x = gx1 + 1; x <= gx2 - 1; x++)
        setBlock(x, hy, gz1, BlockType::PLANKS);

    // Two garage doors on south wall
    for (int x = gx1 + 1; x <= gx1 + 5; x++)
        for (int y = hy + 1; y <= hy + 4; y++)
            setBlock(x, y, gz2, BlockType::AIR);
    for (int x = gx1 + 7; x <= gx1 + 11; x++)
        for (int y = hy + 1; y <= hy + 4; y++)
            setBlock(x, y, gz2, BlockType::AIR);

    // Windows on east wall of garage
    for (int z = gz1 + 2; z <= gz2 - 2; z += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(gx2, y, z, BlockType::GLASS);

    // Garage roof
    for (int x = gx1 - 1; x <= gx2 + 1; x++)
        for (int layer = 0; layer <= 3; layer++) {
            int rz1 = gz1 - 1 + layer;
            int rz2 = gz2 + 1 - layer;
            int ry = hy + wallH + layer;
            if (rz1 <= rz2)
                for (int z = rz1; z <= rz2; z++)
                    setBlock(x, ry, z, roofBlock);
        }

    // ============================================================
    // CEMENT APRON in front of garage doors
    // ============================================================
    int apronZ1 = gz2 + 1, apronZ2 = gz2 + 6;
    fillRect(gx1 - 1, G, apronZ1, gx2 + 1, apronZ2, BlockType::STONE);

    // ============================================================
    // CEMENT PAD to the south of the house
    // ============================================================
    fillRect(s1x1 - 2, G, s1z2 + 12, s1x2 + 2, s1z2 + 22, BlockType::STONE);

    // ============================================================
    // DRIVEWAY - black asphalt (BEDROCK)
    // Goes northeast from cement apron, then turns due east
    // ============================================================
    int driveW = 7;

    // Driveway starts east of the apron, goes straight east
    int driveStartX = gx2 + 2;
    int driveZ = (apronZ1 + apronZ2) / 2; // center of apron

    // Connect apron to driveway start
    for (int x = gx2 + 1; x <= driveStartX; x++)
        for (int z = driveZ - driveW / 2; z <= driveZ + driveW / 2; z++)
            setBlock(x, G, z, BlockType::BEDROCK);

    // Straight east section
    for (int x = driveStartX; x <= propX2; x++)
        for (int z = driveZ - driveW / 2; z <= driveZ + driveW / 2; z++)
            setBlock(x, G, z, BlockType::BEDROCK);

    // ============================================================
    // TREES - border the wider property perimeter
    // ============================================================
    struct TreePos { int x, z, h; };
    TreePos trees[] = {
        // North border
        {-70, -44, 7}, {-60, -44, 8}, {-50, -44, 7}, {-40, -44, 6},
        {-30, -44, 8}, {-20, -44, 7}, {-10, -44, 6}, {0, -44, 7},
        {10, -44, 8}, {20, -44, 7}, {30, -44, 6}, {40, -44, 8},
        {50, -44, 7}, {60, -44, 6}, {70, -44, 7},
        // South border
        {-70, 44, 8}, {-60, 44, 7}, {-50, 44, 6}, {-40, 44, 8},
        {-30, 44, 7}, {-20, 44, 6}, {-10, 44, 7}, {0, 44, 8},
        {10, 44, 7}, {20, 44, 6}, {30, 44, 8}, {40, 44, 7},
        {50, 44, 6}, {60, 44, 8}, {70, 44, 7},
        // West border
        {-78, -35, 7}, {-78, -20, 8}, {-78, -5, 7}, {-78, 10, 6},
        {-78, 25, 7}, {-78, 35, 8},
        // Between pond and pool (open green with scattered trees)
        {-45, -30, 7}, {-38, 28, 8}, {-48, 15, 6}, {-35, -18, 7},
        {-42, 5, 8}, {-30, -28, 6}, {-25, 22, 7}, {-45, 20, 6},
        {-35, -35, 7}, {-28, 32, 8},
        // Around house
        {8, -30, 7}, {8, 28, 8}, {25, 28, 7}, {32, 28, 6},
        {32, -30, 8},
        // Along driveway (east)
        {45, -25, 7}, {55, -25, 8}, {65, -25, 7}, {80, -25, 6}, {95, -25, 8}, {110, -25, 7},
        {45, 25, 6}, {55, 25, 7}, {65, 25, 8}, {80, 25, 7}, {95, 25, 6}, {110, 25, 8},
    };

    for (auto& t : trees)
        placeTree(t.x, G + 1, t.z, t.h);

    // Mark all chunks as dirty so meshes rebuild
    for (auto& [key, chunk] : chunks_)
        chunk->dirty = true;
}
