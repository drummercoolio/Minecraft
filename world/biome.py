"""Biome definitions for the Minecraft world."""

from enum import Enum
from .block import Block


class Biome(Enum):
    PLAINS = "plains"
    DESERT = "desert"
    FOREST = "forest"
    MOUNTAINS = "mountains"
    OCEAN = "ocean"
    TUNDRA = "tundra"
    SWAMP = "swamp"

    @property
    def surface_block(self):
        mapping = {
            Biome.PLAINS: Block.GRASS,
            Biome.DESERT: Block.SAND,
            Biome.FOREST: Block.GRASS,
            Biome.MOUNTAINS: Block.STONE,
            Biome.OCEAN: Block.SAND,
            Biome.TUNDRA: Block.SNOW,
            Biome.SWAMP: Block.CLAY,
        }
        return mapping[self]

    @property
    def subsurface_block(self):
        mapping = {
            Biome.PLAINS: Block.DIRT,
            Biome.DESERT: Block.SANDSTONE,
            Biome.FOREST: Block.DIRT,
            Biome.MOUNTAINS: Block.STONE,
            Biome.OCEAN: Block.GRAVEL,
            Biome.TUNDRA: Block.DIRT,
            Biome.SWAMP: Block.DIRT,
        }
        return mapping[self]

    @property
    def base_height(self):
        mapping = {
            Biome.PLAINS: 64,
            Biome.DESERT: 66,
            Biome.FOREST: 65,
            Biome.MOUNTAINS: 80,
            Biome.OCEAN: 40,
            Biome.TUNDRA: 63,
            Biome.SWAMP: 62,
        }
        return mapping[self]

    @property
    def height_variation(self):
        mapping = {
            Biome.PLAINS: 4,
            Biome.DESERT: 3,
            Biome.FOREST: 5,
            Biome.MOUNTAINS: 25,
            Biome.OCEAN: 8,
            Biome.TUNDRA: 3,
            Biome.SWAMP: 2,
        }
        return mapping[self]
