"""Block types for the Minecraft world."""

from enum import Enum


class Block(Enum):
    AIR = 0
    STONE = 1
    DIRT = 2
    GRASS = 3
    SAND = 4
    WATER = 5
    WOOD = 6
    LEAVES = 7
    BEDROCK = 8
    GRAVEL = 9
    COAL_ORE = 10
    IRON_ORE = 11
    GOLD_ORE = 12
    DIAMOND_ORE = 13
    SNOW = 14
    ICE = 15
    CACTUS = 16
    CLAY = 17
    SANDSTONE = 18
    OBSIDIAN = 19
    LAVA = 20

    @property
    def symbol(self):
        symbols = {
            Block.AIR: " ",
            Block.STONE: "S",
            Block.DIRT: "D",
            Block.GRASS: "G",
            Block.SAND: "~",
            Block.WATER: "W",
            Block.WOOD: "|",
            Block.LEAVES: "&",
            Block.BEDROCK: "#",
            Block.GRAVEL: "%",
            Block.COAL_ORE: "C",
            Block.IRON_ORE: "I",
            Block.GOLD_ORE: "$",
            Block.DIAMOND_ORE: "*",
            Block.SNOW: ".",
            Block.ICE: "=",
            Block.CACTUS: "!",
            Block.CLAY: "c",
            Block.SANDSTONE: "s",
            Block.OBSIDIAN: "O",
            Block.LAVA: "L",
        }
        return symbols.get(self, "?")

    @property
    def is_solid(self):
        return self not in (Block.AIR, Block.WATER, Block.LAVA)
