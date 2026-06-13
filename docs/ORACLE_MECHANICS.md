# Ultima II 機制分析（Ghidra Oracle）

> 來源：`decompile/out/ultima2_decompiled.c`（Alderson Win port，MFC C++，stripped 但保留字串符號），搭配 `oracle_string_map.txt` 導航。
> 方法：字串錨點 grep → awk 抽函式本體。本文聚焦**演算法/規則/常數**，不是逐行還原。
> 信心標示：`[確定]` 反編程式碼直接支持；`[推測]` 從上下文推論；`[未解]` 看不懂或證據不足。

## 全域慣例（先讀，後面常用）

- `this` = 主遊戲物件（C++ class），所有狀態都是 `*(T*)(this + offset)`。
- **RNG**：`FUN_004100b0`（@`004100b0`）`[確定]` — 標準 LCG：
  `seed = seed * 0x343fd + 0x269ec3; return (seed >> 16) & 0x7fff;`（回傳 0..32767）。後續所有 `& mask` 取餘都是基於它。
- **玩家座標**：`this+0x9c` = X、`this+0xa0` = Y（地面圖 64×64，遮罩 `& 0x3f`）。
- **朝向**：`this+0x74c0`，值 0/1/2/3 = 推測 N/E/S/W（戰鬥/地牢轉向用，見下）。
- **地圖 tile buffer**：`this+0x6278` 起。
  - 地面/城鎮：index = `Y*0x40 + X`，tile 取 `byte >> 2`（高 6 bit 是 tile 類型，低 2 bit 是 flag）。
  - 地牢：16×16×多層，index = `(level*0x10 + Y)*0x10 + X`，level 在 `this+0x74bc`。
- **模式 / 場景**：`this+0x7390` = 目前載具（0 步行 / 1 馬 / 2 frigate / 3 plane / 4 rocket）`[確定]`。
- **動作狀態機**：`this+0x738c`（後續按鍵的子模式：方向輸入、施法、交易…）。
- **輸入緩衝**：`this+0x58` 指向字串，`[+1]` 那個 byte 的值（`'0'..'5'`）區分**地面/城鎮（< '4'）vs 地牢/塔（>= '4'）**，這是全程最關鍵的分支條件。
- 主命令分派器：`FUN_004064d0`（@`004064d0`，9626 bytes）`[確定]` — `switch(param_1)` on keypress（大小寫各一 case），是載具/地牢/戰鬥指令的總入口。

---

## 載具（Vehicles）

### 相關函式
| 位址 | 角色 |
|---|---|
| `FUN_004064d0` | 主分派器：`B`(BOARD)/`X`(eXit)/`K`(KLIMB)/`L`(LAUNCH) case |
| `FUN_00406390` | 移動合法性判定（依載具型別決定可走 tile） |
| `FUN_00409320` | 太空飛行（rocket 在軌道/深空）的方向/HYPERWARP/LAND 子分派 |
| `FUN_00409140` | 飛機/火箭地面 LAND 子分派 |
| `FUN_0040e210` | 三元座標 → 行星編號查表（hyperwarp 目的地） |
| `FUN_0040e370` | 軌道/深空狀態文字（YOU ARE ORBITING / DEEP SPACE / HIT THE SUN） |
| `FUN_0040e540` | XENO/YAKO/ZABO 座標輸入（hyperwarp 輸入子狀態 0x1d） |

### 載具型別編碼 `[確定]`
`this+0x7390`：`0`=步行、`1`=HORSE、`2`=FRIGATE、`3`=PLANE、`4`=ROCKET。

