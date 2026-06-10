#!/usr/bin/env python3
"""FM Towns《Ultima Trilogy》(1990) GRAPH/*.TIF 圖檔解碼。

格式(本機逆向,見 docs/FMTOWNS_TILESET.md):
  - 容器是 little-endian TIFF(II*),但 **header 謊報 ImageWidth/Height=32**;
    真實像素資料從 StripOffsets=512 起,到檔尾。
  - 像素:**4bpp chunky,每列寬 32px(= 16 byte/列),高 nibble = 左 px**。
    (自相關:stride 16 byte 最強 → 寬 32px;byte 值多為雙 nibble 相同 = 純色區。)
  - 物件多為 **32×32 sprite**(怪物/人物,常成對 = 2 幀動畫)。
  - ⚠️ TIFF 內無 ColorMap;真實 16 色 palette 在遊戲 EXP / FM Towns 執行期設定,
    本工具用佔位 palette(EGA 或灰階)出圖看形狀;正式上色需取得真 palette。

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
FMT = [(0,0,0),(0,255,0),(255,0,0),(255,0,255),(0,0,255),(0,255,255),(255,255,0),(255,255,255),
       (0,0,0)]*1 + [(85,85,85)]*8   # 8-15 未觀測(sprite 未用),填灰佔位
FMT = FMT[:16]
EGA16 = FMT  # 預設用驗證過的 FM Towns palette
GRAY = [(i*17,i*17,i*17) for i in range(16)]

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
            b = d[p]; p += 1
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
        print(f"wrote {out}: {n} sprites (32x32, 佔位 palette)")
    else:
        strip.save(out)
        print(f"wrote {out}: 32x{strip.height} strip")


if __name__ == "__main__":
    main()
