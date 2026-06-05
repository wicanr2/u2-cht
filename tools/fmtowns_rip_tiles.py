#!/usr/bin/env python3
"""從 FM Towns Ultima II overworld 截圖抽出 32×32 地形 tile。

來源:Tsugaru 模擬器跑 FM Towns《Ultima Trilogy》→ Ultima II overworld,
import 截無損 PNG(SCALE 100 native;見 docs/FMTOWNS_TILESET.md 的模擬器流程)。

格式發現:
  - FM Towns U2 overworld tile = **32×32 px**(比 DOS 16×16 大;enhanced)。
  - palette = **純數位 RGB**(0/255 三通道):黑/藍/綠/青/紅/黃/白 + (0,34,238) 水藍。
    草地是黑點 dither 疊在純綠上;水是藍/青 dither。
  - 視窗左側為地圖 viewport(藍框內);右側為狀態面板,需裁掉。

用法:
  fmtowns_rip_tiles.py <overworld.png> <out_sheet.png>
    [--vp x0 y0 x1 y1]   viewport bbox(預設自動估)
"""
import sys
import numpy as np
from PIL import Image
from collections import Counter


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    a = np.array(Image.open(sys.argv[1]).convert("RGB")).astype(int)
    # viewport bbox(藍框內左圖區)
    vp = (14, 38, 358, 418)
    if "--vp" in sys.argv:
        i = sys.argv.index("--vp")
        vp = tuple(int(x) for x in sys.argv[i + 1:i + 5])
    x0, y0, x1, y1 = vp
    img = a[y0:y1, x0:x1]
    T = 32

    # tile 格線相位(對 32 取邊緣強度峰)
    def phase(diff_axis):
        d = np.abs(np.diff(img, axis=diff_axis)).sum(axis=(1, 2) if diff_axis == 0 else (0, 2))
        p = np.zeros(T)
        for k in range(len(d)):
            p[k % T] += d[k]
        return int(np.argmax(p))
    oy, ox = phase(0), phase(1)

    # 純色 snap palette(viewport 最常見色)
    cnt = Counter(map(tuple, img.reshape(-1, 3)))
    PURE = np.array([c for c, _ in cnt.most_common(12)])

    def snap(cell):
        flat = cell.reshape(-1, 3)
        d = ((flat[:, None, :] - PURE[None, :, :]) ** 2).sum(2)
        return PURE[d.argmin(1)].reshape(cell.shape).astype(np.uint8)

    tiles, order, freq = {}, [], {}
    y = oy
    while y + T <= img.shape[0]:
        x = ox
        while x + T <= img.shape[1]:
            cell = snap(img[y:y + T, x:x + T])
            key = cell.tobytes()
            if key not in tiles:
                tiles[key] = cell; order.append(key); freq[key] = 0
            freq[key] += 1
            x += T
        y += T
    order.sort(key=lambda k: -freq[k])

    cols, S, pad, sc = 8, T, 2, 2
    rows = (len(order) + cols - 1) // cols
    sheet = Image.new("RGB", (cols * (S * sc + pad) + pad, rows * (S * sc + pad) + pad), (40, 40, 55))
    for i, k in enumerate(order):
        t = Image.fromarray(tiles[k]).resize((S * sc, S * sc), Image.NEAREST)
        sheet.paste(t, (pad + (i % cols) * (S * sc + pad), pad + (i // cols) * (S * sc + pad)))
    sheet.save(sys.argv[2])
    print(f"wrote {sys.argv[2]}: {len(order)} unique 32x32 tiles "
          f"(grid phase {ox},{oy}; top freq {[freq[k] for k in order[:5]]})")


if __name__ == "__main__":
    main()
