# 《創世紀 II:女巫的復仇》完整中文攻略

> **Ultima II: The Revenge of the Enchantress**(1982,Richard Garriott / Origin)
> 繁體中文攻略 · 對應本 repo 的 SDL2 重製引擎(u2-cht)
> 編寫日期:2026-06-13

---

## 這份攻略怎麼讀

本攻略分成四大塊,各自獨立成章,從速查到通關一條龍:

| 章節 | 內容 | 連結 |
|---|---|---|
| **一、怪物圖鑑** | 每種怪物的 HP、攻擊、特殊能力、掉落、出沒地;DOS/EGA 與 FM Towns 雙版本 sprite 對照 | [`guide/01-monsters.md`](guide/01-monsters.md) |
| **二、世界地圖** | 五大時代 overworld 地圖畫面 + 地標;太陽系九大行星座標 | [`guide/02-world-map.md`](guide/02-world-map.md) |
| **三、迷宮（地牢）** | 16 層地牢結構、樓梯、寶箱陷阱、可施法術、第一人稱線框 | [`guide/03-dungeons.md`](guide/03-dungeons.md) |
| **四、通關流程** | 建角 → 蒐集任務道具 → 各時代/行星該做什麼 → 擊敗米娜克斯;**時間之門開啟時機完整解說** | [`guide/04-walkthrough.md`](guide/04-walkthrough.md) |

> 趕時間想直接破關 → 直接看 [第四章 · 通關流程](guide/04-walkthrough.md) 的「最速通關路線」一節。

---

## 故事背景

第一作《創世紀 I》的反派巫師蒙登(Mondain)被英雄打倒後,他的學徒兼情人 **米娜克斯(Minax)** 為復仇而來。她撕裂時空,讓地球的歷史在五個時代之間崩壞,並以位於**傳說時代(Time of Legends)** 的暗影衛城(Castle Shadowguard)為巢穴。

玩家要從**西元前 1423 年**的地球出發,穿梭五大時代與太陽系九大行星,蒐齊登船/飛行/上太空所需的關鍵道具與**力場之戒(Ring)**、**迅捷之劍 ENILNO**,最後殺進傳說時代,在座標 **9-9-9** 終結米娜克斯。

> 結局字幕:`MINAX IS DEAD! / ALL HER WORKS SHALL DIE / YOU HAVE SAVED THE UNIVERSE`,並預告下一作的反派 Exodus(指向《創世紀 III》)。

---

## 核心系統速覽

### 五大時代(Time of Eras)

| 時代碼 | 名稱 | 說明 |
|---|---|---|
| 0 | **傳說時代(Time of Legends)** | 米娜克斯巢穴所在,終戰場 |
| 1 | **盤古大陸(9,000,000 B.C. / Pangea)** | 上古超大陸 |
| 2 | **西元前 1423 年** | 遊戲起點 |
| 3 | **西元 1990 年** | 現代,主要補給/升級時代 |
| 4 | **浩劫餘生(2112 A.D. / Aftermath)** | 末日後地球 |

時代之間靠**時間之門(Time Door)** 穿越。各時代有 4 道門,各通往不同時代——詳見 [第四章 · 時間之門](guide/04-walkthrough.md#時間之門time-door完整解說)。

### 載具鏈(移動能力升級)

| 載具 | 需要道具 | 行走範圍 |
|---|---|---|
| 步行 | — | 陸地 |
| 馬 HORSE | — | 陸地(較快) |
| 船 FRIGATE | 藍流蘇 BLUE TASSLE | 水面 |
| 飛機 PLANE | 骷髏鑰 SKULL KEY(登)+ 黃銅鈕扣 BRASS BUTTON(起飛) | 任意地形 |
| 火箭 ROCKET | Ankh(登)+ 三鋰 TRI-LITHIUM(燃料) | 陸地 + 太空 |

### 太陽系九大行星(火箭 HYPERWARP 座標)

| 行星 | 座標 (XENO,YAKO,ZABO) | 行星 | 座標 |
|---|---|---|---|
| 地球 EARTH | 6,6,6 | 土星 SATURN | 2,8,5 |
| 水星 MERCURY | 5,4,5 | 天王星 URANUS | 9,4,6 |
| 金星 VENUS | 3,3,4 | 海王星 NEPTUNE | 4,0,5 |
| 火星 MARS | 6,2,3 | 冥王星 PLUTO | 0,1,4 |
| 木星 JUPITER | 1,3,4 | (太陽) | 4,4,4 = 撞日死亡 |

> ⚠️ 輸入錯誤座標會落到「深空 DEEP SPACE」;座標 4,4,4 會 `HIT THE SUN` 直接死亡。

---

## 資料可信度說明

本攻略的**數值與機制**以兩個來源為主、攻略網站為輔:

1. **引擎真值**:本 repo 重製引擎 `src/game_main.c` 的 `mob_type()`(怪物 HP/攻擊)、`ERA_GATES[5][4]`(時間之門拓樸),這些都對齊過正典。
2. **反組譯 oracle**:[`docs/ORACLE_MECHANICS.md`](ORACLE_MECHANICS.md),從原版 binary 反編譯而來,信心標 [確定]/[推測]。
3. **外部攻略**:Codex of Ultima Wisdom、StrategyWiki、RPGClassics(用於補齊地名、時間之門目的地、流程細節)。

衝突時以引擎/oracle 為準並註明。推測或資料不足處皆明確標示,不編造。完整來源見各章末。

---

## 圖片與素材出處

攻略內所有遊戲畫面來自本 repo 的 SDL2 重製引擎(headless 截圖)或既有素材:

- 各時代 overworld:`docs/guide/img/overworld_*.png`(本次以 docker headless 重製引擎產生)
- 地牢線框:`docs/screenshots/dungeon_wireframe.png`、`dungeon_descend.gif`
- 戰鬥/載具/角色表:`docs/screenshots/combat_encounter.png`、`vehicle_ship.png`、`char_sheet.png`
- FM Towns 怪物 sprite:`docs/monsters/m*.png`、`docs/screenshots/fmtowns_monsters_colored.png`
- 完整試玩:`docs/demo/u2cht_demo.gif`
</content>
</invoke>
