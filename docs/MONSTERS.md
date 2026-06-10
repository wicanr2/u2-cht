# Ultima II 怪物 / 角色圖鑑(FM Towns sprite + 中英名)

> **sprite 圖**:FM Towns《Ultima Trilogy》`GRAPH/UT1TILE0.TIF` 的 32×32 sprite(64 幀 = 32 個 ×2 幀動畫),
> 以**驗證過的 FM Towns palette**(`0黑 1綠 2紅 3洋紅 4藍 5青 6黃 7白`,見 [`FMTOWNS_TILESET.md`](FMTOWNS_TILESET.md) 附錄 C)解碼。
> **名稱**:bestiary 取自 [Codex of Ultima Wisdom — Ultima II monster data](https://wiki.ultimacodex.com/wiki/Ultima_II_monster_data);
> 中譯依 CRPG 慣例 + [`CONTEXT.md`](../CONTEXT.md)。
>
> ⚠️ **誠實標註**:FM Towns sprite 的「index → 哪隻怪物」官方順序尚未破解
> (DOS `monsters` 檔為 shape 表、無名;exe 僅人型職業有名)。下表 sprite↔名除「視覺較確定」者外屬**推測**。

## U2 官方 bestiary(名稱權威,Codex wiki)

| # | 英文 | 中文 | HP | 類別 |
|---|---|---|---|---|
| 1 | Orc | 半獸人 | 16 | 地面 |
| 2 | Goblin | 哥布林 | 5 | 地面 |
| 3 | Thief | 盜賊 | 32 | 地面 |
| 4 | Daemon | 惡魔 | 64 | 地面 |
| 5 | Sea Serpent | 海蛇 | 64 | 海上 |
| 6 | Fighter | 戰士 | 128 | 地面 |
| 7 | Pirate Ship | 海盜船 | 160 | 海上 |
| 8 | Devil | 魔鬼 | 192 | 地面 |
| 9 | Wizard | 巫師 | 224 | 地面 |
| 10 | Balron | 炎魔(巴爾龍) | 255 | 地面 |
| 11 | Jester | 弄臣 | — | 城鎮 |
| 12 | Merchant | 商人 | — | 城鎮 |
| 13 | Cleric | 牧師 | — | 城鎮 |
| 14 | Guard | 守衛 | 255 | 城鎮 |
| 15 | Ghost | 幽靈 | — | 地牢 |
| 16 | Carrion Creeper | 食屍爬蟲 | — | 地牢 |
| 17 | Viper | 毒蛇 | — | 地牢 |
| 18 | Gremlin | 小妖精 | — | 地牢 |
| 19 | King | 國王 | 255 | NPC |
| 20 | Minax | 米娜克斯(女巫) | 100 dmg | 魔王 |

> exe 內嵌字串已確認:FIGHTER/戰士、CLERIC/牧師、WIZARD/巫師、THIEF/盜賊、DWARF/矮人、
> GREMLIN/小妖精、ENCHANTRESS·MINAX/女巫·米娜克斯。

## FM Towns sprite 圖鑑(32 個,取第 1 幀)

| 圖 | 編號 | 推測對應 | 中文 | 信心 |
|---|---|---|---|---|
| ![](monsters/m00.png) | m00 | Fighter / Guard | 戰士 / 守衛 | 推測(人型甲) |
| ![](monsters/m01.png) | m01 | Fighter | 戰士 | 推測(人型) |
| ![](monsters/m02.png) | m02 | Cleric / Merchant | 牧師 / 商人 | 推測(人型) |
| ![](monsters/m03.png) | m03 | Wizard | 巫師 | 推測(持杖) |
| ![](monsters/m04.png) | m04 | Fighter / Orc | 戰士 / 半獸人 | 推測 |
| ![](monsters/m05.png) | m05 | Fighter | 戰士 | 推測 |
| ![](monsters/m06.png) | m06 | Thief / Ranger | 盜賊 / 遊俠 | 推測(動作姿態) |
| ![](monsters/m07.png) | m07 | Thief | 盜賊 | 推測 |
| ![](monsters/m08.png) | m08 | Jester | 弄臣 | 較確定(花格服) |
| ![](monsters/m09.png) | m09 | Wizard / Cleric | 巫師 / 牧師 | 推測(白袍) |
| ![](monsters/m10.png) | m10 | Fighter | 戰士 | 推測 |
| ![](monsters/m11.png) | m11 | Sea Serpent | 海蛇 | 較確定(蛇形) |
| ![](monsters/m12.png) | m12 | Carrion Creeper | 食屍爬蟲 | 推測(蟲形) |
| ![](monsters/m13.png) | m13 | Fighter | 戰士 | 推測 |
| ![](monsters/m14.png) | m14 | (object) | 物件 / 砲座 | 推測(非生物) |
| ![](monsters/m15.png) | m15 | Daemon / Devil | 惡魔 / 魔鬼 | 較確定(綠巨型) |
| ![](monsters/m16.png) | m16 | Devil / Balron | 魔鬼 / 炎魔 | 較確定(紅惡魔) |
| ![](monsters/m17.png) | m17 | (whirlpool/effect) | 漩渦 / 效果 | 推測(藍噪) |
| ![](monsters/m18.png) | m18 | Fighter | 戰士 | 推測(持劍) |
| ![](monsters/m19.png) | m19 | Orc / Goblin | 半獸人 / 哥布林 | 推測 |
| ![](monsters/m20.png) | m20 | Fighter | 戰士 | 推測(紅甲持劍) |
| ![](monsters/m21.png) | m21 | Orc | 半獸人 | 推測 |
| ![](monsters/m22.png) | m22 | Gremlin | 小妖精 | 推測 |
| ![](monsters/m23.png) | m23 | Guard | 守衛 | 推測 |
| ![](monsters/m24.png) | m24 | Wizard | 巫師 | 推測 |
| ![](monsters/m25.png) | m25 | Wizard / Ghost | 巫師 / 幽靈 | 推測(高瘦) |
| ![](monsters/m26.png) | m26 | Balron / Devil | 炎魔 / 魔鬼 | 推測(紅) |
| ![](monsters/m27.png) | m27 | Sea Serpent / Daemon | 海蛇 / 惡魔 | 較確定(蛇形) |
| ![](monsters/m28.png) | m28 | Sea Serpent | 海蛇 | 較確定 |
| ![](monsters/m29.png) | m29 | Sea Serpent | 海蛇 | 較確定 |
| ![](monsters/m30.png) | m30 | Sea Serpent | 海蛇 | 較確定 |
| ![](monsters/m31.png) | m31 | Sea Serpent | 海蛇 | 較確定 |

> 完整 sprite sheet(含第 2 幀動畫、上色):[`screenshots/fmtowns_monsters_colored.png`](screenshots/fmtowns_monsters_colored.png)。
> 抽取工具:[`tools/fmtowns_decode.py`](../tools/fmtowns_decode.py)。

## 來源
- [Codex of Ultima Wisdom — Ultima II monster data](https://wiki.ultimacodex.com/wiki/Ultima_II_monster_data)
- exe 內嵌字串(`translations/exe_translatable_strings.tsv`)
</content>
