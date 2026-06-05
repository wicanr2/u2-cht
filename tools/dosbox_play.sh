#!/usr/bin/env bash
# 還原一份已建角色存檔 → 進遊戲(按 P)→ 截在遊戲內的狀態列。
# 目的:讀出起始 HP/食物/黃金/經驗的「顯示值」,以對應 player 存檔 byte offset。
set -x
export DISPLAY=:99
Xvfb :99 -screen 0 640x480x16 >/dev/null 2>&1 &
sleep 2
printf '[autoexec]\nmount c /game\nc:\nultimaii.exe\n' > /tmp/db.conf
dosbox -conf /tmp/db.conf >/dev/null 2>&1 &
DOSPID=$!
shoot() { import -window root "/out/p_$1.png" 2>/dev/null; }
key()  { xdotool key --clearmodifiers "$1"; sleep "${2:-1}"; }

sleep 26                 # 等選單 CHOICE:
shoot 00
key p 4                  # P = play a game(載入 player)
shoot 01
sleep 3; shoot 02        # 等世界載入
key Return 2; shoot 03   # 可能有提示需確認
sleep 2; shoot 04
kill $DOSPID 2>/dev/null; sleep 1
