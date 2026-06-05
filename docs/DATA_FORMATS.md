# Ultima II 資料檔格式盤點

> 來源:本機解碼驗證(`dos-original/ultima2/`)+ shikadi ModdingWiki(TheAlmightyGuru 逆向)交叉比對。
> 用途:乾淨重寫的**資料介面真值**;標 ✅=本機驗證,📖=文件來源待驗證。

## 檔案總覽(DOS 原版 = Win 移植版讀取對象)

| 檔案 | 大小 | 數量 | 格式 | 中文化相關 |
|---|---|---|---|---|
| `mapxNN` | 4224 B | 33 | 64×66 tile array,無 header | 否(圖塊) |
| `monxNN` | 384 B | 33 | sector 怪物配置 📖 | 否 |
| `tlkxNN` | 384 B | 15 | **NPC 對話**(high-bit ASCII) | ✅ **是(主目標)** |
| `monsters` | 2176 B | 1 | 怪物屬性表 📖 | 怪物名(待查) |
| `player` | 384 B | 1 | 角色/隊伍存檔 📖 | 否 |
| `picXXX` | 16384 B | 6 | 320×200 interlaced CGA 圖 📖 | 標題字可能在圖內 |
| `ultimaii.exe` | 37344 B | 1 | DOS 16-bit 原版 | terrain tile 圖 @base 0x7C42 |

> Win 移植版額外 `MAPG*/MONG*/TLKG*` = Mike Marcelais 補的 planet 地圖(`G` suffix);DOS 原版用 `x` suffix。

## mapxNN — 地圖 ✅📖
- **4224 byte = 64×66 cell**,**無 header / 無 metadata**,純 tile array(逐 byte)。
- **tile id = byte 值 ÷ 4**(存檔時 ×4)。例:`0x10`(16)→ tile 4(山)。tile id 範圍 0–63。
- 命名尾碼:**0=行星總圖,1/2/3=城鎮,4/5=地牢**。
- ⚠️ 地圖**不存**「哪個 NPC/招牌在哪」;遊戲執行時**動態改寫 map 檔**存 NPC/怪物即時位置(= readme 說「進城就存檔」的原因)。
- tile 圖:**主要 ground truth = U2 Upgrade 獨立 tileset**(見下);`ultimaii.exe @0x7C42` 僅 PoC。

## tileset — 主要 ground truth:U2 Upgrade CGATILES / EGATILES ✅
> 來源(玩家自備,**本 repo 不散布 raw art**):
> https://github.com/mcmagi/ultima-exodus/releases → `u2upgrade-2.1.zip`(取 `CGATILES`/`EGATILES`)。

- **CGATILES** = 65 tile × 64 byte = 16×16 **CGA 2bpp**(4 px/byte)。
- **EGATILES** = 65 tile × 128 byte = 16×16 **EGA 4bpp**(8 byte/row,每 byte 2 nibble,高 nibble=左 px;nibble = 標準 EGA 16 色 index)。`EGATHEME.*/EGACOLOR`(32B)為 theme 變體 palette。
- map 的 `tile_id`(byte÷4)直接索引此 tileset(0–64);實測 mapx20(地球)渲染出正確的藍色波浪海、綠樹叢、灰白山([`screenshots/terrain_in_context.png`](screenshots/terrain_in_context.png))。
- **逐 id 名稱(經 U2 Upgrade sheet 視覺確認)**:0 water · 1 water2 · 2 grass · 3 forest · 4 mountain · 5 town · 6 castle · 7 tower · 8 keep · 9 dungeon · 10 sign · 11 shore · 12 lizardman · 13 ghost · 14 devil · 15 balron · 16 person · **17 horse** · **18 ship** · 19 aircar · 20 rocket · 21 serpent-sign · **22 sword** · 23 barrier · 24–27 person/fighter · **28 wall** · 30 white · **32–57 = A–Z**(招牌字,乾淨 16×16 tile!)· 60–63 monster · 64 orb。
- 工具:[`tools/decode_u2upgrade_tiles.py`](../tools/decode_u2upgrade_tiles.py)(`sheet` 模式產 65-tile 對照圖;`strip` 模式產引擎 tileset)、[`tools/render_map.py`](../tools/render_map.py) `--tileset <EGATILES>`。
- **引擎管線**:`build_poc.sh` / `build_demo.sh` 優先用玩家自備的 **EGATILES** 產引擎 tileset strip(`U2UP_TILES` env 或 `<data>/EGATILES`);找不到才 fallback `ultimaii.exe @0x7C42`(PoC,非正確 tileset)。poc/movement 截圖已用 U2 Upgrade tiles。
- **授權(專案裁示:研究/保存用途允許)**:Ultima II(1982)年代久遠、廣泛流傳;U2 Upgrade 為社群保存專案。本 repo 收錄 tileset 供逆向研究:
  - raw tileset:[`tileset/CGATILES`](../tileset/CGATILES)、[`tileset/EGATILES`](../tileset/EGATILES)(見 [`tileset/README.md`](../tileset/README.md))。
  - 完整對照圖:[`screenshots/tileset_egatiles.png`](screenshots/tileset_egatiles.png)(EGA 65 tile + id 名)、[`screenshots/tileset_cgatiles.png`](screenshots/tileset_cgatiles.png)(CGA)、map-context [`screenshots/terrain_in_context.png`](screenshots/terrain_in_context.png)。

