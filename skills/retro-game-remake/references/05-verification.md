# 05 · 驗證(headless 確定性 + 可達性)

## Feedback loop 優先
動手前先建**快速、確定性、agent 可執行的 pass/fail 訊號**。引擎做 `--script` headless 模式:固定 seed、`SDL_VIDEODRIVER=dummy`、把每步 `g.msg` 印到 stdout(可 grep `[step NN] <msg>`),逐幀存 PNG。

## 三類回歸(都進 tests/,docker 內跑)
1. **資料層**(test_data):格式 decode 斷言 + fixture 存檔欄位。
2. **可破關鏈**(regression_winnable):一條固定指令字串走「建角→取關鍵道具→時空旅行→打 BOSS→結局」,grep `GAME WON`。
3. **正典玩法**(town_canon):城鎮/地牢/overworld 多情境(交談/商店/偷竊/守衛稅/寶箱陷阱/法術/擊殺給金…)各 grep 預期訊息。
- headless 用 debug hook(`I`發道具/`P`時間門/`O`強制進城/`&`生弱怪/`*`開箱…)製造確定性情境。

## ⚠️ debug hook 會遮住真 bug ── 一定要另驗「正常玩家路徑」
可破關回歸**全 PASS 不代表能正常玩**:它用 debug hook 繞過正常行走。
**真實案例**:回歸全過,但全新角色開局被 `find_start` 放在「只連城堡的 12 格小島」→ 村鎮全 on-foot 不可達、出不了島。
**對策 — 世界可達性分析(無 debug)**:
- 用 **flood-fill 連通分量**分析地圖(陸地 = 可通行 tile;水不可走)。
- 玩家**落點必在最大陸地分量(主大陸)**,且**城鎮 landmark 與落點同分量**(可步行到)。
- 船要放在**鄰接玩家陸地分量的水格**(否則登不上,B 永遠失敗)。
- 可用 Python 對真實地圖檔重現引擎的落點邏輯來驗(tile=byte÷4、passable=tile≠0、4-連通)。

## 其他驗證
- 行為對照 oracle:公式抽出後逐項對照(機率分佈、傷害、掉落)。
- 改 binary 行為可疑時:截圖逐幀比對當決定性 loop;對照反組譯。
- 互動視窗需顯示器;CI/headless 走 dummy driver + script。可用 LD_PRELOAD shim 驅動互動式選單做端到端測(別改 repo,測完還原)。
- headless 預設**不要寫回** `player_save`(會覆寫餵入的 fixture);要顯式 `--save` 才寫。
- 跑 game-tester 子代理做「認真對齊原版」的獨立驗證(背景)。
