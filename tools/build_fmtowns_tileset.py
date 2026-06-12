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


TS = 32   # 輸出 tile 邊長(FM Towns sprite 原生 32×32,引擎依 strip 高度自動判定)


def sprite(path, si, sp=32):
    """解 path 第 si 個 sprite → 原生 32×32 RGB(FillOrder=2,不縮)。"""
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
    return im


def main():
    graph, ega, out = sys.argv[1], sys.argv[2], sys.argv[3]
    src = Image.open(ega).convert("RGB")             # EGA 底 1040×16(65 個 16×16)
    n = src.height                                   # 16
    ntiles = src.width // n
    # 底圖每 tile 16→32 nearest 放大(像素地形保持銳利),組成 32×32 strip
    base = Image.new("RGB", (ntiles*TS, TS), (0, 0, 0))
    for i in range(ntiles):
        t = src.crop((i*n, 0, i*n+n, n)).resize((TS, TS), Image.NEAREST)
        base.paste(t, (i*TS, 0))

    def put(tid, img32):
        base.paste(img32, (tid*TS, 0))               # FM Towns sprite 原生 32×32

    put(16, sprite(f"{graph}/PLAYER.TIF", 0))         # 主角(藍甲騎士)
    # 怪物 tile → ENEMY sprite(frame-1 偶數 si);視覺分析對應(MonsterMap agent,
    # 配正典 bestiary;官方順序未解故為視覺判斷):
    #   12 蜥蜴人/Orc=E9(s18 持棍綠鱗壯) · 13 幽靈=E11(s22 白骷髏) · 14 魔鬼/Devil=E20(s40 蝙蝠翼魔)
    #   15 炎魔/Balron=E28(s56 頂級翼魔) · 60 哥布林=E13(s26 瘦綠小兵) · 61 盜賊=E1(s2 輕裝持刀)
    #   62 惡魔/Daemon=E26(s52 石像鬼) · 63 海蛇=E18(s36 盤蛇)
    mons = {12: 18, 13: 22, 14: 40, 15: 56, 60: 26, 61: 2, 62: 52, 63: 36}
    for tid, si in mons.items():
        put(tid, sprite(f"{graph}/ENEMY.TIF", si))
    put(24, sprite(f"{graph}/ENEMY.TIF", 8))          # 守衛=E4(s8 全甲戰士)
    put(26, sprite(f"{graph}/ENEMY.TIF", 12))         # NPC=E6(s12 便裝市民)

    # 地形 tile(從 FM Towns overworld 截圖裁的 32×32,放在輸出同目錄 fmt_*.png):
    #   id 0=水 · 2=草 · 3=林(u2_passable/mapx 直方確認)。有檔才疊。
    import os
    tdir = os.path.dirname(out) or "."
    for tid, nm in ((0, "water"), (2, "grass"), (3, "forest"), (4, "mountain")):
        p = os.path.join(tdir, f"fmt_{nm}.png")
        if os.path.exists(p):
            put(tid, Image.open(p).convert("RGB").resize((TS, TS), Image.NEAREST))

    base.save(out)
    print(f"built {out} {base.size} (tile {TS}px)")

    # 城鎮專用變體(引擎 in_town 時用,--town-tiles):
    #   tile 3=城鎮戶外綠地(overworld 是森林)→ 草;30=建築內地板→ 草;
    #   31=建築磚牆 → 紅磚(mapx21 結構:31 紅牆 + 30 地板 + 3 草)。
    gp = os.path.join(tdir, "fmt_grass.png")
    if os.path.exists(gp):
        town = base.copy()
        grass32 = Image.open(gp).convert("RGB").resize((TS, TS), Image.NEAREST)
        for tid in (3, 30):
            town.paste(grass32, (tid*TS, 0))
        bp = os.path.join(tdir, "fmt_brick.png")
        if os.path.exists(bp):
            brick32 = Image.open(bp).convert("RGB").resize((TS, TS), Image.NEAREST)
            for tid in (31, 58, 59):     # 31 牆 + 58/59 商店建築 → 紅磚(消除黑塊)
                town.paste(brick32, (tid*TS, 0))
        tout = out.replace(".png", "_town.png") if out.endswith(".png") else out + "_town"
        town.save(tout)
        print(f"built {tout} (城鎮變體:tile 3/30→草,31→紅磚)")


if __name__ == "__main__":
    main()