## ~~ultimaii.exe @0x7C42 embedded tiles~~(PoC,**非主要 ground truth**)
- 早期 PoC 從 `ultimaii.exe @0x7C42` 抽 16×16 2bpp tile,作為「能用真資料渲染」的概念驗證。
- ⚠️ **誠實更正**:逐 byte 比對發現此 EXE tile set 與 U2 Upgrade CGATILES **只有 id 0 相同**,id 1–31 皆不同 —— 故 **EXE @0x7C42 不是正確 tileset**,先前用它推測 ship/horse/castle 會錯。已改以 U2 Upgrade 為主。
- EXE 抽取流程保留於 `tools/extract_tiles.py`(標為 PoC/fallback);幾何(16×16/2bpp/stride 64)成立,但**內容非遊戲實際 tileset**。
- font/招牌字(EXE id 32+)為變動-stride、未解 glyph(`screenshots/font_raw_stream.png` 僅 raw dump)—— 在 U2 Upgrade 中招牌字就是乾淨的 32–57 A–Z tile;中文化時招牌走訊息系統 + SDL_ttf 翻譯,故無論如何非必要。

## tlkxNN — 對話 ✅(中文化主目標)
- **編碼:high-bit-set ASCII**(每 byte `OR 0x80`),清掉 bit7 還原;`\x00` 分段,`\r`(0x0d)換行。
- 範例(`tlkx32`):`"SANTRE THE SWASHBUCKLER WARNS:BEWARE, I'VE A QUICK BLADE!"`、`"A CLERK STATES: WELCOME\rTO THE HOTEL CALIFORNIA!"`。
- 檔尾有組譯殘渣(`PRBYTE/PRINT/$0D...`),解碼時過濾。
- 全 15 檔已解 → **108 行對話**,輸出 `extract/talk_dialogue.tsv`(file/index/原文/zh_hant 欄)。
- 中文化原則(沿用 U3):**不寫回 `.tlkx`**(會破壞長度);用外部 UTF-8 覆蓋層以 `(檔名, index)` 為 key。

## 地牢(mapxN4/N5)📖
- 每樓層 **256 byte = 16×16 cell**,每 cell 1 byte。

## picXXX — CGA 圖 📖
- 320×200 interlaced CGA(`picdng`地牢 / `picdra`龍 / `picmin`Minax / `picout`戶外 / `picspa`太空 / `pictwn`城鎮)。
- 重寫時轉 PNG 載入(SDL_image),非文字。

## monxNN — sector 實體 (NPC/怪物) ✅(已破解,本機交叉驗證)
- 384 byte = **32 格平行陣列**,每實體一個 slot index i (0..31):

  | 欄位 | offset | 內容 |
  |---|---|---|
  | X | `0x00 + i` | x 座標 (0..63) |
  | Y | `0x20 + i` | y 座標 (0..65) |
  | status | `0x40 + i` | 狀態/旗標 (固定物=`0xff`,活動實體=其他) |
  | tile | `0x60 + i` | **tile_id × 4**(同地圖編碼);`0`=空 slot |
  | flag | `0x80 + i` | 次要旗標 (多為 `0x08`);`0x100` 處固定 `0x1a` 標頭 |

- **驗證**:monx21 中 tile=24 的 8 個實體,其 (X,Y) 與 mapx21 地圖上 8 個 id-24 格**完全吻合** → 格式確認。
- 用途:地圖 (mapxNN) 只有地形/建築/招牌;**實體層 (NPC/怪物) 由 monxNN 疊上** → 空城變活城。
- 引擎:`u2_mon.{h,c}` 解析;`render` 先畫地形 tile,再疊 monxNN 實體 tile。

## monsters / player 📖(待本機驗證)
- `monsters`(2176 B):怪物屬性表(全域,oracle 以 `s_Monsters` 載入)。
- `player`(384 B):角色屬性/隊伍/存檔。重寫存檔相容性需逐欄位比對(對照 oracle `FUN_004060a0` 狀態列讀取邏輯)。

## 中文化文字來源總結(兩處)
1. **exe 內嵌 UI 字串**:392 條 → `extract/exe_translatable_strings.tsv`(vaddr/category/原文/zh_hant)。
2. **tlkx 對話**:108 行 → `extract/talk_dialogue.tsv`。

## 參考
- [Ultima II Map Format — shikadi ModdingWiki](https://moddingwiki.shikadi.net/wiki/Ultima_II_Map_Format)
- [Ultima II Dungeon Format](https://moddingwiki.shikadi.net/wiki/Ultima_II_Dungeon_Format)
- [Ultima II 總覽 — ModdingWiki](https://moddingwiki.shikadi.net/wiki/Ultima_II:_Revenge_of_the_Enchantress)
- [Dino's Ultima Page — U2](https://gigi.nullneuron.net/ultima/u2.php)
