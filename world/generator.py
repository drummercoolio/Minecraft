"""World terrain generator using procedural noise."""

import random

from .block import Block
from .biome import Biome
from .chunk import Chunk, CHUNK_WIDTH, CHUNK_DEPTH, CHUNK_HEIGHT
from .noise import PerlinNoise


class WorldGenerator:
    """Generates terrain, ores, trees, and structures for chunks."""

    SEA_LEVEL = 62

    def __init__(self, seed=None):
        self.seed = seed if seed is not None else random.randint(0, 2**32 - 1)
        self._terrain_noise = PerlinNoise(self.seed)
        self._biome_noise = PerlinNoise(self.seed + 1)
        self._cave_noise = PerlinNoise(self.seed + 2)
        self._rng = random.Random(self.seed)

    def _get_biome(self, world_x, world_z):
        temp = self._biome_noise.octave_noise2d(world_x * 0.005, world_z * 0.005, octaves=3)
        moisture = self._biome_noise.octave_noise2d(
            world_x * 0.005 + 1000, world_z * 0.005 + 1000, octaves=3
        )

        if temp < -0.3:
            return Biome.TUNDRA
        if moisture > 0.3:
            if temp > 0.2:
                return Biome.SWAMP
            return Biome.OCEAN
        if temp > 0.4:
            return Biome.DESERT
        height_val = self._terrain_noise.octave_noise2d(world_x * 0.01, world_z * 0.01, octaves=2)
        if height_val > 0.3:
            return Biome.MOUNTAINS
        if moisture > -0.1:
            return Biome.FOREST
        return Biome.PLAINS

    def _get_height(self, world_x, world_z, biome):
        noise_val = self._terrain_noise.octave_noise2d(
            world_x * 0.02, world_z * 0.02, octaves=5, persistence=0.5
        )
        return int(biome.base_height + noise_val * biome.height_variation)

    def _is_cave(self, world_x, y, world_z):
        if y <= 5 or y > 55:
            return False
        val = self._cave_noise.octave_noise2d(
            world_x * 0.05 + y * 0.1, world_z * 0.05 + y * 0.1, octaves=2
        )
        return val > 0.6

    def _place_ores(self, chunk, x, y, z):
        rng = random.Random(self.seed ^ (x * 73856093) ^ (y * 19349663) ^ (z * 83492791))
        r = rng.random()
        if y < 16 and r < 0.004:
            return Block.DIAMOND_ORE
        if y < 32 and r < 0.008:
            return Block.GOLD_ORE
        if y < 64 and r < 0.02:
            return Block.IRON_ORE
        if y < 80 and r < 0.03:
            return Block.COAL_ORE
        return None

    def _place_tree(self, chunk, x, surface_y, z):
        trunk_height = self._rng.randint(4, 6)
        for dy in range(1, trunk_height + 1):
            chunk.set_block(x, surface_y + dy, z, Block.WOOD)
        # Leaves canopy
        top = surface_y + trunk_height
        for dy in range(-1, 3):
            radius = 2 if dy < 2 else 1
            for dx in range(-radius, radius + 1):
                for dz in range(-radius, radius + 1):
                    lx, ly, lz = x + dx, top + dy, z + dz
                    if 0 <= lx < CHUNK_WIDTH and 0 <= lz < CHUNK_DEPTH and ly < CHUNK_HEIGHT:
                        if chunk.get_block(lx, ly, lz) == Block.AIR:
                            chunk.set_block(lx, ly, lz, Block.LEAVES)

    def generate_chunk(self, cx, cz):
        chunk = Chunk(cx, cz)

        for lx in range(CHUNK_WIDTH):
            for lz in range(CHUNK_DEPTH):
                world_x = cx * CHUNK_WIDTH + lx
                world_z = cz * CHUNK_DEPTH + lz

                biome = self._get_biome(world_x, world_z)
                height = self._get_height(world_x, world_z, biome)
                height = max(1, min(height, CHUNK_HEIGHT - 2))

                # Bedrock
                chunk.set_block(lx, 0, lz, Block.BEDROCK)

                # Stone fill
                for y in range(1, height - 3):
                    if self._is_cave(world_x, y, world_z):
                        if y < 10:
                            chunk.set_block(lx, y, lz, Block.LAVA)
                        continue
                    ore = self._place_ores(chunk, world_x, y, world_z)
                    chunk.set_block(lx, y, lz, ore if ore else Block.STONE)

                # Subsurface
                for y in range(max(1, height - 3), height):
                    chunk.set_block(lx, y, lz, biome.subsurface_block)

                # Surface
                chunk.set_block(lx, height, lz, biome.surface_block)

                # Water fill for ocean/low areas
                if height < self.SEA_LEVEL:
                    for y in range(height + 1, self.SEA_LEVEL + 1):
                        if biome == Biome.TUNDRA:
                            chunk.set_block(lx, y, lz, Block.ICE if y == self.SEA_LEVEL else Block.WATER)
                        else:
                            chunk.set_block(lx, y, lz, Block.WATER)

        # Decorations pass (trees, cacti)
        dec_rng = random.Random(self.seed ^ (cx * 341) ^ (cz * 743))
        for lx in range(2, CHUNK_WIDTH - 2):
            for lz in range(2, CHUNK_DEPTH - 2):
                world_x = cx * CHUNK_WIDTH + lx
                world_z = cz * CHUNK_DEPTH + lz
                biome = self._get_biome(world_x, world_z)
                surface_y = chunk.get_highest_block_y(lx, lz)

                if chunk.get_block(lx, surface_y, lz) == Block.GRASS and biome == Biome.FOREST:
                    if dec_rng.random() < 0.08:
                        self._place_tree(chunk, lx, surface_y, lz)
                elif chunk.get_block(lx, surface_y, lz) == Block.GRASS and biome == Biome.PLAINS:
                    if dec_rng.random() < 0.01:
                        self._place_tree(chunk, lx, surface_y, lz)
                elif chunk.get_block(lx, surface_y, lz) == Block.SAND and biome == Biome.DESERT:
                    if dec_rng.random() < 0.005:
                        for dy in range(1, dec_rng.randint(2, 4)):
                            chunk.set_block(lx, surface_y + dy, lz, Block.CACTUS)

        return chunk
