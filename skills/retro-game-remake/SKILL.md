---
name: retro-game-remake
description: 把 1980s–90s 老遊戲(尤其 CRPG)逆向工程 + 乾淨重寫成跨平台 C/SDL2 引擎 + 繁中化的完整方法論。核心策略「反編當 oracle,不照抄」:Ghidra/capstone 反編原版當行為真值,破解原版資料格式,手寫可維護的乾淨引擎,挖出各版本(含 FM Towns 日版)美術/CD音樂/音效,headless 驗證可破關,Docker 跨平台打包(AppImage/Windows/Mac GitHub Actions),引擎與版權資料分離。觸發條件:使用者要「重製/移植/中文化某老遊戲」、「反組譯遊戲執行檔」、「破解老遊戲資料格式」、「抽取 FM Towns/DOS 遊戲美術或音樂」、「把老遊戲做成跨平台可玩」、或接續 u2-cht/u3-cht/u6-cht/opendw 這類重製專案。本 skill 階層式:先讀本檔總覽 + 踩雷,需要某階段細節再讀 references/ 下對應檔。基於 Ultima II 繁中重製(u2-cht, 2026-06)完整經驗。
---

# 老遊戲逆向 + 乾淨重製 Skill(階層式)

> **怎麼用這個 skill**:先讀本檔(總覽 + 決策 + **踩雷**)。實際做某一階段時,**才**去讀 `references/` 下對應檔取細節 ── 避免一次載入全部。

## 何時啟用
重製 / 移植 / 中文化 1980s–90s 老遊戲(CRPG 為主);反組譯遊戲 binary;破解資料格式;抽取美術/音樂/音效;做跨平台可玩版本。

## 核心策略:反編當 oracle,乾淨重寫
直接把反編出的 `FUN_xxx`(纏繞 runtime、無型別)當引擎是死路。改採:

```
Ghidra 反編原版 binary ──(只當「行為真值 oracle」,抽演算法,不照抄 runtime 殼)─┐
破解原版資料格式(地圖/對話/存檔/sprite)──────────────────────────────┼─▶ 手寫乾淨 SDL2 C 引擎
原版資料檔(玩家自備,不散布)─────────────────────────────────────────┘    (deep modules,可公開可維護好中文化)
```

## 七階段流程(各階段細節見 references/)
| 階段 | 做什麼 | 細節檔 |
|---|---|---|
| 1. 反編當 oracle | 取可反編 binary;Ghidra/capstone 設定;字串錨定找函式;抽 RNG/戰鬥/移動演算法 | `references/01-decompile-oracle.md` |
| 2. 破解資料格式 | 地圖/實體/對話/存檔格式;DOSBox 差分驗證 | `references/02-data-formats.md` |
| 3. 美術/音訊考古 | 各版本 tileset/sprite、FM Towns TIF、CD 音樂、音效抽取 | `references/03-asset-archaeology.md` |
| 4. 乾淨引擎 + 中文化 | deep modules 垂直切片;CJK 雙層渲染;UTF-8 覆蓋層翻譯 | `references/04-engine-localization.md` |
| 5. 驗證 | headless 確定性回歸;可破關鏈;**正常玩法可達性** | `references/05-verification.md` |
| 6. 打包 | Docker first;引擎/資料分離;AppImage/Windows/Mac CI | `references/06-packaging.md` |
| 7. 攻略/文件 | 玩家向 README + 工程文件分離;繁中攻略 | (見各 repo;README 玩家向、ENGINEERING.md 技術) |

## ⚠️ 最痛的踩雷(這些用時間換來的,務必記住)

1. **debug hook 會遮住真 bug**。可破關回歸測試常用 debug hook(發全道具/瞬移/強制進城)繞過正常行走 → **測得過卻不能正常玩**。
   - 真實案例:回歸全 PASS,但全新角色開局被放在「只連城堡的 12 格小島」soft-lock,進不了任何城鎮。
   - 對策:**一定要另外驗「無 debug 的正常玩家路徑」**。世界可達性用**連通分量(flood-fill)分析**:落點必在最大陸地分量、且城鎮與落點同分量(可步行到)。船要放在「鄰接玩家陸地分量的水格」才登得上。
2. **反編 auto-analysis 進不了遊戲主碼**。stripped binary 的遊戲邏輯多在 indirect call / jump table 後,Ghidra 自動分析只覆蓋到 runtime。**靠「線索常數」跳進去**(例:CD-BIOS 中斷號 0x93 當 data 出現 → 從那反查播放鏈)。
3. **打包要帶「全部」資料,不要只帶 demo 子集**。只帶幾張地圖 → 玩家「進不了城鎮/找不到資料」。
4. **不要打包測試角色存檔**:玩家會看到「滿狀態怪角色」;開局該走建角流程。headless 預設**不要寫回** `player_save`(會覆寫餵入的 fixture)。
5. **引擎與版權資料分離**:原版 binary/資料/美術/音樂一律 gitignore、不散布;公開包只給引擎,玩家自備合法副本。
6. **全程 Docker build**(不污染系統);Python 用 docker 的 uv.venv/容器。
7. **FM Towns 是素材富礦但有陷阱**:TIF 是 FillOrder=2(LSB-first);CDDA 走 INT 93h 但**經 RUN386 反射**(0x93 是 data 不是字面指令);遊玩音樂是 EUP(非 CDDA);.SND 是 sign-magnitude PCM。細節見 `03-asset-archaeology.md`。

## 姊妹專案(同一套 RE+乾淨重寫方法論)
- `u2-cht`(Ultima II)— 本 skill 主要來源 · `u3-cht`(Ultima III)· `u6-cht`(Ultima VI)· `opendw`(Dragon Wars)。
- 註:QB64/BASIC 老遊戲移植是**另一套**做法(不反編、直接跨編 .bas),見 `qb64pe-game-linux-port` skill,與本 skill 無關。
