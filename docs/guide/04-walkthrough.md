# 第四章 · 通關流程（Walkthrough）

> 回 [攻略總目錄](../GUIDE.md)

本章從建角一路帶到擊敗米娜克斯。流程細節交叉自 [Codex U2 walkthrough](https://wiki.ultimacodex.com/wiki/Ultima_II_walkthrough)、StrategyWiki、RPGClassics,**機制與時間之門拓樸以本 repo 引擎 / oracle 為準**。

---

## 最速通關路線（懶人包）

1. **建角**:女性 + 高敏捷(船/劍/撬鎖都看敏捷)。
2. **1423 B.C.(起點)**:殺怪刷錢,蒐**藍流蘇 BLUE TASSLE**(可登海盜船)。
3. 穿時間之門到 **1990 A.D.**:去 New San Antonio 的 **Hotel California** 用「100 金 → Alakazam」反覆**衝屬性**(別讓單屬性 >99 或 Int+Cha >160,會重置)。
4. 拿 **骷髏鑰 + 黃銅鈕扣**(飛機)→ 飛到 **Pirate Harbor**(2112)→ 火箭發射點。
5. 火箭上太空 → **HYPERWARP 9,9,9 → Planet X → Castle Barataria 找 Father Antos 取祝福**。
6. 回地球(6,6,6),向 New San Antonio 老人**付 900 金換力場之戒 RING**(需 Antos 祝福)。
7. 在 New San Antonio 用 **500 金**從 Sentri 處取得**迅捷之劍 ENILNO**(需敏捷 49)。
8. 屬性/HP 練到夠(建議 HP 9900+、動力盔甲),穿時間之門到**傳說時代**。
9. 暗影衛城東北角找米娜克斯,**RING 護身穿力場 + ENILNO 一擊**斃命 → 通關。

---

## 第一步 · 建立角色

![角色卡](../screenshots/char_sheet.png)

- 分配 **90 點**到六大屬性:STR(力量)、AGI(敏捷)、STA(體力)、CHA(魅力)、WIS(智慧)、INT(智力)。
- 流程:每屬性打**兩位數**即提交 → 性別 M/F → 種族(HUMAN/ELF/DWARF/HOBBIT)→ 職業(FIGHTER/CLERIC/WIZARD/THIEF)→ 命名 → SATISFACTORY? Y/N。
- **建角建議**:
  - **敏捷 AGI 是關鍵屬性**——決定命中率、能揮哪把武器、能穿哪件防具、撬鎖/解陷阱成功率、偷竊得手率。
  - **女性嚮導**自動獲得 Int=20 / Cha=20,可把剩餘點全砸力量與敏捷(攻略提到可享 38% 商店折扣)。
  - 想用法術 → 選牧師 / 巫師(地牢內施法很有用)。

> 種族/職業會給屬性加成(存檔存「套用加成後」的值,見 [`DATA_FORMATS.md`](../DATA_FORMATS.md))。

---

## 第二步 · 1423 B.C.（起點:武裝與起步）

新角色從**北美大湖區**附近開始,身上有錢但沒裝備。

1. 向**西北跨白令海峽陸橋到歐洲**,在 **Towne Linda** 買斧頭(武器)與鎖子甲(防具)。
2. 到**非洲村莊囤食物**(沒食物會餓死)。
3. **首要任務:殺怪 + 盜賊,蒐集特殊物品**,特別是 **藍流蘇(BLUE TASSLE)**——它讓你能登上海盜船並開砲。
4. 城鎮服務:
   - **商店(Z 開)**:升級武器/防具、買食物、買載具關鍵道具、King 獻金。
   - **King 獻金**:獻金可提升最低屬性(對齊 oracle)。
   - **酒館 / 市民對話**:給任務線索(Father Antos 線、城鎮梗)。

> 城鎮戰鬥提醒:別亂打**守衛**(255 HP,惹毛全城圍毆);但擊倒守衛能拿**鑰匙**。詳見 [第一章 · 城鎮居民](01-monsters.md#12-城鎮居民)。

---

## 第三步 · 1990 A.D.（衝屬性 + 取飛機 + 取劍與戒指）

穿時間之門到 **1990 年**(這是補給核心時代)。

### 衝屬性(Hotel California)

- 去 **New San Antonio 的 Hotel California**,向 **The Clerk** 提供 **100 金幣**,他回 `Alakazam!` 並提升你的隨機屬性。
- 反覆 [Pass] / 查 [Status] 後再給金幣,堆高屬性。
- ⚠️ **上限**:任何**單一屬性 > 99**、或 **Int + Cha > 160** 會觸發**重置**,務必停手。

> 引擎對齊:ALAKAZAM 慷慨市民(oracle `FUN_00408e50` else 分支)在線索鏈過後交談有機率提升隨機屬性。

### 取得飛機(骷髏鑰 + 黃銅鈕扣)

- **骷髏鑰 SKULL KEY**:登上飛機(`AVIATORS USE SKULL KEYS`)。
- **黃銅鈕扣 BRASS BUTTON**:飛機起飛需要(`PLANES NEED BRASS BUTTONS`)。
- 兩者可在 New San Antonio 或 Pirate Harbor 的飛機點附近取得。

### 取得迅捷之劍 ENILNO

- **Sentri** 被囚在 **New San Antonio**,需 **500 金幣**「贖」出迅捷之劍,且要求**敏捷 49**。
- 引擎對齊:King 收 **≥500 金**貢禮才賜劍(oracle `FUN_00408e50` `0x81` 分支)。

### 取得力場之戒 RING（需先完成第四步的 Antos 祝福）

- 完成 Planet X 的 Father Antos 祝福後,回 New San Antonio,向**老人付 900 金幣**換取**力場之戒(RING)**。
- ⚠️ RING 可能被**盜賊偷走**,拿到後小心保管(別在有盜賊的地方久留)。

---

## 第四步 · 上太空（Ankh + 三鋰 + Planet X 找 Antos）

### 取得火箭鑰匙與燃料

| 道具 | 用途 | 取得 |
|---|---|---|
| **Ankh** | 登上火箭(`YOU MUST HAVE AN ANKH`) | 太空 / 任務取得 |
| **三鋰 TRI-LITHIUM** | 火箭燃料(`SHIP INCAPABLE OF LAUNCH`) | **地牢寶箱**(約 17%,FM Towns 版設定) |

### 發射與躍遷

1. 在 **Pirate Harbor(2112)的火箭發射點(紅色方格)** 登上火箭(需 Ankh)。
2. **L**(LAUNCH)發射(需三鋰燃料;某屬性不足會 `YOU HAVE EXPLODED` 死亡)。
3. 進入太空後,初始軌道在地球(6,6,6)。按 **H**(HYPERWARP),輸入三元座標 **9,9,9**(XENO/YAKO/ZABO)。
4. 抵達 **Planet X** 軌道後,按 **L**(LAND)降落到草地。

### Planet X · Father Antos

1. 進 **Castle Barataria**,找 **Father Antos**,**取得他的祝福(blessing)**。
2. 任務鏈(引擎對齊 oracle):需先向**非 Antos 市民**打聽到 `EARN THE RING` 線索(線索詩:水流自由的城鎮、樹下無名老人握線索),再找 Antos(他在 mapx93)才會賜福/指引取戒。
3. 回火箭,HYPERWARP 回 **地球(6,6,6)** 降落。

> ⚠️ **正典 bug**:PC CD-ROM 版部分行星到不了。主線只需 Planet X(9,9,9);太陽座標 4,4,4 = 撞日死亡,別輸錯。

---

## 第五步 · 傳說時代決戰米娜克斯

備齊清單(出發前確認):

- ✅ **力場之戒 RING**(穿米娜克斯力場,否則 1000 傷必死)
- ✅ **迅捷之劍 ENILNO**(一擊必殺她)
- ✅ HP 充足(攻略建議 **9900+**)+ 動力盔甲
- ✅ **奇異硬幣**(NEGATE TIME,凍結雜怪)

決戰步驟:

1. 穿時間之門到**傳說時代**(各門目的地見下節)。
2. 前往**暗影衛城(Castle Shadowguard)東北角**,米娜克斯在此。
3. 她喊 `DIE FOOL`,對你造**固定 100 傷/回合**(範圍 3)。**RING 護身**才能穿過她周圍 1000 傷的力場。
4. 用 **NEGATE TIME**(奇異硬幣)凍結周圍無敵雜怪,避免被纏。
5. 她被打會**瞬移到西南角**(`SHE'S GONE`),追過去。
6. 用 **ENILNO** 攻擊命中她 → **一擊必殺** → 勝利結局:

   > `MINAX IS DEAD! / ALL HER WORKS SHALL DIE / YOU HAVE SAVED THE UNIVERSE`
   > (並預告下一作反派 Exodus → 《創世紀 III》)

---

## 時間之門（Time Door）完整解說

時間之門是穿越五大時代的唯一方法。本節整合**引擎 `ERA_GATES[5][4]`**(已對齊正典)、oracle 機制與攻略網站。

### 開啟時機（月相驅動）

時間之門的位置與通往何處,由**月相(moon phase)** 驅動,**會隨遊戲回合移動、定時升起又消散**:

- **每回合 tick**(oracle `FUN_0040cd20`):僅在**地球五個時代的 overworld** 生效(行星/城鎮/地牢不算)。
- 倒數計時器歸零時,**舊門消失、新門在別處升起**,同時**月相前進**(週期 0→1→2→3 循環)。
- **月相決定當前那道門通往哪個時代**——同一塊地、不同月相,門會指向不同時代。
- 本引擎:門升起後可見約 14 回合 → 消散 → 隱沒約 6 回合 → 在新位置(新月相對應的固定象限)再升起。原版同理「定時升起、很快消散」。

> **踏門規則**(oracle 行 5705-5727):踏上門時 `phase = 月相值`。
> 目的地時代 = `(當前時代 ≤ phase) ? phase+1 : phase`,落地座標另有專表。
> 本引擎改用**正典每時代多目的地網**(下表),月相選當前那道門。

### 各時代四道門目的地（引擎 ERA_GATES,對齊 RPGClassics/StrategyWiki）

| 所在時代 | 門 1 | 門 2 | 門 3 | 門 4 |
|---|---|---|---|---|
| **傳說(Legends)** | →盤古 | →1423BC | →1990 | →2112 |
| **盤古(Pangea)** | 北→傳說 | 東(歐亞)→1423BC | 中→2112 | 南→1990 |
| **1423 B.C.** | 北美→傳說 | 南美→2112 | 西歐→1990 | 東歐→盤古 |
| **1990 A.D.** | 格陵蘭→盤古 | 南美→2112 | 英→1423BC | 澳洲→傳說 |
| **2112 A.D.** | 西北美→1990 | 東南美→傳說 | 中亞→1423BC | 南亞→盤古 |

> ⚠️ **版本差異**(StrategyWiki):盤古的「原非洲 / 原南極」兩門目的地在某些版本對調(上表為 C64 port);引擎採與 RPGClassics PC 版一致的拓樸。

### 怎麼用時間之門到目標時代

- **傳說時代特殊**:四道門在傳說時代**相鄰並排**(無時間的時代,門都聚在一起)。從別的時代要到傳說,看上表選對門。
- 例:在 1423 B.C. 想去 1990 → 找**西歐**那道門(月相對應時)。想去傳說 → 找**北美**門。
- 因為門會隨月相移動,若當前位置的門指向不對的時代,**多走幾步等月相推進**,或到正典固定的另一道門位置(本引擎門在各時代固定地理象限,可探索發現)。

### 引擎與原版的差異（誠實揭露）

- **拓樸已對齊**:引擎 `ERA_GATES` 的目的地集合與正典一致(來源 RPGClassics + StrategyWiki 交叉)。
- **精確座標近似**:原版精確門座標表(`DAT_0043e260/e261`)不在反組譯 dump(疑在 resource file),引擎改用**各門地名方位映到 64×64 象限**做近似(N=低y/S=高y/E=高x/W=低x/C=中心),門會 snap 到最近可通行陸地。**方位對、精確格位近似**。
- 詳見 [`ORACLE_MECHANICS.md` · 時間之門](../ORACLE_MECHANICS.md#時間之門moongate排程-確定機制--座標表未解)。

---

## 城鎮指令速查

| 鍵 | 指令 | 作用 |
|---|---|---|
| Z | 商店 TRANSACT | 買賣武器/防具/食物/載具道具、King 獻金 |
| B | BOARD | 登載具(馬/船/飛機/火箭) |
| X | eXit | 下載具 |
| L | LAUNCH | 飛機/火箭起飛 |
| K / D | KLIMB / DESCEND | 地牢上下樓 |
| C | CAST | 施法(限地牢/塔) |
| I | IGNITE | 點火把 |
| N | NEGATE TIME | 奇異硬幣凍結怪物 20 回合 |
| E | ENTER | 進入腳下地點 |
| VIEW / YELL / STEAL | — | 鳥瞰 / 喊話引市民台詞 / 向商店行竊 |

---

## 互動試玩

本 repo 重製引擎的完整試玩流程:

![試玩 demo](../demo/u2cht_demo.gif)

---

## 來源

- 主線流程:[Codex of Ultima Wisdom — Ultima II walkthrough](https://wiki.ultimacodex.com/wiki/Ultima_II_walkthrough)
- 時間之門目的地:[StrategyWiki — Time Gates](https://strategywiki.org/wiki/Ultima_II:_The_Revenge_of_the_Enchantress/Time_Gates) · [RPGClassics — Overworld](https://shrines.rpgclassics.com/pc/ultima2/overworld.shtml)
- 時間之門 / 任務 / 戰鬥機制真值:[`docs/ORACLE_MECHANICS.md`](../ORACLE_MECHANICS.md) · 引擎 `src/game_main.c`(`ERA_GATES`、任務鏈)
- 道具門檻:[`docs/ROADMAP_REMAKE.md`](../ROADMAP_REMAKE.md) §2.2–2.4
</content>
