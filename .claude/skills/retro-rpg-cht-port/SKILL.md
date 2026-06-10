---
name: retro-rpg-cht-port
description: >
  把 1980–90 年代 tile-based RPG(Ultima 世代:Ultima I–III、Akalabeth 等)
  逆向 + 以 C/SDL2 重寫引擎 + 繁體中文化 + 打包成 Linux AppImage / Windows zip
  的完整方法論。適用情境:有 MIT/乾淨源碼或可逆向的原版 binary、要做跨平台繁中
  重製、需要 headless 截圖驗證與可寫存檔。實證來源:u2-cht(Ultima II 繁中)。
---

# Retro RPG 繁中化 + SDL2 移植 playbook

> 這份 skill 把 u2-cht(Ultima II: 女巫的復仇 繁中重製)一路踩過的決策與雷整理成
> 可複用流程。下一個專案(u3-cht / Ultima III、或任何 Ultima 世代 tile RPG)直接照走。
> 原則:**正確性 / 領域對齊 > 可落地交付 > 時程 > 可維護性 > 效能**。

## 何時用

- 要把某個老 tile RPG 做成繁中、且要在現代 Linux/Windows 跑。
- 手上有:原版資料檔(map/save/talk)+ 可選的乾淨源碼或反組譯文件。
- 目標是「移植 + 在地化」,不是從零設計遊戲。

## 0. 選基礎(別挑最硬的路)

| 選項 | 評價 |
|---|---|
| MIT / 開源的官方授權重製版源碼 | **最佳**。保留遊戲邏輯,只重寫平台層。 |
| 乾淨重寫 + 反組譯當 oracle | 次佳。邏輯自己寫,行為對照反組譯(mcmagi 風格)當 ground truth。 |
| 直接 patch 原版 binary | **最差**。字型/字串空間死巷,等於完整逆向。不要。 |

- **反組譯文件是「行為真值 oracle」**:對不上時拿它核對原版語意(命中公式、EXP、timeout…),不是拿來編譯。
- 建一份 `CONTEXT.md`(ubiquitous language)+ `docs/DATA_FORMATS.md`(資料格式破解)+ `docs/adr/`(重大決策)。

## 1. 逆向資料格式(原版檔保持不動)

- **地圖**:常見「tile_id ×4」quirk → 載入時 `/4` 還原(u2:`raw[y*W+x]/4`)。先做 tile 直方圖找 landmark。
- **世界圖 landmark**:城/堡/地牢入口常是「每種一格」的 singleton tile(u2 raw 0x14–0x28 → id 5–10,各通往不同 mapxNN)。**別只認一個 id**,否則玩家那塊陸地的城進不去。
- **存檔**:Ultima 世代多為 **BCD**(0x15→十進位15;4 位數 HP 兩 byte 高位在前)。欄位用實機建角差分逐一消歧,寫進 `u2_save.h` 註解。
- **對話**:offset 表 + null 結尾字串。**中文不要寫回原檔**(破壞 offset);用外部 UTF-8 字串表以 `(map, offset)` 為 key,載入時覆蓋。

## 2. SDL2 引擎(deep module,vertical slice)

- tileset 用「橫條 strip」:`u2_tileset_blit(dst, strip, id, x, y, size)` = `SDL_BlitScaled` nearest 放大。介面收斂成一個函式。
- 主畫面:viewport 置中玩家 + 鏡頭 clamp;狀態列走翻譯覆蓋層。
- **headless 驗證迴圈(最重要)**:`--script <CMDS> <prefix>` 用 `SDL_Init(0)` 離屏跑,逐步存 PNG;固定 LCG seed(`rng=1`)→ 決定性可重現。這是你的 pass/fail 訊號,先建它再寫功能(見 `feedback-loop-priority`)。
- **第 00 張 PNG 是「移動前」初始畫面**;第 N 個指令的結果在第 N 張。看錯 frame 會誤判功能壞掉。

## 3. CJK 文字

- **用系統 TTF 重建字庫**,不要點陣字:`SDL_ttf` + `wqy-zenhei.ttc`(繁中涵蓋全)。
- 早期想從 EXE 抽點陣字(變動 stride)是死巷 —— 招牌字走訊息系統 + TTF 翻譯即可,不必解 glyph。
- 翻譯放 TSV(`exe_translatable_strings.tsv` / `talk_dialogue.tsv`),引擎載入覆蓋。

