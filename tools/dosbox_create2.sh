#!/usr/bin/env bash
# 差分建角:屬性全部不同 + 不同 race/type/sex/name,用來與第一份樣本 diff 定位欄位。
# STR=21 AGI=11 STA=12 CHA=13 WIS=14 INT=19 (sum=90, BCD 後各 byte 互異)
# SEX=F  RACE=2(ELF)  TYPE=3(WIZARD)  NAME=ABCD
set -x
export DISPLAY=:99
Xvfb :99 -screen 0 640x480x16 >/dev/null 2>&1 &
sleep 2
printf '[autoexec]\nmount c /game\nc:\nultimaii.exe\n' > /tmp/db.conf
dosbox -conf /tmp/db.conf >/dev/null 2>&1 &
DOSPID=$!
shoot() { import -window root "/out/d_$1.png" 2>/dev/null; }
key()  { xdotool key --clearmodifiers "$1"; sleep "${2:-0.6}"; }
two()  { xdotool key --clearmodifiers "$1"; sleep 0.3; xdotool key --clearmodifiers "$2"; sleep 0.6; }

sleep 26
shoot 00
key c 2
shoot 01
two 2 1   # STR 21
two 1 1   # AGI 11
two 1 2   # STA 12
two 1 3   # CHA 13
two 1 4   # WIS 14
two 1 9   # INT 19
shoot 02
key f 1   # SEX = F
key 2 1   # RACE = ELF
key 3 1   # TYPE = WIZARD
shoot 03
xdotool type --delay 150 "ABCD"; sleep 1; key Return 2
shoot 04
key y 2
shoot 05
sleep 2; shoot 06
echo "=== player hex ==="
od -An -tx1 /game/player | head -6
kill $DOSPID 2>/dev/null; sleep 1
