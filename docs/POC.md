# 垂直切片 PoC — 三假設一次驗證

> 狀態:✅ 通過(2026-06-04)。對應 PLAN.md 步驟 4 / U3 評估建議的「風險最高三點一次打掉」。

## 目的

在動工整個引擎前,用最小程式一次驗證三個最關鍵假設:

| # | 假設 | 驗證方式 | 結果 |
|---|---|---|---|
| 1 | **繪圖換得掉** | SDL2 離屏 surface 畫 tile 地圖(整數倍放大) | ✅ |
| 2 | **資料讀得到** | 解析真實 `mapx21`(tile = byte ÷ 4) | ✅ |
| 3 | **中文畫得出** | SDL2_ttf + WQY 畫真實 U2 字串的中文翻譯 | ✅ |

## 截圖

![PoC](screenshots/poc_map_cjk.png)

左側為 `mapx21`(城鎮)中段 viewport — 中央庭院建築、護城河(藍)、草地(綠)結構清晰可辨,證明 tile÷4 解析正確。右側為三來源中文文字(標題 / exe 內嵌 UI / tlkx 對話 / 結局),CJK 在內部 3× 解析度原生繪製、銳利。

## 架構(deep modules / 垂直層)

```
src/
  u2_map.{h,c}     # data 層:mapxNN 解析,隱藏 ×4 quirk,介面只有 load / tile
  u2_render.{h,c}  # render 層:tile id→色塊 (placeholder) + viewport 繪製
  u2_text.{h,c}    # text 層:SDL2_ttf UTF-8 繪字 (ADR 0001 文字層原生繪製)
  poc_main.c       # 接線:load → render → draw CJK → 存 PNG
```

## 建置 / 重現

```bash
# 需自備合法 Ultima II 資料檔 (本 repo 不含)
./build_poc.sh ../dos-original/ultima2/mapx21 build/poc_out.png
```
全程 Docker(`docker/Dockerfile`:SDL2 + SDL2_ttf + SDL2_image + fonts-wqy-zenhei),
headless 離屏渲染存 PNG,不需顯示器。

## 已知簡化(後續任務)

- **tile 美術為 placeholder 色塊** → task #5 抽真實 U2 tile(CGA 解碼)。
- **CJK 用 SDL2_ttf** → production 可換 u6-cht 點陣字 atlas(BDF→自家格式)。
- **viewport 靜態置中** → 引擎階段接上隊伍移動 / 鏡頭跟隨。
- **換行未 CJK-aware** → 引擎階段補全形 2 倍寬換行(對照 oracle `RewrapString`)。
