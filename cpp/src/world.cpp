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

    const int G = 64; // Ground level
    const float SCALE = 3.0f; // 1 block = 3 feet

    // User's angle system: 0°=West, 90°=North, 180°=East, 270°=South
    // Convert to standard bearing: standard = (user + 270) % 360
    // Movement: dx = sin(bearing), dz = -cos(bearing)
    auto userToStandard = [](float userDeg) {
        return fmodf(userDeg + 270.0f, 360.0f);
    };
    auto degToRad = [](float deg) { return deg * 3.14159265f / 180.0f; };
    auto moveDir = [&](float userDeg, float dist, float& x, float& z) {
        float stdDeg = userToStandard(userDeg);
        float rad = degToRad(stdDeg);
        x += sinf(rad) * dist / SCALE;
        z += -cosf(rad) * dist / SCALE;
    };

    // ============================================================
    // LOT BOUNDARY - starting from NE corner (point of origin)
    // ============================================================
    std::vector<std::pair<float, float>> lotVerts;
    float lx = 0, lz = 0;
    lotVerts.push_back({lx, lz}); // NE corner

    // Southern boundary runs west for 414 ft
    moveDir(0, 414, lx, lz); lotVerts.push_back({lx, lz});
    // 300° SW for 140 ft
    moveDir(300, 140, lx, lz); lotVerts.push_back({lx, lz});
    // West for 120 ft
    moveDir(0, 120, lx, lz); lotVerts.push_back({lx, lz});
    // 320° SW for 218 ft
    moveDir(320, 218, lx, lz); lotVerts.push_back({lx, lz});
    // 80° NW for 350 ft
    moveDir(80, 350, lx, lz); lotVerts.push_back({lx, lz});
    // 180° east for 935 ft back to start (force closure)
    lotVerts.push_back({0, 0});

    // Find lot bounding box
    float lotMinX = 1e9, lotMaxX = -1e9, lotMinZ = 1e9, lotMaxZ = -1e9;
    for (auto& v : lotVerts) {
        lotMinX = std::min(lotMinX, v.first);
        lotMaxX = std::max(lotMaxX, v.first);
        lotMinZ = std::min(lotMinZ, v.second);
        lotMaxZ = std::max(lotMaxZ, v.second);
    }

    // Point-in-polygon test (ray casting)
    auto pointInLot = [&](float px, float pz) -> bool {
        int n = lotVerts.size();
        bool inside = false;
        for (int i = 0, j = n - 1; i < n; j = i++) {
            float xi = lotVerts[i].first, zi = lotVerts[i].second;
            float xj = lotVerts[j].first, zj = lotVerts[j].second;
            if (((zi > pz) != (zj > pz)) &&
                (px < (xj - xi) * (pz - zi) / (zj - zi) + xi))
                inside = !inside;
        }
        return inside;
    };

    // ============================================================
    // HOUSE OUTLINE - traced from surveyor description
    // House placed 438 feet from eastern boundary = 146 blocks from east
    // ============================================================
    std::vector<std::pair<float, float>> houseVerts;
    float houseOffsetX = -438.0f / SCALE; // 438 ft from east boundary (which is at x=0)
    float houseOffsetZ = -181.0f / SCALE / 2; // Centered N-S (181 ft total N-S span)

    float hx = 0, hz = 0;
    houseVerts.push_back({hx, hz}); // Starting point

    // Trace house outline per surveyor description
    moveDir(0, 35, hx, hz); houseVerts.push_back({hx, hz});     // west 35 ft
    moveDir(90, 5, hx, hz); houseVerts.push_back({hx, hz});     // north 5 ft
    moveDir(0, 25, hx, hz); houseVerts.push_back({hx, hz});     // west 25 ft
    moveDir(90, 35, hx, hz); houseVerts.push_back({hx, hz});    // north 35 ft
    moveDir(60, 16, hx, hz); houseVerts.push_back({hx, hz});    // 60° NW 16 ft
    moveDir(120, 16, hx, hz); houseVerts.push_back({hx, hz});   // 120° NE 16 ft
    moveDir(20, 5, hx, hz); houseVerts.push_back({hx, hz});     // 20° NW 5 ft
    moveDir(120, 45, hx, hz); houseVerts.push_back({hx, hz});   // 120° NE 45 ft
    moveDir(210, 40, hx, hz); houseVerts.push_back({hx, hz});   // 210° SE 40 ft
    moveDir(300, 32, hx, hz); houseVerts.push_back({hx, hz});   // 300° SW 32 ft
    moveDir(270, 14, hx, hz); houseVerts.push_back({hx, hz});   // south 14 ft
    moveDir(180, 3.5, hx, hz); houseVerts.push_back({hx, hz});  // east 3.5 ft
    moveDir(270, 15, hx, hz); houseVerts.push_back({hx, hz});   // south 15 ft
    moveDir(180, 23, hx, hz); houseVerts.push_back({hx, hz});   // east 23 ft
    // south 32 ft back to origin (force closure)
    houseVerts.push_back({0, 0});

    // Offset house vertices to final position
    for (auto& v : houseVerts) {
        v.first += houseOffsetX;
        v.second += houseOffsetZ;
    }

    // Find house bounding box
    float houseMinX = 1e9, houseMaxX = -1e9, houseMinZ = 1e9, houseMaxZ = -1e9;
    for (auto& v : houseVerts) {
        houseMinX = std::min(houseMinX, v.first);
        houseMaxX = std::max(houseMaxX, v.first);
        houseMinZ = std::min(houseMinZ, v.second);
        houseMaxZ = std::max(houseMaxZ, v.second);
    }

    // Point-in-house test
    auto pointInHouse = [&](float px, float pz) -> bool {
        int n = houseVerts.size();
        bool inside = false;
        for (int i = 0, j = n - 1; i < n; j = i++) {
            float xi = houseVerts[i].first, zi = houseVerts[i].second;
            float xj = houseVerts[j].first, zj = houseVerts[j].second;
            if (((zi > pz) != (zj > pz)) &&
                (px < (xj - xi) * (pz - zi) / (zj - zi) + xi))
                inside = !inside;
        }
        return inside;
    };

    // ============================================================
    // FILL LOT WITH GRASS (only inside lot boundary)
    // ============================================================
    int iMinX = (int)floorf(lotMinX) - 5;
    int iMaxX = (int)ceilf(lotMaxX) + 5;
    int iMinZ = (int)floorf(lotMinZ) - 5;
    int iMaxZ = (int)ceilf(lotMaxZ) + 5;

    for (int x = iMinX; x <= iMaxX; x++) {
        for (int z = iMinZ; z <= iMaxZ; z++) {
            bool inLot = pointInLot((float)x, (float)z);
            // Clear air above ground
            for (int y = G + 1; y < G + 40; y++)
                setBlock(x, y, z, BlockType::AIR);

            if (inLot) {
                setBlock(x, G, z, BlockType::GRASS);
                for (int y = G - 1; y >= G - 4; y--)
                    setBlock(x, y, z, BlockType::DIRT);
            } else {
                // Outside lot - leave as generated terrain or stone
                setBlock(x, G, z, BlockType::STONE);
                for (int y = G - 1; y >= G - 4; y--)
                    setBlock(x, y, z, BlockType::STONE);
            }
        }
    }

    // ============================================================
    // BUILD HOUSE WALLS AND ROOF
    // ============================================================
    int hy = G + 1;
    int wallH = 6;
    BlockType roofBlock = BlockType::CLAY;
    BlockType wallBlock = BlockType::COBBLESTONE;

    // Draw walls along house polygon edges
    for (size_t i = 0; i < houseVerts.size(); i++) {
        size_t j = (i + 1) % houseVerts.size();
        float x1 = houseVerts[i].first, z1 = houseVerts[i].second;
        float x2 = houseVerts[j].first, z2 = houseVerts[j].second;

        // Bresenham-like line for walls
        float dx = x2 - x1, dz = z2 - z1;
        float len = sqrtf(dx * dx + dz * dz);
        if (len < 0.5f) continue;

        int steps = (int)ceilf(len);
        for (int s = 0; s <= steps; s++) {
            float t = (float)s / (float)steps;
            int wx = (int)roundf(x1 + dx * t);
            int wz = (int)roundf(z1 + dz * t);

            // Build wall column
            for (int y = hy; y < hy + wallH; y++) {
                setBlock(wx, y, wz, wallBlock);
            }
        }
    }

    // Fill house floor and interior
    int ihMinX = (int)floorf(houseMinX);
    int ihMaxX = (int)ceilf(houseMaxX);
    int ihMinZ = (int)floorf(houseMinZ);
    int ihMaxZ = (int)ceilf(houseMaxZ);

    for (int x = ihMinX; x <= ihMaxX; x++) {
        for (int z = ihMinZ; z <= ihMaxZ; z++) {
            if (pointInHouse((float)x, (float)z)) {
                setBlock(x, G, z, BlockType::STONE); // Foundation
                setBlock(x, hy, z, BlockType::PLANKS); // Floor
                // Clear interior
                for (int y = hy + 1; y < hy + wallH; y++)
                    setBlock(x, y, z, BlockType::AIR);
            }
        }
    }

    // Add windows along walls (every 4 blocks)
    for (size_t i = 0; i < houseVerts.size(); i++) {
        size_t j = (i + 1) % houseVerts.size();
        float x1 = houseVerts[i].first, z1 = houseVerts[i].second;
        float x2 = houseVerts[j].first, z2 = houseVerts[j].second;

        float dx = x2 - x1, dz = z2 - z1;
        float len = sqrtf(dx * dx + dz * dz);
        if (len < 4.0f) continue;

        int steps = (int)ceilf(len);
        for (int s = 4; s <= steps - 2; s += 4) {
            float t = (float)s / (float)steps;
            int wx = (int)roundf(x1 + dx * t);
            int wz = (int)roundf(z1 + dz * t);

            // Window at height 2-3
            setBlock(wx, hy + 2, wz, BlockType::GLASS);
            setBlock(wx, hy + 3, wz, BlockType::GLASS);
        }
    }

    // Flat roof over house
    for (int x = ihMinX - 1; x <= ihMaxX + 1; x++) {
        for (int z = ihMinZ - 1; z <= ihMaxZ + 1; z++) {
            if (pointInHouse((float)x, (float)z)) {
                setBlock(x, hy + wallH, z, roofBlock);
                setBlock(x, hy + wallH + 1, z, roofBlock);
            }
        }
    }

    // ============================================================
    // TREES along lot boundary
    // ============================================================
    for (size_t i = 0; i < lotVerts.size(); i++) {
        size_t j = (i + 1) % lotVerts.size();
        float x1 = lotVerts[i].first, z1 = lotVerts[i].second;
        float x2 = lotVerts[j].first, z2 = lotVerts[j].second;

        float dx = x2 - x1, dz = z2 - z1;
        float len = sqrtf(dx * dx + dz * dz);
        if (len < 10.0f) continue;

        int numTrees = (int)(len / 15.0f);
        for (int t = 1; t <= numTrees; t++) {
            float frac = (float)t / (float)(numTrees + 1);
            int tx = (int)roundf(x1 + dx * frac);
            int tz = (int)roundf(z1 + dz * frac);
            // Offset slightly inside lot
            float nx = -dz / len * 3;
            float nz = dx / len * 3;
            tx += (int)nx;
            tz += (int)nz;
            int treeH = 6 + (t % 3);
            placeTree(tx, G + 1, tz, treeH);
        }
    }

    // Mark all chunks as dirty
    for (auto& [key, chunk] : chunks_)
        chunk->dirty = true;
}
