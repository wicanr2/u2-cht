# Ultima2.exe 反編 Oracle 導航圖

> 來源:`ultima2_decompiled.c`(Ghidra 12.1,1190 函式反編,1 失敗 `FUN_004102f2`)。
> 用途:**乾淨重寫的行為真值 oracle**,非編譯標的。重寫某子系統前先 grep 對應 `FUN_` 對照原行為。
> 字串錨點清單見 `oracle_string_map.txt`(43 個含 ≥3 遊戲字串的函式)。

## 關鍵 landmark 函式

| 位址 | 推測功能 | 錨點字串 |
|---|---|---|
| `FUN_004064d0` (9626 B) | **主命令派發**(移動/攻擊/進入/hyper) | WEST/NORTH/EAST/SOUTH _INVALID_MOVE_ / _PARALIZED_, TURN_LEFT, ADVANCE, ENTER_* |
| `FUN_004060a0` | **狀態列繪製** | H_P_, FOOD_, EXP, GOLD, FUEL, XENO/YAKO/ZABO |
| `FUN_0040b3d0` | **戰鬥判定** | __MISS, __HIT___, SHE_S_GONE, KILLED__GOLD/EXP |
| `FUN_00427624` (94 B) | MFC `CString` 賦值(`lstrlenA`+copy,`__thiscall`) | — |
| `FUN_0041fc40` | **DBCS Windows 偵測**:讀 `win.ini [windows] kanjimenu/hangeulmenu`,調版面高度 0x1e→0x1f。**非 CJK 渲染引擎**,僅版面餘裕。 | windows, kanjimenu, kanji, hangeulmenu, hangeul, roman |

## 文字/字型機制(中文化核心)
- 字型:執行時從 **`Font.txt`** 載入點陣字(每字 16-bit 寬 × 8 row),經 GDI 繪製;`CFont` 物件。
- 中文化 hook:重寫時把「Font.txt 點陣字 + GDI 繪字」單一出口換成 SDL_ttf / BDF CJK atlas + UTF-8。
- 注意:原 exe 已預留 DBCS 版面(0x1f 高度),但無真正 CJK glyph 渲染 → 需自建。

## 子系統聚落(節錄,完整見 oracle_string_map.txt)
- 武器店 / NPC 對話 / 角色生成 / 標題畫面 / 深太空+行星軌道 / 地牢法術(LIGHT/PASSWALL/MAGIC_MISSILE/BLINK)/ 道具(STAFF/BOOTS/RED_GEM…)/ 結局(MINAX_IS_DEAD…)/ 招牌時代(ANOS 9,000,000 B.C.…)。

## 已知反編限制
- stripped → 全 `FUN_<addr>` / `DAT_<addr>`,無型別/結構名。
- MFC C++(`__thiscall`/CString/CDC)→ 大量 Windows 殼,**只取演算法,不照抄**。
