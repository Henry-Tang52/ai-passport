#!/usr/bin/env python3
"""Generate Time Ledger LVGL bitmap fonts from licensed CJK/Latin sources."""

from __future__ import annotations

import argparse
import os
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "main" / "time_track_text.h"
DEFAULT_LATIN_FONT = Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")
DEFAULT_CJK_FONT = Path("/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf")
CONVERTER = os.environ.get("TIME_TRACK_FONT_CONV_VERSION", "1.5.3")
HEADER_LATIN = "0123456789hm "


def header_macro(name: str) -> str:
    text = HEADER.read_text(encoding="utf-8")
    match = re.search(rf"#define {name} \"([^\"]+)\"", text)
    if not match:
        raise SystemExit(f"{name} not found")
    return match.group(1)


def unique_chars(text: str) -> str:
    seen: set[str] = set()
    out: list[str] = []
    for char in text:
        if char not in seen:
            seen.add(char)
            out.append(char)
    return "".join(out)


def cjk_symbols() -> str:
    return header_macro("TIME_TRACK_FONT_SYMBOLS")


def title_symbols() -> str:
    return unique_chars(header_macro("TIME_TRACK_TEXT_TITLE"))


def total_symbols() -> str:
    return unique_chars(
        header_macro("TIME_TRACK_TEXT_TODAY_TOTAL")
        + header_macro("TIME_TRACK_TEXT_YESTERDAY_TOTAL")
    )


def generate(
    size: int,
    name: str,
    output: Path,
    cjk: str,
    *,
    latin_range: str | None = None,
    latin_symbols: str | None = None,
) -> None:
    latin_src = Path(os.environ.get("TIME_TRACK_LATIN_FONT_SRC", DEFAULT_LATIN_FONT))
    cjk_src = Path(os.environ.get("TIME_TRACK_FONT_SRC", DEFAULT_CJK_FONT))
    for path in (latin_src, cjk_src):
        if (latin_range or latin_symbols) and path == latin_src and not path.is_file():
            print(f"ERROR: missing source font: {path}", file=sys.stderr)
            raise SystemExit(1)
        if path == cjk_src and not path.is_file():
            print(f"ERROR: missing source font: {path}", file=sys.stderr)
            raise SystemExit(1)

    output.parent.mkdir(parents=True, exist_ok=True)
    command = [
        "npx",
        "--yes",
        f"lv_font_conv@{CONVERTER}",
        "--size",
        str(size),
        "--bpp",
        "4",
        "--format",
        "lvgl",
        "--no-compress",
        "--lv-font-name",
        name,
        "--lv-include",
        "lvgl.h",
    ]
    if latin_range or latin_symbols:
        command.extend(["--font", str(latin_src)])
        if latin_range:
            command.extend(["--range", latin_range])
        if latin_symbols:
            command.extend(["--symbols", latin_symbols])
    command.extend(["--font", str(cjk_src), "--symbols", cjk, "--output", str(output)])
    subprocess.run(command, check=True)
    print(f"Wrote {output}")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--size",
        type=int,
        action="append",
        choices=(12, 16, 20),
        help="generate only the listed size (repeatable). Default: 12, 16, and 20",
    )
    args = parser.parse_args()
    sizes = tuple(args.size) if args.size else (12, 16, 20)

    jobs = {
        12: lambda: generate(
            12,
            "time_track_font_12",
            ROOT / "assets" / "fonts" / "time_track_font_12.c",
            title_symbols(),
        ),
        16: lambda: generate(
            16,
            "time_track_font_16",
            ROOT / "assets" / "fonts" / "time_track_font_16.c",
            cjk_symbols(),
            latin_range="0x20-0x7E",
        ),
        20: lambda: generate(
            20,
            "time_track_font_20",
            ROOT / "assets" / "fonts" / "time_track_font_20.c",
            total_symbols(),
            latin_symbols=HEADER_LATIN,
        ),
    }
    for size in sizes:
        jobs[size]()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
