# FM Towns 版 Ultima II 美術導入評估

> 研究日期:2026-06-05
> 研究人:Claude(協助 L.CY / anr2)
> 目的:評估能否把 **FM Towns 版 Ultima II** 的較高品質 tileset / 怪物圖導入 u2-cht 引擎,換掉目前 CGA/EGA 美術。
> 標記:[已證實]=多來源交叉佐證或硬體事實;[推測]=由相關案例/平台規格合理外推;[未知]=查無資料,需使用者補件或實機驗證。

---

## 0. TL;DR

| 問題 | 結論 |
|---|---|
| FM Towns U2 有沒有更好的美術? | **有**。1990 Fujitsu《Ultima Trilogy I II III》,三作共用一套**重畫的彩色 tileset**,hi-res(約 512×212),外加原創音樂。[已證實] |
| 美術規格 | tile **[推測]仍 16×16**;色彩 **[推測]16 色為主**(對齊 PC Ultima V 風格,見 U4 FM Towns 先例);畫面解析度 ~512×212。沒有逐 tile 規格文件。 |
| 有現成 ripped sprite sheet 嗎? | **沒有**。The Spriters Resource / VG-Resource 查無 FM Towns Ultima 任何作品的 sheet。[已證實:查無] |
| 拿得到原始資料嗎? | 整套 CD/disk image 在 archive.org(Neo Kobe FM Towns 合輯)。但**無 U2-specific 解包工具或圖檔格式文件**。[已證實 image 存在 / 未知 解包路徑] |
| tile id 能對上 DOS 版嗎? | **不能直接對應**。FM Towns 是獨立重製,tile 排列/編號與 DOS 不同,**必為手工建對應表**。[推測] |
| 導入可行性 | **中偏低**。風險集中在「無公開解包路徑」+「需逐 tile 人工對應」+「散布灰色地帶」。引擎側(塞 16×16 PNG)反而最簡單。 |
| 最務實路徑 | 不碰 FM Towns 二進位;**改用授權更乾淨、已是 PNG 的社群 tileset(現行 U2 Upgrade EGA / 或 xu4 風格)**;FM Towns 僅當「視覺風格參考」。 |

---

## 1. FM Towns U2 美術規格

### 1.1 出身與定位 [已證實]
- FM Towns 版 U2 = 1990 年 Fujitsu 在日本發行的 **《Ultima Trilogy I II III》** 合輯之一,**僅日本、僅 FM Towns 平台**,海外幾乎無人知。
- 三作(U1/U2/U3)**共用同一套重畫 tileset**;U2 是唯一**有配樂**的 U2 版本(原創音樂)。
- 另有一段日文 hi-res 開場動畫(無英文,與遊戲本體分離)。
- 玩法差異(與美術無關,但影響移植對齊):tri-lithium 只在地牢取得、藍流蘇每次登船消失。

### 1.2 視覺規格 [混合]
| 項目 | 值 | 證據等級 |
|---|---|---|
| 遊戲畫面解析度 | 約 **512×212**(社群描述「higher (512x212) resolution」) | [已證實:多來源同述] |
| 色彩 | 「much more colored」「hi-res, plenty of colour, a little cartoony」;overworld/town 觀感對齊 Ultima IV,dungeon 細節近 Ultima V | [已證實:定性];精確色深 [未知] |
| tile 尺寸 | **推測 16×16**(FM Towns 硬體 sprite 即 16×16;且同系列 U4 FM Towns in-game 圖是 PC Ultima V 16×16 tile 的複製) | [推測] |
| tile / sprite 色深 | **推測 16 色 / tile**(對齊 PC Ultima V EGA-style 16 色;FM Towns 用 mode overlay 把 16 色高解析 tile 疊在高彩背景上) | [推測] |
| 怪物圖 | U1 FM Towns「敵人完全重畫、用美術圖取代線框,有簡單動畫(揮手、噴火),但無完整行走循環」;U2 共用同套素材 | [已證實 U1,推測 U2 同] |
| 怪物 sprite 數量/尺寸 | [未知] —— 無 rip、無解包文件可數 |

### 1.3 FM Towns 硬體上限(可塞什麼)[已證實]
- 圖形模式 320×200 ～ 720×512;同屏 16 ～ 32,768 色(視模式)。
- **硬體 sprite:最多 1024 個,每個 16×16,256 色(來自 32,768 調色盤)。** ← 這強烈佐證 U2 角色/怪物以 16×16 sprite 呈現。
- mode overlay:可把「高彩低解析 tile 層」疊「640×480 16 色 kanji 文字層」—— 典型 FM Towns RPG 作法。