### BOARD（`B`，case 0x42/0x62）`[確定]`
- 先決條件：必須在地面/城鎮（`input[1] < '4'`）且目前 `this+0x7390 == 0`（步行）。否則印 `BOARD` + `THINK AGAIN`。
- 讀玩家腳下 tile：`tile = buf[Y*0x40 + X] >> 2`，依值決定登載具：
  - `0x11` → HORSE：`0x7390=1`，腳下 tile 改寫成 `8`（步行 tile）。
  - `0x12` → SHIP/FRIGATE：需 `this+0x154 != 0`（**推測=持有 BLUE TASSLE**，對應 "SAYLORS WEAR BLUE TASSLES"）。否則印 `THE CREW OF THIS SHIP / WILL NOT LET YOU BOARD`。成功則 `0x7390=2`。`[確定 邏輯, 推測 道具語意]`
  - `0x13` → PLANE：需 `this+0x148 != 0`（**推測=持有 SKULL KEY**，對應 "AVIATORS USE SKULL KEYS"）。否則印 `STRANGE YOU CAN'T GET IN`。成功 `0x7390=3`。`[確定 邏輯, 推測 道具]`
  - `0x14` → ROCKET：需 `this+0x140 != 0`（**推測=持有 ANKH**，對應 "ANKHS OPEN SPACE" / "YOU MUST HAVE AN ANKH"）。否則印 `A METALIC VOICE COMMANDS / YOU MUST HAVE AN ANKH`。成功 `0x7390=4`。`[確定 邏輯, 推測 道具]`
  - 其他 → `WHAT?`

### eXit / LAND-from-board（`X`，case 0x58/0x78）`[確定]`
- 若 `0x7390==0`（步行）→ `WHAT?`。
- 否則：腳下 tile 須為 `8`（空地），或（frigate 時）`0`。成立則 `tile = (型別) << 2` 把載具放回地圖、`0x7390=0`（下載具）。否則 `NOT HERE`。

### 移動合法性 `FUN_00406390` `[確定]`
- 地面/城鎮（`input[1] < '4'`）：
  - 步行或騎馬（`0x7390` 0 或 1）：用 `switch(tile>>2)` 白名單可走 tile（含 1,2,3,5..10,0x11..0x17,0x1c,0x1d,0x30）。`[確定]`
  - frigate（`0x7390==2`）：只有 `tile & 0xfc == 0`（水）可走。`[確定]`
- 地牢/塔（`input[1] >= '4'`）：座標須 0..15；可走 tile = `0x00/0x10/0x20/0x30/0x40/0xC0/0xE0`（走廊/門/梯/特殊）。`[確定]`
- 移動成功回 1，否則 0；分派器用它決定 `INVALID MOVE`。

### KLIMB（`K`，case 0x4b/0x6b）/ DESCEND（`D`，case 0x44/0x64）`[確定]`
- 都是地牢內換層：讀 `input[1]`，`'4'`=上方向、`'5'`=下方向（推測 ladder 方向碼），檢查腳下 tile flag（DESCEND 查 `& 0x20` down-ladder，KLIMB 查 `& 0x10` up-ladder）。
- `this+0x74bc`（dungeon level）`+1` 或 `-1`；level 0 時繼續 KLIMB-up 會離開地牢（回 overworld，恢復 `0x73a8/0x73ac` 存的入口座標）。
- 印 `TO LEVEL: %d`。

### LAUNCH（`L`，case 0x4c/0x6c）`[確定]`
- **PLANE**（`0x7390==3`）：需 `this+0x150 != 0`（**推測=BRASS BUTTON**，對應 "PLANES NEED BRASS BUTTONS" / "FUNNY THIS PLANE IS / MISSING A BRASS BUTTON"）。成功：`SetTimer(id=0x67, 250ms)`、`0x738c=0x1a`（飛行子狀態），起飛動畫。`[確定 邏輯, 推測 道具]`
- **ROCKET**（`0x7390==4`）：需 `this+0x160 != 0`（**推測=TRI-LITHIUM 燃料**；TRI-LITHIUM 由寶箱取得寫到 `0x160`，見戰鬥/搜尋）。否則 `A METALLIC VOICE SAYS / SHIP INCAPABLE OF LAUNCH`。
  - 成功：`PREPARE FOR LAUNCH`、`SetTimer(0x68,200ms)`、`0x738c=0x1b`。
  - **發射檢查**：若 `this+0x88 < 5`（推測=某屬性/燃料不足）→ `YOU HAVE EXPLODED` + 死亡 `FUN_0040cc70`。否則進入太空 `FUN_0040e370`。`[確定 邏輯, 推測 0x88 語意]`

