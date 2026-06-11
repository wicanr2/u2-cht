#!/usr/bin/env bash
# 容器內錄製 demo 影片:Xvfb 開窗 → ffmpeg 連續錄製 → xdotool 驅動完整流程
# (原版標題→選單→建角→進城→交談→切 FM Towns 畫風→F1)。
# ※ 不帶 --splash:repo 公開影片不含私人全家福。
# 產出:build/demo/u2cht_demo.mp4 + .gif。需 u2cht-test image。
set -eu
OUT=/work/build/demo
mkdir -p "$OUT"
FONT=/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc
MAP=/data/mapx20
TITLE=/work/docs/screenshots/fmtowns_title_decoded.png

cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
cmake --build /work/build --target u2_game -j >/dev/null 2>&1
python3 /work/tools/decode_u2upgrade_tiles.py /work/tileset/EGATILES /work/build/ts/ega.png ega strip >/dev/null 2>&1 || true
python3 /work/tools/build_fmtowns_strip.py /work/build/ts/ega.png /work/tileset/fmtowns /work/build/ts/fmtowns.png >/dev/null 2>&1 || true
TS=/work/build/ts/ega.png,/work/build/ts/fmtowns.png

export DISPLAY=:99
Xvfb :99 -screen 0 960x600x24 >/tmp/xvfb.log 2>&1 &
sleep 1
rm -f /root/.local/share/LairWare-cht/Ultima2/player 2>/dev/null || true

# 啟動遊戲(不帶 --splash)
/work/build/u2_game "$MAP" "$FONT" "$TS" /work/translations/exe_translatable_strings.tsv \
    /work/tests/fixtures/player_sample_abcd --title "$TITLE" \
    > /tmp/game_out.log 2>&1 &
sleep 2
W=$(xdotool search --name "Ultima" | head -1)
xdotool windowactivate "$W" 2>/dev/null || true
key(){ xdotool key --window "$W" "$1"; sleep "${2:-0.6}"; }
type_slow(){ for c in $(echo "$1" | sed 's/./& /g'); do xdotool key --window "$W" "$c"; sleep 0.18; done; }

# 開始連續錄製(背景),固定 30s
ffmpeg -y -f x11grab -video_size 960x600 -framerate 15 -i :99.0 -t 32 \
    -pix_fmt yuv420p -movflags +faststart "$OUT/u2cht_demo.mp4" >/tmp/ffmpeg.log 2>&1 &
FF=$!
sleep 2.5                 # 原版標題畫面停留
key space 2.0             # → 開場選單
key Return 1.2            # 新遊戲 → 建角(姓名)
type_slow hero; sleep 0.8 # 輸入姓名 HERO
key Return 1.2            # 姓名→性別
key Return 1.0            # 性別→種族
key Right 0.7             # 種族:精靈
key Return 1.0            # 種族→職業
key Right 0.7             # 職業:牧師
key Return 1.0            # 職業→屬性
key Down 0.5; key Right 0.5; key Right 0.5; key Right 0.5   # 加點
key Return 1.8            # 完成建角 → 進遊戲(城旁起點)
key Up 2.2               # 往北踏上城堡 → 進城
key Down 0.6; key Right 0.6   # 城內走動
key t 2.0                # 與 NPC 交談
key x 1.5               # 離開城鎮
key g 2.2               # 切換到 FM Towns 畫風(主角變騎士)
key F1 2.5              # 指令表
key F1 0.6              # 關閉
wait $FF 2>/dev/null || true
kill %1 2>/dev/null || true   # Xvfb

# mp4 → 最佳化 GIF(480 寬,10fps,palette)
ffmpeg -y -i "$OUT/u2cht_demo.mp4" -vf "fps=10,scale=480:-1:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" \
    "$OUT/u2cht_demo.gif" >/tmp/gif.log 2>&1 || true
echo "=== 產物 ==="; ls -la "$OUT"
