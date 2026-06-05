# player 存檔測試 fixtures

兩份「已建角色」的 Ultima II player 存檔(各 384 byte),由 DOSBox headless 自動建角產生
(`tools/dosbox_create.sh` / `tools/dosbox_create2.sh`),用於 `tests/test_data.c` 的
存檔欄位回歸驗證。

| 檔案 | 角色 | 屬性(建角輸入) | 用途 |
|---|---|---|---|
| `player_sample_hero` | HERO,human/fighter/male | 全部 15 | 基準樣本 |
| `player_sample_abcd` | ABCD,elf/wizard/female | STR21 AGI11 STA12 CHA13 WIS14 INT19 | 差分樣本(各值互異,定位欄位) |

兩份 diff + 建角畫面截圖交叉比對 → 確定欄位 offset(見 `docs/DATA_FORMATS.md` 的 player 段)。

> 授權:Ultima II(1982)已過版權、廣泛流傳;此為自行產生的小型存檔,供逆向研究 / 回歸測試。
