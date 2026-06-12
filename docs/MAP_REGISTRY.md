# Ultima II — 地圖角色登記表(MAP_REGISTRY)草稿

> M1 起手:掃描玩家自備 `mapxNN`(33 個,皆 64×66)。分類為**資料驅動推定**
> (地形比例 + landmark + NPC/對話);精確時代/行星歸屬待對照 oracle 座標 + Codex。

> 圖例:landmark tile id 5–10 = 城/塔/堡/地牢入口。原版資料檔不入庫,本表僅統計特徵。

| map | 水% | landmark(id@x,y) | NPC | 對話 | 對話主題(節錄) | 推定角色 |
|---|---|---|---|---|---|---|
| mapx00 | 52 | 8@32,32 10@32,54 | Y | - |  | 待確認 |
| mapx03 | 1 | — | Y | Y | THE SHE CREATURE BEGS: TAKE ME I'M YOURS! | 城鎮(有對話) |
| mapx10 | 60 | 5@20,55 10@33,58 | Y | - |  | 待確認 |
| mapx11 | 4 | — | Y | Y | AN ASTRONAUT CLAIMS: THERE IS A PLANET 'X'! | 城鎮(有對話) |
| mapx15 | 43 | 8@59,2 | Y | - |  | 待確認 |
| mapx20 | 67 | 5@38,46 6@36,29 7@24,40 8@35,22 9@25,5 10@58,51 | Y | - |  | overworld / 大地圖 |
| mapx21 | 0 | — | Y | Y | GRENDEL THE BUM SAYS: MASTERS OF RIDDLE ARE MAST | 城鎮(有對話) |
| mapx22 | 4 | — | Y | Y | ITHILIAN THE WENCH CALLS: MAKE ME AN OFFER SAILO | 城鎮(有對話) |
| mapx23 | 17 | — | Y | Y | BROTHER ANTOS ORDAINS: SEARCH THE STARS FOR MY K | 城鎮(有對話) |
| mapx24 | 41 | 8@21,1 | Y | - |  | 待確認 |
| mapx25 | 46 | 8@59,2 | Y | - |  | 待確認 |
| mapx30 | 67 | 5@34,20 6@15,26 7@38,31 8@35,22 9@25,5 10@58,51 | Y | - |  | overworld / 大地圖 |
| mapx31 | 15 | — | Y | Y | LADY SHERRIE CRIES: GAG ME WITH A SPOON! | 城鎮(有對話) |
| mapx32 | 12 | — | Y | Y | SANTRE THE SWASHBUCKLER WARNS:BEWARE, I'VE A QUI | 城鎮(有對話) |
| mapx33 | 17 | — | Y | Y | BROTHER ANTOS INQUIRES: HAVE YOU FOUND MY FATHER | 城鎮(有對話) |
| mapx34 | 25 | 8@21,1 | Y | - |  | 待確認 |
| mapx35 | 46 | 8@59,2 | Y | - |  | 待確認 |
| mapx40 | 70 | 5@44,14 9@25,5 10@58,51 | Y | - |  | overworld / 大地圖 |
| mapx41 | 12 | — | Y | Y | WAREN BEATTY ASKS: HAVE YOU SEEN DIANE KEATON? | 城鎮(有對話) |
| mapx44 | 34 | 8@21,1 | Y | - |  | 待確認 |
| mapx45 | 46 | 8@59,2 | Y | - |  | 待確認 |
| mapx50 | 52 | — | Y | - |  | 待確認 |
| mapx60 | 2 | 5@27,39 10@62,62 | Y | - |  | 待確認 |
| mapx61 | 5 | — | Y | Y | THE SWAMP JESTER LAUGHS: YOU LOSE, CADET! | 城鎮(有對話) |
| mapx70 | 0 | 5@31,31 | Y | - |  | 地點 / 地牢 / 城堡 |
| mapx71 | 3 | — | Y | Y | DEBBIE THE LIFEGUARD YELLS: GET A BIG GRIP! | 城鎮(有對話) |
| mapx80 | 2 | 5@56,59 6@38,32 9@29,33 | Y | - |  | 待確認 |
| mapx81 | 2 | — | Y | Y | MARGOT TOMMERVIK EXCLAIMS: TALK SOFTLY TO ME! | 城鎮(有對話) |
| mapx82 | 3 | — | Y | Y | THE SEAWORTHY PIRATE SAYS: SEE THE CLERK IN NEW  | 城鎮(有對話) |
| mapx85 | 59 | 8@55,1 | Y | - |  | 待確認 |
| mapx90 | 48 | 6@8,31 8@12,12 | Y | - |  | 待確認 |
| mapx92 | 9 | — | Y | Y | UGLY IRVING STATES: NO MAGES ALLOWED! | 城鎮(有對話) |
| mapx93 | 2 | — | Y | Y | FATHER ANTOS CHANTS: YOU HAVE EARNED MY BLESSING | 城鎮(有對話) |

## 參考:oracle 行星座標

EARTH 6,6,6 · Mercury 5,4,5 · Venus 3,3,4 · Mars 6,2,3 · Jupiter 1,3,4 · Saturn 2,8,5 · Uranus 9,4,6 · Neptune 4,0,5 · Pluto 0,1,4。

## ★ Oracle 校正:時代 = map 檔名首位數字(高信度,已釘死)

`FUN_0040c270`(ENTER THE SIGN READS / ANOS …)以 **map 名首位數字** switch 決定時代:

