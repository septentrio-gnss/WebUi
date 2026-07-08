import gzip
import shutil
from pathlib import Path

SRC_DIR = Path("data_src")
OUT_DIR = Path("data")

FILES_TO_GZIP = ["index.html", "style.css", "chart.js"]

OUT_DIR.mkdir(exist_ok=True)

# Remove old compressed frontend files
for filename in FILES_TO_GZIP:
    gz = OUT_DIR / f"{filename}.gz"
    if gz.exists():
        gz.unlink()

# Compress editable frontend files into data/
for filename in FILES_TO_GZIP:
    src = SRC_DIR / filename
    dst = OUT_DIR / f"{filename}.gz"

    with open(src, "rb") as f_in:
        with gzip.open(dst, "wb", compresslevel=9) as f_out:
            shutil.copyfileobj(f_in, f_out)

    print(f"[OK] {src} -> {dst}")

# Optional: copy SVG/assets from data_src/assets or data_src root
for asset in SRC_DIR.glob("*.svg"):
    shutil.copy2(asset, OUT_DIR / asset.name)
    print(f"[ASSET] {asset.name}")