### 太空飛行 `FUN_00409320` `[確定]`
火箭在太空時的方向鍵改變軌道參數（`0x74c8`/`0x74cc` 速度向量、`0x74d8` 行星索引 mod 10）：
- `LEFT/RIGHT`（0x25/0x27）：調整橫向速度向量。
- `NORTH/CLIMB`（0x26/0x28）：`0x74d8`（行星序）±1 mod 10，重畫軌道狀態。
- `HYPERWARP`（H，0x48/0x68）：印 `HYPERWARP TO:` + `XENO:`，KillTimer(0x68)，進子狀態 `0x738c=0x1c` 等三元座標輸入。
- `LAND`（L，0x4c/0x6c）：呼 `FUN_0040e210` 確認目前軌道有對應行星（`>=0`），有則 `LANDING REQUESTED` 並降落（`0x74dc`=目的地行星），否則 `REQUEST DENIED`。

### Hyperwarp 三元座標 → 行星表 `FUN_0040e210` `[確定]`
輸入三個 0..9 值存於 `0x74d0 / 0x74d4 / 0x74d8`（XENO/YAKO/ZABO，見 `FUN_00406030` 系列輸入），查表回行星編號：

| XENO,YAKO,ZABO (`74d0,74d4,74d8`) | 行星 |
|---|---|
| 6,6,6 | 0 EARTH |
| 5,4,5 | 1 MERCURY |
| 3,3,4 | 2 VENUS |
| 6,2,3 | 3 MARS |
| 1,3,4 | 4 JUPITER |
| 2,8,5 | 5 SATURN |
| 9,4,6 | 6 URANUS |
| 4,0,5 | 7 NEPTUNE |
| 0,1,4 | 8 PLUTO |
| 9,9,9 | 9 (DAT_0043fec0，推測 Pluto/特殊或冥外) |
| 4,4,4 | 10 SUN（HIT THE SUN → 死亡） |
| 其他 | -1 DEEP SPACE |

> `FUN_0040e370` 用此回傳值印 ORBITING/DEEP SPACE，編號 10 觸發 `YOU HIT THE SUN` + `FUN_0040cc70`（死亡）。`[確定]`
> 太空初始座標被設為 6,6,6（= EARTH 軌道），見 `~line 1498`。`[確定]`

### 道具旗標位置彙整（多為推測）
| offset | 推測道具 | 證據 |
|---|---|---|
| `0x140` | ANKH | ROCKET BOARD 條件 + "YOU MUST HAVE AN ANKH" |
| `0x148` | SKULL KEY | PLANE BOARD 條件 + "AVIATORS USE SKULL KEYS" |
| `0x150` | BRASS BUTTON | PLANE LAUNCH 條件 + "PLANES NEED BRASS BUTTONS" |
| `0x154` | BLUE TASSLE | SHIP BOARD 條件 + "SAYLORS WEAR BLUE TASSLES" |
| `0x160` | TRI-LITHIUM | ROCKET LAUNCH 燃料 + 寶箱取得寫 `0x160` |
| `0x158` | STRANGE COIN | NEGATE TIME 用（"YOU RUN A COIN"，見下） |
| `0x138` | MAGIC HELM 次數 | VIEW 用，戰鬥掉落補充 |

> ⚠️ 這些 offset 對應的道具名稱是從相鄰字串/行為**推測**，需與道具清單函式（`FUN_00405c30`：STAFF/BOOTS/CLOAK/RED GEM/SKULL KEY/GREEN GEM/BRASS BUTTON/BLUE TASSLE/STRANGE COIN/GREEN IDOL/TRI-LITHIUM）交叉驗證才能定稿。`[未解：精確 offset↔道具映射]`

---

## 地牢（Dungeon）

### 相關函式
| 位址 | 角色 |
|---|---|
| `FUN_0040d000` | **3D 線框地牢繪製主迴圈**（landmark，3374 bytes） |
| `FUN_0040dd90` | LineTo（畫線，座標依 client rect 縮放） |
| `FUN_0040de10` | MoveTo（移動畫筆起點） |
| `FUN_004039e0` | 在走廊盡頭畫怪物 sprite（縮放） |
| `FUN_00406390` | 地牢移動合法性（見載具節） |
| `FUN_0040c610` | 每回合怪物行動（含地牢，見戰鬥節） |

