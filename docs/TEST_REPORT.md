# Ultima II 繁中重製 — 測試報告

> 對象:u2-cht SDL2 重寫引擎(正式版)
> 日期:2026-06-11
> 環境:全程 Docker 容器(`u2cht-build` / `u2cht-test`),不污染主機。
> 方法:headless 截圖回歸(`--script` / `--screens`)+ 容器內互動測試(Xvfb + xdotool + ffmpeg)。

## 0. Demo 影片

完整流程(原版標題 → 選單 → 建角 → 進城 → 交談 → FM Towns 畫風 → F1):

![demo](demo/u2cht_demo.gif)

> 高畫質版:[`demo/u2cht_demo.mp4`](demo/u2cht_demo.mp4)。影片不含私人開場圖(未帶 `--splash`)。

## 1. 測試環境

| 項目 | 內容 |
|---|---|
| 編譯 | `u2cht-build`(ubuntu 24.04 + SDL2/ttf/image + wqy-zenhei),CMake Release |
| 互動測試 | `u2cht-test`(延伸 + Xvfb / xdotool / ffmpeg / x11-utils) |
| 離屏驗證 | `SDL_Init(0)` + `--script <CMDS> <prefix>`,固定 LCG seed(rng=1)→ 決定性可重現 |
| 互動驗證 | `tools/interactive_test.sh`:Xvfb 開真實視窗 → xdotool 送鍵 → ffmpeg 擷圖 |
| 影片 | `tools/record_demo.sh`:連續錄製完整流程 → `docs/demo/u2cht_demo.{mp4,gif}` |

## 2. 測試案例與結果

| # | 案例 | 方法 | 結果 | 證據 |
|---|---|---|---|---|
| T1 | 開場序列(原版標題 → 選單) | 互動 | ✅ 原版 FM Towns 標題顯示;選單依存檔狀態顯示「新遊戲/試玩範例/離開」 | `test_evidence/01_menu.png` |
| T2 | **建立新角色** | 互動 | ✅ 姓名輸入 → 性別 → 種族 → 職業 → 屬性分配 5 步完整;鍵盤輸入正確 | `test_evidence/03_name.png`、`test_evidence/07_stats.png` |
| T3 | 新角色進遊戲 | 互動 | ✅ 起點落在城堡入口南側(城旁),載具(船)放相鄰水域 | `test_evidence/08_game_start.png` |
| T4 | **主角進城** | 互動 | ✅ 按 ↑ 往北踏上城堡 landmark → 「你進入了城鎮」(mapx31:REST ROOM、NPC、旅店) | `test_evidence/09_in_town.png` |
| T5 | 存檔寫回 | headless + 互動 | ✅ 離開自動存檔至可寫目錄(`SDL_GetPrefPath`);`store(load(x))` 位元組無損 round-trip | game stdout「已存檔」 |
| T6 | 建角存檔正確性 | headless `--screens` | ✅ 建出角色重載:name/sex/race/class/stats/hp/gold 全部一致 | `--screens` stdout |
| T7 | 進城(各 landmark) | headless `--script` | ✅ 世界圖 6 個 landmark(id 5/6/7/8/10)各通往有 NPC 的城;不再只認單一 id | `game_step_*.png` |
| T8 | FM Towns 主角隨畫風切換 | headless + 互動 | ✅ G 切到 fmtowns,主角 avatar 變 FM Towns 騎士(tile16 雜湊與 EGA 不同) | `fmp_01.png` |
| T9 | F1 指令表 | 互動 | ✅ overlay 顯示全部操作 | demo 影片 |
| T10 | 城鎮 NPC 交談 | 互動 | ✅ T 鍵觸發對話(繁中譯文覆蓋層) | demo 影片 |

## 3. 重點修正驗證(本輪)

- **進城 bug**:世界圖 landmark 只認單一 id → 玩家那塊陸地的城進不去。已修為「全 landmark 可進、各對應有對話的城」+「起點落城旁」。T4/T7 通過。
- **FM Towns 主角不隨畫風變**:strip 只覆寫地形。已補主角 avatar(從模擬器乾淨截圖 rip)。T8 通過。
- **AppImage 內資料唯讀無法存檔**:改用 `SDL_GetPrefPath` 可寫目錄。T5 通過。

## 4. 已知限制(誠實揭露)

- FM Towns sprite 目前僅地形 + 主角為乾淨來源;**船 / 怪物未換**(需航行/戰鬥場景的乾淨模擬器截圖,既有怪物 sheet 為雜訊較重的 raw atlas 解碼,不採用)。
- 本報告為早期驗證範圍快照;主線/結局/城鎮玩法後續已補齊,可端到端破關(見 `tests/regression_winnable.sh`、`docs/GUIDE.md`)。
- 互動測試的「進城」走位依賴固定起點;不同世界圖需重新確認 landmark 佈局。

## 5. 重現方式

```bash
# headless 回歸(逐畫面 PNG)
./build_game.sh ../dos-original/ultima2/mapx20 NTX build/reg_

# 互動測試(容器內:新遊戲→建角→進城,逐階段擷圖)
docker build -t u2cht-test -f docker/Dockerfile.test docker/
docker run --rm -v "$PWD":/work -v <ultima2資料>:/data:ro \
    u2cht-test bash /work/tools/interactive_test.sh   # → build/itest/*.png

# 錄製 demo 影片
docker run --rm -v "$PWD":/work -v <ultima2資料>:/data:ro \
    u2cht-test bash /work/tools/record_demo.sh        # → build/demo/u2cht_demo.{mp4,gif}
```

> 註:互動 / 錄影流程**不帶 `--splash`**,公開 repo 影片不含私人開場圖。
