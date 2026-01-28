#pragma once
#include "chunk.h"
#include <GL/glew.h>
#include <vector>
#include <functional>

struct ChunkMesh {
    GLuint vao = 0, vbo = 0;
    int vertexCount = 0;

    void upload(const std::vector<float>& vertices);
    void draw() const;
    void destroy();
};

// Neighbor lookup: given local x,z + direction, returns block type
// Allows looking into adjacent chunks
using NeighborLookup = std::function<BlockType(int worldX, int y, int worldZ)>;

void buildChunkMesh(const Chunk& chunk, const NeighborLookup& getWorldBlock,
                    std::vector<float>& outVertices);
