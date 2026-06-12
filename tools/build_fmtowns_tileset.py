#!/usr/bin/env python3
"""組 FM Towns 風格 tileset:EGA 底圖 + 疊 FM Towns sprite(主角/怪物/守衛/NPC)。

FM Towns GRAPH/*.TIF 解碼見 fmtowns_decode.py(FillOrder=2 + EGA-16 palette)。
sprite 32×32 → downscale 16×16 疊到 engine tile id(blit 用 16×16 源)。
怪物 tile→sprite 對應目前為 provisional(視覺判定,待校正)。

用法:build_fmtowns_tileset.py <GRAPH目錄> <EGA底圖.png> <輸出.png>
"""
import sys
from PIL import Image

# FM Towns U2 sprite palette(對齊 ref_u2_play.jpg / 頭像截圖校準):sprite 只用偶數 nibble
# = real index 0-7;EGA[2k] = 第 k 色。0=透明/黑、6=主色(藍甲)、3=黃、2=綠、5=膚、7=白。
_PAL8 = [(0,0,0),(215,45,45),(45,175,80),(235,205,60),(70,120,225),(235,190,155),(40,70,180),(250,250,255)]
EGA = [(0,0,0)]*16
for _k in range(8):
    EGA[2*_k] = _PAL8[_k]
REV = [int(f"{b:08b}"[::-1], 2) for b in range(256)]


def sprite(path, si, sp=32):
    """解 path 第 si 個 32×32 sprite → 16×16 RGB(FillOrder=2)。"""
    d = open(path, "rb").read()[512:]
    bpr = sp // 2
    im = Image.new("RGB", (sp, sp), (0, 0, 0))
    for y in range(sp):
        for xb in range(bpr):
            off = si*sp*bpr + y*bpr + xb
            if off >= len(d):
                break
            b = REV[d[off]]
            im.putpixel((xb*2, y), EGA[(b >> 4) & 0xF])
            im.putpixel((xb*2+1, y), EGA[b & 0xF])
    return im.resize((16, 16), Image.NEAREST)


def main():
    graph, ega, out = sys.argv[1], sys.argv[2], sys.argv[3]
    base = Image.open(ega).convert("RGB")            # 1040×16 EGA 底

    def put(tid, img):
        base.paste(img, (tid*16, 0))

    put(16, sprite(f"{graph}/PLAYER.TIF", 0))         # 主角
    # 8 個怪物 tile ← ENEMY 前 8 隻(frame-1,偶數 sprite);provisional
    mons = {12: 0, 60: 2, 13: 4, 61: 6, 14: 8, 62: 10, 15: 12, 63: 14}
    for tid, si in mons.items():
        put(tid, sprite(f"{graph}/ENEMY.TIF", si))
    put(24, sprite(f"{graph}/HITO.TIF", 0))           # 守衛
    put(26, sprite(f"{graph}/HITO.TIF", 2))           # NPC

    base.save(out)
    print(f"built {out} {base.size}")


if __name__ == "__main__":
    main()
