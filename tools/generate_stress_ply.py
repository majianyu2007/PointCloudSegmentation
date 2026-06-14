#!/usr/bin/env python3
"""Generate synthetic PLY files for point-cloud segmentation stress tests."""

import math
import random
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data"


def jitter(rng, sigma):
    return rng.gauss(0.0, sigma)


def add_point(points, x, y, z, color, rng, noise=0.0):
    points.append((
        x + jitter(rng, noise),
        y + jitter(rng, noise),
        z + jitter(rng, noise),
        color[0],
        color[1],
        color[2],
    ))


def add_plane(points, rng, count, origin, u, v, color, noise=0.0):
    ox, oy, oz = origin
    ux, uy, uz = u
    vx, vy, vz = v
    for _ in range(count):
        a = rng.random()
        b = rng.random()
        add_point(
            points,
            ox + a * ux + b * vx,
            oy + a * uy + b * vy,
            oz + a * uz + b * vz,
            color,
            rng,
            noise,
        )


def add_sphere_patch(points, rng, count, center, radius, color, noise=0.0,
                     theta_range=(0.0, math.pi), phi_range=(0.0, 2.0 * math.pi)):
    cx, cy, cz = center
    t0, t1 = theta_range
    p0, p1 = phi_range
    for _ in range(count):
        theta = t0 + (t1 - t0) * rng.random()
        phi = p0 + (p1 - p0) * rng.random()
        x = cx + radius * math.sin(theta) * math.cos(phi)
        y = cy + radius * math.sin(theta) * math.sin(phi)
        z = cz + radius * math.cos(theta)
        add_point(points, x, y, z, color, rng, noise)


def add_cylinder(points, rng, count, center, radius, height, color, noise=0.0,
                 angle_range=(0.0, 2.0 * math.pi)):
    cx, cy, cz = center
    a0, a1 = angle_range
    for _ in range(count):
        a = a0 + (a1 - a0) * rng.random()
        z = cz + height * (rng.random() - 0.5)
        x = cx + radius * math.cos(a)
        y = cy + radius * math.sin(a)
        add_point(points, x, y, z, color, rng, noise)


def add_torus(points, rng, count, center, major, minor, color, noise=0.0):
    cx, cy, cz = center
    for _ in range(count):
        u = 2.0 * math.pi * rng.random()
        v = 2.0 * math.pi * rng.random()
        x = cx + (major + minor * math.cos(v)) * math.cos(u)
        y = cy + (major + minor * math.cos(v)) * math.sin(u)
        z = cz + minor * math.sin(v)
        add_point(points, x, y, z, color, rng, noise)


def add_outliers(points, rng, count, bounds, color):
    (xmin, xmax), (ymin, ymax), (zmin, zmax) = bounds
    for _ in range(count):
        points.append((
            rng.uniform(xmin, xmax),
            rng.uniform(ymin, ymax),
            rng.uniform(zmin, zmax),
            color[0],
            color[1],
            color[2],
        ))


def write_ply(path, points, neutral_color=(210, 210, 210)):
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="ascii") as out:
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
        r, g, b = neutral_color
        for x, y, z, *_ in points:
            out.write(f"{x:.6f} {y:.6f} {z:.6f} {r} {g} {b}\n")


def make_complex_scene():
    rng = random.Random(20260614)
    points = []
    add_plane(points, rng, 9000, (-3.0, -3.0, 0.0), (6.0, 0.0, 0.0), (0.0, 6.0, 0.0), (160, 160, 160), 0.010)
    add_plane(points, rng, 5000, (-2.3, 1.0, 0.05), (2.8, 0.0, 0.0), (0.0, 0.9, 1.9), (230, 140, 70), 0.009)
    add_plane(points, rng, 4200, (0.25, -2.2, 0.03), (0.0, 3.8, 0.0), (0.0, 0.0, 1.7), (80, 170, 235), 0.008)
    add_sphere_patch(points, rng, 6200, (-1.1, -0.8, 0.75), 0.72, (220, 85, 95), 0.012,
                     theta_range=(0.15, 2.75), phi_range=(-0.2, 1.75 * math.pi))
    add_cylinder(points, rng, 5200, (1.4, 0.7, 0.75), 0.42, 1.5, (80, 210, 135), 0.010)
    add_torus(points, rng, 3800, (0.45, -1.35, 0.7), 0.55, 0.13, (180, 110, 230), 0.009)
    add_outliers(points, rng, 1800, ((-3.5, 3.5), (-3.5, 3.5), (-0.2, 2.4)), (30, 30, 30))
    rng.shuffle(points)
    return points


