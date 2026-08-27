#!/usr/bin/env python3
"""Generate and validate the LVGL binary fonts used by AppLocale."""

from __future__ import annotations

import argparse
import hashlib
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CATALOG = ROOT / "components" / "apps" / "setting" / "Locale.cpp"
OUTPUT_DIR = ROOT / "components" / "apps" / "setting" / "fonts"
SIZES = (16, 20, 24, 30, 38)


def catalog_symbols() -> str:
    source = CATALOG.read_text(encoding="utf-8")
    return "".join(sorted({char for char in source if ord(char) > 0x7F}, key=ord))


def run_converter(converter: list[str], font: Path, symbols: str, size: int, output: Path, fmt: str) -> None:
    command = [
        *converter,
        "--font",
        str(font),
        "--range",
        "0x20-0x7E",
        "--symbols",
        symbols,
        "--size",
        str(size),
        "--format",
        fmt,
        "--bpp",
        "4",
        "--no-compress",
        "--no-kerning",
        "--output",
        str(output),
    ]
    subprocess.run(command, check=True)


def validate_rasters(converter: list[str], font: Path, symbols: str) -> None:
    with tempfile.TemporaryDirectory(prefix="app-locale-font-") as temporary:
        dump = Path(temporary) / "dump"
        run_converter(converter, font, symbols, 20, dump, "dump")
        images = [dump / f"{ord(char):x}.png" for char in symbols]
        missing = [image.name for image in images if not image.is_file()]
        if missing:
            raise RuntimeError(f"converter omitted {len(missing)} glyphs: {', '.join(missing[:8])}")

        hashes = {hashlib.sha256(image.read_bytes()).digest() for image in images}
        if len(hashes) != len(images):
            raise RuntimeError(
                f"font produced only {len(hashes)} distinct rasters for {len(images)} catalog glyphs"
            )


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--font",
        type=Path,
        default=Path(r"C:\Windows\Fonts\simhei.ttf"),
        help="CJK source font (default: Windows SimHei)",
    )
    args = parser.parse_args()

    converter_path = shutil.which("lv_font_conv")
    if converter_path is None:
        raise RuntimeError("lv_font_conv is not installed or not on PATH")
    if Path(converter_path).suffix.lower() in {".cmd", ".bat"}:
        node = shutil.which("node")
        script = Path(converter_path).parent / "node_modules" / "lv_font_conv" / "lv_font_conv.js"
        if node is None or not script.is_file():
            raise RuntimeError("could not resolve node and lv_font_conv.js")
        converter = [node, str(script)]
    else:
        converter = [converter_path]
    if not args.font.is_file():
        raise FileNotFoundError(args.font)

    symbols = catalog_symbols()
    validate_rasters(converter, args.font, symbols)
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    for size in SIZES:
        destination = OUTPUT_DIR / f"app_locale_{size}.fnt"
        run_converter(converter, args.font, symbols, size, destination, "bin")
        print(f"generated {destination.relative_to(ROOT)} ({destination.stat().st_size} bytes)")
    print(f"validated {len(symbols)} non-ASCII catalog glyphs")


if __name__ == "__main__":
    main()