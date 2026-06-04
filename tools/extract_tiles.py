#!/usr/bin/env python3
"""[PoC/fallback] 從 DOS ultimaii.exe @0x7C42 抽 tile,輸出 tileset PNG。

⚠️ 注意:此 EXE-embedded tile 與遊戲實際 tileset(U2 Upgrade CGATILES/EGATILES)
**只有 id 0 相同**,id 1+ 皆不同 → **非正確 ground truth**。正確 tileset 請用
`decode_u2upgrade_tiles.py` + `render_map.py --tileset`。本檔僅保留 PoC 用途。

格式 (docs/DATA_FORMATS.md):
  - **terrain/sprite tile (id 0–31)**:@ base **0x7C42**,16×16,**CGA Linear 2bpp**
    (4 px/byte,bit7-6=最左像素…bit1-0=最右),64 bytes/tile,stride 64。
    map 的 tile_id (byte÷4) 直接索引。地形/載具/NPC/怪物皆在此區,**已驗證對齊**
    (tile 27 在 0x7C42 為乾淨置中人形;0x7C40/0x7C43 會 wrap/bleed)。
  - **font/招牌字 (id 32+)**:變動-stride 字型子區塊,**未解 glyph 邊界**(raw),
    本工具將 id ≥ 32 渲中性 placeholder。招牌文字走訊息系統 + SDL_ttf 翻譯。

調色盤 (palette,預設 blue):
  blue     = 黑/綠樹/深藍水/灰白 (= 預設,取樣自 Alderson Win port 綠樹參考畫面,可讀)
  red      = 黑/綠樹/紅水/灰白
  original = 黑/青/洋紅/白 (alternate:DOS CGA,較刺眼,非主圖)

用法:
  python3 extract_tiles.py <ultimaii.exe> <out.png> [original|red|blue]
  輸出:1024×16 橫向 strip (64 tile × 16px);engine 以 id*16 取 src x。

注意:tile 美術屬 Origin/EA 版權,輸出 PNG **不散布**;請自備合法 Ultima II。
"""
import os
import sys
from PIL import Image

# tile 資料在 ultimaii.exe 的起始 offset。可用環境變數 U2_TILE_BASE 覆寫測試。
TILE_BASE = int(os.environ.get("U2_TILE_BASE", "0x7C42"), 0)
N_TILES = 64
N_TERRAIN = 32          # id 0–31 為有效 terrain/sprite;32+ 為 font 子區塊

# palette[color_index] → RGB。color 1=植被、2=水、3=地形/建物/sprite。
# 預設 "blue":取樣自 Alderson Win port 綠樹/藍水參考畫面 (可讀的 U2 視覺)。
# "original" CGA 青/洋紅為 alternate(較刺眼,僅附錄/比較用,非主圖)。
PALETTES = {
    "blue":     [(0, 0, 0), (0, 170, 0), (40, 60, 180), (205, 205, 205)],  # 預設:綠樹/深藍水/灰白
    "ega":      [(0, 0, 0), (0, 170, 0), (40, 60, 180), (205, 205, 205)],  # 同 blue 別名
    "red":      [(0, 0, 0), (0, 170, 0), (190, 40, 40), (205, 205, 205)],  # 綠樹/紅水
    "original": [(0, 0, 0), (85, 255, 255), (255, 85, 255), (255, 255, 255)],  # alternate: DOS CGA
}
DEFAULT_PAL = "blue"


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
    pal_name = sys.argv[3] if len(sys.argv) > 3 else DEFAULT_PAL
    pal = PALETTES[pal_name]

    img = Image.new("RGB", (N_TILES * 16, 16), (0, 0, 0))
    for t in range(N_TERRAIN):          # 只抽有效 terrain/sprite tile
        off = TILE_BASE + t * 64
        if off + 64 > len(exe):
            break
        px = decode_tile(exe, off)
        for y in range(16):
            for x in range(16):
                img.putpixel((t * 16 + x, y), pal[px[y][x]])
    # id 32–63:font 變動-stride 子區塊 (未解 glyph),渲中性 placeholder (招牌走翻譯)
    for t in range(N_TERRAIN, N_TILES):
        for y in range(16):
            for x in range(16):
                edge = x in (0, 15) or y in (0, 15)
                img.putpixel((t * 16 + x, y), (40, 40, 48) if edge else (12, 12, 16))
    img.save(out)
    print(f"寫出 tileset → {out} ({N_TILES} tiles, palette={pal_name})")


if __name__ == "__main__":
    main()
