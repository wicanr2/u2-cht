# 邁向完整 U2 重製 — 路線圖(Roadmap to a Complete Remake)

> 建立:2026-06-11 · 接續現有試玩版(demo)。
> 舊 [`PLAN.md`](../PLAN.md) 是「逆向 + 中文化引擎重寫」的策略計畫(歷史);本檔是
> 從**現況 demo → 可從建角破關到結局的完整重製**的前瞻路線。
> 所有里程碑以 oracle(`docs/ORACLE_MECHANICS.md`、`oracle_string_map.txt`、
> `oracle/ultima2_decompiled.c`)為行為真值,不憑空設計。

---

## 0. 「完整」的定義(驗收總目標)

能用**新建的角色**,經由**時間旅行 + 星際旅行**蒐齊任務道具,於 **Legends 時代座標 9-9-9**
擊敗女巫 **Minax**,看到結局序列:

> MINAX IS DEAD! / ALL HER WORKS SHALL DIE / YOU HAVE SAVED THE UNIVERSE / …SEEK NOW TO CONQUER WICKED EXODUS(指向 U3)。

達成此「完整破關鏈」即視為**完整重製 v1.0**。

---

## 1. 現況(已完成 M1–M2 核心 + M5 任務鏈 + M6 法術)

- **世界**:5 時代 overworld(時間之門 + oracle 時代校正 + toroidal wrap)、世界圖環球。
- **地點**:登記表驅動進入(村莊/城鎮/城堡/塔/地牢,world-aware)、跨時代 enter/exit。
- **載具**:馬/船/飛機/火箭 + 門檻道具(oracle 0x7390);**星際旅行**(火箭發射→深空→HYPERWARP 9 行星→降落行星地表→往返)。
- **城鎮經濟**:商店(武防/食物/載具道具/習得法術)+ King 獻金;戰鬥用上武防;裝備/道具 sidecar 持久化。
- **法術(M6)**:9 法術(手冊 MAGIC SPELLS),職業限制(雙修/牧師/巫師),限地牢/塔施放,消耗制,地牢 HUD。
- **任務 / 結局(M5)**:可破關鏈(道具→傳說時代→米娜克斯對決→結局)+ 死亡復活;**破關回歸腳本**鎖端到端。
- **基礎**:建角(原版流程)/ 開場選單 / 可寫存檔、6 套 tileset(含 FM Towns)、原版標題、
  UI 369/369 + 對話 108/108 繁中、**F4 繁中↔English↔日本語(字典可擴充)**、AppImage + Windows 打包、headless + 互動驗證。

**評分(2026-06-12 更新)**:作為「中文化 + 引擎重寫地基」約 9/10;作為「可破關 demo」約 8/10;作為
「完整正典重製」約 **6/10**(本 session 補:對話蒐線索任務鏈、ENILNO 金幣閘門、ALAKAZAM、
YELL/STEAL 城鎮指令、地牢寶箱陷阱;確認地牢戰鬥已對齊 oracle、狀態效果/防護已實作。
仍缺大功能:戰術 overworld 戰鬥、完整守衛系統、UNLOCK/NEGATE、美術/音樂)。
> **誠實揭露(對齊外部審查;2026-06-12 更新)**:可破關鏈已通,任務鏈本 session 由
> **旗標短路**進化為**對話蒐線索驅動**(Antos 需先打聽 `EARN_THE_RING` 才賜戒、King 收
> **≥500 金**貢禮才賜 ENILNO,皆對齊 oracle FUN_00408e50/FUN_00402a90);`VIEW`/`YELL`
> 已實作(YELL 引正典市民台詞、STEAL 向商店行竊)。**仍缺**:`UNLOCK`/`NEGATE`/guards(稅/ID/KEY)、
> 戰術 overworld 戰鬥(目前撞擊制)、角色升級系統、時間門真值表。詳見 §3 Gap 與 §4 M3/M4 待辦。

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
  - ✅ **草稿已產出**:[`docs/MAP_REGISTRY.md`](MAP_REGISTRY.md)(33 圖資料驅動分類 + 高信度推定:`mapx20/30/40`=地球三時代、`mapx93`=Antos 賜戒指處…)。精確時代/行星歸屬待對照。
