#!/usr/bin/env python3
"""把真實 FM Towns 地形 tile 覆寫到引擎 tileset strip 的對應 DOS id。

來源:模擬器 overworld 截圖抽出的 16×16 FM Towns tile(tileset/fmtowns/*.png;
見 docs/FMTOWNS_TILESET.md)。其餘 id 沿用 base strip(EGA),形成「FM Towns 地形 +
EGA 其餘」的混合 tileset,接進引擎 G 鍵切換。

DOS tile id ↔ FM Towns tile:0/1=water 2=grass 3=forest 11=shore;16=主角 avatar。

用法:build_fmtowns_strip.py <base_ega_strip.png> <fmtowns_dir> <out_strip.png>
"""
import sys
from PIL import Image

# DOS tile id -> FM Towns tile 檔名(地形 + 主角 avatar,讓 G 切換時主角也變)
MAP = {0: "water", 1: "water", 2: "grass", 3: "forest", 11: "shore", 16: "player"}


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)
    base = Image.open(sys.argv[1]).convert("RGB")  # 65×16 tile 橫條 (1040×16)
    fdir = sys.argv[2]
    strip = base.copy()
    for tid, name in MAP.items():
        try:
            t = Image.open(f"{fdir}/{name}.png").convert("RGB").resize((16, 16), Image.NEAREST)
        except FileNotFoundError:
            continue
        strip.paste(t, (tid * 16, 0))
    strip.save(sys.argv[3])
    print(f"wrote {sys.argv[3]} (FM Towns 地形覆寫 id {sorted(MAP)} + EGA 其餘)")


if __name__ == "__main__":
    main()
