# Minecraft World Generator

A procedural Minecraft world generator written in Python. Generates terrain with biomes, caves, ores, trees, and other features using Perlin noise.

## Features

- **7 biomes**: Plains, Desert, Forest, Mountains, Ocean, Tundra, Swamp
- **21 block types**: Stone, ores, wood, leaves, water, lava, and more
- **Terrain generation**: Multi-octave Perlin noise for realistic heightmaps
- **Cave systems**: Underground cave generation with lava at low levels
- **Ore distribution**: Diamond, gold, iron, and coal at appropriate depths
- **Decorations**: Trees in forests/plains, cacti in deserts
- **Chunk-based**: 16x16x256 chunks with lazy generation
- **Save/Load**: gzip-compressed JSON world persistence

## Usage

```bash
# Create a world with default settings
python main.py

# Create with a specific seed
python main.py --seed 42

# Save the world
python main.py --seed 42 --save saves/my_world

# Load a saved world
python main.py --load saves/my_world

# Generate larger spawn area
python main.py --radius 4

# View a specific Y level
python main.py --slice-y 30
```

## Project Structure

```
├── main.py              # Entry point and CLI
└── world/
    ├── __init__.py
    ├── block.py         # Block type definitions
    ├── biome.py         # Biome definitions and properties
    ├── chunk.py         # 16x16x256 chunk storage
    ├── noise.py         # Perlin noise generator
    ├── generator.py     # Terrain and feature generation
    └── world.py         # World management and persistence
```
