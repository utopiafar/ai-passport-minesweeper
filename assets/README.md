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

### Minesweeper Chinese subset

- `fonts/NotoSansCJKsc-Minesweeper.otf` is a 204-character subset of
  [Noto Sans CJK SC Regular](https://github.com/notofonts/noto-cjk/tree/main/Sans).
  Original font SHA-256: `2c76254f6fc379fddfce0a7e84fb5385bb135d3e399294f6eeb6680d0365b74b`.
  Redistribution is covered by `fonts/NotoSansCJK-LICENSE.txt` (SIL OFL 1.1).
- `fonts/ms_font_12.c` and `fonts/ms_font_14.c` contain uncompressed 4 bpp
  LVGL bitmap fonts linked into the application from `main/CMakeLists.txt`.
  They cover printable ASCII and all fixed application text; no arbitrary user
  text or LVGL symbol icons are accepted. Cell numbers use Montserrat 14.
- `fonts/minesweeper_codepoints.h` supplies the startup coverage inventory.
  `tools/check_minesweeper_fonts.py` checks generated cmap tables; startup
  and the LVGL host preview also test glyph descriptors and a negative probe.
- Reproduce with fontTools 4.60.2 and lv_font_conv 1.5.3:

  ```bash
  python3 tools/generate_minesweeper_fonts.py --font /path/to/NotoSansCJKsc-Regular.otf \
    --converter /path/to/lv_font_conv
  python3 tools/check_minesweeper_fonts.py
  ```

  The script gathers printable ASCII plus non-ASCII characters from the UI
  sources, verifies the source cmap, subsets the OTF, and regenerates both
  sizes without compression or kerning. Font bitmaps reside in read-only Flash;
  actual Chinese display and runtime heap still require device checks.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/minesweeper-preview.png`](images/minesweeper-preview.png) | 240 × 320, PNG RGB | Minesweeper software preview rendered from application code with LVGL 9.5.0 and the rounded-screen BSP geometry. Sample battery value; not a device photograph. |
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