### 地牢資料結構 `[確定]`
- 多層 16×16：tile index = `(level*0x10 + Y)*0x10 + X + 0x6278`，`level = this+0x74bc`。
- 每格一個 byte。**高 nibble 是 tile 類型，低 nibble 是 monster/物件 index**（繪製時用 `& 0x80`/`& 0xf0`/`& 0xfc` 等遮罩判斷）。
- 觀察到的 tile 高位語意（從繪製分支）`[確定/部分推測]`：
  - `& 0x80 != 0`：實心牆（擋住前方視線，畫滿牆面）。
  - `& 0x20`：左/右側有開口（畫側邊牆+門框，深度透視）。
  - `& 0x10`：另一種側開口（對稱於 0x20）。
  - `& 0xf0 == 0xc0`：門（在牆面中央畫小矩形門板）。
  - `== 0x40`：特殊地標/梯（畫額外方框，`bVar==0x40` 分支）。
  - `0xC0 / 0xE0`：走廊端景特徵（畫遠端框）。

#### 低 nibble = 怪物類型 index → sprite `[確定]`（本次查證 `FUN_004039e0` @2049 + 掃描 @9558）
- 地牢掃描每深度 `iVar16`(1..4)取前方格 byte `bVar1`,**低 nibble `bVar1 & 0xf` = 怪物類型 index**(1..15,0=無)。
- 深度編碼後傳 `FUN_004039e0(this, dc, uVar14)`:深度 2/3/4 各對應 `0xffffffff/0xfffffffe/0xfffffffd`、深度 1 直接傳 index。
- `FUN_004039e0` 用 index 算 sprite 表偏移 `index*0x100 + 0x5a79`,取怪物 tile/動畫幀 `bVar2`,
  以 `(bVar2 & 0x1f)*4`、`index*4+0x20` 為 sprite sheet 座標,blit 到該深度透視框(深度越遠 sprite 越小)。`[確定]`
- ⇒ **怪物存儲**:類型 index 寫在 cell 低 nibble(運行時由生成函式寫入;mapx 靜態檔低 nibble 實測=0);
  怪物實體屬性(X/Y/HP/type)在 `0x7278` 怪物陣列(32 槽)。低 nibble 只是「該格有此類型怪」的渲染標記。
- **重製對齊狀態**:重寫引擎用獨立 `dgent` 陣列(= oracle 0x7278 概念)存實體 + 渲染查最近怪物,
  機制等價;**怪物類型 index → 不同外觀已對齊**(`mob_tile_color` 依 tile 上色,8 種怪各異)。
  尚未做:把 index 同步寫回 cell 低 nibble(目前 cell 唯讀、渲染走 dgent 查詢,行為等價)。`[簡化]`

### 3D 線框繪製 `FUN_0040d000` `[確定 演算法輪廓]`
- 視窗預設 320×200（`0x140 × 0xC8`），`FUN_0040dd90`/`FUN_0040de10` 把邏輯座標依 client rect 等比縮放（`right*px/0x140`）。
- **first-person raycast 風格**：沿玩家朝向（`this+0x74c0` 的 4 個 case）往前掃 8 格深度（`local_54` 0..7），每格：
  - 取**前方格** `bVar1`、**左格** `local_60`、**右格** `local_5f` 三個 tile。
  - 四個朝向 case 用不同 index 算式取這三格（N: Y-；S: Y+；E: X+；W: X-，對應 0/1/2/3）。`[確定]`
  - 每往深一格，畫面框縮小：`local_5c`（左邊界 X）`+= iVar6`、`local_58`（上邊界 Y）`+= iVar6/2`，`iVar5` 是該格的透視高度（由 `FUN_004102d0`/`__ftol` 算的浮點深度係數）。`[確定 機制, 推測 細節]`
  - 依左/右 tile 的開口 flag 畫側牆梯形（一堆 `0xce/0x160/0x29e/0x2dc` 之類的乘法是透視內插係數）。
  - 前方實心牆（`& 0x80`）→ 畫滿正面方框並中止往前（`bVar2=true`）。
  - 走到 `local_54==7` 或撞牆即停。
- 掃描結束後，再往玩家正前方 1..4 格找**最近的怪物**（低 nibble != 0），呼 `FUN_004039e0(this, dc, depthIndex)` 在對應深度畫怪物 sprite（depthIndex 0/-1/-2/-3 控制縮放大小）。`[確定]`

