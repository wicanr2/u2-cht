# 01 · 反編當 oracle

## 原則
反編出的東西**只當「原版實際怎麼算」的真值參考**,抽演算法 → 在乾淨引擎重寫;**絕不照抄** `FUN_xxx`(無型別、纏繞 MFC/runtime、不可維護)。
信心標 `[確定]/[推測]/[未解]`。把分析寫成 `docs/ORACLE_MECHANICS.md`,函式索引 `oracle/functions_index.tsv`、字串對應 `oracle/oracle_string_map.txt`。

## 取得可反編的 binary
- 優先找**官方授權重製版 / 移植版**的 binary(LairWare、Alderson Windows port…),通常結構比原 DOS exe 好反編。
- 反編產物(`oracle/*.c`,可能數萬行)可收進 repo 當「CRPG 史保存」,但原 binary 本身 gitignore。

## Ghidra headless(docker)
```dockerfile
FROM ubuntu:24.04
RUN apt-get update && apt-get install -y openjdk-21-jdk-headless wget unzip ca-certificates python3
RUN wget -q https://github.com/NationalSecurityAgency/ghidra/releases/download/Ghidra_11.1.2_build/ghidra_11.1.2_PUBLIC_20240709.zip -O /tmp/g.zip \
 && unzip -q /tmp/g.zip -d /opt && mv /opt/ghidra_* /opt/ghidra
ENV GHIDRA=/opt/ghidra
```
分析 + 跑後處理腳本(同一 docker run 內,因 `--rm` 容器無持久 project):
```
mkdir -p /tmp/proj
$GHIDRA/support/analyzeHeadless /tmp/proj proj -import <image.bin> \
  -processor x86:LE:32:default -loader BinaryLoader -loader-baseAddr 0x0 \
  -postScript find.py
```
- flat/raw binary 要自己定 base addr + processor。protected-mode .EXP 先解標頭(Phar Lap "MP":pages*512+mod512=size,header para×16=資料起點)再把「image 去標頭」匯入(base 0)。
- Jython(Py2)腳本**第一行加 `# -*- coding: utf-8 -*-`** 才能含中文。

## ⚠️ auto-analysis 進不了遊戲主碼
stripped binary 的遊戲邏輯多在 **indirect call / jump table** 後,Ghidra/capstone 從 entry 遞迴只覆蓋到 runtime(MetaWare/RUN386 startup、DOS int 21h wrapper)。
**破法 = 線索常數**:已知某機制會用某常數(例 CD-BIOS 中斷號 `0x93`、埠號、magic),decompile **全部函式**並 grep 該常數出現的函式 → 從那反查呼叫鏈。
Ghidra 腳本實用招:`DecompInterface` 反編每個 func → grep C 文字找線索;`getReferencesTo` 找呼叫端/寫入者;看 caller 的 `PUSH imm`/`MOV` 取常數參數。

## capstone(快速反組譯,免 Ghidra)
docker `pip install --break-system-packages capstone`。線性掃要**遇壞 byte 跳 1 續掃**(否則遇 data 即停)。注意:gap-skip 在 data 區會「out/in dx」誤判,要用 Ghidra 反編確認是真 code。

## 抽演算法的常見真值
- RNG:多為 LCG `seed = seed*0x343fd + 0x269ec3`。
- 戰鬥命中/傷害/掉落、狀態效果(麻痺/睡眠)計時器、亂數機率 → 抽公式後在引擎重寫,再用回歸對照。
- 用「字串錨定」導航:先在反編裡找已知文字(訊息、選單),回溯誰引用它 → 定位那段邏輯。
