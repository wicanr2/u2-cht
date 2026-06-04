# Ultima II 反組譯取得流程(可重複逆向管線)

> 目的:把「編譯好的 Win 移植 exe + DOS 原版資料」轉成「乾淨重寫 + 中文化所需的全部素材」的**可重複流程**。
> 重點是**流程方法**;CJK 繪製/字型 pipeline 已在 `u3-cht` / `u6-cht` 完成,本案直接複用,不重做。
> 全程 Docker(符合環境 [HARD] 規則),產物落在 `decompile/out/` 與 `extract/`。

---

## Phase A — 二進位分流(decompile 還是 reimplement?)

目的:判定 exe 性質,決定「忠實反編」vs「反編當 oracle + 乾淨重寫」。

```bash
file Ultima2.exe                    # PE32? DOS? 架構
objdump -h Ultima2.exe              # sections / 程式碼量
objdump -p Ultima2.exe | grep "DLL Name"   # 相依 → 移植目標 (GDI? DirectX?)
objdump -t Ultima2.exe              # 有無符號 (沒有符號 = stripped)
strings -n 4 Ultima2.exe | grep -iE "C[A-Z][a-z]+(Doc|View|App)"  # MFC class?
```

**本案判定**:PE32 / stripped / **MFC C++** / GDI → 反編出來是 Windows 殼 + 無型別 `FUN_xxx`。
**決策**:反編**當 oracle**,不照抄;用文件化資料格式 + 還原演算法**乾淨重寫**。
→ 判斷準則:stripped + 重框架(MFC/MFC-like)→ 選 oracle 路;有源碼/輕框架 → 可考慮忠實移植。

---

## Phase B — 反編成 Oracle(Ghidra headless / Docker)

```bash
docker pull blacktop/ghidra:latest            # 含 analyzeHeadless,Ghidra 12.1
# 自動分析 + import(第一次)
docker run --rm -v "$PWD":/work blacktop/ghidra \
  /ghidra/support/analyzeHeadless /work/decompile/proj U2 -import /work/Ultima2.exe
# 後處理反編匯出(專案已 import → 用 -process -noanalysis,快)
docker run --rm -v "$PWD":/work blacktop/ghidra \
  /ghidra/support/analyzeHeadless /work/decompile/proj U2 \
  -process Ultima2.exe -noanalysis \
  -postScript DecompileToC.java -scriptPath /work/decompile/scripts
```

**踩雷**:Ghidra **12.1 移除 Jython** → Python postScript 報 `Ghidra was not started with PyGhidra`。
**解法**:用 **Java** GhidraScript(`DecompileToC.java`,見 `decompile/scripts/`),原生可跑,無需 Python。

**產物**:
- `decompile/out/ultima2_decompiled.c` — 1190 函式反編 C(navigable oracle,**非**編譯標的)。
- `decompile/out/functions_index.tsv` — 函式名/位址/大小(1 失敗 `FUN_004102f2`)。

---

## Phase C — 字串錨定導航(把 FUN_xxx 對到功能)

stripped 反編無函式名 → 用**遊戲字串符號**反推每個函式做什麼:

```bash
# 每個含 ≥3 個具名字串 (s_XXX_addr) 的函式,列出引用字串 → 推測功能
awk '/^\/\* ===== FUN_/{cur=$3;strs="";n=0}
 {tmp=$0;while(match(tmp,/s_[A-Za-z0-9_]+_[0-9a-f]{8}/)){s=substr(tmp,RSTART,RLENGTH);tmp=substr(tmp,RSTART+RLENGTH);
   name=s;sub(/^s_/,"",name);sub(/_[0-9a-f]{8}$/,"",name);
   if(index(strs,"|"name"|")==0){strs=strs"|"name"|";n++}}}
 /^}$/{if(n>=3&&cur!=""){...printf...}}' ultima2_decompiled.c | sort -k2 -rn
```

**產物**:`oracle_string_map.txt` + `ORACLE_MAP.md`(landmark)。
**landmark**:`FUN_004064d0`=主命令派發、`FUN_004060a0`=狀態列、`FUN_0040b3d0`=戰鬥、`FUN_0041fc40`=DBCS 偵測。

---

## Phase D — 抽可中文化字串(兩來源)

### D1. exe 內嵌 UI 字串(vaddr → file offset → 讀 null-terminated)
section 表給對應:`file_off = vaddr - section.VMA + section.file_off`(`.rdata`/`.data`)。
讀到 `\x00` 為止,`cp1252` 解碼。過濾 format string / registry key / MFC class。

**產物**:`extract/exe_translatable_strings.tsv`(vaddr / category=GAME|SKIP / 原文 / zh_hant)。**392 條 GAME**。

### D2. 資料檔對話(tlkxNN,high-bit ASCII)
`byte & 0x7f` 清 bit7 → `\x00` 分段 → 過濾組譯殘渣(`PRBYTE/PRINT/$00`)。

**產物**:`extract/talk_dialogue.tsv`(file / index / 原文 / zh_hant)。**108 行**。

---

## Phase E — 資料格式真值(本機驗證 + 文件交叉比對)

```bash
xxd mapx00 | head            # 結構/header?
python3 -c "print(bytes(b&0x7f for b in open('tlkx32','rb').read()))"  # 解碼驗證
```
配合 shikadi ModdingWiki(map=4224B/64×66/tile÷4、dungeon=16×16、talk=high-bit)。
**產物**:`DATA_FORMATS.md`。

---

## Phase F — 乾淨重寫(下一階段)

1. 依 `ORACLE_MAP.md` landmark,逐 feature 從 oracle 還原演算法(亂數/機率對照原行為)。
2. SDL2 framebuffer + tile blit;資料用 Phase E 格式解析。
3. **文字層直接複用 `u3-cht`/`u6-cht` 的 CJK pipeline**(BDF→點陣字 / SDL_ttf + UTF-8 換行),套上 Phase D 兩張翻譯表。
4. 全程 Docker build;截圖 vs Alderson exe(Wine)當 pass/fail loop。

---

## 流程總表(輸入 → 工具 → 產物)

| Phase | 輸入 | 工具 | 產物 |
|---|---|---|---|
| A 分流 | Ultima2.exe | file/objdump/strings | 路線決策 |
| B 反編 | Ultima2.exe | Ghidra 12.1 (Docker, **Java** script) | `ultima2_decompiled.c` + index |
| C 導航 | 反編 C | awk 字串錨定 | `ORACLE_MAP.md` |
| D 抽字串 | exe + tlkx | python(section 映射 / high-bit 解碼) | 2 張翻譯 tsv |
| E 格式 | DOS 資料檔 | xxd/python + ModdingWiki | `DATA_FORMATS.md` |
| F 重寫 | 全部以上 | SDL2 + u3/u6 CJK pipeline | 中文化引擎 |
