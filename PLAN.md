# Ultima II: Revenge of the Enchantress 中文化計畫

> 📍 **本檔為初期「中文化 + 引擎重寫策略」計畫(歷史)。** 試玩版已完成後,邁向
> **完整重製(可破關到結局)** 的前瞻路線見 [`docs/ROADMAP_REMAKE.md`](docs/ROADMAP_REMAKE.md)。

> 建立日期:2026-06-04
> 規劃人:Claude(協助 L.CY / anr2)
> 子專案:`u3-cht/u7-cht/WinUltima2`
> 方向定案(使用者確認):**反組譯 Alderson Windows 移植版當 oracle → 乾淨重寫 C 引擎 → SDL2 + CJK 中文化**,目標 **Linux/跨平台**,對齊 U3/U6 移植模式。

---

## 0. TL;DR

| 問題 | 結論 |
|---|---|
| 手上有什麼? | John Alderson《Windows Native Ultima II》v1.01(2000),301 KB **PE32 / MFC / GDI** 移植版 exe + 附加 planet maps(MAPG/MONG/TLKG)+ 字型文字檔(Font.txt)。 |
| exe 能直接反編成可用引擎嗎? | **不能直接用**。214 KB **stripped MFC** 機器碼,Ghidra 只能還原成無型別 `FUN_xxx`,與 MFC runtime 纏繞,清成可編譯引擎是 month++。 |
| 採用路徑? | **Ghidra 反編 C 當「行為 oracle」**(等同 U3 的 mcmagi 反組譯角色)→ 用文件化的 U2 資料格式 + 還原演算法,**手寫乾淨 C 引擎**。 |
| 繪圖/字型? | SDL2 framebuffer 取代 GDI;SDL_ttf / BDF 點陣字做 CJK;UTF-8 字串管線(沿用 U3/U6 經驗)。 |
| 中文化文字來源 | **兩處**:① exe 內嵌 UI 字串(`ENTER-DUNGEON`、`ATTACK--PARALIZED!`…);② 原版 U2 資料檔(地圖/城鎮/對話)。 |
| 還缺什麼? | 核心 U2 資料檔不在此目錄(readme:解壓到既有 U2 安裝);實機測試需從 Ultima Collection 補。 |
| 工程量 | 「月」級;風險低 —— 是「重寫小型且文件齊全的引擎」,非「逆向大型未知系統」。 |

---

## 1. 標的物盤點(本機驗證,2026-06-04)

### 1.1 Ultima2.exe
- 身分:**Windows Native Ultima II v1.01**,© 2000 John Alderson;readme 自述「quick and dirty port to Windows」。
- 技術:PE32 / i386 / Win32 GUI;`.text` 214 KB;**完全 stripped**(無符號、Debug Directory 空)。
- 架構:**MFC Document/View**(`CUltima2App`/`CUltima2Doc`/`CUltima2View`/`CPalette`)。
- 繪圖:**GDI32**(無 DirectX);可調 zoom(1X=320×200 整數倍)、三組 palette、Alternate Render。
- 相依 DLL:KERNEL32 / USER32 / GDI32 / comdlg32 / WINSPOOL / ADVAPI32 / SHELL32 / COMCTL32。
- 執行前提:**讀原版 U2 資料檔**(readme:需合法 U2,以 Ultima Collection CD 版測試)。

### 1.2 本目錄其他檔案
| 檔案 | 性質 | 用途 |
|---|---|---|
| `Font.txt`(15 KB) | 點陣字文字定義(每字 16-bit 寬 × 8 row,`0/1` 圖案) | Alderson 自製字型來源 → CJK 字型 pipeline 的對照基準 |
| `MAPG10/15/20/30/32/40/41/45` | 附加 planet 地圖(稀疏,開頭全 0) | readme 提到的 missing planet maps(by Mike Marcelais) |
| `MONG20/32/41`(256 B) | 對應 planet 的 monster 配置 | 同上 |
| `TLKG32/41`(256 B) | 對應 planet 的 talk/對話資料 | 同上(中文化對話來源之一) |

### 1.3 缺檔(實機測試前需補)
- 核心 U2 資料檔(世界地圖、城鎮、角色、原始字型等)—— 來自合法 Ultima II / Ultima Collection。
- 引擎與資料分離:中文化僅動「外部字串覆蓋層」,不改原始資料檔 offset(沿用 U3 原則)。

---

## 2. 採用路徑:oracle + 乾淨重寫

### 2.1 為何不直接用反編 C
- stripped → 函式皆 `FUN_<addr>`、變數 `DAT_<addr>`,無型別/結構。
- MFC → 大量 `CWnd`/`CDC`/message map dispatch,屬要剝掉的 Windows 殼,非遊戲邏輯。
- 反編 C 不可編譯、不可維護,作為**最終引擎**價值低;作為**演算法/行為真值**價值高。

