#!/usr/bin/env python3
"""從 DOS ultimaii.exe 抽取 64 個 tile 美術,輸出 tileset PNG。

格式 (docs/DATA_FORMATS.md / ModdingWiki):
  - tile 資料 @ offset 0x7C43,64 個 tile。
  - 每 tile 16×16,**CGA Linear 2bpp**:4 px/byte,bit7-6=最左像素…bit1-0=最右。
  - 64 bytes/tile。map 的 tile_id (byte÷4) 直接索引此表 0..63。
  - tile ~0–31 為地形/物件,~32+ 為字符集 (招牌文字用)。

CGA 調色盤 (對應 Alderson Win port 三組):
  original = 黑/青/洋紅/白 (原版,magenta ocean)
  red      = 黑/綠/紅/白
  blue     = 黑/綠/藍/白

用法:
  python3 extract_tiles.py <ultimaii.exe> <out.png> [original|red|blue]
  輸出:1024×16 橫向 strip (64 tile × 16px);engine 以 id*16 取 src x。

注意:tile 美術屬 Origin/EA 版權,輸出 PNG **不散布**;請自備合法 Ultima II。
"""
import sys
from PIL import Image

TILE_BASE = 0x7C43
N_TILES = 64

PALETTES = {
    "original": [(0, 0, 0), (85, 255, 255), (255, 85, 255), (255, 255, 255)],
    "red":      [(0, 0, 0), (85, 255, 85), (255, 85, 85), (255, 255, 255)],
    "blue":     [(0, 0, 0), (85, 255, 85), (85, 85, 255), (255, 255, 255)],
}


def decode_tile(exe, off):
    px = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for xb in range(4):
            b = exe[off + y * 4 + xb]
            for i, s in enumerate((6, 4, 2, 0)):
                px[y][xb * 4 + i] = (b >> s) & 3
    return px


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    exe = open(sys.argv[1], "rb").read()
    out = sys.argv[2]
    pal = PALETTES[sys.argv[3] if len(sys.argv) > 3 else "original"]

    img = Image.new("RGB", (N_TILES * 16, 16), (0, 0, 0))
    for t in range(N_TILES):
        off = TILE_BASE + t * 64
        if off + 64 > len(exe):
            break
        px = decode_tile(exe, off)
        for y in range(16):
            for x in range(16):
                img.putpixel((t * 16 + x, y), pal[px[y][x]])
    img.save(out)
    print(f"寫出 tileset → {out} ({N_TILES} tiles, palette={sys.argv[3] if len(sys.argv)>3 else 'original'})")


if __name__ == "__main__":
    main()
