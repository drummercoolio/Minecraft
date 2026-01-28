#!/usr/bin/env python3
"""Minecraft world generator - creates and displays a procedurally generated world."""

import argparse
import os
import sys
import time

from world.world import World


def create_world(name="My World", seed=None, spawn_radius=2):
    """Create a new Minecraft world."""
    print(f"Creating world '{name}'...")
    if seed is not None:
        print(f"  Seed: {seed}")

    world = World(name=name, seed=seed)
    print(f"  World seed: {world.seed}")

    print(f"  Generating spawn area (radius={spawn_radius})...")
    start = time.time()
    world.generate_spawn_area(radius=spawn_radius)
    elapsed = time.time() - start

    total_chunks = len(world.chunks)
    print(f"  Generated {total_chunks} chunks in {elapsed:.2f}s")
    print(f"  Spawn point: ({world.spawn_x}, {world.spawn_y}, {world.spawn_z})")

    return world


def display_world(world, y=None):
    """Display a horizontal slice of the world."""
    if y is None:
        y = world.spawn_y - 1  # Show surface level
    print(f"\nWorld slice at y={y} (64x64 blocks):")
    print("-" * 64)
    print(world.render_slice(y))
    print("-" * 64)
    print("Legend: G=Grass D=Dirt S=Stone ~=Sand W=Water |=Wood &=Leaves #=Bedrock")
    print("        .=Snow ==Ice !=Cactus C=Coal I=Iron $=Gold *=Diamond L=Lava")


def main():
    parser = argparse.ArgumentParser(description="Minecraft World Generator")
    parser.add_argument("--name", default="My World", help="World name")
    parser.add_argument("--seed", type=int, default=None, help="World seed")
    parser.add_argument("--radius", type=int, default=2, help="Spawn area radius in chunks")
    parser.add_argument("--save", type=str, default=None, help="Directory to save world")
    parser.add_argument("--load", type=str, default=None, help="Directory to load world from")
    parser.add_argument("--slice-y", type=int, default=None, help="Y level for world slice display")
    parser.add_argument("--no-display", action="store_true", help="Skip rendering the world slice")
    args = parser.parse_args()

    if args.load:
        print(f"Loading world from '{args.load}'...")
        world = World.load(args.load)
        print(f"  World: {world.name} (seed: {world.seed})")
        print(f"  Loaded {len(world.chunks)} chunks")
    else:
        world = create_world(name=args.name, seed=args.seed, spawn_radius=args.radius)

    if not args.no_display:
        display_world(world, y=args.slice_y)

    if args.save:
        print(f"\nSaving world to '{args.save}'...")
        world.save(args.save)
        print("  World saved!")

    return world


if __name__ == "__main__":
    main()
