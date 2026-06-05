#!/usr/bin/env bash
# 自動在 DOSBox 內完成 Ultima II 建角,產生一份真實 PLAYER 存檔。
# 用途:取得非空 player 檔以核對 stat 欄位 offset (HP/Food/Gold/屬性)。
# 建角機制 (實機觀察):
#   - PLAYER GENERATION 共 90 點,依序填 6 個屬性,每個屬性「打兩位數」即提交並前進。
#   - STRENGTH/AGILITY/STAMINA/CHARISMA/WISDOM/INTELLIGENCE 各填 15 (6×15=90)。
#   - 接著 M/F 打 'm';RACE 打 '1'(HUMAN);TYPE 打 '1'(FIGHTER)。
#   - NAME 打 "HERO" + Return;SATISFACTORY(Y/N) 打 'y' 寫檔。
set -x
export DISPLAY=:99
Xvfb :99 -screen 0 640x480x16 >/dev/null 2>&1 &
sleep 2
printf '[autoexec]\nmount c /game\nc:\nultimaii.exe\n' > /tmp/db.conf
dosbox -conf /tmp/db.conf >/dev/null 2>&1 &
DOSPID=$!
shoot() { import -window root "/out/c_$1.png" 2>/dev/null; }
key()  { xdotool key --clearmodifiers "$1"; sleep "${2:-0.6}"; }
two()  { xdotool key --clearmodifiers "$1"; sleep 0.3; xdotool key --clearmodifiers "$2"; sleep 0.6; }

sleep 26                        # 等選單 CHOICE: 就緒
shoot 00
key c 2                         # 進入建角
shoot 01
# 6 個屬性各 15 點
two 1 5; two 1 5; two 1 5; two 1 5; two 1 5; two 1 5
shoot 02
key m 1                         # M/F
key 1 1                         # RACE = HUMAN
key 1 1                         # TYPE = FIGHTER
shoot 03
xdotool type --delay 150 "HERO"; sleep 1; key Return 2   # NAME
shoot 04
key y 2                         # SATISFACTORY = Y -> 寫檔
shoot 05
sleep 2; shoot 06
echo "=== player hex (非零列) ==="
od -An -tx1 /game/player | grep -vE '^( 00)+ *$' | head -40
kill $DOSPID 2>/dev/null; sleep 1
