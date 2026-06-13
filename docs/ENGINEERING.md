# 工程技術文件 — Ultima II 繁中重製

> 本文是給開發者 / 逆向工程同好的技術紀錄。玩家導向的介紹見 [README.md](../README.md)。
>
> 標 **[史實]** 為公開可考的史實;**[本專案]** 為本 repo 的逆向發現或推測。
> 信心標示沿用 `[確定] / [推測] / [未解]`。

## 目錄
- [策略:反編當 oracle,乾淨重寫](#策略反編當-oracle乾淨重寫)
- [引擎架構(deep modules)](#引擎架構deep-modules)
- [資料格式破解亮點](#資料格式破解亮點)
- [oracle 機制分析](#oracle-機制分析)
- [FM Towns 考古](#fm-towns-考古)
- [中文化管線](#中文化管線)
- [建置與執行](#建置與執行)
- [完整進度](#完整進度)
- [專案結構](#專案結構)
- [致謝與參考](#致謝與參考)

---

## 策略:反編當 oracle,乾淨重寫

原始標的是 **John Alderson《Windows Native Ultima II》v1.01 (2000)**——一個 stripped 的
**MFC / GDI** Windows 移植 exe。直接反編成可用引擎是最硬的路(無型別 `FUN_xxx`、纏繞 MFC runtime)。
因此採與姊妹作 [u3-cht](https://github.com/wicanr2/u3-cht) 相同的成功模式:**反編只當行為參考,引擎手寫乾淨版**。

```
Ghidra 反編 C  ──(只當行為/演算法 oracle,不照抄 MFC 殼)──┐
破解 U2 原版資料格式 ───────────────────────────────────┼──▶ 手寫乾淨 SDL2 C 引擎
原版資料檔(玩家自備)─────────────────────────────────┘     (可公開、可維護、好中文化)
```

| 角色 | 內容 | 公開? |
|---|---|---|
| **行為 oracle** | [`oracle/ultima2_decompiled.c`](../oracle/)(~46k 行)+ 函式索引 | ✅ 已收錄(CRPG 史保存) |
| **資料格式真值** | [`docs/DATA_FORMATS.md`](DATA_FORMATS.md)(map / monx / tlkx / player 等) | ✅ |
| **乾淨重寫** | [`src/`](../src/)(SDL2 取代 GDI、UTF-8 + CJK) | ✅ |
| 原版 binary / 資料檔 | Alderson exe + DOS 原版資料 | ❌ 玩家自備 |

---

## 引擎架構(deep modules)

按 feature 切而非按抽象層攤平,adapter 只放邊界(SDL2)。`src/` 的垂直切片:
`u2_map / u2_mon / u2_talk / u2_strings / u2_tileset / u2_render / u2_text / u2_play / u2_save / u2_dungeon`,
入口 `game_main`(互動視窗 + headless 腳本)、`poc_main` / `demo_main` / `sheet_main` / `dungeon_main`。

整合主迴圈為**單一 `u2_game` 狀態機**:地面 ↔ 城鎮 ↔ 地牢 ↔ 角色表 ↔ 太空,同一份引擎、同一套 CJK 文字層。

詳見 [`PLAN.md`](../PLAN.md)。

---

## 資料格式破解亮點

完整見 [`docs/DATA_FORMATS.md`](DATA_FORMATS.md)。四個值得一提的破解:

### 1. 地圖 `mapxNN` — tile id 要 ÷4
4224 byte = 64×66 cell,**無 header**,純 tile array。存檔時 `tile id × 4`(低 2 bit 是 flag),
讀取要 **÷4**。尾碼 0=行星總圖、1/2/3=城鎮、4/5=地牢。

![地球大地圖](screenshots/terrain_in_context.png)
> mapx20(地球):海 / 森林 / 山以正確 tileset 渲染,驗證 tile÷4 解碼。

### 2. 對話 `tlkxNN` — high-bit ASCII
NPC 對話每 byte `OR 0x80`(ModdingWiki 誤稱 encrypted,實為 high-bit);
解碼 `byte & 0x7f`、`\r` 換行。15 檔共解出 **108 行對話**。

### 3. 實體層 `monxNN` — 32 格平行陣列
地圖只存地形 / 建築;NPC / 怪物座標另存於 `monxNN`(X / Y / status / tile / flag 五個平行陣列)。
**交叉驗證**:monx21 中 tile=24 的 8 個實體座標,與 mapx21 上 8 個 id-24 格完全吻合 → 「空城 → 活城」。
**NPC→對話行對應**:monxNN 第 6 陣列 `0xA0+i`(`&0x80`=可交談,行索引 `(dlg&0x7f)-1`);
monx21 的 4 個交談 NPC 正好對上 tlkx21 的 4 行對話。

### 4. 存檔 `player` — BCD 屬性(DOSBox 實機差分驗證)
用 headless DOSBox 自動建兩隻角色、逐 byte diff,並與建角畫面交叉比對,確認:
名字(ASCII)、性別(`'M'` / `'F'`)、職業 / 種族(0-indexed)、六屬性(**BCD**,順序 STR/AGI/STA/CHA/WIS/INT)。
屬性存的是「套用 race/class 加成後」的值。**HP / 食物 / 經驗 / 黃金**為 2-byte BCD。
回歸測試用 [`tests/fixtures/`](../tests/fixtures/) 兩份實機存檔斷言。

![繁中角色資料表](screenshots/char_sheet.png)
> `u2_sheet`:載入實機建角存檔 → `u2_save` 解析 → 繁中角色資料表(姓名 / 性別 / 種族 / 職業 + 六屬性 BCD)。

---

## oracle 機制分析

`oracle/` 經字串錨定導航後,抽出載具 / 地牢 / 戰鬥演算法。完整見 [`docs/ORACLE_MECHANICS.md`](ORACLE_MECHANICS.md)。

- **RNG**:標準 LCG `seed = seed*0x343fd + 0x269ec3`。
- **3D 線框地牢**:first-person 風格,沿朝向掃深度畫梯形側牆(`SDL_RenderDrawLine` 重寫);
  真實地牢檔 16 層,`level*256+Y*16+X`,樓梯 `&0x10` / `&0x20`。
- **戰鬥命中 / 傷害 / 掉落公式**、狀態效果(麻痺 / 睡眠)計時器 offset。
- **太空 hyperwarp** 以三元座標(XENO / YAKO / ZABO)定位行星——撞太陽座標即死;X 行星 = (9,9,9)。
- **時間之門**:每時代多目的地拓樸(各時代 4 門通往不同時代),月相選門;精確座標表
  `DAT_0043e260/e261` 不在 Ghidra dump,目前以攻略方位映 64×64 象限做近似。
- **米娜克斯對決**:力場 1000 傷、戒指免疫、ENILNO 殺;本案實作為 NE↔SW 位移 + 火球追擊。

---

## FM Towns 考古

1990 年的 **FM Towns 日版**(彩色重畫、CD 配樂)是本專案的素材富礦。從《Ultima Trilogy》CD 映像
與執行檔 `enchant.exp` 中挖出大量資產:

### 美術:`.TIF`(FillOrder=2)
GRAPH 目錄的 `.TIF` 是 **FillOrder=2(LSB-first 位元反轉)** 的 TIFF,且 header 謊報尺寸、
資料自 offset 512 起、sprite 用偶數 nibble、8 色調色盤。解碼修正後抽出真實 palette + tile +
怪物 sprite 圖鑑。見 [`docs/FMTOWNS_TILESET.md`](FMTOWNS_TILESET.md) · [`docs/MONSTERS.md`](MONSTERS.md)。

> ⚠️ Ultima II 實際只有 CGA(原版)+ EGA(U2 Upgrade);FM Towns(1990)為彩色重畫;**無 VGA/NES/PCE**。

### 音樂與音效:CD `/SOUND/` + 執行檔 RE
- **CDDA**:CD 上 7 軌音訊為 U1/U2/U3 **合集共用**;經 Ghidra 反組譯 `enchant.exp` + 比對影片確認
  U2 只用 **track05(intro)** 與 **track07(結局)**;遊玩中是 FM 合成(EUP),不放 CDDA。
- **CDDA 觸發機制(RE 成果)**:`enchant.exp` 為 MetaWare High C / RUN386 protected-mode binary;
  CDDA 經 **INT 93h 反射(RUN386)** 而非字面 `int 0x93` 指令——播放鏈 `場景 id → 表 → bgm_idx →
  MSF 表 → CD 音軌`,觸發點為標題函式(引用 `U2TITLE*.TIF`)與結局函式(日文「ミナクスは倒れた」)。
- **遊玩音樂 = EUP**:CD `/SOUND/*.EUP`(FM Towns EUPHONY 序列,檔名即場景:MAP / TOWN / DUNGEON /
  OSIRO)+ `ULTIMA.FMB` 音色庫。可用 EUPPlayer(Hayasaka)離線算繪(目前對 U2 純 FM 檔有靜音問題,
  詳見開發紀錄)。
- **音效 = `.SND`**:RF5C68 PCM(8-bit sign-magnitude),已寫 [`tools/decode_fmtowns_snd.py`](../tools/decode_fmtowns_snd.py)
  解成 WAV,接進引擎(攻擊 / 魔法 / 開門 / 撞牆 / 穿門 / 怪物)。

---

## 中文化管線

### 兩個翻譯來源
| 來源 | 數量 | 已譯 | 翻譯表 |
|---|---|---|---|
| Alderson exe 內嵌 UI 字串 | 392 條(可譯 369) | **369/369** | [`translations/exe_translatable_strings.tsv`](../translations/exe_translatable_strings.tsv) |
| DOS `tlkx` NPC 對話 | 108 行 | **108/108** | [`translations/talk_dialogue.tsv`](../translations/talk_dialogue.tsv) |

原則:**不寫回原始檔**,以外部 UTF-8 覆蓋層、`(來源, key)` 為索引,載入時覆蓋原文(查無譯文則 fallback 原文)。
引擎硬編訊息另有 [`translations/ui_strings.tsv`](../translations/ui_strings.tsv)(繁中 / EN / 日),`F4` 切換。
劇情對照見 [`docs/DIALOGUE.md`](DIALOGUE.md)。

### 雙層渲染 + 內外解析度解耦(見 [ADR 0001](adr/0001-rendering-resolution-cjk.md))
- **像素圖層**:原版 16×16 tile 整數倍放大(nearest 預設)。
- **文字圖層**:CJK glyph 在內部高解析度**原生繪製**,永不被縮放 → 恆銳利。
- **內部 render** 320×200×N(N 決定 glyph 大小,推薦 3× = 960×600 / CJK 24×24);視窗解析度獨立。

---

## 建置與執行

> 全程 Docker。需自備合法 Ultima II 資料檔(本 repo 不含)。

```bash
git clone https://github.com/wicanr2/u2-cht.git && cd u2-cht
```

### 三平台打包
```bash
# u2cht-pkg 容器:產 Linux AppImage / Windows zip / macOS 原始碼包
docker run --rm -v "$PWD":/work -v <資料夾>:/data:ro -v /tmp/out:/out \
  u2cht-pkg bash -c 'cd /work && bash build_release.sh'
```
macOS 原生 `.app` 由 GitHub Actions 在 macOS runner 上編譯,見 [`.github/workflows/build-mac.yml`](../.github/workflows/build-mac.yml)。

### 開發 / 驗證
```bash
# headless 腳本驗證(逐步存 PNG)
./build_game.sh ../dos-original/ultima2/mapx20 EEESSSWWWNNNN build/game_step_
# 地牢 3D 線框
build/u2_dungeon_demo ../dos-original/ultima2/mapx15 <font.ttf> build/dungeon.png
# 繁中角色資料表(從存檔解析)
build/u2_sheet tests/fixtures/player_sample_abcd <font.ttf> build/char_sheet.png
# data 層自動化測試
./run_tests.sh ../dos-original/ultima2
# 破關回歸 + 城鎮正典(headless,需 U2_TILESET)
bash tests/regression_winnable.sh . ../dos-original/ultima2
bash tests/test_town_canon.sh . ../dos-original/ultima2
```

---

## 完整進度

> 路線圖見 [`docs/ROADMAP_REMAKE.md`](ROADMAP_REMAKE.md)(M1 多世界地圖 → M2 載具/太空 → M3 城鎮經濟
> → M4 戰鬥/地牢 → M5 任務/結局 → M6 美術 → M7 音樂 → M8 破關回歸)。

### ✅ 已完成
| 段落 | 證據 |
|---|---|
| 二進位分流 + Ghidra 反編 oracle(~46k 行) | [`oracle/`](../oracle/) |
| oracle 機制分析、資料格式破解(map / monx / tlkx / player) | [`ORACLE_MECHANICS.md`](ORACLE_MECHANICS.md) · [`DATA_FORMATS.md`](DATA_FORMATS.md) |
| 完整繁中化(UI 369/369 + NPC 對話 108/108)+ 三語切換 | [`translations/`](../translations/) · [`src/u2_i18n.h`](../src/u2_i18n.h) |
| 整合主迴圈狀態機(地面↔城鎮↔地牢↔角色表↔太空) | [`src/game_main.c`](../src/game_main.c) |
| 城鎮 NPC 對話、地面隨機遭遇 / 戰鬥、城鎮經濟(商店) | [`src/game_main.c`](../src/game_main.c) |
| 地牢 3D 線框 + 多層換層 + 法術 + 寶箱陷阱 | [`src/u2_dungeon.c`](../src/u2_dungeon.c) |
| 時間旅行(時間之門 + 5 時代,正典多目的地拓樸) | [`ORACLE_MECHANICS.md`](ORACLE_MECHANICS.md) |
| 載具系統(馬/船/飛機/火箭)、星際旅行(9 行星 + X 行星) | [`MAP_REGISTRY.md`](MAP_REGISTRY.md) |
| 任務鏈正典化、怪物專屬攻擊、米娜克斯 NE↔SW 位移對決 | [`src/game_main.c`](../src/game_main.c) |
| FM Towns 彩色美術 / CD 音樂(intro/結局)/ 原版音效 | [`FMTOWNS_TILESET.md`](FMTOWNS_TILESET.md) · [`tools/decode_fmtowns_snd.py`](../tools/decode_fmtowns_snd.py) |
| **破關回歸 + 城鎮正典 headless 測試** | [`tests/`](../tests/) |

### ⏳ 進行中 / 已知差異
| 項目 | 現況 |
|---|---|
| 遊玩中 BGM(EUP→可播放音訊) | EUPPlayer 算繪對 U2 純 FM 檔靜音,待解 / 或改 emulator 錄音 |
| 時間之門精確座標 | 原版 `.data` 座標表未取得,目前方位象限近似 |
| player 部分 stat offset | 主要欄位已解,其餘待更多真實存檔樣本 |
| 行為對照 oracle(機率 / 戰鬥公式) | 公式已抽出,逐項對照中 |

---

## 專案結構

```
u2-cht/
  README.md                 # 玩家導向介紹
  docs/ENGINEERING.md        # 本文(工程技術)
  PLAN.md / CONTEXT.md / LICENSE
  CMakeLists.txt / build_*.sh / run_tests.sh / build_release.sh
  .github/workflows/build-mac.yml   # macOS runner 原生打包
  docker/                   # Docker 建置環境(SDL2 + CJK 字型;含 RE / 模擬器 Dockerfile)
  oracle/                   # Ghidra 反編 oracle(行為參考,~46k 行)
  docs/                     # DATA_FORMATS / ORACLE_MECHANICS / FMTOWNS_TILESET / MONSTERS / adr …
    screenshots/ demo/
  src/                      # 乾淨重寫引擎(垂直切片)
  tileset/                  # U2 Upgrade CGATILES / EGATILES(研究用途)
  translations/             # 翻譯表(原文 + zh_hant + 多語)
  tools/                    # 抽取 / 反編 / 渲染 / DOSBox 建角 / FM Towns 音效解碼 script
  tests/                    # data 層測試 + 破關回歸 + 城鎮正典 + 實機存檔 fixtures
```

> 原版 binary / 資料檔皆 **gitignore**(版權物,玩家自備);oracle 與 tileset 經專案裁示收錄供研究保存。

---

## 致謝與參考

- **Richard Garriott (Lord British) / Sierra On-Line** — Ultima II 原作(1982)。
- **John Alderson** — Windows Native Ultima II 移植版(2000),本案行為 oracle 來源。
- **mcmagi** / **TheAlmightyGuru** / **shikadi ModdingWiki** — 資料格式逆向與保存。
- **Tomoaki Hayasaka** — EUPPlayer / EUPHONY(FM Towns 音樂)格式與算繪工具。
- **姊妹專案** [u3-cht](https://github.com/wicanr2/u3-cht) / [u6-cht](https://github.com/wicanr2/u6-cht) — CJK pipeline 與移植經驗。

**外部參考**
- [Ultima II Map Format — shikadi ModdingWiki](https://moddingwiki.shikadi.net/wiki/Ultima_II_Map_Format)
- [Ultima II Dungeon Format](https://moddingwiki.shikadi.net/wiki/Ultima_II_Dungeon_Format)
- [Dino's Ultima Page — U2](https://gigi.nullneuron.net/ultima/u2.php)
- [Ultima Codex wiki](https://wiki.ultimacodex.com/wiki/Ultima_II:_The_Revenge_of_the_Enchantress)
- [EUPHONY (FM Towns) — VGMRips wiki](https://vgmrips.net/wiki/Euphony)
