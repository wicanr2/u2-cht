# 04 · 乾淨引擎 + 中文化

## 引擎架構:deep modules / 垂直切片
- 按 **feature 切**(`u2_map / u2_mon / u2_talk / u2_save / u2_dungeon / u2_render / u2_text / u2_tileset / u2_play`),不要按抽象層攤平(controllers/services/repos)。
- adapter(SDL2)只放邊界。對外介面窄、內部複雜 = deep。
- 整合主迴圈做成**單一狀態機**(地面↔城鎮↔地牢↔角色表↔太空),同一份引擎、同一套文字層。
- C11 + SDL2 + SDL2_ttf + SDL2_image + SDL2_mixer(mixer 可選,`pkg_check_modules` + `HAVE_SDL_MIXER`)。CMake。
- 入口分多個:`game_main`(互動 + headless --script)、`poc_main`、`demo_main`、`sheet_main`、`dungeon_main`。

## CJK 渲染:雙層 + 內外解析度解耦
- **像素圖層**:原版 16×16 tile 整數倍 nearest 放大。
- **文字圖層**:CJK glyph 在**內部高解析度原生繪製**,永不被縮放 → 恆銳利。
- 內部 render 320×200×N(N 決定 glyph 大小,推薦 3× = 960×600 / CJK 24×24);視窗解析度獨立。寫成 ADR。
- 字型:wqy-zenhei(Linux/打包)或 Noto Sans CJK(CI 下載穩定);**不要 cubic,用優質系統 TTF**。

## 翻譯管線
- 兩類來源:exe 內嵌 UI 字串(抽出成 TSV)+ 資料檔對話(解碼後翻)。引擎硬編訊息另存多語 TSV。
- **不寫回原始檔**:外部 UTF-8 覆蓋層,`(來源,key)` 索引,查無譯文 fallback 原文。
- 多語切換(F4 循環 繁中/EN/日):`tr(zh)` 依當前語言查表。加語言只需 TSV 加欄。
- 字串放 `translations/*.tsv`(zh\ten\tja…)。

## 正典對齊心法
- 機制/數值以反編 oracle 為主、攻略網站為輔,衝突標註並採可信者。
- 任務鏈、時間之門拓樸、怪物專屬攻擊、BOSS 對決等逐項對齊正典;改動要保「可破關鏈」不回退(見 05)。
- 缺資料/推測明確標,不編造。
