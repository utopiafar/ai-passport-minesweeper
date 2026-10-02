#!/usr/bin/env python3
"""Check fixed UI inventory against the actual generated LVGL cmap tables."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent.parent


def inventory():
    chars = set(range(32, 127))
    for filename in ("main/minesweeper_text.h", "main/minesweeper_ui.c"):
        chars.update(ord(c) for c in (ROOT / filename).read_text() if ord(c) > 127)
    return chars


def coverage(source):
    lists = {}
    for name, values in re.findall(r"static const uint16_t (unicode_list_\d+)\[\] = \{(.*?)\};", source, re.S):
        lists[name] = [int(v, 0) for v in re.findall(r"0x[0-9a-fA-F]+|\b\d+\b", values)]
    blocks = re.findall(r"\.range_start = (\d+), \.range_length = (\d+),.*?\.unicode_list = (\w+),.*?\.type = (\w+)", source, re.S)
    assert blocks, "No cmap tables found"
    result = set()
    for start, length, name, kind in blocks:
        start, length = int(start), int(length)
        if kind == "LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY":
            result.update(range(start, start + length))
        elif kind == "LV_FONT_FMT_TXT_CMAP_SPARSE_TINY":
            result.update(start + offset for offset in lists[name])
        else:
            raise AssertionError(f"Unsupported cmap type: {kind}")
    return result


if __name__ == "__main__":
    required = inventory()
    header = (ROOT / "assets/fonts/minesweeper_codepoints.h").read_text()
    declared = {int(cp, 16) for cp in re.findall(r"0x([0-9a-f]+)", header)}
    assert required == declared, "Regenerate the font inventory after changing UI text"
    for size in (12, 14):
        actual = coverage((ROOT / f"assets/fonts/ms_font_{size}.c").read_text())
        assert required <= actual, f"Missing glyphs ({size}px): {required - actual}"
        assert 0x9F98 not in actual, "Negative coverage probe should be absent"
    print(f"Chinese fonts: PASS ({len(required)} codepoints at 12/14px; negative probe checked)")
