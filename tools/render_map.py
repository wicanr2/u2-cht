#!/usr/bin/env python3
"""Debug 全圖渲染器:把整張 mapxNN 用 tile 畫出,可疊 monxNN 實體層。

tileset 來源(優先序):
  --tileset <EGATILES|CGATILES>  使用 U2 Upgrade 獨立 tileset(**主要 ground truth**)
                                  4bpp(EGA,128B/tile)或 2bpp(CGA,64B/tile)自動判定
  --exe <ultimaii.exe>            (PoC,fallback)從 EXE @0x7C42 抽 2bpp tile;
                                  注意:此來源與 U2 Upgrade 只有 id 0 相同,不建議當 ground truth

用法:
  python3 render_map.py <mapxNN> <out.png> [--tileset EGATILES|--exe ultimaii.exe]
                        [--mon monxNN] [--pal blue]
注意:不散布原版資料 / U2 Upgrade art;請自備合法 Ultima II 與 U2 Upgrade。
"""
import sys
from PIL import Image, ImageDraw

EGA16 = [
    (0, 0, 0), (0, 0, 170), (0, 170, 0), (0, 170, 170),
    (170, 0, 0), (170, 0, 170), (170, 85, 0), (170, 170, 170),
    (85, 85, 85), (85, 85, 255), (85, 255, 85), (85, 255, 255),
    (255, 85, 85), (255, 85, 255), (255, 255, 85), (255, 255, 255),
]
PALETTES = {
    "blue":     [(0, 0, 0), (0, 170, 0), (40, 60, 180), (205, 205, 205)],
    "red":      [(0, 0, 0), (0, 170, 0), (190, 40, 40), (205, 205, 205)],
    "original": [(0, 0, 0), (85, 255, 255), (255, 85, 255), (255, 255, 255)],
}


class Tileset:
    """以 idx 取 16×16 tile 圖。來源:U2 Upgrade EGA/CGA tileset 或 EXE。"""
    def __init__(self, tileset=None, exe=None, pal="blue"):
        self.pal = PALETTES[pal]
        self.data = None
        self.kind = None
        self.exe = None
        if tileset:
            self.data = open(tileset, "rb").read()
            self.kind = "ega" if len(self.data) >= 65 * 128 else "cga"
        elif exe:
            self.exe = open(exe, "rb").read()
            self.kind = "exe"
        self.cache = {}

    def tile(self, idx):
        if idx in self.cache:
            return self.cache[idx]
        im = Image.new("RGB", (16, 16))
        if self.kind == "ega":
            off = idx * 128
            for y in range(16):
                for xb in range(8):
                    b = self.data[off + y * 8 + xb] if off + y * 8 + xb < len(self.data) else 0
                    im.putpixel((xb * 2, y), EGA16[(b >> 4) & 0xF])
                    im.putpixel((xb * 2 + 1, y), EGA16[b & 0xF])
        elif self.kind == "cga":
            off = idx * 64
            for y in range(16):
                for xb in range(4):
                    b = self.data[off + y * 4 + xb] if off + y * 4 + xb < len(self.data) else 0
                    for i, s in enumerate((6, 4, 2, 0)):
                        im.putpixel((xb * 4 + i, y), self.pal[(b >> s) & 3])
        else:  # exe (PoC fallback);id>=32 placeholder(EXE font 子區塊未解)
            if idx >= 32:
                for y in range(16):
                    for x in range(16):
                        edge = x in (0, 15) or y in (0, 15)
                        im.putpixel((x, y), (40, 40, 48) if edge else (12, 12, 16))
            else:
                off = 0x7C42 + idx * 64
                for y in range(16):
                    for xb in range(4):
                        b = self.exe[off + y * 4 + xb]
                        for i, s in enumerate((6, 4, 2, 0)):
                            im.putpixel((xb * 4 + i, y), self.pal[(b >> s) & 3])
        self.cache[idx] = im
        return im


def parse_mon(path):
    m = open(path, "rb").read()
    return [(m[0x00 + i], m[0x20 + i], m[0x40 + i], m[0x60 + i] // 4)
            for i in range(32) if m[0x60 + i]]


def opt(a, name, default=None):
    return a[a.index(name) + 1] if name in a else default


def main():
    a = sys.argv
    if len(a) < 3:
        sys.exit(__doc__)
    mp = open(a[1], "rb").read()
    out = a[2]
    ts = Tileset(tileset=opt(a, "--tileset"), exe=opt(a, "--exe"),
                 pal=opt(a, "--pal", "blue"))
    if ts.kind is None:
        sys.exit("需 --tileset <EGATILES/CGATILES> 或 --exe <ultimaii.exe>")
    mon = opt(a, "--mon")

    s = 8
    big = Image.new("RGB", (64 * s, 66 * s))
    scaled = {}
    def sc(idx):
        if idx not in scaled:
            scaled[idx] = ts.tile(idx).resize((s, s), Image.NEAREST)
        return scaled[idx]
    for y in range(66):
        for x in range(64):
            idx = mp[y * 64 + x] // 4
            big.paste(sc(idx if idx <= 64 else 0), (x * s, y * s))
    if mon:
        dr = ImageDraw.Draw(big)
        for x, y, st, tile in parse_mon(mon):
            if 0 <= x < 64 and 0 <= y < 66:
                big.paste(sc(tile if tile <= 64 else 0), (x * s, y * s))
                dr.rectangle([x * s, y * s, x * s + s - 1, y * s + s - 1], outline=(255, 80, 40))
    big.save(out)
    print(f"wrote {out} (tileset={ts.kind})" + (f" + {len(parse_mon(mon))} entities" if mon else ""))


if __name__ == "__main__":
    main()