> 重點推論:FM Towns sprite 硬體本就是 **16×16**,與 u2-cht 現行 16×16 tile **幾何相容**。真正不確定的是「色深」與「tile 內容排列」,不是尺寸。

### 1.4 與 DOS CGA/EGA 版的視覺對比 [已證實:定性]
- DOS/Apple II/Atari:tile 黑底、用色稀疏、整體偏暗。
- FM Towns:全 tile 上色(草地綠、水藍…)、hi-res、卡通感,觀感跳到 Ultima IV/V 級別。
- 來源 blog(Pix's Origin Adventures)定性評價:「比我習慣的 U2 好看」,但因與 U1 共用太多素材,**史詩感反而下降**。

---

## 2. 資產可取得性

### 2.1 現成 sprite sheet / tileset [已證實:查無]
- **The Spriters Resource / VG-Resource:沒有任何 FM Towns Ultima(I/II/III)的 sheet。** 站上 Ultima 條目多為 NES / PC / mobile。
- 一般 ROM 站也查無「FM Towns U2 tileset PNG」。
- ⇒ **沒有可直接下載的 PNG sprite sheet**。要用就得自己從 disk image 抽。

### 2.2 原始 disk / CD image [已證實 存在]
- archive.org 有完整 FM Towns 合輯(如 *Neo Kobe Fujitsu FM Towns*、*Fujitsu FM-Towns CD Collection*),內含《Ultima Trilogy I II III》。
- 格式:CD image(.iso / .cue/.bin / .chd)或 HDD image。**體積大**(合輯動輒數十 GB;單作仍需從合輯取)。
- ⚠️ 本研究**未下載**(依限制:不取大型二進位)。

### 2.3 圖檔內部格式 / 解包工具 [未知 → 偏負面]
- **查無** U2(或 Trilogy)專屬的圖檔格式文件或解包工具。
- FM Towns 有泛用圖格式(如 **ICN / ICNFILE** icon 格式),但無證據顯示 U2 用它存 tile;遊戲很可能用**自家私有打包**(常見於當年日廠移植)。
- 對照組:**FM Towns U4** 的 in-game 圖經查是「PC Ultima V 16 色圖的低品質複製」——暗示這批日版移植的 in-game tile **不是全新繪製的高彩原畫,而是 PC 16 色素材的搬運/微調**。若 U2 同理,則「FM Towns 比 EGA 好多少」要打折。[推測,需實機驗證]

### 2.4 授權 / 散布現實 [事實陳述]
- U2 遊戲設計(1982)年代久遠、廣泛流傳;但 **FM Towns 版(1990)的重畫美術 + 原創音樂是 Fujitsu/Origin 1990 年的新創作**,著作權狀態比 1982 原版**新 8 年**,且為日本商業作品。
- 現實:abandonware 圈普遍流通,但**散布該版美術仍是灰色地帶**,比用「社群保存的 U2 Upgrade EGA tileset」風險高。
- 與 repo 既有裁示一致:**引擎與資料分離**,raw art 不入 repo,玩家自備。

---

## 3. 資料格式可行性(若硬要自抽)

| 步驟 | 工具 | 可行性 |
|---|---|---|
| 取得 disk image | archive.org 合輯 → 取 Trilogy | [已證實] 可得,但大 |
| 掛載 / 瀏覽 FM Towns 檔案系統 | MAME FM Towns driver、Diskedit(FM Towns HD 工具)、或 ScummVM/Tsugaru 模擬器 dump | [推測] 可行但繁瑣 |
| 定位 tile 圖檔 | 無文件 → 需在 image 內**盲掃**(找 16×16 規律的 bitmap 區塊) | [未知] **最大風險點** |
| 解碼 palette / bitplane | 需逆出 FM Towns 私有格式(色深、平面排列、palette 表位置) | [未知] 從零逆向,工程量「週」級 |
| 轉 PNG | 自寫 Python 解碼器(類比 repo 既有 `decode_u2upgrade_tiles.py` / U4 的 `ega.py`) | [已證實 流程模式可複用],但前提是先逆出格式 |

> 結論:**技術上可行,但屬「無文件的私有格式逆向專案」**,不是「跑現成工具」。投入產出比低 —— 為了換一套 tile,要先打通一條完整的 FM Towns 解包鏈。

---

## 4. 導入 u2-cht 的可行性評估

### 4.1 tile id 對應 [推測:需重建對應表]
- u2-cht 現行 tile id 來自 **DOS / U2 Upgrade** 編碼(`map byte ÷ 4`,id 0–64;0=water、2=grass、3=forest、4=mountain、5=town、16=person、17=horse、18=ship、12–15/60–63=怪物、32–57=A–Z 招牌字)。見 `docs/DATA_FORMATS.md`。
- FM Towns 是**獨立重製**,其內部 tile 順序/編號**不保證**與 DOS 一致(不同畫法、不同數量、可能多/少 tile)。
- ⇒ **必為手工建立 `FMTowns_tile_index → DOS_tile_id` 對應表**:把抽出的 FM Towns sheet 攤開,逐格人工指認語意(這格是水、這格是船…),映回 DOS id。
- 好消息:DOS tile 語意已在 repo 完整盤點(0–64 已命名),**對應表是「填空」而非「研究」**,但仍需逐格人工 + 視覺確認。

### 4.2 尺寸 / 色彩塞進引擎 [低風險]
- 引擎現況:16×16 tile、SDL2、整數倍 nearest-neighbor 放大。
- FM Towns sprite 本就 16×16 → **尺寸零障礙**。
- 色彩:FM Towns 16 色(或更高)PNG → SDL2 `SDL_image` 直接吃,**比現行 EGA bitplane 解碼還簡單**。
- 唯一要處理:若 FM Towns tile 比 16×16 大(例如 town/castle「zoomed in」可能是大圖非單 tile),需裁切或當特例處理。[未知:尺寸是否全 16×16]

### 4.3 怪物 sprite 對應 [推測:可對應但需人工]
- DOS 怪物 tile:id 12–15(lizardman/ghost/devil/balron)、60–63。
- FM Towns 怪物:重畫美術圖,有簡單動畫幀。
- 對應做法:同 4.1,人工把 FM Towns 怪物圖映到 DOS 怪物 id。
- ⚠️ **動畫幀**:FM Towns 怪物有多幀(揮手/噴火),DOS/現行引擎是**單張靜態 tile**。要嘛只取第一幀(放棄動畫),要嘛擴充引擎支援多幀 sprite(額外工)。

### 4.4 綜合可行性評等

| 面向 | 評等 | 說明 |
|---|---|---|
| 引擎側塞圖(16×16 PNG) | **高** | 尺寸相容、SDL2 直吃 |
| tile id 對應 | **中** | 必手工建表,但 DOS 語意已知,屬填空 |
| 取得可用 PNG 素材 | **低** | 無 rip,需自抽私有格式(無文件) |
| 散布授權 | **中低** | 1990 新美術,灰色地帶比 U2 Upgrade 高 |
| **整體** | **中偏低** | 卡在「拿到乾淨 PNG」這一關;一旦有 PNG,後段都不難 |

---

## 5. 建議導入路徑(務實 → 理想)

### 路徑 A（最務實,建議優先):FM Towns 僅當風格參考,改用授權乾淨的 PNG tileset
- **不碰** FM Towns 二進位。
- 維持現行 **U2 Upgrade EGA tileset**(已是可解碼、repo 已支援、授權較乾淨);若嫌不夠漂亮,挑選 **xu4 / Ultima IV-V 風格的社群 PNG tileset** 或自繪一套 16×16 彩色 tile,**對齊 FM Towns 的配色觀感**。
- 步驟:(1) 截幾張 FM Towns U2 畫面當美術 reference(Pix's Origin Adventures blog 有圖);(2) 用既有 `decode_u2upgrade_tiles.py` 管線產 sheet;(3) 視覺向 FM Towns 靠攏(上色、加底色)。
- 工具:現有 Python 解碼器 + SDL2 + 繪圖工具。
- 風險:**低**。產出可控、授權清楚、不依賴逆向。
- 代價:不是「真・FM Towns 像素」,是「FM Towns 風」。

### 路徑 B（中等):自抽 FM Towns tileset,半自動建對應表
- (1) archive.org 取 Trilogy disk image → MAME/Tsugaru 跑起來;
- (2) 從模擬器 VRAM dump 或 image 內盲掃定位 tile 區塊;
- (3) 逆出 FM Towns 圖格式(palette + bitplane)→ 自寫解碼器轉 PNG sheet;
- (4) 人工建 `FMTowns_index → DOS_id` 對應表;
- (5) PNG 餵引擎,引擎不改(除非要動畫幀)。
- 工具:MAME FM Towns driver / Tsugaru emulator、Diskedit、自寫 Python 解碼器。
- 風險:**高**(步驟 2-3 無文件,工程量週級;可能卡在私有格式)。
- 代價:能拿到真・FM Towns 像素,但投入大、授權灰。

### 路徑 C（理想但最重):完整 FM Towns 資產管線 + 引擎支援動畫
- 在 B 基礎上,額外**逆出怪物多幀動畫**並擴充引擎 sprite 動畫支援、處理 town/castle「zoomed-in」大圖特例。
- 風險:**最高**;只有在「就是要 100% 重現 FM Towns 觀感」時才值得。

> **裁示建議**:先走 **路徑 A**(風格參考 + 乾淨 PNG),把美術升級和「真 FM Towns 逆向」脫鉤。除非使用者明確要「就是要 FM Towns 原始像素」,否則 B/C 的逆向成本與授權風險不划算。

---

## 6. 還需要使用者提供 / 決策

1. **目標釐清**:要的是「**FM Towns 風格的更好看美術**」(→ 路徑 A,我能直接做),還是「**真・FM Towns 原始像素**」(→ 路徑 B/C,需逆向 + 授權考量)?
2. 若走 B/C:請使用者提供
   - (a) 一份《Ultima Trilogy I II III》FM Towns **disk/CD image** 連結或檔案(我不主動下大型二進位);
   - (b) 或任何已知的 **FM Towns U2 圖檔格式文件 / 解包工具**(目前查無)。
3. **授權底線確認**:是否接受 1990 FM Towns 新美術的散布灰色地帶?(現行 repo 對 U2 Upgrade 已採「引擎/資料分離、玩家自備」原則,可沿用。)
4. **動畫需求**:怪物是否要做多幀動畫?(影響是否要擴充引擎 sprite 系統。)

---

## 附錄:實際讀過的來源

- Pix's Origin Adventures — Ultima 2 FM Towns Part 1:https://www.pixsoriginadventures.co.uk/ultima-2-fm-towns-part-1/
- Pix's Origin Adventures — Ultima 2 FM Towns Part 2:https://www.pixsoriginadventures.co.uk/ultima-2-fm-towns-part-2/(系列僅到 part 2,無 part 3)
- The Codex of Ultima Wisdom — Computer ports of Ultima II:https://wiki.ultimacodex.com/wiki/Computer_ports_of_Ultima_II
- The Codex of Ultima Wisdom — Ultima Trilogy I II III:https://wiki.ultimacodex.com/wiki/Ultima_Trilogy_I_II_III
- Editable Codex (Fandom) — Computer Ports of Ultima II:https://ultima.fandom.com/wiki/Computer_Ports_of_Ultima_II
- Backlog RPGs — FM Towns Ultima 1:https://backlogrpgs.github.io/ultima1fmtowns/
- VOGONS — Ultima IV FM Towns Tileset Conversion for PC(對照:FM Towns U4 in-game = PC Ultima V 16 色圖複製):http://www.vogons.org/viewtopic.php?p=1355724
- FM Towns 硬體規格 — Wikipedia:https://en.wikipedia.org/wiki/FM_Towns(1024 sprite × 16×16、256 色、圖形模式 320×200–720×512)
- ICN (FM Towns) 圖格式 — Just Solve the File Format Problem(泛用 icon 格式,非 U2 專屬):http://justsolve.archiveteam.org/wiki/ICN_(FM_Towns)
- Internet Archive — Neo Kobe Fujitsu FM Towns / FM-Towns CD Collection(disk image 來源,未下載):https://archive.org/details/Neo_Kobe_Fujitsu_FM_Towns_2016-02-25
- The Spriters Resource(查證:無 FM Towns Ultima sheet):https://www.spriters-resource.com/

> 未能讀取(伺服器 403 / 連線拒絕):The Spriters Resource 站內搜尋頁、justsolve ICN 頁(改以搜尋結果摘要佐證)。
</content>
</invoke>