> 結論：這是經典 U-style 線框地牢，**程式化畫線**而非 tile bitmap。SDL2 重寫可直接用 `SDL_RenderDrawLine`，把 `FUN_0040dd90/de10` 換成 MoveTo/LineTo 等價，透視係數照搬即可。`[確定]`

### 地牢內法術 / 命令
地牢專屬法術在 CAST（`C`）分派器內，依施法者 class（`input[1]` 是 '4'/'5' 時走地牢分支）：見戰鬥節「施法」。地牢相關咒語清單 `FUN_00405a90`：`LIGHT, DOWN LADDER, UP LADDER, PASSWALL, SURFACE, PRAYER, MAGIC MISSILE, BLINK`。`[確定]`
- **PASSWALL**（spell index 3）：對前方一格，若 tile `& 0x80`（牆）則清成 0（打通）；否則 FAILED。`[確定]`
- **LIGHT / IGNITE TORCH**（`I`，case 0x49/0x69）：需 `this+0x90 > 0`（火把數），消耗 1 支，設 `this+0x74c4 = 0x96`（150，光照計時）。`[確定]`
- **UP/DOWN LADDER**（spell 1/2）：等同 KLIMB/DESCEND，內部 re-dispatch `FUN_004064d0(this, 'D'/'K')`。`[確定]`
- 火把熄滅 / 黑暗訊息：`FUN_004030d0`（`TORCH BURNED OUT / IT'S DARK`，光照計時 `0x74c4` 歸零時）。`[確定]`

---

## 戰鬥（Combat）

### 相關函式
| 位址 | 角色 |
|---|---|
| `FUN_0040b3d0` | **近戰攻擊主函式**（landmark，1339 bytes）：命中/傷害/獎勵/掉落 |
| `FUN_0040c610` | **每回合怪物行動**（1388 bytes）：怪物攻擊玩家、狀態效果、偷竊、傷害結算 |
| `FUN_0040cbd0` | 怪物對玩家的相鄰/視線判定（給 c610 用） |
| `FUN_0040ba50` | READY WEAPON 檢查（敏捷夠不夠揮） |
| `FUN_0040bbd0` | WEAR ARMOUR 檢查（力量夠不夠穿） |
| `FUN_004100b0` | RNG（見全域慣例） |

### 攻擊流程 `FUN_0040b3d0` `[確定]`
入口參數 `param_1` = 攻擊方向（0x25..0x28 = 西/北/東/南）。地面戰鬥時由 ATTACK 命令依朝向 `0x74c0` 轉成方向碼再呼叫。

1. 算目標格座標（玩家座標 + 方向，地面 `& 0x3f` wrap）。
2. **找目標怪物**：掃 32 隻怪物陣列（`X[]@0x7278`、`Y[]@0x7298`、`HP[]@0x72b8`、`type[]@0x72d8`、`origTile[]@0x72f8`、`dungeonLevel/flag[]@0x7318`）。找不到 → `..MISS`（無目標）。`[確定]`
3. **命中判定** `[確定]`：
   - 地面戰鬥：`if (rng % 0x50 >= this+0x60) → MISS`。即命中率 = `min(this+0x60, 0x50) / 0x50`。`this+0x60`（推測=玩家某戰鬥屬性/技能值，0x50=80 上限）。`[確定 公式, 推測 0x60 語意]`
   - 地牢戰鬥（`0x738c==2`，DIRECT 攻擊模式）：跳過命中擲骰，必中。`[確定]`
4. 命中 → 印 `..HIT..`，播動畫（`FUN_00403be0` 算螢幕格、`FUN_00404a40` blit、`Sleep(50ms)`）。
5. **傷害** `[確定]`：
   - 地牢：`dmg = rng & 0x3f | 0x20`（32..95）。
   - 地面：`dmg = (this+0x5c + this+0x84*8) >> 2`。`this+0x5c`=護甲/力量值、`this+0x84`=已備武器索引 → 武器越強傷害越高。`[確定 公式, 推測屬性語意]`
   - `怪物HP -= dmg`。若沒打死且目標是 `'@'`（BOSS/MINAX 標記）→ `SHE'S GONE`（逃走）並還原 tile。
