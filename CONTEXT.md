# CONTEXT — Ultima II 中文化 Ubiquitous Language

> 專案共用術語表。命名變數 / 寫文件前優先用這裡的詞;新概念先進此表再用。
> 格式:`Term — definition. _Avoid_: 禁用同義詞`

## 標的與基礎
- **Alderson port** — John Alderson《Windows Native Ultima II》v1.01 (2000),PE32/MFC/GDI Win 移植版 exe。本案反組譯與行為對照的來源。 _Avoid_: 「Windows 版」泛稱
- **DOS 原版** — 1983 Origin Ultima II DOS 版(`ultimaii.exe` + 資料檔),資料格式真值來源。
- **oracle** — Ghidra 反編出的 C(`ultima2_decompiled.c`),只當**行為/演算法參考**,非編譯標的、不公開。 _Avoid_: 「反編源碼」當成可用 source
- **乾淨重寫 (clean rewrite)** — 自寫的 SDL2 C 引擎(`src/`),可公開上 GitHub。對照 oracle 行為但不照抄 MFC 殼。

## 資料格式
- **tile id** — 地圖圖塊編號 0–63。map 檔存的是 `tile id × 4`,讀取要 ÷4。
- **mapxNN** — 64×66 tile 地圖檔(4224 B,無 header)。**首位數字=時代**(0 傳說/1 盤古/2 B.C./3 A.D./4 浩劫;oracle FUN_0040c270 釘死);**次位數字=子地點**(0 overworld、1–3 城鎮、4–5 地牢/塔)。mapx5x–9x 疑為行星。見 `docs/MAP_REGISTRY.md`。
- **tlkxNN** — NPC 對話檔(384 B)。high-bit ASCII 編碼。
- **high-bit ASCII** — U2 文字編碼:每 byte `OR 0x80`;解碼 `byte & 0x7f`。`\r`(0x0d)為換行。 _Avoid_: 「加密」(ModdingWiki 稱 encrypted,實為 high-bit)
- **覆蓋層 (overlay)** — 中文化字串以 `(來源, key)` 為索引的外部 UTF-8 表,載入時覆蓋原文;**不寫回**原始資料檔(避免破壞 offset)。

## 渲染(見 ADR 0001)
- **像素圖層** — tile/sprite 層,16×16 來源整數倍放大(nearest 預設)。
- **文字圖層** — CJK/UI 層,內部高解析度原生繪製,不被縮放。
- **內部 render 解析度** — 320×200 × N(N 決定 CJK glyph 大小);與**視窗解析度**解耦。
- **CJK-aware 換行** — 全形字 2 倍寬的換行/寬度計算。

## 中文化文字三來源
- **exe 字串表** — Alderson exe 內嵌 UI 字串(`translations/exe_translatable_strings.tsv`,vaddr 索引)。
- **對話表** — tlkx 解碼對話(`translations/talk_dialogue.tsv`,(檔名,index) 索引)。
- **引擎多語字典** — 引擎硬編訊息(`translations/ui_strings.tsv`,欄 `zh`=key、`en`、…)。
  `tr("中文")` 依 `u2_lang` 查表;F4 循環 `u2_nlang`(= 字典欄數)。
  **加新語言(如日文)= 加一欄 `ja` + 填譯文,零改碼**(欄數即語言數)。
  新增引擎字串:`tr("中文")` 包裝 + 進 `ui_strings.tsv` 加一列。
  資料字串(exe/對話)非 ZH 時 fallback 原文(英文),待補各語言欄。

## 專有名詞譯名(對齊 u6-cht / u3-cht 系列一致性)
> 來源:Ultima Codex wiki + u6-cht 既定譯名。翻譯 `translations/*.tsv` 時以此為準。