| 首位 | 時代 | overworld | 該時代城鎮/地點(mapxNN) |
|---|---|---|---|
| 0 | **Legends(傳奇時代)** ← Minax 在此 9-9-9 | mapx00 | mapx03 |
| 1 | **9,000,000 B.C.**(Pangea) | mapx10 | mapx11, mapx15 |
| 2 | **1423 B.C.** | mapx20 | mapx21/22/23/24/25 |
| 3 | **1990 A.D.** | mapx30 | mapx31/32/33/34/35 |
| 4 | **2112 A.D.**(Aftermath) | mapx40 | mapx41/44/45 |

⇒ **5 個地球時代 overworld = mapx00/10/20/30/40**(非先前以為的 3 個)。引擎時間之門已依此
循環(`next_era_world`)、招牌時代名已對齊(`era_name`)。**行星**(Mercury…Pluto)仍待校正
(疑 mapx5x–9x;需 oracle 軌道→map 或 emulator)。

## 高信度推定(本輪資料即可推得)

- **`mapx20` / `mapx30` / `mapx40` = 地球 overworld 三個時代**:三者共享 landmark 指紋
  `9@25,5`(地牢)+ `10@58,51`,= **同一塊大陸的不同時代**(城鎮 landmark 隨時代位移/增減)。
  對應 5 個地球時代中的 3 個(餘 2 個待找,可能在 `mapx00/10` 或同系列)。
- **`mapx93` = Father Antos 賜戒指處**(`FATHER ANTOS CHANTS: YOU HAVE EARNED MY BLESSING`)
  → **任務主鏈關鍵**(`EARN THE RING`)。`mapx23`(`SEARCH THE STARS FOR MY KIN`)、
  `mapx33`(`HAVE YOU FOUND MY FATHER`)= Antos 相關城鎮,構成「找父親→賜戒指」線。
- **城鎮梗(可對 Codex 城鎮名)**:`mapx21` = Wizardry 開發者 Andy Greenberg / Robert Woodhead +
  Hotel California 梗鎮;`mapx81` = Tommervik(Softalk 創辦人)梗;`mapx82` = 海盜指向
  「New …」的 clerk(疑 New San Antonio / Hotel California 線)。
- **`mapx70`**(landmark `5@31,31`、0% 水、無對話)= **城堡 / 塔候選**(疑 Lord British 城堡)。
- **`mapx24/25` `mapx34/35` `mapx44/45` `mapx85`**(landmark 8 在角落、水 41–59%)= 形態相近,
  疑**行星表面或塔/地牢宿主圖**,待 oracle 座標 + emulator 截圖對照。

## ★ Oracle 校正:landmark tile → 地點 = deterministic 規則(2026-06-13,已釘死)

`FUN_004064d0`(ENTER 派發,oracle 行 4916-5056)以**腳下 landmark tile id 直接決定地點類型**,
並把目的地 `mapxNN` 第 2 碼(kind 碼)寫死(`FUN_00427a50(mapname,1,'?')`):

| tile id | 類型 | kind 碼 | 目的地 |
|---|---|---|---|
| 5 | VILLAGE | '1' | `mapx<時代>1` |
| 6 | TOWN | '2' | `mapx<時代>2` |
| 8 | CASTLE | '3' | `mapx<時代>3` |
| 7 | TOWER | '4' | `mapx<時代>4` |
| 9 | DUNGEON | '5' | `mapx<時代>5` |
| 10 | 時間之門 | — | `FUN_0040c270` 印招牌 ANOS \<era\>(非載圖) |

第 1 碼 = 當前 overworld 時代數字。⇒ **目的地 = `mapx<時代碼><kind碼>`,非逐 landmark 推定。**
(原先「哪個門通哪座城」的 provisional 問題到此解除。)

## 引擎地點登記表(已照 oracle 規則重排,2026-06-13)

`src/game_main.c` 的 `LOC_REG[]` 只列「各時代 overworld 內 64×64 區實際存在的 landmark tile」
且目的地檔存在者(以原版 `mapxE0` 掃描 + 上表規則交叉驗證):

| world(時代) | landmark tile → 目的地 mapxNN(類型) |
|---|---|
| mapx00 (Legends) | 8→03(城堡=Minax 巢穴,走 minax_encounter 攔截) |
| mapx10 (9,000,000 B.C.) | 5→11(村莊) |
| mapx20 (1423 B.C.) | 5→21 村 · 6→22 鎮 · 8→23 堡 · 7→24 塔 · 9→25 地牢 |
| mapx30 (1990 A.D.) | 5→31 村 · 6→32 鎮 · 8→33 堡 · 7→34 塔 · 9→35 地牢 |
| mapx40 (2112 A.D.) | 5→41 村 · 9→45 地牢(無 6/7/8 landmark) |

> tile 10(時間之門)不入表;mapx00/10/40 缺少的 kind 碼目的地檔本就不存在(該時代無該類地點)。
> **時代↔overworld** 與 **landmark↔地點** 皆已由 oracle 釘死,不再 provisional。
> 驗證:破關回歸(`tests/regression_winnable.sh`)+ 城鎮正典(`tests/test_town_canon.sh`)PASS。

> 驗證:以 `mapx20` / `mapx30` 為 overworld headless 進城,各自載入對應城鎮(不同地圖)。
> 改 landmark→map 對應只需編輯 `LOC_REG[]`,無需動進入邏輯。

## 待辦(M1)
- [ ] overworld 類對上 5 個地球時代(Legends / 9,000,000 B.C. / 1423 B.C. / 1990 A.D. / 2112 A.D.)
- [ ] 無對話大地圖對上 9 行星(座標 + emulator 截圖對照)
- [ ] 城鎮類由對話主題對上 Codex 城鎮名
- [ ] 標出城堡 / 塔 / 地牢入口宿主圖
