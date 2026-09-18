#!/usr/bin/env python3
"""Check that generated Time Ledger fonts cover the glyphs each size draws."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "main" / "time_track_text.h"
FONT_12 = ROOT / "assets" / "fonts" / "time_track_font_12.c"
FONT_16 = ROOT / "assets" / "fonts" / "time_track_font_16.c"
FONT_20 = ROOT / "assets" / "fonts" / "time_track_font_20.c"
STRING_MACROS = (
    "TIME_TRACK_TEXT_TITLE",
    "TIME_TRACK_TEXT_IDLE",
    "TIME_TRACK_TEXT_RUNNING",
    "TIME_TRACK_TEXT_TODAY_TOTAL",
    "TIME_TRACK_TEXT_YESTERDAY_TOTAL",
)


def decode_c_string(literal: str) -> str:
    return literal.encode("utf-8").decode("unicode_escape") if "\\u" in literal else literal


def header_macro(name: str) -> str:
    text = HEADER.read_text(encoding="utf-8")
    match = re.search(rf'#define {name} "([^"]*)"', text)
    if not match:
        raise AssertionError(f"missing {name}")
    return match.group(1)


def header_strings() -> list[str]:
    values = [header_macro(name) for name in STRING_MACROS]
    text = HEADER.read_text(encoding="utf-8")
    labels = re.findall(r'^\s*"(.*)",\s*$', text, re.M)
    if len(labels) < 6:
        raise AssertionError("expected six label strings")
    values.extend(labels[:6])
    values.extend(["0123456789hms :%"])
    return values


def required_codepoints(texts: list[str]) -> set[int]:
    points: set[int] = set()
    for text in texts:
        for char in text:
            code = ord(char)
            if code >= 32:
                points.add(code)
    return points


def font_codepoints(path: Path) -> set[int]:
    text = path.read_text(encoding="utf-8")
    return {int(match, 16) for match in re.findall(r"U\+([0-9A-Fa-f]{4,6})", text)}


def assert_covers(test: unittest.TestCase, path: Path, texts: list[str]) -> None:
    test.assertTrue(path.is_file(), f"missing generated font {path}")
    missing = sorted(required_codepoints(texts) - font_codepoints(path))
    test.assertEqual(
        missing,
        [],
        f"{path.name} missing glyphs: " + ", ".join(f"U+{point:04X}" for point in missing),
    )


class TimeTrackFontTest(unittest.TestCase):
    def test_title_font_covers_title(self) -> None:
        assert_covers(self, FONT_12, [header_macro("TIME_TRACK_TEXT_TITLE")])

    def test_generated_font_covers_ui_inventory(self) -> None:
        assert_covers(self, FONT_16, header_strings())

    def test_total_font_covers_day_header(self) -> None:
        assert_covers(
            self,
            FONT_20,
            [
                header_macro("TIME_TRACK_TEXT_TODAY_TOTAL"),
                header_macro("TIME_TRACK_TEXT_YESTERDAY_TOTAL"),
                "0123456789hm ",
            ],
        )

    def test_bar_palette_is_distinct(self) -> None:
        ui = (ROOT / "main" / "time_track_ui.c").read_text(encoding="utf-8")
        match = re.search(
            r"TT_BAR_COLORS\[TIME_TRACK_LABEL_COUNT\] = \{([^}]+)\}",
            ui,
            re.S,
        )
        self.assertIsNotNone(match, "missing TT_BAR_COLORS")
        colors = re.findall(r"0x[0-9A-Fa-f]+", match.group(1))
        self.assertEqual(len(colors), 6)
        self.assertEqual(len(set(color.lower() for color in colors)), 6)


if __name__ == "__main__":
    unittest.main()
