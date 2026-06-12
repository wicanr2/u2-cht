#!/usr/bin/env python3
"""從 FM Towns U2 overworld 截圖裁地形 tile(32×32)→ build/fmt_<name>.png。

來源是 FM Towns 實機 overworld 截圖(640×480,原生 2× → tile 32px)。
build_fmtowns_tileset.py 會把 build/fmt_water/grass/forest.png 疊到地形 tile id
(0 水 / 2 草 / 3 林;u2_passable + mapx 直方確認)。

截圖非 repo 資產(版權);使用者自備。座標對 U2FMTowns28.jpg(playfield 起點 8,48,格 32px)。
用法:crop_fmtowns_terrain.py <overworld截圖.jpg> <輸出目錄>
"""
import sys
import os
from PIL import Image

# (name, 格col, 格row) — 對 640×480 截圖、playfield 原點 (8,48)、格 32px。
TILES = [("water", 8, 0), ("grass", 2, 2), ("forest", 1, 0)]
OX, OY, TS = 8, 48, 32


def main():
    src, outdir = sys.argv[1], sys.argv[2]
    im = Image.open(src).convert("RGB")
    os.makedirs(outdir, exist_ok=True)
    for name, col, row in TILES:
        x, y = OX + col*TS, OY + row*TS
        im.crop((x, y, x+TS, y+TS)).save(os.path.join(outdir, f"fmt_{name}.png"))
        print(f"wrote fmt_{name}.png from ({x},{y})")


if __name__ == "__main__":
    main()
