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
| `ultimaii.exe` | 37344 B | 1 | DOS 16-bit 原版 | tile 圖 @offset 0x7C43 |

> Win 移植版額外 `MAPG*/MONG*/TLKG*` = Mike Marcelais 補的 planet 地圖(`G` suffix);DOS 原版用 `x` suffix。

## mapxNN — 地圖 ✅📖
- **4224 byte = 64×66 cell**,**無 header / 無 metadata**,純 tile array(逐 byte)。
- **tile id = byte 值 ÷ 4**(存檔時 ×4)。例:`0x10`(16)→ tile 4(山)。tile id 範圍 0–63。
- 命名尾碼:**0=行星總圖,1/2/3=城鎮,4/5=地牢**。
- ⚠️ 地圖**不存**「哪個 NPC/招牌在哪」;遊戲執行時**動態改寫 map 檔**存 NPC/怪物即時位置(= readme 說「進城就存檔」的原因)。
- tile 圖塊嵌在 `ultimaii.exe`(DOS **@offset 0x7C43 = 31811**);Win 版用自家 tileset + `Font.txt`。

## tile 美術格式 ✅(已破解,task #5)
- 位置:`ultimaii.exe` **@0x7C42**,**64 個 tile**,每 tile **16×16**。
  - ⚠️ 校正:ModdingWiki 標 31811(0x7C43),實測 sprite 會**水平左移 4px**(人形底部出現 bleed);正確 base 為 **0x7C42**(對照 ultima2.voyd.net reference 圖,字母/人形置中、無 bleed)。可用 env `U2_TILE_BASE` 覆寫。
  - 仍待釐清:字母區(tile ~33+,招牌用)在 0x7C42 仍有不一致,font 子區塊對齊另案處理。
- 編碼:**CGA Linear 2bpp** —— 4 px/byte,bit7-6=最左像素、bit5-4 次之…bit1-0=最右。**64 byte/tile**。
- map 的 `tile_id`(byte÷4)直接索引此表 0..63。
- 配置:tile ~0–31 為地形/物件(0=洋紅海洋、青色森林、城堡、人形 NPC…),**~32+ 為字符集**(A–Z 等,地圖招牌文字用)。
- CGA 調色盤三組(對應 Alderson Win port):`original` 黑/青/洋紅/白、`red` 黑/綠/紅/白、`blue` 黑/綠/藍/白。
- 抽取工具:`tools/extract_tiles.py`(輸出 1024×16 tileset strip PNG,**EA 版權不散布**)。

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
