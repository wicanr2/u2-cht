#!/usr/bin/env python3
"""從 FM Towns U2 overworld 截圖裁地形 tile(32×32)→ build/fmt_<name>.png。

來源是 FM Towns 實機 overworld 截圖(640×480,原生 2× → tile 32px)。
build_fmtowns_tileset.py 會把 build/fmt_water/grass/forest.png 疊到地形 tile id
(0 水 / 2 草 / 3 林;u2_passable + mapx 直方確認)。

截圖非 repo 資產(版權);使用者自備。座標對 640×480 截圖(playfield 起點 8,48,格 32px)。
水/草/林取自 U2FMTowns28.jpg;山取自 U2FMTowns31.jpg(綠山,城堡周圍)。
用法:crop_fmtowns_terrain.py <overworld截圖.jpg> <輸出目錄> [山截圖.jpg]
"""
import sys
import os
from PIL import Image

OX, OY, TS = 8, 48, 32
# (name, 格col, 格row) — 對 U2FMTowns28.jpg。
TILES = [("water", 8, 0), ("grass", 2, 2), ("forest", 0, 2)]
# 山:U2FMTowns31.jpg 的綠山格(col9 row4,綠帶紅紋,與黃橘螺旋的樹區隔)。
MTN = ("mountain", 9, 4)


def crop(im, outdir, name, col, row):
    x, y = OX + col*TS, OY + row*TS
    im.crop((x, y, x+TS, y+TS)).save(os.path.join(outdir, f"fmt_{name}.png"))
    print(f"wrote fmt_{name}.png from ({x},{y})")


def main():
    src, outdir = sys.argv[1], sys.argv[2]
    os.makedirs(outdir, exist_ok=True)
    im = Image.open(src).convert("RGB")
    for name, col, row in TILES:
        crop(im, outdir, name, col, row)
    mtn = sys.argv[3] if len(sys.argv) > 3 else None
    if mtn and os.path.exists(mtn):
        crop(Image.open(mtn).convert("RGB"), outdir, *MTN)


if __name__ == "__main__":
    main()
