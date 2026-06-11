# 邁向完整 U2 重製 — 路線圖(Roadmap to a Complete Remake)

> 建立:2026-06-11 · 接續現有試玩版(demo)。
> 舊 [`PLAN.md`](../PLAN.md) 是「逆向 + 中文化引擎重寫」的策略計畫(歷史);本檔是
> 從**現況 demo → 可從建角破關到結局的完整重製**的前瞻路線。
> 所有里程碑以 oracle(`docs/ORACLE_MECHANICS.md`、`oracle_string_map.txt`、
> `decompile/out/ultima2_decompiled.c`)為行為真值,不憑空設計。

---

## 0. 「完整」的定義(驗收總目標)

能用**新建的角色**,經由**時間旅行 + 星際旅行**蒐齊任務道具,於 **Legends 時代座標 9-9-9**
擊敗女巫 **Minax**,看到結局序列:

> MINAX IS DEAD! / ALL HER WORKS SHALL DIE / YOU HAVE SAVED THE UNIVERSE / …SEEK NOW TO CONQUER WICKED EXODUS(指向 U3)。

達成此「完整破關鏈」即視為**完整重製 v1.0**。

---

## 1. 現況(demo,已完成)

世界單圖行走 + 隨機遭遇戰(lite)、進城 + NPC 對話(繁中)、地牢 3D 線框 + 換層、
建角 / 開場選單 / 可寫存檔、載具(船,示範)、6 套 tileset(含 FM Towns 地形 + 主角)、
原版標題、UI 369/369 + 對話 108/108 繁中、AppImage + Windows 打包、headless + 互動驗證。

**評分**:作為「中文化 + 引擎重寫地基」約 8/10;作為「可破關完整遊戲」約 5/10 —— 缺核心玩法鏈。

---

## 2. 完整 U2 的系統地圖(oracle-grounded)

### 2.1 世界結構
- **地球 5 個時代**(`FUN_0040c270` 招牌):`LEGENDS` / `9,000,000 B.C.`(Pangea)/ `1423 B.C.` / `1990 A.D.` / `2112 A.D.`(Aftermath)。經**時間之門 (Time Door)** 依開閉時刻切換,座標保留。
- **太陽系 9 行星**(`FUN_0040e370` 軌道):EARTH / MERCURY / VENUS / MARS / JUPITER / SATURN / URANUS / NEPTUNE / PLUTO(+ `YOU HIT THE SUN`)。經**火箭**進入深空 / 軌道 / 降落。
- 行星座標表(oracle):Mercury 5,4,5 · Venus 3,3,4 · Mars 6,2,3 · Jupiter 1,3,4 · Saturn 2,8,5 · Uranus 9,4,6 · Neptune 4,0,5 · Pluto 0,1,4 · `9,9,9` 特殊。

### 2.2 載具鏈(`this+0x7390`:0 步行 / 1 馬 / 2 船 / 3 飛機 / 4 火箭)
| 載具 | 門檻道具(offset) | oracle |
|---|---|---|
| HORSE | — | BOARD HORSE |
| FRIGATE(船) | BLUE TASSLE(`0x154`) | 否則 `THE CREW…WILL NOT LET YOU BOARD` |
| PLANE(飛機) | BRASS BUTTON(`0x150`) | 否則 `…MISSING A BRASS BUTTON` |
| ROCKET(火箭) | ANKH(`0x140`)登艦 + TRI-LITHIUM(`0x160`)燃料 | 否則 `YOU MUST HAVE AN ANKH` / `SHIP INCAPABLE OF LAUNCH` |

### 2.3 道具(`FUN_00405c30` + 任務道具)
STAFF · BOOTS · CLOAK · RED GEM · SKULL KEY · GREEN GEM · BRASS BUTTON · BLUE TASSLE ·
STRANGE COIN · GREEN IDOL · TRI-LITHIUM · **RING** · **ENILNO(Quicksword)** · **ANKH**。
- 戰鬥防護(`FUN_0040c610`):BOOTS 防腿麻痺 · CLOAK 防手麻痺 · IDOL 防睡眠。
- **RING**:破 Minax 力場(`FIELD_CAUSES_1000_DAMAGE` / `RING_PROTECTS_FROM_FIELD`)。

