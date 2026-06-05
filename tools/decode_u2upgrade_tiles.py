#!/usr/bin/env python3
"""解碼 U2 Upgrade (mcmagi) 的獨立 tileset:CGATILES (2bpp) / EGATILES (4bpp)。

來源(玩家自備,**本 repo 不內嵌**):
  https://github.com/mcmagi/ultima-exodus/releases  →  u2upgrade-2.1.zip
  解開後取 CGATILES / EGATILES (及 EGATHEME.*/EGACOLOR)。

格式:
  CGATILES  = 65 tile × 64 byte  = 16×16 **CGA 2bpp**(4 px/byte,bit7-6=最左)。
  EGATILES  = 65 tile × 128 byte = 16×16 **EGA 4bpp**(8 byte/row,每 byte 2 nibble,
              高 nibble=左 px;nibble = EGA 16 色 index,標準 EGA palette)。

為何用它:U2 Upgrade 提供**乾淨、可辨識的官方社群 tileset**,id 語意明確
(0 water / 3 forest / 4 mountain / 5 town / 6 castle / 7 tower / 9 dungeon /
 17 horse / 18 ship / 22 sword / 28 wall / 32–57 A–Z / 60+ monsters)。
原始 ultimaii.exe @0x7C42 抽出的 tile 與此**只有 id 0 相同**,故改以此為主 ground truth。

用法:
  python3 decode_u2upgrade_tiles.py <CGATILES|EGATILES> <out_sheet.png> [cga|ega]
"""
import sys
from PIL import Image, ImageDraw

# 標準 EGA 16 色
EGA16 = [
    (0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
]
# FM Towns 風配色:把 EGA 16 色往更亮/更飽和/卡通感重映(風格參考,非真實 FM Towns 色值)
# 依 docs/FMTOWNS_TILESET.md 的觀感(明亮、Ultima IV/V 級)手調。
EGA_FMT = [
    (0, 0, 0), (28, 64, 200), (40, 170, 64), (64, 200, 200),
    (210, 56, 56), (200, 72, 184), (176, 112, 48), (208, 208, 216),
    (104, 104, 128), (84, 138, 255), (96, 230, 120), (128, 236, 236),
    (255, 104, 96), (250, 132, 232), (252, 232, 96), (255, 255, 255),
]
# CGA 2bpp 預設 palette(可讀 blue:黑/綠/深藍/灰白)
CGA_BLUE = [(0, 0, 0), (0, 170, 0), (40, 60, 180), (205, 205, 205)]

N_TILES = 65

# 經 U2 Upgrade sheet 視覺確認的 id 名稱(高信心)
NAMES = {
    0: "water", 1: "water2", 2: "grass", 3: "forest", 4: "mountain",
    5: "town", 6: "castle", 7: "tower", 8: "keep", 9: "dungeon",
    10: "sign", 11: "shore", 12: "lizardman", 13: "ghost", 14: "devil",
    15: "balron", 16: "person", 17: "horse", 18: "ship", 19: "aircar",
    20: "rocket", 21: "serpent-sign", 22: "sword", 23: "barrier",
    24: "person", 25: "person", 26: "person", 27: "fighter", 28: "wall",
    29: "blank", 30: "white", 31: "blank", 48: "space", 64: "orb",
}
for i, ch in enumerate("ABCDEFGHIJKLMNOPQRSTUVWXYZ"):
    NAMES[32 + i] = f"'{ch}'"
for i in range(60, 64):
    NAMES[i] = "monster"


def decode_cga(data, t):
    off = t * 64
    px = [[0] * 16 for _ in range(16)]
    for y in range(16):
        for xb in range(4):
            b = data[off + y * 4 + xb]
            for i, s in enumerate((6, 4, 2, 0)):
                px[y][xb * 4 + i] = (b >> s) & 3
    return [[CGA_BLUE[v] for v in row] for row in px]


def decode_ega(data, t, pal=EGA16):
    off = t * 128
    rows = []
    for y in range(16):
        row = []
        for xb in range(8):
            b = data[off + y * 8 + xb]
            row.append(pal[(b >> 4) & 0xF])
            row.append(pal[b & 0xF])
        rows.append(row)
    return rows


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    data = open(sys.argv[1], "rb").read()
    out = sys.argv[2]
    kind = sys.argv[3] if len(sys.argv) > 3 else (
        "ega" if len(data) >= N_TILES * 128 else "cga")
    mode = sys.argv[4] if len(sys.argv) > 4 else "sheet"
    # kind: ega(標準) / egafmt(FM Towns 風配色) / cga
    if kind == "egafmt":
        dec = lambda d, t: decode_ega(d, t, EGA_FMT)
    elif kind == "ega":
        dec = decode_ega
    else:
        dec = decode_cga

    if mode == "strip":
        # 引擎 tileset:65 tile 橫條 (1040×16),u2_tileset 以 id*16 取 src x
        strip = Image.new("RGB", (N_TILES * 16, 16), (0, 0, 0))
        for t in range(N_TILES):
            rows = dec(data, t)
            for y in range(16):
                for x in range(16):
                    strip.putpixel((t * 16 + x, y), rows[y][x])
        strip.save(out)
        print(f"wrote {out} ({kind} engine strip, {N_TILES} tiles)")
        return

    cols, sc, gp, lab = 8, 5, 10, 14
    tw = 16 * sc
    rows_n = (N_TILES + cols - 1) // cols
    W = cols * (tw + gp) + gp
    H = rows_n * (tw + gp + lab) + gp + 16
    img = Image.new("RGB", (W, H), (40, 40, 48))
    d = ImageDraw.Draw(img)
    for t in range(N_TILES):
        rows = dec(data, t)
        cx = gp + (t % cols) * (tw + gp)
        cy = gp + (t // cols) * (tw + gp + lab)
        for y in range(16):
            for x in range(16):
                img.paste(rows[y][x],
                          (cx + x * sc, cy + y * sc, cx + x * sc + sc, cy + y * sc + sc))
        nm = NAMES.get(t, "unknown")
        col = (220, 220, 120) if t in NAMES else (130, 130, 140)
        d.text((cx, cy + tw + 1), f"{t} {nm}", fill=col)
    d.text((gp, H - 13),
           f"U2 Upgrade {kind.upper()}TILES (65 tiles, 16x16) - canonical ground-truth tileset (mcmagi); art not redistributed",
           fill=(180, 190, 180))
    img.save(out)
    print(f"wrote {out} ({kind}, {N_TILES} tiles)")


if __name__ == "__main__":
    main()
