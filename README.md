# Ultima II: Revenge of the Enchantress 繁體中文化專案

> 把 1983 年的《創世紀 II:女巫的復仇》以**乾淨重寫的跨平台 C 引擎**重建,並完整中文化。
> 系列姊妹作:[u3-cht](https://github.com/wicanr2/u3-cht)(Ultima III)、[u6-cht](https://github.com/wicanr2/u6-cht)(Ultima VI)。

---

## 目錄

- [專案現況](#專案現況)
- [策略:反編當 oracle,乾淨重寫](#策略反編當-oracle乾淨重寫)
- [取得流程概覽](#取得流程概覽)
- [中文化文字兩來源](#中文化文字兩來源)
- [渲染與 CJK 策略](#渲染與-cjk-策略)
- [專案結構](#專案結構)
- [快速開始](#快速開始)
- [路線圖](#路線圖)
- [授權與免責](#授權與免責)
- [致謝](#致謝)

---

## 專案現況

目前處於 **逆向取得 + 格式文件化** 階段(Phase A–E 完成),引擎重寫(Phase F)尚未開始。

| 段落 | 狀態 | 產物 |
|---|---|---|
| 二進位分流 | ✅ | PE32 / stripped / MFC / GDI 判定 |
| 反編 oracle | ✅ | 1190 函式反編 C(不公開)+ 函式索引 |
| 字串錨定導航 | ✅ | [`docs/ORACLE_MAP.md`](docs/ORACLE_MAP.md) |
| 抽可中文化字串 | ✅ | exe 392 條 + 對話 108 行 |
| 資料格式盤點 | ✅ | [`docs/DATA_FORMATS.md`](docs/DATA_FORMATS.md) |
| 渲染解析度決策 | ✅ | [`docs/adr/0001-rendering-resolution-cjk.md`](docs/adr/0001-rendering-resolution-cjk.md) |
| 垂直切片 PoC | ✅ | [`docs/POC.md`](docs/POC.md)(地圖 + 中文一次到位) |
| SDL2 引擎重寫 | ⏳ | `src/`(PoC 骨架已就緒,feature 模組規劃中) |

---

## 策略:反編當 oracle,乾淨重寫

手上的標的是 John Alderson《Windows Native Ultima II》v1.01 (2000) —— 一個
**stripped 的 MFC / GDI** Win 移植 exe。直接把它反編成可用引擎是最硬的路
(無型別 `FUN_xxx`、纏繞 MFC runtime)。

因此採與 [u3-cht](https://github.com/wicanr2/u3-cht) 相同的成功模式:

```
Ghidra 反編 C  ──(只當行為/演算法 oracle,不照抄)──┐
文件化 U2 資料格式 ─────────────────────────────────┼──▶ 手寫乾淨 SDL2 C 引擎
原版資料檔(玩家自備)───────────────────────────────┘     (可公開、可維護、好中文化)
```

- **oracle**:`Ultima2.exe` 的 Ghidra 反編,僅作行為對照(亂數/機率/戰鬥公式),**不公開**(衍生物)。
- **乾淨重寫**:自寫的 `src/`,SDL2 取代 GDI,UTF-8 + CJK 字型,**可公開上 GitHub**。

詳見 [`PLAN.md`](PLAN.md)。

---

## 取得流程概覽

可重複的逆向取得管線(完整見 [`docs/EXTRACTION_PROCESS.md`](docs/EXTRACTION_PROCESS.md)):

| Phase | 工具 | 產物 |
|---|---|---|
| A 分流 | `file` / `objdump` / `strings` | 路線決策 |
| B 反編 | Ghidra 12.1 headless(Docker,Java script) | 反編 C + 函式索引 |
| C 導航 | awk 字串錨定 | `docs/ORACLE_MAP.md` |
| D 抽字串 | [`tools/extract_exe_strings.py`](tools/extract_exe_strings.py) / [`tools/decode_talk.py`](tools/decode_talk.py) | 2 張翻譯表 |
| E 格式 | `xxd` / python + ModdingWiki | `docs/DATA_FORMATS.md` |
| F 重寫 | SDL2 + u6-cht CJK pipeline | 中文化引擎(待做) |

> 全程 Docker;CJK 字型 pipeline 直接複用 u6-cht 的 Big5/BDF 工具,不重造輪子。

---

## 中文化文字兩來源

| 來源 | 數量 | 已譯 | 翻譯表 |
|---|---|---|---|
| Alderson exe 內嵌 UI 字串 | 392 條(可譯 369) | **369/369** ✅ | [`translations/exe_translatable_strings.tsv`](translations/exe_translatable_strings.tsv) |
| DOS `tlkx` NPC 對話 | 108 行 | **78/108**(餘為 buffer 殘片) | [`translations/talk_dialogue.tsv`](translations/talk_dialogue.tsv) |

`zh_hant` 欄已填繁中(依 [CONTEXT.md](CONTEXT.md) 專名詞表)。原則:**不寫回原始檔**,以外部 UTF-8 覆蓋層載入時覆蓋。譯文由 [`tools/apply_translations.py`](tools/apply_translations.py) / [`tools/apply_dialogue.py`](tools/apply_dialogue.py) 套用。

---

## 渲染與 CJK 策略

為了讓中文字清晰塞入,採**雙層渲染 + 內外解析度解耦**(完整見 [ADR 0001](docs/adr/0001-rendering-resolution-cjk.md)):

- **像素圖層**:原版 16×16 tile 整數倍放大(nearest 預設,bilinear/bicubic 選項)。
- **文字圖層**:CJK glyph 在內部高解析度**原生繪製**,永不被縮放 → 恆為銳利。
- **內部 render 解析度** 320×200×N 決定 glyph 大小(推薦 3× = 960×600,CJK 24×24);**視窗解析度**(640×480 等)獨立,present 階段再縮放。

---

## 專案結構

```
u2-cht/
  README.md / PLAN.md / CONTEXT.md / LICENSE
  docs/
    EXTRACTION_PROCESS.md   # 取得流程 methodology
    DATA_FORMATS.md         # U2 資料格式真值
    ORACLE_MAP.md           # 反編 landmark 導航
    functions_index.tsv / oracle_string_map.txt
    adr/0001-rendering-resolution-cjk.md
  translations/             # 兩張翻譯表 (原文 + zh_hant)
  tools/                    # 抽取/反編 script
  src/                      # 乾淨重寫引擎 (規劃中)
```

> 原版 binary / 資料檔 / 反編 oracle C 皆 **gitignore**(版權物,玩家自備)。

---

## 快速開始

目前無可執行引擎(重寫未開始)。重現逆向取得流程:

```bash
# 1. 自備合法 Ultima II(DOS 原版 + Alderson Win port)放到工作目錄
# 2. 抽 exe 內嵌字串
python3 tools/extract_exe_strings.py Ultima2.exe > translations/exe_translatable_strings.tsv
# 3. 解碼 NPC 對話
python3 tools/decode_talk.py path/to/ultima2/ > translations/talk_dialogue.tsv
# 4. (選用) Ghidra 反編當 oracle,見 docs/EXTRACTION_PROCESS.md Phase B
```

---

## 路線圖

- [x] 逆向取得 + 格式文件化(Phase A–E)
- [x] 渲染/CJK 解析度決策(ADR 0001)
- [x] **垂直切片 PoC — 三假設一次驗證**([docs/POC.md](docs/POC.md))✅
- [x] **真實 tile 美術抽取(terrain id 0–31,CGA 2bpp @0x7C42,已驗證對齊)**✅
- [x] **font 子區塊查清(bit-packed proportional,招牌走翻譯不需抽)**✅
- [ ] SDL2 引擎骨架(deep modules:world / town / dungeon / space / combat / party / text)
- [ ] NPC/怪物動態層解析(疊在地形上)
- [ ] CJK 文字層升級(複用 u6-cht 點陣字 pipeline)
- [x] **翻譯 UI 字串(369/369)+ NPC 對話(78/108)** ✅
- [x] **隊伍移動(置中/鏡頭跟隨/海洋碰撞,headless 驗證)** ✅([docs/MOVEMENT.md](docs/MOVEMENT.md))
- [x] **端到端在地化(引擎讀原始 tlkx → 翻譯覆蓋層 → CJK)** ✅
- [ ] 互動視窗(SDL 開窗 + 鍵盤)、player 存檔解析、載具/地牢/戰鬥
- [ ] 行為對照 oracle(亂數/機率/戰鬥公式)
- [ ] 截圖 vs Alderson exe(Wine)pass/fail loop

---

## 授權與免責

「引擎與資料分離」:本專案原創碼/工具/文件採 **MIT**;原版遊戲資料與 Alderson binary
**不包含、不散布**,玩家須自備合法副本。詳見 [`LICENSE`](LICENSE)。

非商業之保存與在地化研究。著作權人如有異議請聯繫移除。

---

## 致謝

- **Origin Systems / Richard Garriott (Lord British)** — Ultima II 原作(1983)。
- **John Alderson** — Windows Native Ultima II 移植版(2000),本案行為 oracle 來源。
- **TheAlmightyGuru / shikadi ModdingWiki** — U2 資料格式逆向文件。
- **姊妹專案** [u3-cht](https://github.com/wicanr2/u3-cht) / [u6-cht](https://github.com/wicanr2/u6-cht) — CJK pipeline 與移植經驗。