### 2.4 任務主鏈
1. 累積屬性 / 金 / 裝備(城鎮商店、King 獻金、Lord British)。
2. 取得載具道具 → 升級移動能力(船→飛機→火箭)。
3. 飛抵**行星 X / 找到 Father Antos** → `EARN THE RING` → 取得 **RING**。
4. 取得 **ENILNO** Quicksword(`ENILNO IS YOURS`)。
5. 進 **Legends 時代、座標 9-9-9** → Minax(type `0x10`,range 3;`DIE FOOL` 固定 100 傷)。
6. RING 破力場 + Quicksword 擊殺 `@` 標記 Minax → `FUN_0040eb60` 勝利結局。

### 2.5 城鎮 / 地牢服務
- 城鎮:武器店(READY 1-9)、防具店(WEAR cloth→power)、食物、TRANSACT / OFFER GOLD、
  King 獻金提升、酒館線索(文本已具)、guards(稅 / ID / KEY)、STEAL / UNLOCK / VIEW / YELL。
- 地牢:16 層 tile 地牢、法術(LIGHT / PASSWALL / MAGIC_MISSILE / BLINK)、寶箱(第 16 層 TRI-LITHIUM)、
  陷阱、火把、gremlin 偷食 / thief 偷物。

---

## 3. Gap 分析(demo → 完整)

| 子系統 | demo 現況 | 完整目標 |
|---|---|---|
| 世界 / 地圖 | 單圖 mapx20 | 5 時代 + 9 行星 + 全城鎮/塔/城堡/地牢,登記表 + 切換 |
| 時間旅行 | 無 | 時間之門 + 時刻開閉 + 跨時代座標保留 |
| 星際旅行 | 無 | 火箭 + 深空/軌道/降落 + HYPERWARP |
| 載具 | 船(示範) | 馬/船/飛機/火箭 + 門檻道具鏈 |
| 城鎮服務 | 進城 + 對話 | 商店/經濟/King/guards/STEAL/UNLOCK… |
| 戰鬥 | lite(撞擊) | 對齊 oracle 公式 + 狀態效果 + 道具防護 + 法術 |
| 地牢 | 線框 + 換層 | tile 實體 + 怪物 + 寶箱 + 陷阱 + 法術 |
| 道具 / 旗標 | 無 | 14+ 道具 + 持有旗標(offset 對齊) |
| 任務 / 結局 | 無 | 完整主鏈 + Minax 戰 + 勝利序列 |
| 美術(FM Towns) | 地形 + 主角 | 船/怪物/城鎮/城堡/地牢 sprite |
| 音樂 / 音效 | 無 | FM Towns 配樂 + SFX |

---

## 4. 分階段路線(里程碑)

> 排序依**相依性**:M1 世界骨架是其餘一切的地基。每階段附 oracle 錨點 + 驗收(優先 headless 腳本)。

### M1 — 多世界地圖骨架 + 場景轉換 + 時間之門 ★地基
- **mapxNN 角色登記表**:逐一辨識 mapx00–93 = 哪個時代/行星/城鎮/城堡/塔/地牢(oracle + Codex 對齊),寫成 `docs/MAP_REGISTRY.md` + 引擎讀的 JSON/表。
- 統一 ENTER(village/town/tower/castle/dungeon)+ 地圖堆疊(world↔location↔dungeon)。
- 時間之門:踏入依「時刻」切到對應時代地圖,座標保留。
- **驗收**:headless 腳本能在 5 時代 + 行星地圖間移動並正確載入。

