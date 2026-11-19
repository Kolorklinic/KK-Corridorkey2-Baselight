#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# CorridorKey 2 for Resolve - colour check against Resolve  (v1.06)
#
# From Creative Video Tips (CVT)
# Created by Chadwick  |  https://creativevideotips.com
# An unofficial plugin around CorridorKey 2.0 by Corridor Digital.
# Licence: CC BY-NC-SA 4.0 with the CorridorKey additional terms (see LICENSE).
#
# What it does:
#   1. chart <folder>: writes ck2_colour_chart.exr, a 1920x1080 chart of 50
#      flat patches in linear Rec.709 (greys from 10 stops under to 6 over,
#      colours, skin, screens, highlights, colours outside Rec.709), 32-bit
#      float, Zip (lossless), and the Resolve steps next to it.
#   2. compare <render folder or files>: reads Resolve's renders of the chart
#      through Color Space Transform (frame 0 untouched, frames 1-12 one space
#      each, in the order of SPACES) and compares every patch with what the
#      plugin's own code gives (build/cmake/ck2_tests convert).
#
# Run with the CorridorKey Python (numpy + OpenEXR):
#   ~/CorridorKey/.venv/bin/python tests/colour_check.py chart <folder>
#   ~/CorridorKey/.venv/bin/python tests/colour_check.py compare <folder>
# ---------------------------------------------------------------------------
import subprocess
import sys
from pathlib import Path

import numpy as np
import OpenEXR

W, H, COLS, ROWS, CELL, PATCH = 1920, 1080, 10, 5, 192, 150
HERE = Path(__file__).resolve().parent
TOOL = HERE.parent / "build" / "cmake" / "ck2_tests"

GREY = [0.18 * 2.0 ** s for s in (-10, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6)]
PATCHES = (
    [("grey %+d" % s, (v, v, v)) for s, v in zip((-10, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6), GREY)]
    + [("black", (0.0, 0.0, 0.0)), ("near black", (0.0005,) * 3), ("dark", (0.01,) * 3),
       ("50%", (0.5,) * 3), ("white", (1.0,) * 3)]
    + [("red", (0.5, 0.05, 0.05)), ("green", (0.05, 0.5, 0.05)), ("blue", (0.05, 0.05, 0.5)),
       ("cyan", (0.05, 0.4, 0.4)), ("magenta", (0.4, 0.05, 0.4)), ("yellow", (0.45, 0.4, 0.05)),
       ("light skin", (0.45, 0.3, 0.22)), ("dark skin", (0.12, 0.07, 0.05)),
       ("green screen", (0.08, 0.35, 0.1)), ("blue screen", (0.04, 0.1, 0.45))]
    + [("full red", (1, 0, 0)), ("full green", (0, 1, 0)), ("full blue", (0, 0, 1)),
       ("full cyan", (0, 1, 1)), ("full magenta", (1, 0, 1)), ("full yellow", (1, 1, 0)),
       ("highlight", (4, 3, 2)), ("hot orange", (12, 6, 3)), ("hot sky", (2, 4, 8)), ("hot white", (16, 16, 16))]
    + [("wide red", (0.6, -0.02, -0.01)), ("wide green", (-0.03, 0.5, -0.02)),
       ("wide blue", (-0.01, -0.02, 0.6)), ("wide lime", (0.3, 0.4, -0.05)),
       ("wide teal", (-0.05, 0.3, 0.35)), ("wide purple", (0.5, -0.05, 0.4)),
       ("below black", (-0.002, -0.002, -0.002)), ("wide orange", (0.9, 0.1, -0.03)),
       ("hot wide red", (1.5, -0.1, -0.05)), ("hot wide green", (-0.1, 1.2, -0.05))]
)
OUTSIDE = 40   # patches from here on are outside Rec.709 or below black

# Frame order of the Resolve render: (Input Is number, name, Color Space Transform output).
SPACES = [
    (4, "DWG / DaVinci Intermediate", "DaVinci Wide Gamut / DaVinci Intermediate"),
    (3, "DWG Linear", "DaVinci Wide Gamut / Linear"),
    (5, "Sony S-Log3 / S-Gamut3.Cine", "Sony S-Gamut3.Cine / Sony S-Log3"),
    (6, "Blackmagic Film Gen 5", "Blackmagic Design Wide Gamut Gen 4/5 / Blackmagic Design Film Gen 5"),
    (2, "ACEScg", "ACES (AP1) / Linear"),
    (7, "ACEScct", "ACES (AP1) / ACEScct"),
    (8, "ARRI LogC3 (EI 800)", "ARRI Wide Gamut 3 / ARRI LogC3"),
    (9, "ARRI LogC4", "ARRI Wide Gamut 4 / ARRI LogC4"),
    (18, "Panasonic V-Log / V-Gamut", "Panasonic V-Gamut / Panasonic V-Log"),
    (11, "Canon Log 2 / Cinema Gamut", "Canon Cinema Gamut / Canon Log 2"),
    (12, "Canon Log 3 / Cinema Gamut", "Canon Cinema Gamut / Canon Log 3"),
    (10, "RED Log3G10 / REDWideGamutRGB", "REDWideGamutRGB / RED Log3G10"),
    # Added in 1.06 (frames 13-15; renders with only 13 frames still work).
    (17, "DJI D-Log / D-Gamut", "DJI D-Gamut / DJI D-Log"),
    (13, "Apple Log / Rec.2020", "Rec.2020 / Apple Log"),
    (14, "Apple Log 2 / Apple Gamut", "Apple Gamut / Apple Log 2 (if Resolve lists it)"),
]


