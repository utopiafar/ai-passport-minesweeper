#!/usr/bin/env python3
"""Rebuild the application subset with fontTools 4.60.2 / lv_font_conv 1.5.3.

Usage: python3 tools/generate_minesweeper_fonts.py --font full-font.otf
       --converter /path/to/lv_font_conv
"""
import argparse
from pathlib import Path
import subprocess
from fontTools import subset
from fontTools.ttLib import TTFont
from check_minesweeper_fonts import ROOT, inventory

parser = argparse.ArgumentParser()
parser.add_argument("--font", type=Path, required=True)
parser.add_argument("--converter", required=True)
args = parser.parse_args()
cps = sorted(inventory())
font = TTFont(args.font)
assert set(cps) <= set(font.getBestCmap()), "Source font lacks required characters"
opts = subset.Options()
opts.recalc_timestamp = False
subsetter = subset.Subsetter(options=opts)
subsetter.populate(unicodes=cps)
subsetter.subset(font)
destination = ROOT / "assets/fonts"
font.save(destination / "NotoSansCJKsc-Minesweeper.otf")
(destination / "minesweeper_codepoints.h").write_text(
    "#pragma once\n#include <stdint.h>\nstatic const uint32_t ms_codepoints[] = {\n    "
    + ", ".join(f"0x{cp:x}" for cp in cps) + "\n};\n")
for size in (12, 14):
    subprocess.run([
        str(Path(args.converter).resolve()), "--font", "assets/fonts/NotoSansCJKsc-Minesweeper.otf",
        "--range", ",".join(f"0x{cp:x}" for cp in cps), "--size", str(size),
        "--bpp", "4", "--format", "lvgl", "--no-compress", "--no-kerning",
        "--lv-font-name", f"ms_font_{size}", "--lv-include", "lvgl.h",
        "--output", f"assets/fonts/ms_font_{size}.c"], check=True, cwd=ROOT)
    output = destination / f"ms_font_{size}.c"
    output.write_text(output.read_text().rstrip() + "\n")