6. **擊殺**（HP <= dmg，且該怪 type `this+0x40 != 8`）`[確定]`：
   - 清掉怪物 6 個陣列槽（X/Y/HP/type/origTile/level 全歸 0）。
   - 印 `KILLED. GOLD: %d  EXP: %d`。
   - **GOLD 獎勵**：`this+0x80 += rng % 0x11 + 1`（1..17）。`[確定]`
   - **EXP 獎勵**：地牢 `this+0x7c += rng & 7`（0..7）；地面 `+= (rng & 3) + 1`（1..4）。`[確定]`（其他分支也見 `& 7` 變體）
   - 若擊殺的是 `'@'`（MINAX）→ 呼 `FUN_0040eb60`（勝利結局：MINAX IS DEAD / YOU HAVE SAVED THE UNIVERSE）。`[確定]`
   - **道具掉落**（依怪物 type `[+0x72d8] >> 2`）`[確定]`：
     - `0x18` → `this+0x94 += 2`（推測某補給）。
     - `0x3c` → 40% 機率 `this+0x138`（MAGIC HELM）+1；`this+0x90`（火把）`+= (rng&3)+1`。
     - `0x3e` → 武器槽 `this+0x124+i*4`（i=1..2）+1。
     - `0x3f` → 40% 機率 `this+0x98`+1；隨機武器槽 +1。

### 施法 `C` / 戰鬥法術（在 `FUN_004064d0` CAST case）`[確定]`
- 需 `this+0x128 != 0` 或 `this+0x12c != 0`（持 WAND 或 STAFF），否則 `NEED WAND OR STAFF`。`[確定]`
- 已備法術 `this+0x8c`，每施一次消耗一張（`this+0x8c*4+0xfc` 計數 -1），`NO SPELL` 若沒了。
- 命中 roll：地面 `input[1] < '4'` 直接 FAILED 條件（推測戶外施法限制）。
- 各 spell（switch on `0x8c-1`）`[確定 部分]`：
  - `0` MAGIC MISSILE：設光照計時 `0x74c4=0x96`（推測點亮+傷害）。
  - `1` BLINK/DOWN-ladder：對前方格找空位傳送/換層。
  - `2` UP-ladder 類。
  - `3` PASSWALL：清前方牆（見地牢節）。
  - `4` SURFACE：把 dungeon level 歸零回地面（恢復入口座標）。
  - `5` KILL（即殺）：50% 命中前方相鄰怪物 → 直接 KILLED + GOLD/EXP（rng%0x11+1 金、rng&7 經驗）。`[確定]`
  - `6` 對相鄰怪物造傷：`dmg = (this+0x7c/100)*2 + 0x1e`（隨經驗成長，基礎 30）。HP 扣完即殺並給獎勵。`[確定]`
  - `7` BLINK/teleport：隨機座標傳送（`rng&0xf | 1`）。`[確定]`
  - `8` 另一種定點殺敵。

### 每回合怪物行動 `FUN_0040c610` `[確定]`
遍歷 32 隻怪物（`pbVar6` 從 `0x72b7` 倒掃），對每隻活著的怪物（HP `[+0x40]` != 0；地面 type 取 `>>2`）：
1. 用 `FUN_0040cbd0` 判定怪物與玩家的相對位置（相鄰=1 / 視線=2 / 特殊=3）。
2. **MINAX**（type `0x10` 且 range 3）：印 `MINAX CRIES: DIE FOOL`，對玩家造 100 點固定傷害（HP `0x74` -100，不足則死 `FUN_0040cc70`）。`[確定]`
3. **遠程狀態攻擊**（roll `< 0x20` ≈ 12.5%）依 type `[確定]`：
   - `0xd` LEGS PARALIZED → 設腿麻 `0x739c = rng&0xf`（1..15 回合）；若持 MAGICAL BOOTS（`0x130`）且 roll>=0x40 則 `SAVED BY MAGICAL BOOTS`。
   - `0xe` ARMS PARALIZED → `0x7398`（臂麻）；MAGICAL CLOAK（`0x134`）可擋。
   - `0xf` SLEEP SPELL → `0x73a0`（睡眠回合）；GREEN IDOL（`0x15c`）可擋（`SAVED BY IDOL`）。
   - `0x3e` MAGIC MISSILE → 累加 `local_18 += 0x28`（40 傷害）。
