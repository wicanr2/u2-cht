#!/usr/bin/env python3
"""Debug 全圖渲染器:把整張 mapxNN 用真 tile 畫出,可疊 monxNN 實體層。

驗證 tile÷4 解析 + monxNN 實體格式 (docs/DATA_FORMATS.md)。
用法:
  python3 render_map.py <ultimaii.exe> <mapxNN> [out.png] [--mon monxNN] [--pal blue]
注意:不散布原版資料;請自備合法 Ultima II。
"""
import sys
from PIL import Image, ImageDraw

PALETTES = {
    "blue":     [(0, 0, 0), (0, 170, 0), (40, 60, 180), (205, 205, 205)],  # 預設:綠樹/深藍水/灰白
    "red":      [(0, 0, 0), (0, 170, 0), (190, 40, 40), (205, 205, 205)],
    "original": [(0, 0, 0), (85, 255, 255), (255, 85, 255), (255, 255, 255)],  # alternate DOS CGA
}


def decode_tile(exe, idx, pal):
    im = Image.new("RGB", (16, 16))
    if idx >= 32:
        # font/招牌字為獨立 bit-packed proportional 子區塊,非 16x16 grid (見 DATA_FORMATS)。
        # 不當固定 tile 解 → 渲中性 placeholder,避免錯位 garbage。
        for y in range(16):
            for x in range(16):
                edge = x in (0, 15) or y in (0, 15)
                im.putpixel((x, y), (40, 40, 48) if edge else (12, 12, 16))
        return im
    off = 0x7C42 + idx * 64        # terrain/sprite id 0-31,已驗證對齊
    for y in range(16):
        for xb in range(4):
            b = exe[off + y * 4 + xb]
            for i, s in enumerate((6, 4, 2, 0)):
                im.putpixel((xb * 4 + i, y), pal[(b >> s) & 3])
    return im


def parse_mon(path):
    m = open(path, "rb").read()
    return [(m[0x00 + i], m[0x20 + i], m[0x40 + i], m[0x60 + i] // 4)
            for i in range(32) if m[0x60 + i]]


def main():
    a = sys.argv
    if len(a) < 3:
        sys.exit(__doc__)
    exe = open(a[1], "rb").read()
    mp = open(a[2], "rb").read()
    out = a[3] if len(a) > 3 and not a[3].startswith("--") else "map.png"
    mon = a[a.index("--mon") + 1] if "--mon" in a else None
    pal = PALETTES[a[a.index("--pal") + 1] if "--pal" in a else "blue"]

    s = 8
    cache = {}
    big = Image.new("RGB", (64 * s, 66 * s))
    for y in range(66):
        for x in range(64):
            idx = mp[y * 64 + x] // 4
            if idx not in cache:
                cache[idx] = decode_tile(exe, idx, pal).resize((s, s), Image.NEAREST)
            big.paste(cache[idx], (x * s, y * s))

    if mon:
        dr = ImageDraw.Draw(big)
        for x, y, st, tile in parse_mon(mon):
            if 0 <= x < 64 and 0 <= y < 66:
                big.paste(decode_tile(exe, tile, pal).resize((s, s), Image.NEAREST),
                          (x * s, y * s))
                dr.rectangle([x * s, y * s, x * s + s - 1, y * s + s - 1],
                             outline=(255, 80, 40))
    big.save(out)
    print(f"寫出 {out}" + (f" + {len(parse_mon(mon))} 實體" if mon else ""))


if __name__ == "__main__":
    main()
