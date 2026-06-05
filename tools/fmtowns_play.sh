#!/usr/bin/env bash
# 在 Tsugaru(FM Towns 模擬器)跑《Ultima Trilogy》→ Ultima II,自動建角 + Journey Onward,
# 截 overworld 無損 PNG。供抽取真實 FM Towns palette + 32×32 tile(見 tools/fmtowns_rip_tiles.py)。
#
# 前置(本 repo 不含,使用者自備,放同一資料夾):
#   - FM Towns 系統 ROM 組:FMT_DIC/DOS/F20/FNT/SYS.ROM  → $ROM_DIR
#   - Ultima Trilogy CD image(CloneCD .cue/.img)+ [UserDisk].hdm → $CD_DIR
#   - Docker image:docker build -f docker/Dockerfile.tsugaru -t tsugaru docker/
#
# 操作流程(實測,FM Towns JP 版):
#   TownsMENU 雙擊 ULTIMA II 圖示 → 過「wrong disk」/標題(連按 space)→
#   主選單按首字母:g=Generate / j=Journey(選項以首字母選,非方向鍵)→
#   Generate: c=Create → 輸入槽位 1-4 → 屬性(Points:30)→ 種族(H/E/D/B)/
#   性別(M/F)/職業(F/C/W/T)/命名 → y 確認 → m=Main Menu →
#   j=Journey Onward → Select Player(已存角色)→ Return → y → overworld。
#
# 用法:ROM_DIR=... CD_DIR=... CUE=... ./fmtowns_play.sh <out_dir>
set -x
ROM_DIR="${ROM_DIR:-/rom}"; CD_DIR="${CD_DIR:-/cd}"; OUT="${1:-/out}"
CUE="${CUE:?需指定 CD 的 .cue 檔名(在 CD_DIR 內)}"
FD="${FD:-}"
export DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1
Xvfb :99 -screen 0 800x600x24 >/dev/null 2>&1 &
sleep 3
TS="${TS:-/build/TOWNSEMU/build/main_cui/Tsugaru_CUI}"
cd "$CD_DIR"
ARGS=(/rom -CD "$CUE" -BOOTKEY CD -SCALE 100)
[[ -n "$FD" ]] && ARGS+=(-FD0 "$FD")
"$TS" "${ARGS[@]}" >/tmp/ts.log 2>&1 &
TSPID=$!
shoot(){ import -window root "$OUT/fm_$1.png" 2>/dev/null; }
dclick(){ xdotool mousemove "$1" "$2"; sleep 0.5;
  xdotool mousedown 1; sleep 0.12; xdotool mouseup 1; sleep 0.08;
  xdotool mousedown 1; sleep 0.12; xdotool mouseup 1; }
sleep 42
dclick 202 181            # ULTIMA II icon (SCALE100)
for i in $(seq 1 15); do xdotool key space; sleep 2.6; done
# 若已有存檔:直接 Journey;否則先建角(見上方流程)
xdotool key j; sleep 3; xdotool key Return; sleep 2; xdotool key y; sleep 4
xdotool key space; sleep 5
shoot overworld
kill $TSPID 2>/dev/null; sleep 1
