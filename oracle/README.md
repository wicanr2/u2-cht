# Oracle — Ultima II 反編碼(行為真值參考)

本目錄收錄 **Ultima II** 的 Ghidra 反編 C,作為乾淨重寫引擎時的**行為/演算法參考 oracle**
(非編譯標的)。為保存 CRPG 歷史、供逆向研究與教育用途而公開。

## 內容
| 檔 | 說明 |
|---|---|
| `ultima2_decompiled.c` | 1190 函式反編 C(46k 行)。來源:John Alderson《Windows Native Ultima II》v1.01 (2000) PE32 / MFC,Ghidra 12.1 headless |
| `functions_index.tsv` | 函式名 / 進入位址 / 大小索引 |
| `oracle_string_map.txt` | 字串錨定函式導航(43 功能聚落) |
| `ORACLE_MAP.md` | landmark 函式(主命令派發 / 狀態列 / 戰鬥 / 字串出口) |

## 用途與限制
- **參考 oracle**:重寫子系統前 grep 對照原行為(亂數 / 機率 / 戰鬥公式 / 座標換算)。
- stripped → 函式皆 `FUN_<addr>`、變數 `DAT_<addr>`,無型別;MFC C++(`__thiscall`/CString/CDC)。
- **不可編譯、不是可用源碼**;僅作行為對照。

## 授權 / 用途
Ultima II(1982,Origin Systems)為年代久遠之作品;網路上已有大量公開的 U2 逆向資料。
本反編碼為**逆向研究、在地化與 CRPG 歷史保存**用途而收錄,不主張著作權。
若權利人有異議請聯繫移除。

> 衍生分析見 [`../docs/ORACLE_MAP.md`](../docs/ORACLE_MAP.md) 與 [`../docs/ORACLE_MECHANICS.md`](../docs/ORACLE_MECHANICS.md)(載具/地牢/戰鬥)。
