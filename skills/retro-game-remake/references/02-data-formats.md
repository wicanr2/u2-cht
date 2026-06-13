# 02 · 破解資料格式

把每種原版資料檔的格式破出來,寫成 `docs/DATA_FORMATS.md`(含 offset、編碼、驗證方法)。原則:**不寫回原始檔**,用外部 UTF-8 覆蓋層以 `(來源,key)` 索引,載入時覆蓋。

## 常見格式陷阱(Ultima 系實例)
- **地圖**:可能是純 tile array 無 header(如 U2 mapxNN = 4224B = 64×66)。tile id 可能被 ×4 存(低 2 bit 是 flag)→ **讀取要 ÷4**。檔名尾碼常編場景型別(0=overworld/1-3=town/4-5=dungeon)。
- **對話**:NPC 文字常每 byte `OR 0x80`(被誤稱 encrypted,其實是 high-bit ASCII)→ 解碼 `byte & 0x7f`,`\r` 換行。
- **實體層**:地圖只存地形/建築;NPC/怪物座標另存於平行陣列檔(X/Y/status/tile/flag 多個平行陣列)。**交叉驗證**:某 tile 值的實體座標 vs 地圖上該 tile 的格子要完全吻合(「空城→活城」)。
- **NPC→對話行對應**:常在實體檔某欄以 bitflag(如 `&0x80`=可交談)+ 1-based 索引指向對話行。
- **存檔**:屬性常用 **BCD** 編碼;名字 ASCII、性別/職業/種族 0-indexed;HP/食物/金 多為 2-byte BCD。存的是「套用 race/class 加成後」的值。

## 驗證:DOSBox 差分(存檔/建角格式真值)
用 **headless DOSBox** 自動建兩隻只差一個欄位的角色,逐 byte diff,並與建角畫面顯示值交叉比對 → 鎖定每個欄位的 offset/編碼。把實機存檔當回歸 fixture(`tests/fixtures/`)做斷言。

## Sprite/tile(CGA/EGA 點陣)
- CGA tile 常 2-bitplane(前半 plane0、後半 plane1;調色盤 00黑/01青/10洋紅/11白)。
- EGA(如 U2 Upgrade EGATILES)是 ground-truth tileset 來源。tile 美術屬版權,repo 只收研究對照,公開包視情況。

## 心法
- 不確定的欄位明確標「待更多真實樣本」。
- 格式破解要可程式驗證(decode → 渲染 → 對照原版/建角畫面)再下定論。
- 怪物 HP/攻擊等數值以**反編 oracle / 原版**為主、攻略網站為輔,衝突標註。
