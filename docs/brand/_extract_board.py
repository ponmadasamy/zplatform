#!/usr/bin/env python3
# Copyright (c) 2026 ZPlatform contributors
# SPDX-License-Identifier: Apache-2.0
#
# Optional helper to re-crop from the master brand board.
# Prefer hand-curated assets in assets/ — do not overwrite cleaned PNGs
# unless you pass --force.
#
# Quote + color palette are text in brand.yml (not images).
#
#   python docs/brand/_extract_board.py [--force]

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image

DEFAULT_SRC = Path.home() / "Downloads" / "15d7b85f-fca3-4db0-b73f-1ec549073fb3.png"
BRAND_DIR = Path(__file__).resolve().parent
ASSETS = BRAND_DIR / "assets"

# Hand-tuned files: skip unless --force
PROTECTED = {
    "logo-lockup.png",
    "logo-mark.png",
    "icon.png",
    "icon-on-light.png",
    "icon-on-dark.png",
    "icon-mono.png",
    "icon-social.png",
    "icons-app-favicon.png",
    "all-icon.png",
    "hero-banner.png",
    "banner-circuit.png",
    "evk-lcd-demo.png",
}


def frac_box(
    w: int, h: int, l: float, t: float, r: float, b: float
) -> tuple[int, int, int, int]:
    return (int(l * w), int(t * h), int(r * w), int(b * h))


def save_crop(
    im: Image.Image, box: tuple[int, int, int, int], name: str, force: bool
) -> Path | None:
    ASSETS.mkdir(parents=True, exist_ok=True)
    path = ASSETS / name
    if path.exists() and name in PROTECTED and not force:
        print(f"  skip {name} (protected; use --force to overwrite)")
        return None
    im.crop(box).convert("RGBA").save(path, optimize=True)
    print(f"  wrote {name:40s} {path.stat().st_size:8d} B")
    return path


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--src", type=Path, default=DEFAULT_SRC)
    ap.add_argument(
        "--force",
        action="store_true",
        help="Overwrite hand-curated assets (destructive)",
    )
    args = ap.parse_args()
    if not args.src.is_file():
        raise SystemExit(f"missing source image: {args.src}")

    im = Image.open(args.src).convert("RGBA")
    w, h = im.size
    print(f"source {args.src} ({w}x{h})")

    def box(l: float, t: float, r: float, b: float) -> tuple[int, int, int, int]:
        return frac_box(w, h, l, t, r, b)

    # No palette.png / quote-card.png — those are brand.yml text.
    crops = [
        (box(0.015, 0.015, 0.285, 0.385), "logo-lockup.png"),
        (box(0.055, 0.025, 0.245, 0.265), "logo-mark.png"),
        (box(0.295, 0.01, 0.99, 0.39), "hero-banner.png"),
        (box(0.01, 0.40, 0.685, 0.74), "banner-circuit.png"),
        (box(0.695, 0.40, 0.99, 0.74), "evk-lcd-demo.png"),
    ]

    for b, name in crops:
        save_crop(im, b, name, args.force)

    full = ASSETS / "brand-board-full.png"
    if full.exists() and not args.force:
        print("  skip brand-board-full.png (protected; use --force)")
    else:
        im.save(full, optimize=True)
        print(f"  wrote brand-board-full.png {full.stat().st_size:8d} B")

    print(f"done -> {ASSETS}")


if __name__ == "__main__":
    main()