4. **近身偷竊/騷擾**（roll `< 0x40` ≈ 25%）依 type `[確定]`：
   - `0x2` 吹熄火把（`TORCH BLOWN OUT`，清光照 `0x74c4`）。
   - `0x5` GREMLIN 偷食物（`0x78` -100，沒食物則死）。
   - `0x7` 強力魔法（強制睡眠 `0x73a0 = rng&7`）。
   - `0x3f` THIEF 偷道具（隨機武器槽 -1，`A THIEF STOLE SOMETHING`）。
5. **物理攻擊**：相鄰且面向玩家時，roll vs 玩家護甲 `0x88`，命中則 `local_18 += monsterPower(pbVar6[0x20]>>2)`；玩家站在某 tile（`& 0xfc == 0xc`，推測陷阱/不利地形）時傷害 `×2`。`[確定]`
6. 回合結束：總傷 `local_18` 經 `& 0x77` 縮放 +1，從玩家 HP `0x74` 扣；HP 不足 → 死亡 `FUN_0040cc70`。`[確定]`

### 狀態效果計時器位置 `[確定]`
| offset | 狀態 |
|---|---|
| `0x7398` | 手臂麻痺（ARMS PARALIZED，剩餘回合） |
| `0x739c` | 腿麻痺（LEGS PARALIZED） |
| `0x73a0` | 睡眠（SLEEP） |
| `0x7390` | （注意：這是載具型別，**非**狀態） |
| `0x74c4` | 光照/火把計時（150 起算） |

> 移動分派器（`FUN_004064d0`）在每個方向 case 開頭都檢查 `this+0x739c`（麻痺），非 0 則印 `*_PARALIZED` 並阻止移動。`[確定]`

### NEGATE TIME（`N`）`[確定]`
需 STRANGE COIN（`this+0x158 != 0`）。成立則 `YOU RUN A COIN`，設 `this+0x7394 = 0x14`（20 回合凍結怪物），消耗一枚。否則 `HOW? / YOU'RE NOT EINSTEIN`。

### READY WEAPON / WEAR ARMOUR 門檻 `[確定]`
- `FUN_0040ba50`（武器）：`need = idx*5 (+3 if odd)`；須 `need < this+0x60`（敏捷）才能 `READY`，否則 `THOU ART NOT AGILE ENOUGH TO WIELD`。`this+0x84` = 已備武器。
- `FUN_0040bbd0`（護甲）：`need = idx*5 (+3 if odd)`；須 `need < this+0x5c`（力量）才能 `READY`，否則 `STRONG ENOUGH TO WEAR` 失敗。`this+0x88` = 已備護甲。
- 兩者都要求未持有（陣列計數 0 且 idx!=0）時印 `NOT OWNED`。`[確定]`

### 進入地點 ENTER（`E`）landmark tile → 地點類型 `[確定]`
`FUN_004064d0` case 0x45/0x65（oracle 行 4916-5056）。僅當腳下 overworld（mapname byte1 == '0'）：讀腳下 tile id（`tile >> 2`），依 id switch 決定地點類型並組出目的地 `mapxNN`：
- 目的地 byte0（時代碼）= 當前 overworld mapname byte0（`this+0x58` 字串）。
- 目的地 byte1（kind 碼）由 `FUN_00427a50(mapname,1,'?')` 寫死：

| tile id | 地點 | kind 碼（byte1） | 目的地 |
|---|---|---|---|
| 5 | VILLAGE | '1' | `mapx<時代>1` |
| 6 | TOWN | '2' | `mapx<時代>2` |
| 8 | CASTLE | '3' | `mapx<時代>3` |
| 7 | TOWER | '4' | `mapx<時代>4` |
| 9 | DUNGEON | '5' | `mapx<時代>5` |
| 10 | 時間之門 | — | 呼叫 `FUN_0040c270`（印招牌 ANOS \<era\>），非載圖 |

⇒ **進入規則是 deterministic（tile id → kind 碼），非逐 landmark 推定**。引擎 `LOC_REG[]` 已照此重排（2026-06-13），只列各時代 overworld 實際存在且目的地檔存在的 landmark。

