#!/usr/bin/env python3
"""Check that the generated 16px font covers the application UI inventory."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
HEADER = ROOT / "main" / "time_track_text.h"
FONT = ROOT / "assets" / "fonts" / "time_track_font_16.c"
STRING_MACROS = (
    "TIME_TRACK_TEXT_TITLE",
    "TIME_TRACK_TEXT_IDLE",
    "TIME_TRACK_TEXT_RUNNING",
    "TIME_TRACK_TEXT_TODAY_TOTAL",
    "TIME_TRACK_TEXT_YESTERDAY_TOTAL",
)


def decode_c_string(literal: str) -> str:
    return literal.encode("utf-8").decode("unicode_escape") if "\\u" in literal else literal


def header_strings() -> list[str]:
    text = HEADER.read_text(encoding="utf-8")
    values: list[str] = []
    for name in STRING_MACROS:
        match = re.search(rf'#define {name} "([^"]*)"', text)
        if not match:
            raise AssertionError(f"missing {name}")
        values.append(match.group(1))
    labels = re.findall(r'^\s*"(.*)",\s*$', text, re.M)
    if len(labels) < 6:
        raise AssertionError("expected six label strings")
    values.extend(labels[:6])
    values.extend(["0123456789hms :%"])
    return values


def required_codepoints() -> set[int]:
    points: set[int] = set()
    for text in header_strings():
        for char in text:
            code = ord(char)
            if code >= 32:
                points.add(code)
    return points


def font_codepoints() -> set[int]:
    text = FONT.read_text(encoding="utf-8")
    return {int(match, 16) for match in re.findall(r"U\+([0-9A-Fa-f]{4,6})", text)}


class TimeTrackFontTest(unittest.TestCase):
    def test_generated_font_covers_ui_inventory(self) -> None:
        self.assertTrue(FONT.is_file(), f"missing generated font {FONT}")
        font_points = font_codepoints()
        missing = sorted(required_codepoints() - font_points)
        self.assertEqual(
            missing,
            [],
            "missing glyphs: " + ", ".join(f"U+{point:04X}" for point in missing),
        )


if __name__ == "__main__":
    unittest.main()
