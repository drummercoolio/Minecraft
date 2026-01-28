#include "texture.h"
#include <vector>
#include <cstdlib>
#include <cmath>
#include <algorithm>

static void setPixel(std::vector<unsigned char>& img, int atlasW, int x, int y,
                     unsigned char r, unsigned char g, unsigned char b) {
    int idx = (y * atlasW + x) * 4;
    img[idx] = r; img[idx+1] = g; img[idx+2] = b; img[idx+3] = 255;
}

static void fillTile(std::vector<unsigned char>& img, int atlasW,
                     int tileX, int tileY, unsigned char r, unsigned char g, unsigned char b,
                     int variation = 10) {
    int ox = tileX * 16, oy = tileY * 16;
    for (int dy = 0; dy < 16; dy++) {
        for (int dx = 0; dx < 16; dx++) {
            int v = (rand() % (variation * 2 + 1)) - variation;
            unsigned char cr = (unsigned char)std::clamp((int)r + v, 0, 255);
            unsigned char cg = (unsigned char)std::clamp((int)g + v, 0, 255);
            unsigned char cb = (unsigned char)std::clamp((int)b + v, 0, 255);
            setPixel(img, atlasW, ox + dx, oy + dy, cr, cg, cb);
        }
    }
}

static void addSpeckles(std::vector<unsigned char>& img, int atlasW,
                        int tileX, int tileY, unsigned char r, unsigned char g, unsigned char b,
                        int count) {
    int ox = tileX * 16, oy = tileY * 16;
    for (int i = 0; i < count; i++) {
        int dx = rand() % 16, dy = rand() % 16;
        setPixel(img, atlasW, ox + dx, oy + dy, r, g, b);
    }
}