### 時間之門（moongate）排程 `[確定機制 / 座標表未解]`
- **每回合 tick** `FUN_0040cd20`：僅 overworld（mapname byte1=='0'）、時代碼 <'5'（即 5 個地球時代 0-4）、非行星（`0x74dc==0`）。倒數 `this+0xac`（init 8）每回合 -1；歸零→重設 8 並**搬移時間之門**：
  - 舊門座標 = `DAT[(phase>>1) + era*4]`，若仍是門 tile（`&0xfc==0xc0`）則還原成記錄的底圖 `this+0xb0`。
  - **月相前進**：`this+0xa8 = (this+0xa8 + 2) & 7`（init 0）⇒ `phase = 0xa8>>1` 週期 0,1,2,3。
  - 新門座標 = `DAT[(new_phase) + era*4]`，記錄底圖到 `0xb0`，蓋上門 tile `0xc0`。
- **踏上時間之門**（移動後腳下 `tile>>2 == 0x30`，即 raw `0xc0`，oracle 行 5705-5727）：
  - `phase = this+0xa8 >> 1`。
  - **目的地時代** `dest_era = (cur_era <= phase) ? phase+1 : phase`（cur_era = mapname byte0 - '0'）。
  - 寫回 `mapname[0] = dest_era + '0'`，並從 `DAT_0043e260[]`（x）/ `DAT_0043e261[]`（y）依 `(phase + dest_era*4)*2` 索引取得**落地座標**。
- ⚠ **座標表 `DAT_0043e260/e261` 不在 Ghidra C dump（`.data` 未含）**：門生成 / 落地座標無法從 oracle 還原，需 emulator dump 或原版 exe `.data` 抽取。
- **與現行引擎差異（2026-06-13 更新：拓樸 + 近似座標皆已落地）**：
  - **拓樸**：`next_era_world`/`time_travel` 已從線性循環改為**正典每時代多目的地網** `ERA_GATES[5][4]`（來源：rpgclassics PC 版 overworld「Time Gates」各門方位+目的地，目的地集合與 strategywiki C64 版一致）。各時代有 4 道門分別通往不同時代——例如 1423BC：北美→傳說 / 南美→2112 / 西歐→1990 / 東歐→盤古。月相 `moon_phase`（0..3，每次穿門推進）選當前那道門。
  - **座標（近似已落地）**：精確表 `DAT_0043e260/e261` 仍不在 Ghidra dump（疑在 resource file，非主程式 code 段）；改依各門地名方位映到 64×64 象限做**近似**（N=低y/S=高y/E=高x/W=低x/C=中心），`place_time_door` snap 到最近可通行陸地。門不再「玩家附近隨機升起」，而是各時代**固定地理位置**可被探索發現。
  - headless 破關鏈相應改為 `PPPP M`（mapx20 起，月相0→傳說→1423BC→1990→傳說，第 4 門抵 Legends），決定性保留、WON 不變。

---

## 待驗證 / 未解清單

1. `[未解]` 道具旗標 offset（`0x140/0x148/0x150/0x154/0x158/0x160`）↔ 具體道具名，目前靠相鄰字串推測，需與存讀檔/道具顯示函式交叉確認。
2. `[推測]` 屬性 offset：`0x5c`=力量、`0x60`=敏捷、`0x84`=武器索引、`0x88`=護甲索引、`0x7c`=EXP、`0x80`=GOLD、`0x74`=HP、`0x78`=食物。多數由用途反推，命名待定。
3. `[未解]` `FUN_0040d000` 中透視內插的精確係數含義（`0xce/0x160/0x29e…`）——機制清楚（梯形側牆），但每個常數對應哪條邊未逐一拆解；SDL2 重寫可直接照抄。
4. `[推測]` 朝向 `0x74c0` 0/1/2/3 = N/E/S/W：由戰鬥 ATTACK case 把 0→東鍵、1→南、2→西、3→北 反推，與地牢繪製方向 case 一致，但絕對對應仍需在跑動畫時確認。
5. `[未解]` `0x7390==2`（frigate）BOARD 條件 `0x154` 與「frigate 砲擊」(`F` 命令, case 0x46) 的彈藥來源關係。
