#!/usr/bin/env python3
"""FM Towns《Ultima Trilogy》(1990) GRAPH/*.TIF 圖檔解碼。

格式(本機逆向,見 docs/FMTOWNS_TILESET.md):
  - 容器是 little-endian TIFF(II*),但 **header 謊報 ImageWidth/Height=32**;
    真實像素資料從 StripOffsets=512 起,到檔尾。
  - 像素:**4bpp chunky,每列寬 32px(= 16 byte/列)**。
  - ⚠️ **關鍵:TIFF tag FillOrder=2(LSB-first)** → 每 byte 8 bits 須先反序再取 nibble,
    否則得滿版雜色(PIL 因尊重此 tag 而解得乾淨,但只讀 header 謊報的 32×32 首格;
    raw 讀 offset 512 才得完整 sprite,故本工具自行套 REV 反序表)。
  - 物件多為 **32×32 sprite**(怪物/人物,常成對 = 2 幀動畫);ENEMY=64 隻、PLAYER 等。
  - palette:EGA-16,對齊 ref_u2_play.jpg(FM Towns overworld 實機截圖)。index 順序可再微調。

用法:
  fmtowns_decode.py <file.TIF> <out.png> [sprite32|strip|raw] [--pal ega|gray]
    sprite32 : 切 32×32 sprite 排格(預設)
    strip    : 32px 寬連續長條
    raw      : 同 strip
"""
import sys
from PIL import Image

# FM Towns U2 真實 palette 暫存器順序(由 U2TITLE1.TIF 與模擬器標題 100% 吻合驗證)
# UT1TILE0 等 sprite 只用 index 0-7。
# FM Towns U2 sprite palette(對齊 ref_u2_play.jpg / 頭像截圖校準):sprite 只用偶數 nibble
# = real index 0-7;EGA16[2k] = 第 k 色。0=透明/黑、6=藍(甲)、3=黃、2=綠、5=膚、7=白、1=紅。
_PAL8 = [(0,0,0),(215,45,45),(45,175,80),(235,205,60),(70,120,225),(235,190,155),(40,70,180),(250,250,255)]
EGA16 = [(0,0,0)]*16
for _k in range(8):
    EGA16[2*_k] = _PAL8[_k]
GRAY = [(i*17,i*17,i*17) for i in range(16)]

# FillOrder=2(LSB-first)位元反序表:TIFF tag FillOrder=2 → 每 byte 8 bits 反轉後再取 nibble。
# (關鍵修正:不反序會得到滿版雜色;PIL 因尊重此 tag 而解得乾淨。)
REV = [int(f"{b:08b}"[::-1], 2) for b in range(256)]

DATA_OFF = 512
WIDTH = 32


def render_strip(path, pal):
    d = open(path, "rb").read()[DATA_OFF:]
    bpr = WIDTH // 2
    h = len(d) // bpr
    im = Image.new("RGB", (WIDTH, h))
    p = 0
    for y in range(h):
        for xb in range(bpr):
            b = REV[d[p]]; p += 1          # FillOrder=2:先反序
            im.putpixel((xb*2, y), pal[(b >> 4) & 0xF])
            im.putpixel((xb*2+1, y), pal[b & 0xF])
    return im


def to_sprites(strip, sp=32, cols=8, pad=2, bg=(40,40,55)):
    n = strip.height // sp
    rows = (n + cols - 1) // cols
    grid = Image.new("RGB", (cols*(sp+pad)+pad, rows*(sp+pad)+pad), bg)
    for i in range(n):
        s = strip.crop((0, i*sp, sp, i*sp+sp))
        c, r = i % cols, i // cols
        grid.paste(s, (pad+c*(sp+pad), pad+r*(sp+pad)))
    return grid, n


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    src, out = sys.argv[1], sys.argv[2]
    mode = sys.argv[3] if len(sys.argv) > 3 and not sys.argv[3].startswith("-") else "sprite32"
    pal = GRAY if "--pal" in sys.argv and "gray" in sys.argv else EGA16
    strip = render_strip(src, pal)
    if mode == "sprite32":
        grid, n = to_sprites(strip)
        grid = grid.resize((grid.width*2, grid.height*2), Image.NEAREST)
        grid.save(out)
        print(f"wrote {out}: {n} sprites (32x32, EGA-16 FillOrder=2)")
    else:
        strip.save(out)
        print(f"wrote {out}: 32x{strip.height} strip")


if __name__ == "__main__":
    main()
