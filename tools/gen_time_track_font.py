#!/usr/bin/env python3
"""Generate assets/fonts/time_track_font_16.c from a licensed CJK source."""

from __future__ import annotations

import os
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "main" / "time_track_text.h"
OUTPUT = ROOT / "assets" / "fonts" / "time_track_font_16.c"
DEFAULT_LATIN_FONT = Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")
DEFAULT_CJK_FONT = Path("/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf")
CONVERTER = os.environ.get("TIME_TRACK_FONT_CONV_VERSION", "1.5.3")


def cjk_symbols() -> str:
    text = HEADER.read_text(encoding="utf-8")
    match = re.search(r"#define TIME_TRACK_FONT_SYMBOLS \"([^\"]+)\"", text)
    if not match:
        raise SystemExit("TIME_TRACK_FONT_SYMBOLS not found")
    return match.group(1)


def main() -> int:
    latin_src = Path(os.environ.get("TIME_TRACK_LATIN_FONT_SRC", DEFAULT_LATIN_FONT))
    cjk_src = Path(os.environ.get("TIME_TRACK_FONT_SRC", DEFAULT_CJK_FONT))
    for path in (latin_src, cjk_src):
        if not path.is_file():
            print(f"ERROR: missing source font: {path}", file=sys.stderr)
            return 1

    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    command = [
        "npx",
        "--yes",
        f"lv_font_conv@{CONVERTER}",
        "--size",
        "16",
        "--bpp",
        "4",
        "--format",
        "lvgl",
        "--no-compress",
        "--lv-font-name",
        "time_track_font_16",
        "--lv-include",
        "lvgl.h",
        "--font",
        str(latin_src),
        "--range",
        "0x20-0x7E",
        "--font",
        str(cjk_src),
        "--symbols",
        cjk_symbols(),
        "--output",
        str(OUTPUT),
    ]
    subprocess.run(command, check=True)
    print(f"Wrote {OUTPUT}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