### 2.2 三個資訊源(對照 U3 成功模式)
| 角色 | U3 對應 | U2 本案 |
|---|---|---|
| C 源碼基底 | `beastie/ultima3`(MIT) | **無** → 改為自寫 |
| 行為 oracle | mcmagi DOS 反組譯文件 | **Ghidra 反編 Ultima2.exe**(`decompile/out/ultima2_decompiled.c`) |
| 資料格式真值 | Ultima Codex wiki + `.ULT` | Ultima Codex wiki(U2 formats)+ 本機資料檔 |

### 2.3 反組譯產物(進行中)
- 工具:`blacktop/ghidra:12.1` headless(Docker,符合 [HARD] Docker 環境規則)。
- 輸出:
  - `decompile/out/ultima2_decompiled.c` —— 全函式反編 C(oracle,不編譯)。
  - `decompile/out/functions_index.tsv` —— 函式名/位址/大小索引(導航用)。
- 用法:重寫某子系統前,先 grep oracle 對照原行為(亂數、機率、戰鬥公式、座標換算)。

---

## 3. 目標引擎架構(deep modules / 垂直切片)

> 依個人規則 `70-deep-modules`:按 feature 切,不按抽象層攤平;adapter 只放邊界。

```
u2-engine/
  platform/            # 邊界 adapter:SDL2 window/event/timer/file
    sdl_backend.c
  render/              # framebuffer(取代 GDI):tile blit + 整數倍縮放
    framebuffer.c
    palette.c          # Original / Red / Blue 三組
  text/                # 中文化核心:UTF-8 + CJK-aware 換行/寬度
    font_cjk.c         # SDL_ttf 或 BDF atlas
    strings.c          # 字串表載入(exe 內嵌 + 資料檔覆蓋層)
  data/                # U2 資料檔解析(world/town/talk/roster/font)
  features/
    world/             # 大地圖移動、月相、載具
    town/              # 城鎮/城堡/塔
    dungeon/           # 3D 地牢
    space/             # 外太空/hyperwarp
    combat/            # 戰鬥、機率公式(對照 oracle)
    party/             # 角色、存檔
  main.c
```

中文化 hook point(收斂):所有繪字走 `text/` 單一出口;所有取字走 `strings.c` 單一查表 → 翻譯介面乾淨,全遊戲一次到位。

---

## 4. 工作拆解

1. **[進行中] Ghidra 反編 oracle** → `ultima2_decompiled.c` + 索引。
2. **抽 exe 內嵌字串** → 位址 + offset + 原文,整理可翻譯字串表。
3. **盤點 U2 資料格式** → world/town/talk/roster/font 格式文件 + 缺檔清單。
4. **垂直切片 PoC**(對齊 U3 建議的「三假設一次打掉」):
   Docker(SDL2 + SDL_image + SDL_ttf)下,讀一張 U2 地圖 tile + 用 CJK 字畫一句中文 UI → 驗證「繪圖換得掉、資料讀得到、中文畫得出」。當第一個決定性 pass/fail loop。
5. **逐子系統重寫**:world → town → combat → dungeon → space → party,每步對照 oracle 行為。
6. **中文化**:翻 exe 字串表 + 資料檔對話覆蓋層;字型 pipeline 複用 `qb64pe-game-linux-port` 的 BDF→點陣字。
7. **驗證**:全程 Docker build;逐畫面截圖比對(Alderson exe 在 Wine vs 重寫版)當 pass/fail loop;機率/公式對照 oracle。

---

## 5. 風險 / 待驗證

1. **🟠 反編品質**:stripped MFC 的反編可讀性待 §1 產物出爐後評估;部分函式可能反編失敗(已在索引標記 N)。
2. **🟠 缺核心資料檔**:實機測試前需補合法 U2 資料;在此之前以 oracle + 文件推導。
3. **🟡 亂數/機率還原**:readme 自承「DOS 亂數 + 混合數字格式」難 100% 還原 → 以 oracle 反編對照戰鬥/事件公式。
4. **🟡 字型寬度**:Font.txt 為單寬點陣;CJK 全形 2 倍寬 → 換行/寬度邏輯需 CJK-aware。
5. **🟡 資產授權**:exe 程式碼非開源、遊戲資料屬 EA IP → 「引擎與資料分離」散布原則,自用 OK。

---

## 6. 下一步

1. 等 Ghidra headless 完成 → 檢視 `functions_index.tsv` 規模與反編可讀性,挑出 world/text/combat 關鍵函式。
2. 抽 exe 內嵌字串表(獨立於反編,先行可做)。
3. 整理 U2 資料格式文件 + 缺檔清單。
4. 啟動垂直切片 PoC(Docker SDL2)。
