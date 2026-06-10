# README 規劃草案 — u2-cht

> 這是 README.md 的**規劃草案(總編輯版)**,尚未覆蓋現有 `README.md`。
> 所有內容皆基於 repo 既有事實(docs / git log / 截圖 / 程式碼),未杜撰。
> 史實段落已標明「常識性史實」與「本專案推測」之分界。
>
> 使用方式:審閱後可整段或分段挑選,替換或增補現有 `README.md`。

---

## 一、章節大綱(建議順序)

| # | 章節 | 目的 | 主要素材來源 |
|---|---|---|---|
| 0 | 標題 + 一句話定位 | 開門見山:中文化 + 乾淨重寫引擎 | 現有 README L1–4 |
| 1 | 徽章列(badges) | 授權 / 語言 / 階段 / 姊妹專案 | placeholder |
| 2 | 遊戲截圖展示區 | 第一眼證據:中文已上畫面 | `docs/screenshots/` |
| 3 | Ultima II 是什麼(遊戲歷史與背景) | 給不熟 CRPG 的讀者脈絡;考證史實 | 外部常識 + CONTEXT.md 譯名表 |
| 4 | 為什麼做這個專案 | 動機:CRPG 史保存 + 繁中化 | PLAN.md / LICENSE / oracle/README |
| 5 | 技術架構(反編 oracle + 乾淨重寫 + 格式破解) | 核心方法論 | PLAN.md / CONTEXT.md / ORACLE_MECHANICS |
| 6 | 已完成 vs 進行中(對照 git) | 誠實進度 | git log + 現有 README 路線圖 |
| 7 | 資料格式破解亮點 | 技術看點:map÷4 / tlkx high-bit / monxNN / player BCD | DATA_FORMATS.md |
| 8 | 中文化管線 | 兩翻譯來源 + 覆蓋層 + CJK 雙層渲染 | CONTEXT.md / ADR 0001 |
| 9 | 建置與執行(Docker) | 重現步驟 | build_*.sh / run_tests.sh / docker/ |
| 10 | 專案結構樹 | 導航 | 實際目錄 |
| 11 | 授權與免責 | 引擎與資料分離 | LICENSE / .gitignore |
| 12 | 致謝與參考連結 | 出處 | 現有 README + DATA_FORMATS 參考 |

---

## 二、引用的既有截圖(可直接嵌入)

| 檔案 | 內容 | 建議 caption |
|---|---|---|
| `docs/screenshots/poc_map_cjk.png` | PoC 主畫面:上方地圖 viewport + 底部中文訊息列 + 右側中文狀態欄 | 「垂直切片 PoC:真實地圖 tile + monxNN 實體層 + 端到端中文(NPC 對話與狀態欄皆走翻譯覆蓋層)」 |
| `docs/screenshots/movement_verify.png` | 5 幀移動序列(東4/南4/西8),含「無法移動!」海洋碰撞 | 「隊伍移動 headless 驗證:玩家置中、鏡頭跟隨捲動、海洋碰撞阻擋」 |
| `docs/screenshots/tileset_egatiles.png` | U2 Upgrade EGATILES 65-tile 對照表(彩色) | 「ground-truth tileset(EGA 65 tile):水/森林/城堡/船/馬/A–Z 招牌字…」 |
| `docs/screenshots/tileset_cgatiles.png` | 同上 CGA 版 | 「CGA 4 色版 tileset 對照(同 65 tile)」 |
| `docs/screenshots/terrain_in_context.png` | mapx20(地球)整張地圖以 EGATILES 渲染 | 「地球大地圖 mapx20:海/森林/山以正確 tileset 渲染,驗證 tile÷4 解碼」 |

> 主截圖建議用 `poc_map_cjk.png` 置頂於「截圖展示區」;`tileset_egatiles.png` 與 `terrain_in_context.png` 放「資料格式破解亮點」章節旁佐證。

---

## 三、README 內容草稿(可直接取用)

