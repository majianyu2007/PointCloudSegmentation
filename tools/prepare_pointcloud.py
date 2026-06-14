#!/usr/bin/env python3
"""Convert simple ASCII PLY/XYZ point clouds to this project's colored PLY format.

The project only needs vertices. This script drops mesh faces and unsupported
properties, optionally samples points, and writes an ASCII PLY with x/y/z/r/g/b.
"""

import argparse
import random
import struct
from pathlib import Path


PLY_SCALAR_TYPES = {
    "char": ("b", 1),
    "uchar": ("B", 1),
    "int8": ("b", 1),
    "uint8": ("B", 1),
    "short": ("h", 2),
    "ushort": ("H", 2),
    "int16": ("h", 2),
    "uint16": ("H", 2),
    "int": ("i", 4),
    "uint": ("I", 4),
    "int32": ("i", 4),
    "uint32": ("I", 4),
    "float": ("f", 4),
    "float32": ("f", 4),
    "double": ("d", 8),
    "float64": ("d", 8),
}


def read_ply(path):
    with Path(path).open("rb") as inp:
        first = inp.readline().decode("ascii", errors="ignore").strip()
        if first != "ply":
            raise ValueError(f"{path} is not a PLY file")

        vertex_count = None
        properties = []
        in_vertex = False
        fmt = None

        for raw in inp:
            line = raw.decode("ascii", errors="ignore")
            parts = line.strip().split()
            if not parts:
                continue
            if parts[0] == "format":
                fmt = parts[1] if len(parts) > 1 else None
            elif parts[0] == "element":
                in_vertex = len(parts) > 1 and parts[1] == "vertex"
                if in_vertex:
                    vertex_count = int(parts[2])
            elif parts[0] == "property" and in_vertex:
                if parts[1] == "list":
                    raise ValueError(f"{path} has unsupported list property in vertex")
                properties.append((parts[1], parts[2]))
            elif parts[0] == "end_header":
                break

        if vertex_count is None:
            raise ValueError(f"{path} has no vertex element")

        prop_index = {name: i for i, (_, name) in enumerate(properties)}
        for name in ("x", "y", "z"):
            if name not in prop_index:
                raise ValueError(f"{path} misses vertex property {name}")

        points = []
        if fmt == "ascii":
            for _ in range(vertex_count):
                line = inp.readline().decode("ascii", errors="ignore")
                if not line:
                    break
                vals = line.strip().split()
                if len(vals) < len(properties):
                    continue
                x = float(vals[prop_index["x"]])
                y = float(vals[prop_index["y"]])
                z = float(vals[prop_index["z"]])
                points.append((x, y, z))
        elif fmt in ("binary_little_endian", "binary_big_endian"):
            endian = "<" if fmt == "binary_little_endian" else ">"
            struct_parts = []
            for typ, _ in properties:
                if typ not in PLY_SCALAR_TYPES:
                    raise ValueError(f"{path} has unsupported property type {typ}")
                struct_parts.append(PLY_SCALAR_TYPES[typ][0])
            row_struct = struct.Struct(endian + "".join(struct_parts))
            for _ in range(vertex_count):
                row = inp.read(row_struct.size)
                if len(row) < row_struct.size:
                    break
                vals = row_struct.unpack(row)
                points.append((
                    float(vals[prop_index["x"]]),
                    float(vals[prop_index["y"]]),
                    float(vals[prop_index["z"]]),
                ))
        else:
            raise ValueError(f"{path} has unsupported PLY format {fmt}")
        return points


def read_xyz(path):
    points = []
    with Path(path).open("r", encoding="utf-8", errors="ignore") as inp:
        for line in inp:
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            vals = line.split()
            if len(vals) < 3:
                continue
            points.append((float(vals[0]), float(vals[1]), float(vals[2])))
    return points


def choose_points(points, max_points, seed):
    if max_points <= 0 or len(points) <= max_points:
        return points
    rng = random.Random(seed)
    indexes = sorted(rng.sample(range(len(points)), max_points))
    return [points[i] for i in indexes]


def write_ply(path, points, color):
    out_path = Path(path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("w", encoding="ascii") as out:
        out.write("ply\n")
        out.write("format ascii 1.0\n")
        out.write(f"element vertex {len(points)}\n")
        out.write("property float x\n")
        out.write("property float y\n")
        out.write("property float z\n")
        out.write("property uchar red\n")
        out.write("property uchar green\n")
        out.write("property uchar blue\n")
        out.write("end_header\n")
        for x, y, z in points:
            out.write(f"{x:.7g} {y:.7g} {z:.7g} {color[0]} {color[1]} {color[2]}\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input")
    parser.add_argument("output")
    parser.add_argument("--max-points", type=int, default=0)
    parser.add_argument("--seed", type=int, default=20260614)
    parser.add_argument("--color", default="210,210,210")
    args = parser.parse_args()

    in_path = Path(args.input)
    if in_path.suffix.lower() == ".ply":
        points = read_ply(in_path)
    else:
        points = read_xyz(in_path)
    if not points:
        raise SystemExit("no points read")

    points = choose_points(points, args.max_points, args.seed)
    color = tuple(int(v) for v in args.color.split(","))
    if len(color) != 3:
        raise SystemExit("--color must be R,G,B")
    write_ply(args.output, points, color)
    print(f"{args.output}: {len(points)} points")


if __name__ == "__main__":
    main()