def make_close_parallel_planes():
    rng = random.Random(271828)
    points = []
    add_plane(points, rng, 9000, (-2.4, -2.0, 0.0), (4.8, 0.0, 0.0), (0.0, 4.0, 0.0), (170, 170, 170), 0.003)
    add_plane(points, rng, 9000, (-2.4, -2.0, 0.075), (4.8, 0.0, 0.0), (0.0, 4.0, 0.0), (210, 120, 80), 0.003)
    add_plane(points, rng, 3500, (-2.4, -2.0, 0.0), (0.0, 4.0, 0.0), (0.0, 0.0, 0.075), (80, 150, 230), 0.002)
    add_plane(points, rng, 3500, (2.4, -2.0, 0.0), (0.0, 4.0, 0.0), (0.0, 0.0, 0.075), (80, 150, 230), 0.002)
    add_cylinder(points, rng, 2200, (0.0, 0.0, 0.04), 0.85, 0.08, (95, 210, 120), 0.003)
    add_outliers(points, rng, 800, ((-2.7, 2.7), (-2.3, 2.3), (-0.05, 0.18)), (25, 25, 25))
    rng.shuffle(points)
    return points


def make_sparse_noisy_scene():
    rng = random.Random(314159)
    points = []
    add_plane(points, rng, 4200, (-3.0, -3.0, 0.0), (6.0, 0.0, 0.0), (0.0, 6.0, 0.0), (150, 150, 150), 0.035)
    add_sphere_patch(points, rng, 2600, (-1.0, -0.8, 0.8), 0.7, (230, 80, 90), 0.040)
    add_cylinder(points, rng, 2400, (1.25, 0.65, 0.75), 0.43, 1.45, (75, 200, 135), 0.035)
    add_plane(points, rng, 2100, (-1.8, 1.3, 0.0), (1.9, 0.0, 0.0), (0.0, 0.8, 1.5), (230, 145, 70), 0.035)
    add_outliers(points, rng, 3500, ((-4.0, 4.0), (-4.0, 4.0), (-0.5, 2.7)), (25, 25, 25))
    rng.shuffle(points)
    return points


def make_large_scene():
    rng = random.Random(1618033)
    points = []
    add_plane(points, rng, 32000, (-4.0, -4.0, 0.0), (8.0, 0.0, 0.0), (0.0, 8.0, 0.0), (160, 160, 160), 0.006)
    add_sphere_patch(points, rng, 18000, (-1.7, -1.2, 0.9), 0.9, (230, 80, 95), 0.008)
    add_sphere_patch(points, rng, 14000, (1.9, -1.0, 0.65), 0.62, (210, 180, 70), 0.008,
                     theta_range=(0.0, 0.88 * math.pi))
    add_cylinder(points, rng, 15000, (1.0, 1.6, 0.9), 0.48, 1.8, (75, 200, 135), 0.008)
    add_plane(points, rng, 9000, (-2.6, 1.5, 0.0), (2.6, 0.0, 0.0), (0.0, 1.1, 2.1), (80, 160, 235), 0.007)
    add_torus(points, rng, 7000, (0.1, -2.4, 0.85), 0.75, 0.14, (185, 110, 230), 0.007)
    add_outliers(points, rng, 5000, ((-4.5, 4.5), (-4.5, 4.5), (-0.35, 3.0)), (30, 30, 30))
    rng.shuffle(points)
    return points


def main():
    cases = {
        "stress_complex_35200.ply": make_complex_scene(),
        "stress_close_parallel_28000.ply": make_close_parallel_planes(),
        "stress_sparse_noisy_14800.ply": make_sparse_noisy_scene(),
        "stress_large_100000.ply": make_large_scene(),
    }
    for name, points in cases.items():
        path = DATA_DIR / name
        write_ply(path, points)
        print(f"{path.relative_to(ROOT)}: {len(points)} points")


if __name__ == "__main__":
    main()