以下為各章節實際要放的內容草稿。標題採中英並列。

---

### 0 / 標題

```markdown
# Ultima II: Revenge of the Enchantress — 繁體中文化專案 (u2-cht)
# 創世紀 II:女巫的復仇 — 繁體中文化 + SDL2 引擎重建

> 把 1983 年的《創世紀 II:女巫的復仇》以**乾淨重寫的跨平台 C / SDL2 引擎**重建,並完整繁體中文化。
> 方法:反編原版當行為 oracle、破解原版資料格式、手寫可公開維護的引擎。
> 系列姊妹作:[u3-cht](https://github.com/wicanr2/u3-cht)(Ultima III)、[u6-cht](https://github.com/wicanr2/u6-cht)(Ultima VI)。
```

---

### 1 / 徽章列(badges,placeholder)

> 以下為 shields.io placeholder,實際 URL / 倉庫名待主 agent 填。

```markdown
![License](https://img.shields.io/badge/code-MIT-blue)
![Engine](https://img.shields.io/badge/engine-SDL2%20clean%20rewrite-green)
![Lang](https://img.shields.io/badge/lang-C-orange)
![i18n](https://img.shields.io/badge/i18n-繁體中文-red)
![Phase](https://img.shields.io/badge/phase-逆向%20%2B%20PoC%20完成-yellow)
![Sister](https://img.shields.io/badge/series-u3--cht%20%7C%20u6--cht-lightgrey)
```

---

### 2 / 遊戲截圖展示區(Screenshots)

```markdown
## 截圖 / Screenshots

### 中文化垂直切片 PoC
![PoC 主畫面](docs/screenshots/poc_map_cjk.png)
> 上方地圖 viewport(真實 tile + monxNN 實體層)、底部中文 NPC 對話、右側中文狀態欄。
> 對話與狀態標籤皆**不是硬編**:引擎讀原始資料 → 翻譯覆蓋層 → CJK 原生繪製。

### 隊伍移動驗證(headless 截圖序列)
![移動驗證](docs/screenshots/movement_verify.png)
> 玩家恆置中,地圖在腳下捲動;最後兩幀的「無法移動!」展示海洋碰撞阻擋。

### Ground-truth tileset(U2 Upgrade EGATILES)
![EGATILES](docs/screenshots/tileset_egatiles.png)
> 65 個 16×16 tile:水 / 森林 / 山 / 城堡 / 船 / 馬 / 招牌 A–Z…(art 由玩家自備,本 repo 僅供研究對照)。
```

> TODO 截圖見本檔第四節。

---

### 3 / Ultima II 是什麼(遊戲歷史與背景)

