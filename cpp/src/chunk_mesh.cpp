#include "chunk_mesh.h"

static constexpr float ATLAS_SIZE = 16.0f;
static constexpr float TILE = 1.0f / ATLAS_SIZE;

// Face data: 6 faces, each with 6 vertices (2 triangles)
// Each vertex: dx, dy, dz, du, dv
struct FaceVert { float dx, dy, dz, du, dv; };

// Face directions: +X, -X, +Y, -Y, +Z, -Z
static const int FACE_NORMALS[6][3] = {
    {1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}
};

static const float FACE_LIGHT[6] = {
    0.7f, 0.7f, 1.0f, 0.4f, 0.8f, 0.8f
};

static const FaceVert FACE_VERTICES[6][6] = {
    // +X
    {{1,0,0, 0,1}, {1,1,0, 0,0}, {1,1,1, 1,0}, {1,0,0, 0,1}, {1,1,1, 1,0}, {1,0,1, 1,1}},
    // -X
    {{0,0,1, 0,1}, {0,1,1, 0,0}, {0,1,0, 1,0}, {0,0,1, 0,1}, {0,1,0, 1,0}, {0,0,0, 1,1}},
    // +Y
    {{0,1,0, 0,1}, {0,1,1, 0,0}, {1,1,1, 1,0}, {0,1,0, 0,1}, {1,1,1, 1,0}, {1,1,0, 1,1}},
    // -Y
    {{0,0,1, 0,1}, {0,0,0, 0,0}, {1,0,0, 1,0}, {0,0,1, 0,1}, {1,0,0, 1,0}, {1,0,1, 1,1}},
    // +Z
    {{1,0,1, 0,1}, {1,1,1, 0,0}, {0,1,1, 1,0}, {1,0,1, 0,1}, {0,1,1, 1,0}, {0,0,1, 1,1}},
    // -Z
    {{0,0,0, 0,1}, {0,1,0, 0,0}, {1,1,0, 1,0}, {0,0,0, 0,1}, {1,1,0, 1,0}, {1,0,0, 1,1}},
};

void buildChunkMesh(const Chunk& chunk, const NeighborLookup& getWorldBlock,
                    std::vector<float>& out) {
    int baseX = chunk.cx * CHUNK_W;
    int baseZ = chunk.cz * CHUNK_D;

    out.clear();
    out.reserve(CHUNK_W * CHUNK_D * 64 * 6); // rough estimate

    for (int y = 0; y < CHUNK_H; y++) {
        for (int z = 0; z < CHUNK_D; z++) {
            for (int x = 0; x < CHUNK_W; x++) {
                BlockType block = chunk.getBlock(x, y, z);
                if (block == BlockType::AIR) continue;

                const BlockDef& def = getBlockDef(block);
                int wx = baseX + x;
                int wz = baseZ + z;

                for (int face = 0; face < 6; face++) {
                    int nx = wx + FACE_NORMALS[face][0];
                    int ny = y  + FACE_NORMALS[face][1];
                    int nz = wz + FACE_NORMALS[face][2];

                    BlockType neighbor = getWorldBlock(nx, ny, nz);
                    // Skip face if neighbor is solid and opaque
                    if (blockIsSolid(neighbor) && !blockIsTransparent(neighbor))
                        continue;
                    // Skip face between same transparent blocks (e.g. water-water)
                    if (block == neighbor && blockIsTransparent(block))
                        continue;

                    float u0 = def.texX[face] * TILE;
                    float v0 = def.texY[face] * TILE;
                    float light = FACE_LIGHT[face];

                    for (int v = 0; v < 6; v++) {
                        const FaceVert& fv = FACE_VERTICES[face][v];
                        out.push_back((float)wx + fv.dx);
                        out.push_back((float)y  + fv.dy);
                        out.push_back((float)wz + fv.dz);
                        out.push_back(u0 + fv.du * TILE);
                        out.push_back(v0 + fv.dv * TILE);
                        out.push_back(light);
                    }
                }
            }
        }
    }
}

void ChunkMesh::upload(const std::vector<float>& vertices) {
    vertexCount = (int)vertices.size() / 6; // 6 floats per vertex

    if (!vao) {
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
    }

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float),
                 vertices.data(), GL_STATIC_DRAW);

    // position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texcoord
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // light
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

void ChunkMesh::draw() const {
    if (vertexCount == 0) return;
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    glBindVertexArray(0);
}

void ChunkMesh::destroy() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
    vao = vbo = 0;
    vertexCount = 0;
}
