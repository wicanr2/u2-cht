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

版面對齊真實 Ultima II(參考 [ultima2.voyd.net](https://ultima2.voyd.net/) / [Codex](https://wiki.ultimacodex.com/wiki/Ultima_II:_The_Revenge_of_the_Enchantress)):**上方地圖 viewport + 底部左訊息列、右狀態欄**。地圖用從 `ultimaii.exe` 抽出的真實 CGA tile(綠樹森林 / 藍色結構 / 白色 NPC),並疊上 `monxNN` 實體層(空城→活城)。右側狀態(生命/食物/經驗/黃金),CJK 內部 3× 原生繪製。

**底部訊息列為端到端在地化展示**:引擎讀**原始 `tlkx21`**(high-bit ASCII 解碼)→ 經**翻譯覆蓋層** `talk_dialogue.tsv`(以原文為 key)查譯文 → 繪 CJK。畫面中「流浪漢格倫德說:精通謎題者方為真正的高手!」「安迪.格林堡抱怨:什麼?沒有軟體?!?」「羅伯.伍德海大喊:防拷!防拷!」皆為**真實 tlkx21 NPC 對話的中譯**(非硬編);查無譯文則 fallback 原文(暗色)。

> 完整在地化管線:**原始資料 → 覆蓋層 → CJK**,模組 `u2_talk`(解碼)+ `u2_strings`(覆蓋層)。
> 注:本 PoC 截圖用早期 `ultimaii.exe @0x7C42` PoC tile;**正確 tileset ground truth 為 U2 Upgrade CGATILES/EGATILES**(此 PoC tile 與其只有 id 0 相同),詳見 [DATA_FORMATS.md](DATA_FORMATS.md)。實體層見 monxNN 段。

## 架構(deep modules / 垂直層)

```
src/
  u2_map.{h,c}      # data 層:mapxNN 解析,隱藏 ×4 quirk
  u2_mon.{h,c}      # data 層:monxNN 實體層 (NPC/怪物,32 格陣列)
  u2_talk.{h,c}     # data 層:tlkxNN 對話解碼 (high-bit ASCII)
  u2_strings.{h,c}  # i18n 層:翻譯覆蓋層 (原文→zh_hant)
  u2_tileset.{h,c}  # render 層:真實 CGA tile (從 ultimaii.exe 抽出) blit
  u2_render.{h,c}   # render 層:viewport + 實體疊繪 (真 tile,色塊 fallback)
  u2_text.{h,c}     # text 層:SDL2_ttf UTF-8 繪字 (ADR 0001 文字層原生繪製)
  poc_main.c        # 接線:仿 U2 版面 + 端到端在地化展示
```

## 建置 / 重現

```bash
# 需自備合法 Ultima II 資料檔 (本 repo 不含)
./build_poc.sh ../dos-original/ultima2/mapx21 build/poc_out.png
```
全程 Docker(`docker/Dockerfile`:SDL2 + SDL2_ttf + SDL2_image + fonts-wqy-zenhei),
headless 離屏渲染存 PNG,不需顯示器。

## 已知簡化(後續任務)

- ~~tile 美術為 placeholder 色塊~~ ✅ **已完成**:真實 CGA tile 抽取(task #5,`tools/extract_tiles.py`)。
- **CJK 用 SDL2_ttf** → production 可換 u6-cht 點陣字 atlas(BDF→自家格式)。
- **viewport 靜態置中** → 引擎階段接上隊伍移動 / 鏡頭跟隨。
- **換行未 CJK-aware** → 引擎階段補全形 2 倍寬換行(對照 oracle `RewrapString`)。
- **NPC/怪物動態位置** → map 檔執行時改寫的層尚未解析(疊在地形 tile 上)。