```markdown
## 關於 Ultima II / About Ultima II

> 以下標 **[史實]** 為公開可考的常識性史實;標 **[本專案]** 為本 repo 的逆向發現或推測。

**《Ultima II: The Revenge of the Enchantress》(創世紀 II:女巫的復仇)** 是 Richard Garriott
(遊戲中化身 **Lord British / 不列顛王**)設計的角色扮演遊戲,**1982 年由 Sierra On-Line 發行**,
是 Ultima(創世紀)系列的第二作。 **[史實]**

- **劇情設定 [史實]**:反派是女巫 **Minax(米娜克斯)**——前作 Ultima I 大魔王 **Mondain(蒙丹)**
  的徒弟兼愛人。Minax 為復仇而操弄時間,玩家(**The Stranger / 異鄉人**)必須穿越
  **Time Doors(時光之門)** 在不同時代與星球間旅行,最終於傳說時代的城堡
  **Shadowguard(影域堡)** 擊敗她。
- **時空與星際旅行 [史實]**:本作以「時空旅行」為招牌設定,玩家可造訪太陽系各行星;
  地球場景橫跨史前、古代,以及 **2111 年核戰後的「浩劫餘生(Aftermath)」** 等紀元。
- **系列與 CRPG 史地位 [史實]**:Ultima II 是早期家用電腦 CRPG 的代表作之一,
  奠定了 Ultima 系列「開放世界 + 圖塊地圖 + NPC 對話」的雛形;系列後續(III–IX)
  成為西方 CRPG 的奠基者之一。

**本專案考證到的細節(逆向發現)**:

- **[本專案]** 太空 hyperwarp 以三元座標(XENO/YAKO/ZABO)定位行星——
  例如 `6,6,6` = 地球、`4,4,4` = 太陽(撞上即死)。座標查表見
  [`docs/ORACLE_MECHANICS.md`](docs/ORACLE_MECHANICS.md)。
- **[本專案]** 載具(馬 / 船 / 飛機 / 火箭)各有道具門檻:火箭需 ANKH、飛機需 SKULL KEY、
  船需 BLUE TASSLE——皆對應遊戲內 NPC 提示語(行為 oracle 推測,offset 待最終定稿)。
- **[本專案]** 唯一能殺死 Minax 的武器是 **Quicksword Enilno(迅捷之劍 Enilno)**(Enilno = online 反拼)。

> 譯名對照(對齊 u6-cht / u3-cht 系列一致性)見 [`CONTEXT.md`](CONTEXT.md)。
```

> 註(給審閱者,不入 README):發行商一向有「Sierra On-Line 發行 / California Pacific 發行 Ultima I」的混淆。
> Ultima II 一般記載為 1982 年 Sierra On-Line 發行;若主 agent 要更嚴謹,可改寫為「1982 年發行(原始發行商 Sierra On-Line)」並保留 [史實] 標記,或刪去發行商只留年份。

---

### 4 / 為什麼做這個專案(Motivation)

```markdown
## 為什麼做這個 / Why

1. **CRPG 歷史保存**:Ultima II(1982)年代久遠、公開逆向資料多。把資料格式、機制公式、
   反編 oracle 系統化文件化,本身就是一種保存。
2. **繁體中文化**:系列前作 [u3-cht](https://github.com/wicanr2/u3-cht) /
   [u6-cht](https://github.com/wicanr2/u6-cht) 已驗證「乾淨重寫 + CJK pipeline」可行,
   本作沿用同一套經驗,讓中文玩家能以母語體驗這款早期 CRPG。
3. **可維護的開源引擎**:直接反編商業 binary 得到的是不可維護的 `FUN_xxx`;
   本專案改採「反編只當行為參考、引擎手寫乾淨版」,產物可公開、可維護、好中文化。

> 非商業之保存與在地化研究;原版遊戲資料不散布,玩家須自備合法副本(見「授權與免責」)。
```

---

### 5 / 技術架構(Architecture)

