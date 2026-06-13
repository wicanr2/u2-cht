# 03 · 美術 / 音訊考古(FM Towns 等各版本)

老遊戲常有多版本(CGA/EGA/PC98/FM Towns/NES…)。**FM Towns 日版(1990 前後)常彩色重畫 + CD 音樂**,是素材富礦,但格式有陷阱。先確認該作哪些版本存在(別硬補不存在的版本)。

## FM Towns 圖檔:`.TIF` 是 FillOrder=2
GRAPH/*.TIF 是 **FillOrder=2(LSB-first 位元反轉)** 的 TIFF,且 **header 謊報尺寸**、實際資料自 **offset 512** 起、sprite 用**偶數 nibble**(=index×2)、**8 色調色盤**。
解碼修正後可抽真 palette + tile + 怪物 sprite。標準 TIFF 解碼器會解錯,要自寫(套 bit-reverse 表)。

## FM Towns 音樂/音效
從《合集》CD 映像(`.cue`/`.img`)+ 執行檔抽:
- **CDDA**:CD 上的 audio track(合集常 U1/U2/U3 共用)。用 cue 的 MSF 算 byte offset(sector 2352B)切 raw PCM → WAV → ogg。**哪軌對應哪場景**要靠 RE exe 或 emulator 觀測 + 影片比對(時長是線索:短軌常是標題/jingle)。
- **CDDA 觸發機制**:protected-mode(RUN386/MetaWare)程式經 **INT 93h(CD-BIOS)但是反射呼叫** ── `0x93` 是傳給「模擬 real-mode INT」服務的 **data 參數**,**不是字面 `int 0x93` 指令**(全檔搜 `CD 93` 只會中 Shift-JIS 文字)。播放鏈:場景 id → 表 → bgm_idx → MSF 表 → CD 音軌;觸發點在標題/結局函式(引用 `*TITLE*.TIF` / 結局文字)。
- **遊玩音樂 = EUP**(FM Towns EUPHONY 序列,`/SOUND/*.EUP` + `*.FMB` 音色庫;**檔名即場景** MAP/TOWN/DUNGEON/OSIRO)。算繪用 **`gzaffin/eupmini`(EUPPlayer,作者 Hayasaka)**:`eupplay -o out.wav X.EUP`。
  - build patch(新 g++):每個 `.hpp` 補 `#include <cstdint>`;`mapTrack_toChannel` 前加 `if(ch>=0)` 跳過未用軌(0xff,否則 assert)。
  - 遊戲 EUP 常缺 header 欄位:`0x6E2` 寫 FMB 名、`0x6D4` 寫 `fm_midi_ch=00 01 02 03 04 05` 才有 FM 裝置監聽。
  - ⚠ 已知:eupmini 的 FM 合成對某些純 FM 檔輸出靜音(本案 U2 卡這;含 PCM 的 WANDER 正常)。最可靠改 **emulator(Tsugaru)逐場景錄音**。
- **音效 `.SND`**:FM Towns RF5C68 PCM,**8-bit sign-magnitude**(bit7=符號,0x00/0x80 皆靜音),標頭 0x20B(名@0、PCM 長度@0x0c)、PCM@0x20。轉 signed16 WAV。取樣率不在標頭明欄,預設試 8000Hz 再耳驗。

## ISO9660 手抽(容器無 isoinfo/pycdlib 時)
track01 = MODE1/2352,每 sector 取 offset 16 起 2048B → ISO;手動解 PVD@sector16、root dir record@offset156、遞迴走目錄。

## tileset 策略
- 多平台 tileset 做成可切換(G 鍵循環),參考 u3-cht 的具名 tileset 清單。
- 引擎 tileset 用水平 strip;由 strip 高度推 tile size(16=EGA / 32=FM Towns,sprite 要原生 32×32 不要縮)。
- 版權音訊/美術一律 build/ 下 gitignore、不入公開包(見 06)。