- ✅ **地點進入 world-aware 登記表驅動**(`LOC_REG[]`):切換 overworld 時 landmark 對到該世代的城。
- ✅ **時間之門雛形**:overworld 放青紫門,踏入(或 `P`)→「招牌:ANOS <時代>」→ 切換時代 overworld(00→10→20→30→40 循環,座標保留,重載地圖/實體/門/船)。
- ✅ **時代↔overworld oracle 校正**(map 首位數字;`era_name` 對齊手冊正名)。
- ✅ **地圖環形(toroidal wrap)**:overworld 64×64 邊緣相接(oracle `& 0x3f`),玩家恆置中、tile/實體/門 wrap 渲染;城鎮/地牢不 wrap。單元測試四邊一致。
- ✅ **場景類型分化**:`LOC_REG` 每筆帶 kind(村莊/城鎮/城堡/塔/地牢);進入訊息與標題依類型顯示;地牢/塔走 dungeon(塔=倒置,flag 就緒,完整反轉待 M4 實體地牢);村莊/城鎮/城堡走 tile-map。
- ✅ **依手冊校正翻譯**(光劍/巨劍/Enilno/時代名)+ 擴充 `CONTEXT.md` 術語表。
- ✅ **地牢登記表驅動(per-location)**:`enter_dungeon_at(dest,tower)` 依 LOC_REG 載入對應地牢圖(快取重載)。
- ✅ **跨時代 enter/exit/dungeon 流程驗證**:headless DXOXPOX 正確回到對應時代 overworld 返回座標。
- ✅ **F4 語系切換**:UI/對話/訊息/面板繁中↔English(見 commit 3c99aa6 / 901b754)。

**M1 引擎骨架 ≈ 完成。** 殘留(需外部資料,非引擎):
- 🟡 **landmark→地點精確校正**:`LOC_REG` 哪個門通哪座城/地牢仍 provisional → 需 emulator/oracle 深挖。
- 🟡 **行星地圖可達**:9 行星(mapx5x–9x?)需 **M2 火箭/太空**才能到 → 併入 M2。
- 🟡 **時間之門真實位置/目的地表**:現為引擎放置 + 自由循環;真值需 oracle 資料表。

- **驗收**:headless 腳本能在 5 時代 overworld 間移動、進出各類地點並正確載入(✅ 已達成;行星待 M2)。

### M2 — 載具系統(完整鏈)+ 太空飛行
- ✅ 馬/船/飛機/火箭 + 門檻道具(藍流蘇/骷髏鑰/Ankh;tile 17–20;oracle 拒絕訊息)。
- ✅ 飛機起飛需黃銅鈕扣;移動規則(船=水/飛機=任意/步行馬火箭=陸地)。
- ✅ 太空:火箭發射→深空→E/W HYPERWARP 躍遷 9 行星(手冊 Xeno/Yako/Zabo)→Y 降落行星地表;行星往返;太陽擦撞扣血。
- 🟡 行星↔mapxNN 精確對應仍 provisional(待校正)。
- **驗收**:取得道具→登艦→飛抵行星→降落,headless 可重現(✅ UEEY/UYYE)。

### M3 — 城鎮服務 / 經濟 / 互動指令
- ✅ 商店(Z 開):升級武器/防具、食物、**載具關鍵道具**(藍流蘇/骷髏鑰/黃銅鈕扣/Ankh/三鋰)、King 獻金(最低屬性+1)。扣黃金。
- ✅ 戰鬥用上武器/防具(player_dmg 含武器;受擊減防具)→ 經濟閉環。
- ✅ `VIEW`(鳥瞰)、`YELL`(城內引正典市民台詞:guard/jester/merchant/fighter/cleric/wizard/thief)。
- ✅ `STEAL`(oracle FUN_00409660:向商店行竊武/防/食,敏捷定得手率,失敗引守衛挨揍)。
- ✅ **守衛索稅**(oracle FUN_00409990 A_GUARD_SAYS / PAY_YOUR_TAXES;守衛 = tile 24 / status 255,
  交談索 30 金,繳不出挨揍 + 敵對)。**substrate**:monx 實體 tile 24 = 守衛(8 個/城,MONSTERS.md Guard 255)。
- ✅ **城鎮戰鬥 + 守衛敵對**:撞守衛 = 攻擊(player_dmg vs 守衛 40 HP,守衛反擊);不繳稅/被攻擊 → 敵對
  (`tent_hostile` bitset),移動經敵對守衛鄰格挨打(`step_town_guards`);擊倒守衛 → 取得鑰匙
  (`guard_key`,[正典] GUARDS_CARRY_KEYS,供日後 UNLOCK)。
- ✅ **UNLOCK 上鎖門**(地牢 0xC0 門改為上鎖):前進撞門 → 有守衛鑰匙自動開、否則 AGI 撬鎖(可重試);
  不改 is_wall(渲染/視野不動),僅閘前進;`I` 除錯含 guard_key 保破關回歸確定性。
  守衛→鑰匙→UNLOCK 正典鏈閉環([正典] GUARDS_CARRY_KEYS + UNLOCK_DIRECTION)。