```markdown
## 技術架構 / Architecture

### 策略:反編當 oracle,乾淨重寫

手上標的是 **John Alderson《Windows Native Ultima II》v1.01 (2000)**——一個 stripped 的
**MFC / GDI** Windows 移植 exe。直接反編成可用引擎是最硬的路(無型別 `FUN_xxx`、纏繞 MFC runtime)。
因此採與 u3-cht 相同的成功模式:

```
Ghidra 反編 C  ──(只當行為/演算法 oracle,不照抄 MFC 殼)──┐
破解 U2 原版資料格式 ───────────────────────────────────┼──▶ 手寫乾淨 SDL2 C 引擎
原版資料檔(玩家自備)─────────────────────────────────┘     (可公開、可維護、好中文化)
```

| 角色 | 內容 | 公開? |
|---|---|---|
| **行為 oracle** | `oracle/ultima2_decompiled.c`(1190 函式、~46k 行)+ 函式索引 | ✅ 已收錄(CRPG 史保存) |
| **資料格式真值** | `docs/DATA_FORMATS.md`(map / monx / tlkx / player 等) | ✅ |
| **乾淨重寫** | `src/`(SDL2 取代 GDI、UTF-8 + CJK) | ✅ |
| 原版 binary / 資料檔 | Alderson exe + DOS 原版資料 | ❌ 玩家自備 |

### oracle 已產出的機制分析(節錄)

`oracle/ultima2_decompiled.c` 經字串錨定導航後,已抽出載具 / 地牢 / 戰鬥的演算法,例如:

- **RNG**:標準 LCG `seed = seed*0x343fd + 0x269ec3`。
- **3D 線框地牢**:first-person raycast 風格,沿朝向掃 8 格深度畫梯形側牆(可直接以 `SDL_RenderDrawLine` 重寫)。
- **戰鬥命中 / 傷害 / 掉落公式**、狀態效果(麻痺 / 睡眠)計時器 offset。

> 完整分析見 [`docs/ORACLE_MECHANICS.md`](docs/ORACLE_MECHANICS.md);信心以 `[確定] / [推測] / [未解]` 標示。

### 目標引擎(deep modules / 垂直切片)

按 feature 切而非按抽象層攤平,adapter 只放邊界(SDL2):
`platform/`(SDL window/event)、`render/`(framebuffer 取代 GDI)、`text/`(CJK 出口)、
`data/`(資料解析)、`features/{world,town,dungeon,space,combat,party}`。詳見 [`PLAN.md`](PLAN.md)。
```

---

### 6 / 已完成 vs 進行中(對照 git)

> 此表已對照實際 `git log` 與 repo 檔案,**未誇大**:戰鬥 / 地牢 / 載具僅有機制文件(oracle 分析),
> 尚未在 `src/` 引擎實作;互動視窗有 `game_main.c` 但需顯示器,CI 走 headless。

```markdown
## 進度 / Status

目前處於 **逆向取得 + 格式破解 + 垂直切片 PoC** 階段;完整引擎(world/town/dungeon/space/combat)逐步重寫中。

### ✅ 已完成

| 段落 | 證據 |
|---|---|
| 二進位分流 + Ghidra 反編 oracle(1190 函式) | `oracle/`、commit `5a4c20c` |
| oracle 機制分析(載具 / 地牢 / 戰鬥) | `docs/ORACLE_MECHANICS.md`、commit `f308794` |
| 資料格式破解(map / monx / tlkx / player) | `docs/DATA_FORMATS.md` |
| 抽可中文化字串(exe 392 條 + 對話 108 行) | `translations/` |
| 渲染 / CJK 解析度決策 | `docs/adr/0001-rendering-resolution-cjk.md` |
| 垂直切片 PoC(繪圖 / 資料 / 中文 一次驗證) | `docs/POC.md`、commit `363ebb5` |
| ground-truth tileset = U2 Upgrade EGATILES | `tileset/`、commit `001694c` |
| 翻譯 UI 字串 369/369 + NPC 對話 78/108 | `translations/`、commit `c8ada91` |
| 隊伍移動(置中 / 鏡頭跟隨 / 海洋碰撞,headless) | `docs/MOVEMENT.md`、commit `4aad753` |
| 端到端在地化(原始 tlkx → 覆蓋層 → CJK) | commit `abdafc7` |
| data 層自動化測試(15 斷言,headless) | `./run_tests.sh`、commit `d171baa` |
| player 存檔結構(BCD 屬性,DOSBox 實機差分驗證) | `docs/DATA_FORMATS.md`、commit `b8d46b0` |

### ⏳ 進行中 / 尚未實作

| 項目 | 現況 |
|---|---|
| 互動視窗(SDL 開窗 + 鍵盤) | `src/game_main.c` 已具雛形,需顯示器;CI 仍走 headless 截圖 |
| **戰鬥 / 地牢 3D / 載具** | **僅有 oracle 機制文件,尚未在 `src/` 引擎實作** |
| player 各 stat offset(HP/食物/黃金/裝備) | 名字 / 性別 / 種族 / 職業 / 六屬性已解;其餘待更多真實存檔樣本 |
| NPC / 怪物動態層(執行時改寫的 map 層) | monxNN 靜態實體已解;動態改寫層待解 |
| CJK 文字層升級(u6-cht 點陣字 atlas) | 目前用 SDL2_ttf + WQY;production 可換 BDF atlas |
| 行為對照 oracle(亂數 / 機率 / 戰鬥公式) | 公式已抽出,待引擎實作後逐項對照 |
| 截圖 vs Alderson exe(Wine)pass/fail loop | 規劃中 |
```

