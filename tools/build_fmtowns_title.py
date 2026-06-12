#!/usr/bin/env python3
"""解碼 FM Towns U2 標題畫面 U2TITLE1.TIF → 金黃版 title PNG。

格式同 fmtowns_decode.py(FillOrder=2 位元反序、4bpp、偶數 nibble = index×2),
但寬度取 TIFF tag(640),且**標題有自己的 8 色 palette**(與 sprite 的 C4 不同)。
標題 palette T4 由雙 agent 校準迴圈對 fmtowns_work/ref_fmtowns_u2.jpg 定案:
金黃「Ultima II」+ 暗紅副標 + 青藍風景。

用法:build_fmtowns_title.py <U2TITLE1.TIF> <out.png>
"""
import sys
from PIL import Image

REV = [int(f"{b:08b}"[::-1], 2) for b in range(256)]
# 標題 8 色 palette(index 0-7,對應偶數 nibble 值);T4 定案。
_PAL8 = [(0,0,0),(110,185,250),(150,40,35),(150,100,35),(45,140,150),(60,180,200),(235,70,55),(240,205,80)]
PAL16 = [(0,0,0)]*16
for _k in range(8):
    PAL16[2*_k] = _PAL8[_k]


def main():
    src, out = sys.argv[1], sys.argv[2]
    im0 = Image.open(src)
    t = im0.tag_v2
    off = t.get(273, (512,))[0]          # StripOffsets
    W = t.get(256)                       # ImageWidth(640)
    H = t.get(257)                       # ImageLength(400)
    d = open(src, "rb").read()[off:]
    bpr = W // 2
    H = min(H, len(d) // bpr)
    im = Image.new("RGB", (W, H))
    for y in range(H):
        for xb in range(bpr):
            o = y*bpr + xb
            if o >= len(d):
                break
            b = REV[d[o]]
            im.putpixel((xb*2, y), PAL16[(b >> 4) & 0xF])
            im.putpixel((xb*2+1, y), PAL16[b & 0xF])
    im.save(out)
    print(f"wrote {out} {im.size}")


if __name__ == "__main__":
    main()
