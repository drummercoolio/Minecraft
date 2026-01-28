"""Chunk representation for the Minecraft world."""

from .block import Block

CHUNK_WIDTH = 16
CHUNK_DEPTH = 16
CHUNK_HEIGHT = 256


class Chunk:
    """A 16x16x256 chunk of blocks."""

    def __init__(self, cx, cz):
        self.cx = cx
        self.cz = cz
        # Store blocks as flat list for efficiency: index = y * (W*D) + z * W + x
        self._blocks = [Block.AIR] * (CHUNK_WIDTH * CHUNK_DEPTH * CHUNK_HEIGHT)

    def _index(self, x, y, z):
        return y * (CHUNK_WIDTH * CHUNK_DEPTH) + z * CHUNK_WIDTH + x

    def get_block(self, x, y, z):
        if not (0 <= x < CHUNK_WIDTH and 0 <= z < CHUNK_DEPTH and 0 <= y < CHUNK_HEIGHT):
            return Block.AIR
        return self._blocks[self._index(x, y, z)]

    def set_block(self, x, y, z, block):
        if 0 <= x < CHUNK_WIDTH and 0 <= z < CHUNK_DEPTH and 0 <= y < CHUNK_HEIGHT:
            self._blocks[self._index(x, y, z)] = block

    def get_highest_block_y(self, x, z):
        """Return the y of the highest non-air block at (x, z)."""
        for y in range(CHUNK_HEIGHT - 1, -1, -1):
            if self.get_block(x, y, z) != Block.AIR:
                return y
        return 0

    def to_dict(self):
        return {
            "cx": self.cx,
            "cz": self.cz,
            "blocks": [b.value for b in self._blocks],
        }

    @classmethod
    def from_dict(cls, data):
        chunk = cls(data["cx"], data["cz"])
        chunk._blocks = [Block(v) for v in data["blocks"]]
        return chunk
