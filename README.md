# Minecraft World Generator

A 3D Minecraft-style voxel world built in C++ with OpenGL. Features procedural terrain generation and a custom property scene with house, pool, pond, and driveway.

## Features

- **3D rendering**: OpenGL 3.3 core profile with textured blocks
- **Procedural terrain**: Multi-octave Perlin/simplex noise via FastNoiseLite
- **4 biomes**: Plains, Desert, Forest, Tundra
- **22 block types**: Stone, ores, wood, leaves, water, glass, and more
- **Chunk-based world**: 16x16x256 chunks with lazy generation
- **Face culling**: Only renders visible faces for performance
- **Procedural textures**: 16x16 texture atlas generated at runtime
- **FPS camera**: WASD + mouse controls with fly mode
- **Custom scene**: Property with L-shaped house, pool, pond, garage, and driveway

## Requirements

- CMake 3.10+
- C++17 compiler (g++, clang++)
- OpenGL 3.3+
- GLFW3
- GLEW

### Install dependencies (Ubuntu/Debian)

```bash
sudo apt-get install cmake build-essential libglfw3-dev libglew-dev
```

## Build & Run

```bash
cd cpp
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
./minecraft
```

## Controls

| Key | Action |
|-----|--------|
| W/A/S/D | Move forward/left/back/right |
| Mouse | Look around |
| Space | Fly up |
| Left Shift | Fly down |
| Left Ctrl | Sprint (3x speed) |
| F3 | Toggle wireframe mode |
| Escape | Quit |

## Project Structure

```
cpp/
├── CMakeLists.txt
├── include/
│   ├── block.h          # Block type definitions
│   ├── camera.h         # FPS camera
│   ├── chunk.h          # 16x16x256 chunk storage
│   ├── chunk_mesh.h     # Mesh builder with face culling
│   ├── shader.h         # GLSL shader loader
│   ├── texture.h        # Procedural texture atlas
│   └── world.h          # World manager
├── src/
│   ├── main.cpp         # Entry point and game loop
│   ├── block.cpp
│   ├── camera.cpp
│   ├── chunk.cpp
│   ├── chunk_mesh.cpp
│   ├── shader.cpp
│   ├── texture.cpp
│   └── world.cpp        # Terrain generation and structures
├── shaders/
│   ├── vertex.glsl
│   └── fragment.glsl
└── lib/
    └── FastNoiseLite.h  # Header-only noise library
```

## Libraries

- [GLFW](https://www.glfw.org/) - Window and input
- [GLEW](http://glew.sourceforge.net/) - OpenGL extension loading
- [GLM](https://github.com/g-truc/glm) - Math library (header-only)
- [FastNoiseLite](https://github.com/Auburn/FastNoiseLite) - Noise generation (header-only)
- [stb_image](https://github.com/nothings/stb) - Image loading (header-only)
