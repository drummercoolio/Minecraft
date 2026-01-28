"""World class that manages chunks and provides block access."""

import json
import gzip
import os

from .chunk import CHUNK_WIDTH, CHUNK_DEPTH, Chunk
from .generator import WorldGenerator


class World:
    """Represents a Minecraft world with lazy chunk generation."""

    def __init__(self, name="New World", seed=None):
        self.name = name
        self.generator = WorldGenerator(seed)
        self.seed = self.generator.seed
        self.chunks = {}
        self.spawn_x = 0
        self.spawn_y = 80
        self.spawn_z = 0

    def _chunk_key(self, cx, cz):
        return (cx, cz)

    def get_chunk(self, cx, cz):
        key = self._chunk_key(cx, cz)
        if key not in self.chunks:
            self.chunks[key] = self.generator.generate_chunk(cx, cz)
        return self.chunks[key]

    def get_block(self, x, y, z):
        cx = x // CHUNK_WIDTH
        cz = z // CHUNK_DEPTH
        lx = x % CHUNK_WIDTH
        lz = z % CHUNK_DEPTH
        return self.get_chunk(cx, cz).get_block(lx, y, lz)

    def set_block(self, x, y, z, block):
        cx = x // CHUNK_WIDTH
        cz = z // CHUNK_DEPTH
        lx = x % CHUNK_WIDTH
        lz = z % CHUNK_DEPTH
        self.get_chunk(cx, cz).set_block(lx, y, lz, block)

    def generate_spawn_area(self, radius=2):
        """Pre-generate chunks around spawn."""
        for cx in range(-radius, radius + 1):
            for cz in range(-radius, radius + 1):
                self.get_chunk(cx, cz)
        # Set spawn Y to surface level at origin
        chunk = self.get_chunk(0, 0)
        self.spawn_y = chunk.get_highest_block_y(0, 0) + 1

    def save(self, directory):
        os.makedirs(directory, exist_ok=True)
        meta = {
            "name": self.name,
            "seed": self.seed,
            "spawn": [self.spawn_x, self.spawn_y, self.spawn_z],
        }
        with open(os.path.join(directory, "level.json"), "w") as f:
            json.dump(meta, f, indent=2)

        chunks_dir = os.path.join(directory, "chunks")
        os.makedirs(chunks_dir, exist_ok=True)
        for (cx, cz), chunk in self.chunks.items():
            path = os.path.join(chunks_dir, f"c.{cx}.{cz}.json.gz")
            data = json.dumps(chunk.to_dict()).encode()
            with gzip.open(path, "wb") as f:
                f.write(data)

    @classmethod
    def load(cls, directory):
        with open(os.path.join(directory, "level.json")) as f:
            meta = json.load(f)

        world = cls(name=meta["name"], seed=meta["seed"])
        world.spawn_x, world.spawn_y, world.spawn_z = meta["spawn"]

        chunks_dir = os.path.join(directory, "chunks")
        if os.path.isdir(chunks_dir):
            for fname in os.listdir(chunks_dir):
                if fname.endswith(".json.gz"):
                    with gzip.open(os.path.join(chunks_dir, fname), "rb") as f:
                        data = json.loads(f.read().decode())
                    chunk = Chunk.from_dict(data)
                    world.chunks[(chunk.cx, chunk.cz)] = chunk

        return world

    def render_slice(self, y, x_range=None, z_range=None):
        """Render a horizontal slice of the world at given y level as text."""
        if x_range is None:
            x_range = range(-32, 32)
        if z_range is None:
            z_range = range(-32, 32)

        lines = []
        for z in z_range:
            row = ""
            for x in x_range:
                row += self.get_block(x, y, z).symbol
            lines.append(row)
        return "\n".join(lines)