def cell(i: int) -> tuple[int, int]:
    """Top-left corner of patch i (rows top to bottom)."""
    x0 = (W - COLS * CELL) // 2 + (i % COLS) * CELL + (CELL - PATCH) // 2
    y0 = (H - ROWS * CELL) // 2 + (i // COLS) * CELL + (CELL - PATCH) // 2
    return x0, y0


def make_chart(folder: Path) -> None:
    folder.mkdir(parents=True, exist_ok=True)
    img = np.full((H, W, 3), 0.18 * 2.0 ** -3, np.float32)     # dark grey surround
    for i, (_, rgb) in enumerate(PATCHES):
        x0, y0 = cell(i)
        img[y0:y0 + PATCH, x0:x0 + PATCH] = rgb
    ch = {c: np.ascontiguousarray(img[..., k]) for k, c in enumerate("RGB")}
    hdr = {"compression": OpenEXR.ZIP_COMPRESSION, "type": OpenEXR.scanlineimage}
    with OpenEXR.File(hdr, ch) as f:
        f.write(str(folder / "ck2_colour_chart.exr"))
    lines = ["CorridorKey 2 colour check: Resolve steps", "",
             "Project: DaVinci YRGB (not colour managed), 32-bit float processing.",
             "Timeline: 1920x1080. Put ck2_colour_chart.exr on V1 16 times, 1 frame each,",
             "one after another. Frame 0 stays untouched. On the Color page give each",
             "of frames 1-15 one Color Space Transform node:",
             "  Input Color Space: Rec.709, Input Gamma: Linear",
             "  Output: as listed below",
             "  Tone Mapping: None, Gamut Mapping: None, OOTF: off, White point adaptation: on", ""]
    lines += [f"  frame {n:2}: {cst}" for n, (_, _, cst) in enumerate(SPACES, 1)]
    lines += ["", "Deliver: EXR, RGB float, compression None or Zip, 1920x1080, one image",
              "sequence of the frames (skip frame 15 if Resolve has no Apple Gamut). Then run:",
              "  ~/CorridorKey/.venv/bin/python tests/colour_check.py compare <render folder>"]
    (folder / "README.txt").write_text("\n".join(lines) + "\n")
    print(f"wrote {folder / 'ck2_colour_chart.exr'} ({len(PATCHES)} patches) and README.txt")


def read_patches(path: Path) -> np.ndarray:
    with OpenEXR.File(str(path)) as f:
        ch = f.channels()
        if "RGB" in ch:
            img = np.asarray(ch["RGB"].pixels, np.float64)
        else:
            img = np.stack([np.asarray(ch[c].pixels, np.float64) for c in "RGB"], -1)
    if img.shape[:2] != (H, W):
        raise SystemExit(f"{path.name}: {img.shape[1]}x{img.shape[0]}, expected {W}x{H}")
    m = PATCH // 4                                              # centre half of each patch
    out = []
    for i in range(len(PATCHES)):
        x0, y0 = cell(i)
        out.append(img[y0 + m:y0 + PATCH - m, x0 + m:x0 + PATCH - m].reshape(-1, 3).mean(0))
    return np.array(out)


def plugin_values(space: int) -> np.ndarray:
    stdin = "\n".join("%.9g %.9g %.9g" % tuple(rgb) for _, rgb in PATCHES) + "\n"
    out = subprocess.run([str(TOOL), "convert", str(space), "encode"], input=stdin,
                         capture_output=True, text=True, check=True).stdout
    return np.array([[float(v) for v in line.split()] for line in out.splitlines()])


def compare(args: list[str]) -> None:
    files = []
    for a in args:
        p = Path(a)
        files += sorted(p.glob("*.exr")) if p.is_dir() else [p]
    if len(files) < 13 or len(files) > len(SPACES) + 1:
        raise SystemExit(f"found {len(files)} EXR files, expected 13 to {len(SPACES) + 1} (frame 0 + one per space)")
    chart = np.array([rgb for _, rgb in PATCHES], np.float64)
    control = read_patches(files[0])
    d0 = np.abs(control - chart).max()
    print(f"frame 0 (untouched): largest difference from the chart {d0:.2e}"
          + ("" if d0 < 1e-4 else "  <- Resolve changed the chart: check the project settings"))
    print()
    print(f"{'frame':5} {'space':44} {'in Rec.709':>11} {'outside':>9}  worst patch")
    for n, (space, name, _) in enumerate(SPACES[:len(files) - 1], 1):
        resolve = read_patches(files[n])
        ours = plugin_values(space)
        # Linear outputs: relative difference; log outputs: code values (0.001 is about 1 in 10 bits).
        linear = space in (2, 3)
        diff = np.abs(resolve - ours)
        if linear:
            diff = diff / np.maximum(np.abs(ours), 0.01)
        per = diff.max(1)
        inside, outside = per[:OUTSIDE].max(), per[OUTSIDE:].max()
        w = int(np.argmax(per))
        unit = "rel" if linear else "code"
        print(f"{n:5} {name:44} {inside:11.5f} {outside:9.5f}  {PATCHES[w][0]} ({unit}): "
              f"Resolve {np.round(resolve[w], 5).tolist()} plugin {np.round(ours[w], 5).tolist()}")
    print()
    print("Within about 0.001 (code) or 0.005 (rel) matches Resolve. 'outside' covers colours")
    print("outside Rec.709 and below black, where Resolve may clip.")


if __name__ == "__main__":
    if len(sys.argv) == 3 and sys.argv[1] == "chart":
        make_chart(Path(sys.argv[2]))
    elif len(sys.argv) >= 3 and sys.argv[1] == "compare":
        compare(sys.argv[2:])
    else:
        raise SystemExit(__doc__ or "usage: colour_check.py chart <folder> | compare <renders>")