---

### 7 / 資料格式破解亮點(Data Format Highlights)

```markdown
## 資料格式破解亮點 / Reverse-Engineered Formats

完整見 [`docs/DATA_FORMATS.md`](docs/DATA_FORMATS.md)。四個值得一提的破解:

### 1. 地圖 `mapxNN` — tile id 要 ÷4
4224 byte = 64×66 cell,**無 header**,純 tile array。
存檔時 `tile id × 4`(低 2 bit 是 flag),讀取要 **÷4**。尾碼 0=行星總圖、1/2/3=城鎮、4/5=地牢。
![地球大地圖](docs/screenshots/terrain_in_context.png)

### 2. 對話 `tlkxNN` — high-bit ASCII
NPC 對話每 byte `OR 0x80`(ModdingWiki 誤稱 encrypted,實為 high-bit);
解碼 `byte & 0x7f`、`\r` 換行。15 檔共解出 **108 行對話**。

### 3. 實體層 `monxNN` — 32 格平行陣列
地圖只存地形 / 建築;NPC / 怪物座標另存於 `monxNN`(X / Y / status / tile / flag 五個平行陣列)。
**交叉驗證**:monx21 中 tile=24 的 8 個實體座標,與 mapx21 上 8 個 id-24 格完全吻合 → 「空城 → 活城」。

### 4. 存檔 `player` — BCD 屬性(DOSBox 實機差分驗證)
用 DOSBox 自動建兩隻角色、逐 byte diff,確認:名字(ASCII)、性別(`'M'`/`'F'`)、
職業 / 種族(0-indexed)、六屬性(**BCD**,順序 STR/AGI/STA/CHA/WIS/INT)。
屬性存的是「套用 race/class 加成後」的值(輸入值與建角畫面顯示值逐一吻合)。
```

---

### 8 / 中文化管線(Localization Pipeline)

```markdown
## 中文化管線 / Localization

### 兩個翻譯來源

| 來源 | 數量 | 已譯 | 翻譯表 |
|---|---|---|---|
| Alderson exe 內嵌 UI 字串 | 392 條(可譯 369) | **369/369** | `translations/exe_translatable_strings.tsv` |
| DOS `tlkx` NPC 對話 | 108 行 | **78/108**(餘為 buffer 殘片) | `translations/talk_dialogue.tsv` |

原則:**不寫回原始檔**,以外部 UTF-8 覆蓋層、`(來源, key)` 為索引,載入時覆蓋原文(查無譯文則 fallback 原文)。

### 雙層渲染 + 內外解析度解耦(見 [ADR 0001](docs/adr/0001-rendering-resolution-cjk.md))

- **像素圖層**:原版 16×16 tile 整數倍放大(nearest 預設)。
- **文字圖層**:CJK glyph 在內部高解析度**原生繪製**,永不被縮放 → 恆銳利。
- **內部 render** 320×200×N(N 決定 glyph 大小,推薦 3× = 960×600 / CJK 24×24);視窗解析度獨立。
```

---

### 9 / 建置與執行(Build & Run)