### M2 — 載具系統(完整鏈)+ 太空飛行
- 馬/船/飛機/火箭 + 門檻道具(`0x140/0x150/0x154/0x160`);拒絕訊息對齊 oracle。
- 太空子系統:深空/行星軌道/LAND/HYPERWARP(`FUN_00409320`/`FUN_0040e370`)。
- **驗收**:取得道具→登艦→飛抵行星→降落,headless 可重現。

### M3 — 城鎮服務 / 經濟 / 互動指令
- 商店(武器 READY 1-9 / 防具 WEAR / 食物)、TRANSACT / OFFER GOLD、King 獻金升屬、guards(稅/ID/KEY)、STEAL/UNLOCK/VIEW/YELL。
- **驗收**:買賣改變金/裝備/屬性;解鎖/被攔/獻金升階皆可腳本驗證。

### M4 — 戰鬥深化 + 地牢實體化
- 命中/傷害/EXP 對齊 oracle;狀態效果(麻痺/睡眠/偷食/偷物)+ BOOTS/CLOAK/IDOL 防護。
- 地牢改 tile 實體(取代線框):怪物、寶箱(第 16 層 TRI-LITHIUM)、陷阱、火把、法術(LIGHT/PASSWALL/MAGIC_MISSILE/BLINK)。
- **驗收**:地牢探索取得三鋰;戰鬥數值對 oracle;法術可用。

### M5 — 道具 / 任務旗標 / 結局
- 道具系統 + 持有旗標 offset 對齊;RING 破力場(1000 傷免疫)、ENILNO Quicksword。
- 任務鏈:Antos→RING、Quicksword、Minax 戰(9-9-9 Legends,@標記,100 固定傷)→ 勝利結局序列(`FUN_0040eb60`)。
- **驗收**:headless 腳本化**完整破關**(建角→…→Minax→結局)。← 完整重製 v1.0 達成點

### M6 — 美術完整化(FM Towns)
- 船/怪物/城鎮/城堡/地牢 sprite 從 Tsugaru 乾淨截圖 rip(沿用地形/主角流程);避免 raw atlas 雜訊。
- 時代/行星 tile 變體。

### M7 — 音效 / 音樂
- FM Towns 原創配樂(CD 抽取轉 ogg)+ SFX,SDL_mixer。

### M8 — 平衡 / 打磨 / 完整破關回歸
- headless 全破關腳本納入 CI 當回歸;平衡、邊界、存檔相容。

---

## 5. 資料 / 美術 / 音樂需求

- **地圖角色辨識**(M1 前置,最高優先):mapx00–93 全部對上時代/行星/地點。
- 道具↔offset 精確映射(`0x140/0x150/0x154/0x160` 已知,其餘待交叉驗證 `FUN_00405c30`)。
- FM Towns 船/怪物/場景 sprite(M6):需模擬器航行/戰鬥/城鎮乾淨截圖。
- FM Towns 配樂(M7):CD `.mov`/CDDA → ogg。

## 6. 風險

- **地圖角色不明**:mapxNN→時代/地點需逐一辨識;錯了會連鎖。→ M1 先做登記表並交叉驗證。
- **時間之門時刻表**:開閉排程細節需 oracle 深挖。
- **MFC oracle 雜訊**:只取演算法不照抄;offset↔道具映射需交叉驗證。
- **散布灰色地帶**:引擎(MIT)與資料/美術/音樂分離,沿用現行政策。

## 7. 驗收(完整重製 v1.0)

決定性 **headless 全破關腳本**:固定 seed,從建角依序走完任務鏈到結局字串,
逐畫面截圖比對。能穩定產出結局 = v1.0。

---

## 立即可做的下一步(M1 起手)

1. 解析 mapx00–93 + monx/tlk,**產出 `docs/MAP_REGISTRY.md`**(每圖:尺寸、tile 直方圖、landmark、推定角色),交叉 oracle/Codex。
2. 引擎加入**多地圖載入 + 地圖堆疊轉換**(world↔location↔dungeon 統一 ENTER/EXIT)。
3. 把現有「單一 town landmark→mapx21」擴成「登記表驅動」的任意地點進入。
