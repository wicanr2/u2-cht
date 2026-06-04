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
- **mapxNN** — 64×66 tile 地圖檔(4224 B,無 header)。尾碼 0=行星總圖、1/2/3=城鎮、4/5=地牢。
- **tlkxNN** — NPC 對話檔(384 B)。high-bit ASCII 編碼。
- **high-bit ASCII** — U2 文字編碼:每 byte `OR 0x80`;解碼 `byte & 0x7f`。`\r`(0x0d)為換行。 _Avoid_: 「加密」(ModdingWiki 稱 encrypted,實為 high-bit)
- **覆蓋層 (overlay)** — 中文化字串以 `(來源, key)` 為索引的外部 UTF-8 表,載入時覆蓋原文;**不寫回**原始資料檔(避免破壞 offset)。

## 渲染(見 ADR 0001)
- **像素圖層** — tile/sprite 層,16×16 來源整數倍放大(nearest 預設)。
- **文字圖層** — CJK/UI 層,內部高解析度原生繪製,不被縮放。
- **內部 render 解析度** — 320×200 × N(N 決定 CJK glyph 大小);與**視窗解析度**解耦。
- **CJK-aware 換行** — 全形字 2 倍寬的換行/寬度計算。

## 中文化文字兩來源
- **exe 字串表** — Alderson exe 內嵌 UI 字串(`translations/exe_translatable_strings.tsv`,vaddr 索引)。
- **對話表** — tlkx 解碼對話(`translations/talk_dialogue.tsv`,(檔名,index) 索引)。

## Flagged ambiguities(待釐清)
- `monsters` / `monxNN` / `player` 檔的欄位結構尚未本機驗證(目前 📖 文件推測)。
- CJK glyph 預設大小(16 vs 24)待 PoC 後定;ADR 0001 暫推薦 24(內部 3×)。
