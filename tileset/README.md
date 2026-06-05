# U2 Upgrade tileset(研究/保存用途)

本目錄存放 Ultima II 的 tileset 資料,作為本專案渲染的 **ground truth**。

| 檔 | 格式 | 說明 |
|---|---|---|
| `CGATILES` | 65 tile × 64 byte,16×16 **CGA 2bpp** | 4 px/byte |
| `EGATILES` | 65 tile × 128 byte,16×16 **EGA 4bpp** | 8 byte/row,packed nibble,標準 EGA 16 色 |
| `EGACOLOR.10` | 32 byte | EGATHEME.10 變體 palette(theme remap,選用) |

## 來源
mcmagi 的 **Ultima II Upgrade**(社群修復/增強版):
https://github.com/mcmagi/ultima-exodus/releases → `u2upgrade-2.1.zip`

## 授權
原始 Ultima II(1982,Origin/EA)為年代久遠之作品;U2 Upgrade 為社群保存專案。
本目錄之 tileset 僅作**逆向研究、在地化與保存**用途收錄(專案擁有者裁示允許),
不主張任何著作權;若權利人有異議請聯繫移除。

## 用法
`build_poc.sh` / `build_demo.sh` 會優先讀此處的 `EGATILES` 產引擎 tileset;
亦可 `tools/decode_u2upgrade_tiles.py EGATILES out.png ega sheet|strip`。