```markdown
## 建置與執行 / Build & Run

> 全程 Docker(`docker/Dockerfile`:SDL2 + SDL2_ttf + SDL2_image + fonts-wqy-zenhei)。
> 需自備合法 Ultima II 資料檔(本 repo 不含)。

### 垂直切片 PoC(headless,存 PNG)
```bash
./build_poc.sh ../dos-original/ultima2/mapx21 build/poc_out.png
```

### 隊伍移動 demo(headless 截圖序列)
```bash
./build_demo.sh ../dos-original/ultima2/mapx20 EEEESSSSWWWWWWWW build/move blue 31 33
# → build/move_00.png .. move_16.png
```

### 互動引擎切片(需顯示器)
```bash
# 方向鍵 / WASD 走路,Q / Esc 離開;另支援 --script 走 headless
# 見 src/game_main.c
```

### data 層自動化測試(headless)
```bash
./run_tests.sh ../dos-original/ultima2   # 15 斷言,Docker 內跑
```
```

---

### 10 / 專案結構樹(Project Layout)

```markdown
## 專案結構 / Layout

```
u2-cht/
  README.md / PLAN.md / CONTEXT.md / LICENSE
  CMakeLists.txt / build_poc.sh / build_demo.sh / run_tests.sh
  docker/                   # Docker 建置環境(SDL2 + CJK 字型)
  oracle/                   # Ghidra 反編 oracle(行為參考,非編譯標的)
    ultima2_decompiled.c    #   1190 函式、~46k 行
    functions_index.tsv / oracle_string_map.txt / ORACLE_MAP.md
  docs/
    DATA_FORMATS.md         # U2 資料格式真值
    ORACLE_MECHANICS.md     # 載具 / 地牢 / 戰鬥機制分析
    EXTRACTION_PROCESS.md   # 逆向取得 methodology
    MOVEMENT.md / POC.md    # 驗證紀錄
    adr/0001-...            # 渲染 / CJK 解析度決策
    screenshots/            # 截圖
  src/                      # 乾淨重寫引擎(垂直切片)
    u2_map / u2_mon / u2_talk / u2_strings / u2_tileset / u2_render / u2_text / u2_play / u2_save
    poc_main.c / demo_main.c / game_main.c
  tileset/                  # U2 Upgrade CGATILES / EGATILES(研究用途)
  translations/             # 兩張翻譯表(原文 + zh_hant)
  tools/                    # 抽取 / 反編 / 渲染 / DOSBox 建角 script
  tests/                    # data 層測試 + 實機存檔 fixtures
```
```

---

### 11 / 授權與免責(License)

```markdown
## 授權與免責 / License

「引擎與資料分離」:

- **本專案原創碼 / 工具 / 文件**:**MIT**(`src/` / `tools/` / `docs/`)。
- **反編 oracle**(`oracle/`):為 CRPG 歷史保存、逆向研究與在地化用途收錄;
  Ultima II(1982)年代久遠、公開逆向資料多,不主張著作權。
- **原版遊戲資料 / Alderson binary / U2 Upgrade art**:**不包含、不散布**,玩家須自備合法副本
  (`.gitignore` 已排除)。

非商業之保存與在地化研究。著作權人如有異議請聯繫移除。詳見 [`LICENSE`](LICENSE)。
```

---

### 12 / 致謝與參考(Credits & References)

```markdown
## 致謝 / Credits

- **Origin Systems / Richard Garriott (Lord British)** — Ultima II 原作(1982)。
- **John Alderson** — Windows Native Ultima II 移植版(2000),本案行為 oracle 來源。
- **mcmagi(Ultima Exodus / U2 Upgrade)** — ground-truth tileset 與保存專案。
- **TheAlmightyGuru / shikadi ModdingWiki** — U2 資料格式逆向文件。
- **姊妹專案** [u3-cht](https://github.com/wicanr2/u3-cht) / [u6-cht](https://github.com/wicanr2/u6-cht) — CJK pipeline 與移植經驗。

## 參考 / References
- [Ultima II Map Format — shikadi ModdingWiki](https://moddingwiki.shikadi.net/wiki/Ultima_II_Map_Format)
- [Ultima II Dungeon Format](https://moddingwiki.shikadi.net/wiki/Ultima_II_Dungeon_Format)
- [Ultima II 總覽 — ModdingWiki](https://moddingwiki.shikadi.net/wiki/Ultima_II:_Revenge_of_the_Enchantress)
- [Dino's Ultima Page — U2](https://gigi.nullneuron.net/ultima/u2.php)
- [Ultima Codex wiki](https://wiki.ultimacodex.com/wiki/Ultima_II:_The_Revenge_of_the_Enchantress)
```

