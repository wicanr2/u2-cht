# 起始位置驗證:重製版 (20,20) = DOS 原版起點

> 結論:本重製版的新遊戲起點 **(20,20)** 與 1983/1989 DOS 原版 **完全一致**,地圖資料亦 byte 相同。
> 玩家曾回報「起始地圖看起來跟 DOS 版不同」,實為 **CGA 配色** 與本版 tileset 配色不同造成的視覺錯覺,非 bug。

## 證據鏈

### 1. 地圖資料與 DOS 原版 byte 相同
`build/dosgame/`(真正的 DOS `ultimaii.exe` + 資料)與本專案使用的 `mapxNN/monxNN/tlkxNN`
逐檔 `cmp` 比對 **0 差異**。mapx20 即「地球」(可辨識北美/南美/歐非/亞洲/澳洲輪廓)。

### 2. DOS 存檔記錄座標 (20,20)
DOS `player` 存檔 offset **0x24–0x25 = `14 14` = (20,20)**(新建角色 HERO)。
與 Alderson《Windows Native Ultima II》逆向(角色結構 0x9c/0xa0 = 0x14,0x14)一致。

### 3. 實機驗證座標欄位語意
把存檔 0x24–0x25 改為 **(50,50)**(全圖該處為深海)後用 DOSBox 載入 →
角色四周全是「洋紅色點」(見 `screenshots/dos_pos_5050_water_proof.png`)。
證明:**① 0x24–0x25 確為座標欄;② DOS CGA 配色中「洋紅 = 海水」。**

### 4. 配色對照(同一張圖、同一位置,只是 tile 顏色不同)

| 地形 | DOS CGA | 本重製版 |
|---|---|---|
| 海水 | 洋紅色點 | 藍色 |
| 陸地 / 草 / 森林 | 青色點 | 綠色 |
| 山 | 灰色 | 灰色 |

`screenshots/start_compare_dos_vs_remake.png` 左為 DOS 原版 @ (20,20)、右為本版 @ (20,20):
地形輪廓(西側陸地、東側海、左側山)一致,僅配色不同。

## 重現方式(DOSBox)

```bash
# dwdos 映像含 dosbox + xvfb-run + imagemagick
cp -r build/dosgame /tmp/dosrun
# (可選)改 /tmp/dosrun/player 的 0x24-0x25 驗證座標
docker run --rm -v /tmp/dosrun:/game -v /tmp/out:/out dwdos bash -c '
  export DISPLAY=:99; Xvfb :99 -screen 0 640x480x16 &
  sleep 3
  printf "[dosbox]\nmachine=cga\n[cpu]\ncycles=400\n[autoexec]\nmount c /game\nc:\nultimaii.exe\n" > /tmp/db.conf
  dosbox -conf /tmp/db.conf & sleep 7
  xdotool key p; sleep 5; import -window root /out/dos.png'   # P = Play(載入存檔)
```

## 啟示

- 本版刻意採用較清晰的 EGA / FM Towns tileset(非 CGA),所以同一張圖視覺上與 DOS 不同 ── 這是設計選擇,非地圖錯誤。
- 玩家若想要「原汁原味 CGA 觀感」,可日後新增 CGA 配色 tileset(資料已就緒)。
- 對照判讀外版畫面時,務必先校準各版本的 tile→顏色對應(本案一度把 DOS 洋紅誤判為森林),再下結論。
