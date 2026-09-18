<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

| File | Size / format | Use and source |
| --- | --- | --- |
| [`fonts/time_track_font_12.c`](fonts/time_track_font_12.c) | LVGL 9 bitmap, 12px, 4 bpp, uncompressed | Time Ledger title subset. CJK title glyphs come from Droid Sans Fallback. Generated with `lv_font_conv` 1.5.3 by `python3 tools/gen_time_track_font.py --size 12`. |
| [`fonts/time_track_font_16.c`](fonts/time_track_font_16.c) | LVGL 9 bitmap, 16px, 4 bpp, uncompressed | Time Ledger body font. ASCII `0x20-0x7E` comes from DejaVu Sans; the application CJK inventory comes from Droid Sans Fallback. Generated with `lv_font_conv` 1.5.3 by `python3 tools/gen_time_track_font.py --size 16` and compiled from `main/CMakeLists.txt`. DejaVu is Bitstream Vera derived; Droid Sans Fallback is Apache-2.0. Only the generated subset is committed. |
| [`fonts/time_track_font_20.c`](fonts/time_track_font_20.c) | LVGL 9 bitmap, 20px, 4 bpp, uncompressed | Time Ledger day-total line. Digits/`h`/`m`/space come from DejaVu Sans; the day-total CJK glyphs come from Droid Sans Fallback. Generated with `lv_font_conv` 1.5.3 by `python3 tools/gen_time_track_font.py --size 20`. |

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.
