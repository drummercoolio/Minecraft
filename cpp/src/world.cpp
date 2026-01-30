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
    // Corrected closing segment: 185.7° for 835.9 ft (calculated to close polygon)
    moveDir(185.7f, 835.9f, lx, lz); lotVerts.push_back({lx, lz});

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
    // Corrected closing segment: 275.3° for 31.8 ft (calculated to close polygon)
    moveDir(275.3f, 31.8f, hx, hz); houseVerts.push_back({hx, hz});

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
    // BUILD HOUSE - full structure within surveyor footprint
    // ============================================================
    int hy = G + 1;
    int wallH = 6;
    BlockType roofBlock = BlockType::CLAY;     // orange roof
    BlockType wallBlock = BlockType::COBBLESTONE;

    // Helper: minimum distance from point to any polygon edge
    auto distToEdge = [&](float px, float pz,
                          const std::vector<std::pair<float,float>>& poly) -> float {
        float minD = 1e9f;
        for (size_t i = 0; i < poly.size(); i++) {
            size_t j = (i + 1) % poly.size();
            float ax = poly[i].first, az = poly[i].second;
            float bx = poly[j].first, bz = poly[j].second;
            float dx = bx - ax, dz = bz - az;
            float len2 = dx*dx + dz*dz;
            if (len2 < 0.01f) continue;
            float t = std::max(0.0f, std::min(1.0f,
                ((px-ax)*dx + (pz-az)*dz) / len2));
            float cx = ax + t*dx, cz = az + t*dz;
            float d = sqrtf((px-cx)*(px-cx) + (pz-cz)*(pz-cz));
            if (d < minD) minD = d;
        }
        return minD;
    };

    // House bounding box
    int ihMinX = (int)floorf(houseMinX) - 2;
    int ihMaxX = (int)ceilf(houseMaxX) + 2;
    int ihMinZ = (int)floorf(houseMinZ) - 2;
    int ihMaxZ = (int)ceilf(houseMaxZ) + 2;

    // Foundation and floor
    for (int x = ihMinX; x <= ihMaxX; x++) {
        for (int z = ihMinZ; z <= ihMaxZ; z++) {
            if (pointInHouse((float)x, (float)z)) {
                setBlock(x, G, z, BlockType::STONE);   // foundation
                setBlock(x, hy, z, BlockType::PLANKS);  // floor
            }
        }
    }

    // Draw walls along polygon edges (full height)
    for (size_t i = 0; i < houseVerts.size(); i++) {
        size_t j = (i + 1) % houseVerts.size();
        float x1 = houseVerts[i].first, z1 = houseVerts[i].second;
        float x2 = houseVerts[j].first, z2 = houseVerts[j].second;

        float dx = x2 - x1, dz = z2 - z1;
        float len = sqrtf(dx * dx + dz * dz);
        if (len < 0.5f) continue;

        int steps = (int)ceilf(len * 2); // extra resolution
        for (int s = 0; s <= steps; s++) {
            float t = (float)s / (float)steps;
            int wx = (int)roundf(x1 + dx * t);
            int wz = (int)roundf(z1 + dz * t);
            for (int y = hy; y < hy + wallH; y++)
                setBlock(wx, y, wz, wallBlock);
        }
    }

    // Clear interior air
    for (int x = ihMinX; x <= ihMaxX; x++) {
        for (int z = ihMinZ; z <= ihMaxZ; z++) {
            if (pointInHouse((float)x, (float)z)) {
                float d = distToEdge((float)x, (float)z, houseVerts);
                if (d > 0.8f) { // inside walls
                    for (int y = hy + 1; y < hy + wallH; y++)
                        setBlock(x, y, z, BlockType::AIR);
                }
            }
        }
    }

    // Windows along each wall segment (every 3 blocks, skip short segments)
    for (size_t i = 0; i < houseVerts.size(); i++) {
        size_t j = (i + 1) % houseVerts.size();
        float x1 = houseVerts[i].first, z1 = houseVerts[i].second;
        float x2 = houseVerts[j].first, z2 = houseVerts[j].second;

        float dx = x2 - x1, dz = z2 - z1;
        float len = sqrtf(dx * dx + dz * dz);
        if (len < 3.0f) continue;

        int steps = (int)ceilf(len);
        for (int s = 2; s <= steps - 2; s += 3) {
            float t = (float)s / (float)steps;
            int wx = (int)roundf(x1 + dx * t);
            int wz = (int)roundf(z1 + dz * t);
            setBlock(wx, hy + 2, wz, BlockType::GLASS);
            setBlock(wx, hy + 3, wz, BlockType::GLASS);
        }
    }

    // Doors - east side (front) and west side (back)
    // Front door: on segment 0 (SE corner going west), near the start
    {
        float x1 = houseVerts[0].first, z1 = houseVerts[0].second;
        float x2 = houseVerts[1].first, z2 = houseVerts[1].second;
        float dx = x2 - x1, dz = z2 - z1;
        float len = sqrtf(dx*dx + dz*dz);
        // Door at 30% along this wall
        int dx1 = (int)roundf(x1 + dx * 0.3f);
        int dz1 = (int)roundf(z1 + dz * 0.3f);
        setBlock(dx1, hy + 1, dz1, BlockType::AIR);
        setBlock(dx1, hy + 2, dz1, BlockType::AIR);
    }
    // Back door: on segment going east 23ft (vertex 13→14)
    {
        float x1 = houseVerts[13].first, z1 = houseVerts[13].second;
        float x2 = houseVerts[14].first, z2 = houseVerts[14].second;
        float dx = x2 - x1, dz = z2 - z1;
        int dx1 = (int)roundf(x1 + dx * 0.5f);
        int dz1 = (int)roundf(z1 + dz * 0.5f);
        setBlock(dx1, hy + 1, dz1, BlockType::AIR);
        setBlock(dx1, hy + 2, dz1, BlockType::AIR);
    }

    // Peaked roof - height based on distance from nearest wall edge
    int maxRoofLayers = 5;
    for (int x = ihMinX - 1; x <= ihMaxX + 1; x++) {
        for (int z = ihMinZ - 1; z <= ihMaxZ + 1; z++) {
            if (pointInHouse((float)x, (float)z)) {
                float d = distToEdge((float)x, (float)z, houseVerts);
                int roofLayer = std::min((int)d, maxRoofLayers);
                // Build roof from wall top up to peak
                for (int layer = 0; layer <= roofLayer; layer++) {
                    setBlock(x, hy + wallH + layer, z, roofBlock);
                }
            }
        }
    }
    // Roof overhang (1 block outside walls)
    for (int x = ihMinX - 1; x <= ihMaxX + 1; x++) {
        for (int z = ihMinZ - 1; z <= ihMaxZ + 1; z++) {
            if (!pointInHouse((float)x, (float)z)) {
                float d = distToEdge((float)x, (float)z, houseVerts);
                if (d < 1.5f) {
                    setBlock(x, hy + wallH, z, roofBlock);
                }
            }
        }
    }

    // ============================================================
    // GARAGE - attached to south side of house (near SE corner)
    // ============================================================
    // SE corner of house is houseVerts[0] after offset
    float garageX1 = houseVerts[0].first - 12; // west of SE corner
    float garageZ1 = houseVerts[0].second;       // south wall of house
    float garageX2 = houseVerts[0].first;
    float garageZ2 = houseVerts[0].second + 8;   // extends south 8 blocks

    int gx1 = (int)roundf(garageX1), gx2 = (int)roundf(garageX2);
    int gz1 = (int)roundf(garageZ1), gz2 = (int)roundf(garageZ2);

    // Foundation and floor
    fillRect(gx1 - 1, G, gz1, gx2 + 1, gz2 + 1, BlockType::STONE);
    hollowBox(gx1, hy, gz1, gx2, hy + wallH - 1, gz2,
              BlockType::COBBLESTONE, BlockType::AIR);
    fillRect(gx1 + 1, hy, gz1 + 1, gx2 - 1, gz2 - 1, BlockType::PLANKS);

    // Remove shared wall between house and garage
    for (int x = gx1 + 1; x <= gx2 - 1; x++)
        for (int y = hy; y < hy + wallH; y++)
            setBlock(x, y, gz1, BlockType::AIR);
    for (int x = gx1 + 1; x <= gx2 - 1; x++)
        setBlock(x, hy, gz1, BlockType::PLANKS);

    // Two garage door openings on south wall
    int gDoorW = (gx2 - gx1 - 2) / 2;
    for (int x = gx1 + 1; x <= gx1 + gDoorW; x++)
        for (int y = hy + 1; y <= hy + 4; y++)
            setBlock(x, y, gz2, BlockType::AIR);
    for (int x = gx2 - gDoorW; x <= gx2 - 1; x++)
        for (int y = hy + 1; y <= hy + 4; y++)
            setBlock(x, y, gz2, BlockType::AIR);

    // Garage windows on east wall
    for (int z = gz1 + 2; z <= gz2 - 2; z += 3)
        for (int y = hy + 2; y <= hy + 3; y++)
            setBlock(gx2, y, z, BlockType::GLASS);

    // Garage roof: peaked north-south
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
    // CEMENT APRON in front of garage
    // ============================================================
    int apronZ1 = gz2 + 1, apronZ2 = gz2 + 4;
    fillRect(gx1 - 1, G, apronZ1, gx2 + 1, apronZ2, BlockType::STONE);

    // ============================================================
    // DRIVEWAY - NE diagonal then straight east
    // ============================================================
    int driveW = 5;
    int driveStartX = gx2 + 2;
    int driveStartZ = (apronZ1 + apronZ2) / 2;
    int diagLen = 12;

    // Connect apron to driveway
    for (int x = gx2 + 1; x <= driveStartX; x++)
        for (int z = driveStartZ - driveW/2; z <= driveStartZ + driveW/2; z++)
            setBlock(x, G, z, BlockType::BEDROCK);

    // Diagonal NE section (+X, -Z)
    for (int i = 0; i < diagLen; i++) {
        int cx = driveStartX + i;
        int cz = driveStartZ - i;
        for (int w = 0; w < driveW; w++) {
            setBlock(cx + w, G, cz, BlockType::BEDROCK);
            setBlock(cx + w, G, cz - 1, BlockType::BEDROCK);
        }
    }

    // Straight east from end of diagonal
    int straightX = driveStartX + diagLen;
    int straightZ = driveStartZ - diagLen;
    // Extend east to lot boundary
    int lotEastBlock = (int)roundf(lotMaxX);
    for (int x = straightX; x <= lotEastBlock; x++)
        for (int z = straightZ - driveW/2; z <= straightZ + driveW/2; z++)
            setBlock(x, G, z, BlockType::BEDROCK);

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