GLuint generateTextureAtlas() {
    const int TILE_SIZE = 16;
    const int TILES = 16;
    const int W = TILE_SIZE * TILES; // 256
    const int H = TILE_SIZE * TILES;
    std::vector<unsigned char> img(W * H * 4, 255);

    srand(12345); // deterministic

    // Row 0
    fillTile(img, W, 0, 0, 200, 0, 200);           // 0,0: unused (air) - magenta debug
    fillTile(img, W, 1, 0, 120, 100, 60, 8);        // 1,0: grass side
    // Add green top strip to grass side
    for (int dx = 0; dx < 16; dx++) {
        for (int dy = 0; dy < 3; dy++) {
            int v = (rand() % 20) - 10;
            setPixel(img, W, 1*16+dx, 0*16+dy,
                     (unsigned char)std::clamp(80+v, 0, 255),
                     (unsigned char)std::clamp(160+v, 0, 255),
                     (unsigned char)std::clamp(50+v, 0, 255));
        }
    }
    fillTile(img, W, 2, 0, 80, 165, 50, 15);        // 2,0: grass top
    fillTile(img, W, 3, 0, 120, 85, 55, 8);          // 3,0: dirt
    fillTile(img, W, 4, 0, 128, 128, 128, 12);       // 4,0: stone
    fillTile(img, W, 5, 0, 215, 205, 160, 8);        // 5,0: sand
    fillTile(img, W, 6, 0, 40, 80, 200, 6);          // 6,0: water
    // Make water semi-transparent
    {
        int ox = 6*16, oy = 0*16;
        for (int dy = 0; dy < 16; dy++)
            for (int dx = 0; dx < 16; dx++)
                img[((oy+dy)*W + ox+dx)*4 + 3] = 180;
    }
    fillTile(img, W, 7, 0, 140, 110, 60, 8);         // 7,0: wood side (bark)
    // Bark lines
    for (int dy = 0; dy < 16; dy++) {
        if (dy % 3 == 0) {
            for (int dx = 0; dx < 16; dx++)
                setPixel(img, W, 7*16+dx, 0*16+dy, 110, 85, 45);
        }
    }
    fillTile(img, W, 8, 0, 160, 130, 70, 6);         // 8,0: wood top (rings)
    // Draw ring pattern
    for (int dy = 0; dy < 16; dy++) {
        for (int dx = 0; dx < 16; dx++) {
            float dist = sqrtf((dx-7.5f)*(dx-7.5f) + (dy-7.5f)*(dy-7.5f));
            if (fmod(dist, 3.0f) < 0.8f)
                setPixel(img, W, 8*16+dx, 0*16+dy, 130, 100, 50);
        }
    }
    fillTile(img, W, 9, 0, 40, 120, 30, 15);         // 9,0: leaves
    addSpeckles(img, W, 9, 0, 30, 90, 20, 30);
    fillTile(img, W, 10, 0, 60, 60, 60, 5);           // 10,0: bedrock
    addSpeckles(img, W, 10, 0, 40, 40, 40, 20);
    fillTile(img, W, 11, 0, 140, 135, 130, 10);       // 11,0: gravel
    addSpeckles(img, W, 11, 0, 100, 95, 90, 30);

    // Ores: stone base + colored speckles
    fillTile(img, W, 12, 0, 128, 128, 128, 12);       // 12,0: iron ore
    addSpeckles(img, W, 12, 0, 200, 180, 160, 25);
    fillTile(img, W, 13, 0, 128, 128, 128, 12);       // 13,0: coal ore
    addSpeckles(img, W, 13, 0, 30, 30, 30, 30);
    fillTile(img, W, 14, 0, 128, 128, 128, 12);       // 14,0: gold ore
    addSpeckles(img, W, 14, 0, 230, 200, 50, 25);
    fillTile(img, W, 15, 0, 128, 128, 128, 12);       // 15,0: diamond ore
    addSpeckles(img, W, 15, 0, 80, 220, 240, 25);

    // Row 1
    fillTile(img, W, 0, 1, 200, 200, 210, 5);         // 0,1: snow side
    for (int dx = 0; dx < 16; dx++)
        for (int dy = 0; dy < 3; dy++)
            setPixel(img, W, 0*16+dx, 1*16+dy, 240, 245, 255);
    fillTile(img, W, 1, 1, 240, 245, 255, 3);         // 1,1: snow top
    fillTile(img, W, 2, 1, 210, 200, 150, 8);         // 2,1: sandstone
    fillTile(img, W, 3, 1, 160, 155, 145, 6);         // 3,1: clay
    fillTile(img, W, 4, 1, 110, 110, 110, 12);        // 4,1: cobblestone
    // Cobble cracks
    for (int i = 0; i < 20; i++) {
        int dx = rand()%16, dy = rand()%16;
        setPixel(img, W, 4*16+dx, 1*16+dy, 80, 80, 80);
    }
    fillTile(img, W, 5, 1, 180, 145, 80, 5);          // 5,1: planks
    for (int dy = 0; dy < 16; dy += 4)
        for (int dx = 0; dx < 16; dx++)
            setPixel(img, W, 5*16+dx, 1*16+dy, 150, 120, 60);
    fillTile(img, W, 6, 1, 180, 210, 230, 3);         // 6,1: glass
    for (int dx = 0; dx < 16; dx++) {
        setPixel(img, W, 6*16+dx, 1*16+0, 200, 220, 240);
        setPixel(img, W, 6*16+dx, 1*16+15, 200, 220, 240);
    }
    for (int dy = 0; dy < 16; dy++) {
        setPixel(img, W, 6*16+0, 1*16+dy, 200, 220, 240);
        setPixel(img, W, 6*16+15, 1*16+dy, 200, 220, 240);
    }
    fillTile(img, W, 7, 1, 220, 100, 20, 10);         // 7,1: lava
    addSpeckles(img, W, 7, 1, 255, 200, 50, 30);
    fillTile(img, W, 8, 1, 30, 90, 20, 10);           // 8,1: cactus side
    for (int dy = 0; dy < 16; dy += 4)
        for (int dx = 0; dx < 16; dx++)
            setPixel(img, W, 8*16+dx, 1*16+dy, 50, 110, 35);
    fillTile(img, W, 9, 1, 40, 100, 30, 8);           // 9,1: cactus top

    // Upload
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return tex;
}