## 4. FM Towns 美術(品質的關鍵雷)

- FM Towns 版(Fujitsu《Ultima Trilogy》)美術比 CGA/EGA 好,**但別碰二進位 raw atlas**:直接解 raw stream 顏色會雜訊爆掉(palette/stride 解錯),比 EGA 還醜。
- **正確做法:模擬器(Tsugaru)無損截圖 → rip 16×16 tile**。地形/主角從乾淨的 overworld 截圖裁(u2:`GOLDEN_overworld.png`,純數位 RGB palette:黑/藍/綠/青/紅/黃/白)。
- **切換畫風時主角也要變**:tileset strip 不只覆寫地形,**主角 avatar(玩家 tile id)也要換**,否則 `ega==fmtowns` 的玩家 tile,使用者一眼看穿。船/怪物要「航行中 / 戰鬥中」的乾淨截圖才 rip 得到 —— 沒有就誠實標後續,別塞雜訊圖。
- blit 是不透明的:avatar tile 自帶背景(rip 時連地形背景一起裁,overworld 上最自然)。

## 5. 存檔 / 讀檔(AppImage 的隱形雷)

- **AppImage 內資料在唯讀 squashfs**:存回打包路徑必失敗。用 `SDL_GetPrefPath("廠牌","遊戲")` 取跨平台可寫目錄(Linux `~/.local/share`、Windows `%APPDATA%`),打包的 player 檔當**唯讀範本**。
- 開場選單「新遊戲 / 繼續」:`繼續` 載入可寫存檔、`新遊戲` 跑建角 → 立即存檔。離開自動存檔。
- 建角用 state machine(`create_feed(key)` / `render_create`),互動與 headless `--screens` **共用 render**,版面才驗得到。
- 存檔正確性:`store(load(x))` 位元組完全相同(無損 round-trip)就是最強證據。

## 6. 打包(Docker first)

- 全程 Docker build(`Dockerfile.pkg`:ubuntu 22.04 + libsdl2-{dev,ttf,image} + fonts-wqy-zenhei + mingw-w64 + linuxdeploy + appimagetool)。
- **AppImage 自我遞迴雷**:linuxdeploy 把 `AppRun` 建成指向 `usr/bin/<exe>` 的 **symlink**;你 `cat > AppRun` 會**寫穿 symlink 覆寫掉真 ELF** → 無限遞迴。**寫 AppRun 前先 `rm -f AppRun`**;真 ELF 用獨特名(`u2cht_bin`)避免撞名。
- AppImage 在無 FUSE 環境用 `--appimage-extract-and-run`。
- Windows:mingw 交叉編譯(`x86_64-w64-mingw32-gcc -Dmain=SDL_main … -lmingw32 -lSDL2main -lSDL2… -mwindows`),用 SDL2/ttf/image 的 **mingw devel** 包,絕對路徑 INC/LIB,收 runtime DLL + `.bat` + README。

## 7. 驗證

- 主驗證 = headless 截圖逐畫面比對(決定性 LCG)。
- 互動流程(選單/建角)用 **Xvfb + xdotool** 驅動真實視窗:送鍵 → 確認乾淨退出 + 存檔產生 + 解碼存檔欄位正確。無截圖工具也能靠「行為 + 存檔內容」驗。
- 每完成一段就 commit + push;commit message 寫清楚根因與修法(英文 co-author trailer)。

## 反面教材(別重蹈)

- 只認單一 town tile id → 大半地圖的城進不去。
- 看 frame 00 當功能結果 → 誤判「沒進城/沒移動」(那是移動前畫面)。
- 直接解 FM Towns raw atlas → 顏色雜訊,不如 EGA。
- 存回 AppImage 內路徑 / 寫穿 AppRun symlink → 存檔失敗 / binary 自我遞迴。
- tileset 只換地形不換主角 → 使用者切換畫風發現主角不動。

## 參考實作(u2-cht)

- `src/game_main.c`:模式狀態機 + 開場序列(標題→splash→選單→建角→遊戲)+ headless `--script`/`--screens`。
- `src/u2_save.c`:BCD 編解碼 + `u2_save_store` 無損 round-trip。
- `tools/build_fmtowns_strip.py`:FM Towns tile(含主角)覆寫到 EGA strip。
- `build_release.sh` / `docker/Dockerfile.pkg`:AppImage + Windows 打包。