- 🟡 守衛敵對追擊(目前固定不尋路)、ID 查驗、`NEGATE` 尚未。
- **驗收**:買賣改變金/裝備/屬性(✅ headless OZ.. 驗證)。

### M4 — 戰鬥深化 + 地牢實體化
- ✅ **法術系統(原列 M4,已提前實作;commit 標 M6)**:9 法術(手冊 MAGIC SPELLS)、職業限制、
  限地牢/塔施放、消耗制、地牢 HUD、商店習得/建角起始。LIGHT/PASSWALL/SURFACE/上下梯/MISSILE/BLINK/KILL/PRAYER 全可用。
- ✅ **地牢戰鬥對齊 oracle**:DIRECT 必中、dmg=rng&0x3f|0x20(32..95)、EXP=rng&7+1、金=rng%0x11+1(`dungeon_fight`)。
- ✅ **地牢實體化**:怪物實體(低 nibble 類型/上色)、寶箱(BOOTS/CLOAK/IDOL/HELM/三鋰)、
  **寶箱陷阱**(ARGH_A_TRAP / ESCAPED_BY_USE_OF_TOOLS,AGI 決定解除)、火把光照/視野受限(LIGHT 法術)。
- 🟡 overworld 戰鬥仍為撞擊制(戰術戰鬥地圖未實作);狀態效果(麻痺/睡眠/偷食/偷物)部分。
- 🟡 攻擊法術(MISSILE/KILL/PRAYER)目前接隨機遭遇模型(無持久怪物實體),待地牢實體化後深化。
- **驗收**:✅ 法術可用(headless 巫師施法驗證);❌ 戰鬥數值對 oracle、地牢實體化。

### M5 — 道具 / 任務旗標 / 結局
- ✅ 道具系統(runtime + sidecar 持久化);RING 破力場(1000 傷)、ENILNO Quicksword。
- ✅ **任務鏈(對話蒐線索驅動;2026-06-12 改寫)**:
  - **RING**:非 Antos 市民兩段線索(`quest_clue`)→ Antos(mapx93)`EARN_THE_RING`(需 `quest_clue≥1`)→ 賜戒;線索詩對齊 oracle FUN_00402a90(「水流自由的城鎮、樹下無名老人握線索」)。
  - **ENILNO**:King 收 **≥500 金**貢禮才賜劍,不足回絕(對齊 oracle FUN_00408e50 `0x81` 分支);非持戒者仍走一般貢禮(療傷+最低屬性+1)。
  - Legends(mapx00)landmark tile 8 = Minax 巢穴 → 踏入觸發;戒指+ENILNO 殺 Minax → **勝利結局**(`render_ending`,oracle FUN_0040eb60)。
  - ✅ **ALAKAZAM 慷慨市民**(FUN_00408e50 else 分支):clue 鏈過後交談有 1/6 機率提升隨機屬性。
  - 🟡 殘:酒館付金線索(本引擎無建築模型,已由市民對話線索等價覆蓋)、`0x83` 隨機賜物(對應 8 個 reagent counter,與本引擎 quest-item bitmask 不合,不採)、Antos/King 完整對話分支。
- ✅ **任務目標引導**(角色表 quest_hint:依持有道具推進)。
- ✅ **完整破關回歸腳本**(`tests/regression_winnable.sh`):固定 seed 走建角存檔→商店→地牢(含施法)
  →時空旅行→道具→傳說時代→米娜克斯→結局,grep `GAME WON` 判 pass/fail。
- 🟡 道具持有旗標**存檔 offset** 對齊(目前 sidecar);怪物實體 Minax(@標記)/Shadowguard 城內戰待深化。
- **驗收**:✅ headless 完整破關回歸 PASS(GAME WON)+ Minax 閘門 + 結局畫面。

### M6 — 美術完整化(FM Towns)
- 船/怪物/城鎮/城堡/地牢 sprite 從 Tsugaru 乾淨截圖 rip(沿用地形/主角流程);避免 raw atlas 雜訊。
- 時代/行星 tile 變體。

### M7 — 音效 / 音樂
- FM Towns 原創配樂(CD 抽取轉 ogg)+ SFX,SDL_mixer。

### M8 — 平衡 / 打磨 / 完整破關回歸
- ✅ headless 全破關腳本(`tests/regression_winnable.sh`,含地牢施法)當回歸 pass/fail loop。
- ❌ 納入 CI(需自備版權資料,目前本機跑);平衡、邊界、存檔相容。

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
