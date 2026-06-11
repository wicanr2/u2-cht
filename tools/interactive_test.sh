#!/usr/bin/env bash
# 容器內互動測試:Xvfb 開真實視窗 → xdotool 驅動「新遊戲→建角→走進城」→ ffmpeg 擷圖。
# 需在 u2cht-test image 內跑(見 docker/Dockerfile.test)。掛載 /work(repo)、/data(資料)。
set -eu
OUT=/work/build/itest
mkdir -p "$OUT"
FONT=/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc
MAP=/data/mapx20

# 1) 編譯(若無)+ 產生 tileset(ega + fmtowns 主角)
cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
cmake --build /work/build --target u2_game -j >/dev/null 2>&1
python3 /work/tools/decode_u2upgrade_tiles.py /work/tileset/EGATILES /work/build/ts/ega.png ega strip >/dev/null 2>&1 || true
python3 /work/tools/build_fmtowns_strip.py /work/build/ts/ega.png /work/tileset/fmtowns /work/build/ts/fmtowns.png >/dev/null 2>&1 || true
TS=/work/build/ts/ega.png,/work/build/ts/fmtowns.png
TITLE=/work/docs/screenshots/fmtowns_title_decoded.png

# 2) Xvfb
export DISPLAY=:99
Xvfb :99 -screen 0 960x600x24 >/tmp/xvfb.log 2>&1 &
sleep 1
rm -f /root/.local/share/LairWare-cht/Ultima2/player 2>/dev/null || true   # 確保是「新遊戲」

grab(){ ffmpeg -y -f x11grab -video_size 960x600 -i :99.0 -frames:v 1 "$OUT/$1" >/dev/null 2>&1 || true; echo "  grabbed $1"; }
key(){ xdotool key --window "$W" "$1"; sleep "${2:-0.6}"; }

# 3) launch
/work/build/u2_game "$MAP" "$FONT" "$TS" /work/translations/exe_translatable_strings.tsv \
    /work/tests/fixtures/player_sample_abcd --title "$TITLE" --splash /work/build/splash.png \
    > /tmp/game_out.log 2>&1 &
GPID=$!
sleep 2
W=$(xdotool search --name "Ultima" | head -1)
echo "win=$W"
xdotool windowactivate "$W" 2>/dev/null || true

key space 1.0        # 跳過原版標題
key space 1.0        # 跳過全家福 splash
grab 01_menu.png     # 開場選單(新遊戲/試玩範例/離開)
key Return 1.0       # 選 新遊戲(sel 0)→ 進建角
grab 02_name_empty.png
for c in t e s t e r; do key "$c" 0.15; done   # 輸入姓名 TESTER
grab 03_name.png
key Return 0.8       # 姓名→性別
grab 04_sex.png
key Return 0.8       # 性別→種族
key Right 0.5; key Right 0.5   # 種族切到 矮人(示範可選)
grab 05_race.png
key Return 0.8       # 種族→職業
grab 06_class.png
key Return 0.8       # 職業→屬性
key Down 0.4; key Right 0.4; key Right 0.4   # 屬性加點示範
grab 07_stats.png
key Return 1.0       # 完成建角 → 進遊戲
grab 08_game_start.png   # 起點(城旁)
# 4) 走進城:起點在城堡南側,按 Up 往北踏上城堡入口
key Up 1.0
grab 09_in_town.png
key Up 0.8
grab 10_in_town2.png
# 5) 收尾:離開(自動存檔)
key q 1.0
sleep 1
echo "=== game stdout ==="; tail -3 /tmp/game_out.log
echo "=== 存檔 ==="; ls -la /root/.local/share/LairWare-cht/Ultima2/player 2>&1 || echo "(無存檔)"
kill %1 2>/dev/null || true   # Xvfb
echo "DONE → $OUT/*.png"