| 原文 | 譯名 | 備註 |
|---|---|---|
| Minax | **米娜克斯** | 本作反派女巫(the Enchantress);沿用 u6 |
| Mondain | **蒙丹** | U1 反派,米娜克斯之師/愛人;沿用 u6 |
| Lord British | **不列顛王** | 收金幣治療玩家的 NPC;沿用 u6 |
| The Stranger | **異鄉人** | 玩家角色 |
| Sosaria | **索薩里亞** | U1 世界 |
| Shadowguard | **影域堡** | 米娜克斯的城堡(傳說時代) |
| Time of Legends | **傳說時代** | 米娜克斯城堡所在紀元 |
| Aftermath | **浩劫餘生** | 2111 核戰後的地球 |
| Quicksword Enilno | **迅捷之劍 Enilno** | 唯一能殺米娜克斯的武器(Enilno=online 反拼,保留) |
| Force Field Ring | **力場之戒** | 破米娜克斯力場(`RING PROTECTS FROM FIELD`);譯文可簡稱「戒指」 |
| Tri-lithium | **三鋰** | 火箭燃料 + 地牢第 16 層寶箱;關鍵資源 |
| Time Door | **時光之門** | 時空旅行傳送門 |
| H.P. / Food / Exp. / Gold | **生命 / 食物 / 經驗 / 黃金** | 狀態欄(對齊真 U2 版面) |

### 五大時代(手冊 THE TIME MAP;oracle 由 map 首位數字定)
| 原文 | 譯名 | overworld | 備註 |
|---|---|---|---|
| Legends | **傳說時代** | mapx0x | 米娜克斯力量最強、9-9-9 所在 _Avoid_: 傳奇時代 |
| Pangea (9,000,000 B.C.) | **盤古大陸** | mapx1x | 地球未分裂的單一大陸 |
| B.C. (1423 B.C.) | **西元前 1423 年** | mapx2x | |
| A.D. (1990) | **西元 1990 年** | mapx3x | 「現在」,時光門地圖以此為底 |
| Aftermath (2112 A.D.) | **浩劫餘生** | mapx4x | 2111 大毀滅後 |

### 武器(READY 1–9,威力遞增;手冊 ARMS AND ARMOUR)
匕首 Dagger · 錘矛 Mace · 斧 Ax · 弓 Bow · 劍 Sword · 巨劍 Greatsword ·
**光劍 Light Sword**(_Avoid_: 輕劍)· 相位槍 Phaser · 迅捷之劍 Quicksword(Enilno,需 earn)

### 防具(WEAR 1–6)
布甲 Cloth · 皮甲 Leather · 鎖甲 Chain · 板甲 Plate · 反射甲 Reflect · 動力甲 Power(起始 Skin=赤身)

### 法術(9 種;手冊 MAGIC SPELLS)
光明 Light · 下梯 Ladder down · 上梯 Ladder up · 穿牆 Passwall · 返地表 Surface ·
祈禱 Prayer · 魔法飛彈 Magic Missile · 瞬移 Blink · 擊殺 Kill

### 載具與門檻道具(oracle 0x7390)
馬 Horse · 船 Frigate(需 藍流蘇 Blue Tassle)· 飛機 Plane(需 黃銅鈕扣 Brass Button)·
火箭 Rocket(需 生命符 Ankh 登艦 + 三鋰 Tri-lithium 燃料)

### 行星(手冊 GALACTIC MAP;Xeno/Yako/Zabo 三維座標)
水星 Mercury(5,4,5)· 金星 Venus(3,3,4)· 地球 Earth(6,6,6)· 火星 Mars(6,2,3)·
木星 Jupiter(1,3,4)· 土星 Saturn(2,8,5)· 天王星 Uranus(9,4,6)· 海王星 Neptune(4,0,5)· 冥王星 Pluto(0,1,4)

### 地點類型(ENTER;手冊)
村莊 Village · 城鎮 Town · 城堡 Castle · 地牢 Dungeon · 塔 Tower(= 倒置地牢)

### 關鍵人物
Father Antos **安托斯神父**(住行星,賜力場之戒)· Hotel California **加州旅館**(梗)

## UI 版面(對齊真實 U2,參考 ultima2.voyd.net / Codex)
- **viewport**:畫面上方地圖區(真 tile + monxNN 實體層)。
- **訊息列**:底部左側,指令回饋 + NPC 對話(`CMD:` → `指令:`)。
- **狀態欄**:底部右側,生命/食物/經驗/黃金。
- **調色盤**:Original(青樹/洋紅海)、Red(綠樹/紅磚)、Blue(綠樹/藍海);PoC 預設 **blue**(綠樹較順眼)。

## Flagged ambiguities(待釐清)
- `monsters` / `monxNN` / `player` 檔的欄位結構尚未本機驗證(目前 📖 文件推測)。
- CJK glyph 預設大小(16 vs 24)待 PoC 後定;ADR 0001 暫推薦 24(內部 3×)。