---

## 四、TODO 截圖清單(建議補拍)

現有截圖已能撐起 README 的「靜態證據」,但若要讓首頁更有說服力,建議補拍:

| # | 截圖 | 為何要補 | 怎麼產 |
|---|---|---|---|
| 1 | **互動式遊玩 GIF / 截圖**(真開 SDL 視窗、鍵盤走路) | 現有 movement 是 headless 拼幀;一張真互動畫面更有「能玩」說服力 | `src/game_main.c` 互動模式,需顯示器;錄一段 GIF |
| 2 | **城鎮(town)畫面**(mapxN1/N2/N3 + monxNN 活 NPC) | 目前主截圖是地球大地圖;城鎮能展示「空城→活城」實體層 | 用 town map 跑 `build_poc.sh` |
| 3 | **狀態欄特寫 / 建角畫面中文化** | player BCD 已解;若有中文建角畫面可佐證存檔破解 | 引擎接 player 解析後產 |
| 4 | **3D 線框地牢**(oracle 已分析,引擎實作後) | 地牢是 U2 一大特色,目前只有文字機制 | 待 dungeon 模組實作 |
| 5 | **before/after 對照**(英文原版 vs 繁中) | 直觀展示中文化成果 | Alderson exe 在 Wine 截圖 vs PoC |

> 第 4、5 項需等對應功能實作,屬中後期;第 1、2 項現在就可補,優先度較高。

---

## 五、還缺哪些素材(需主 agent 補)

1. **正確的 GitHub 倉庫 URL / repo 名**:徽章與姊妹專案連結目前用 `wicanr2/u3-cht`、`wicanr2/u6-cht` placeholder,
   本 repo 自身的 GitHub URL(u2-cht?)需確認後填入徽章與 clone 指令。
2. **Ultima II 發行商史實的最終定調**:草稿採「1982 年 Sierra On-Line 發行」並標 [史實];
   若要更保守可改為只標年份。需主 agent 拍板用哪個版本(見第三節第 3 章後的審閱註)。
3. **互動 GIF / 城鎮截圖**(TODO 清單 #1、#2):需要實際跑引擎錄製,本草稿只能引用既有 6 張靜態圖。
4. **`docs/EXTRACTION_PROCESS.md` 是否要在 README 露出**:現有 README 有引用,本草稿放進結構樹但未展開;
   若要在「技術架構」加一張 Phase A–F 取得流程表,可從該檔搬。
5. **u6-cht 點陣字 pipeline 的實際工具名**:ADR 0001 提到 `build-big5-font-wqysharp.py`;
   若 README 要寫「字型升級」步驟,需確認該工具在本 repo 或姊妹 repo 的實際路徑。

---

## 六、誠實原則檢核(本草稿自我審查)

- ✅ 戰鬥 / 地牢 / 載具明確標「僅有 oracle 機制文件,尚未在引擎實作」,未宣稱可玩。
- ✅ 翻譯數字照實:UI 369/369、對話 78/108(非 108/108)。
- ✅ player 存檔只宣稱已解的欄位(name/sex/race/class/六屬性),HP/食物/黃金標「待補」。
- ✅ tileset 誠實標注 EXE @0x7C42 為 PoC、非正確 tileset,正解是 U2 Upgrade。
- ✅ 史實 / 推測分界明確標記;發行商爭議在審閱註點出。
- ✅ oracle 為「已收錄、公開」(對照 git log + .gitignore,非舊 README 所述 gitignored)。
```
