#!/usr/bin/env bash
# 城鎮正典玩法回歸 — 確定性 headless 跑數個城鎮互動情境,grep stdout 訊息判 pass/fail。
#
# 鎖住本批 M5/M3 正典切片不回退:
#   - 迅捷之劍 ENILNO 金幣閘門(<500 回絕 / ≥500 賜劍;oracle FUN_00408e50 0x81)
#   - 對話蒐線索 clue 鏈(oracle FUN_00402a90 線索詩)
#   - YELL 市民台詞(oracle FUN_00409990)
#   - ALAKAZAM 慷慨市民(oracle FUN_00408e50 else 分支)
# 依賴 headless 的 g.msg stdout 儀器([step NN] <msg>)。需自備 Ultima II 資料(版權,gitignore)。
#
# 用法(容器內或本機具 SDL2):
#   tests/test_town_canon.sh <repo根> <資料目錄>
#   tileset 經 U2_TILESET 環境變數提供(免 PIL),或裝 python3-pil 現生。
set -uo pipefail
REPO="${1:-$(pwd)}"
DATA="${2:-${U2_DATA:-$REPO/../dos-original/ultima2}}"
BUILD=/tmp/u2town_build
OUT=/tmp/u2town_out
FONT="$(ls /usr/share/fonts/truetype/wqy/wqy-zenhei.ttc 2>/dev/null | head -1)"
export SDL_VIDEODRIVER=dummy

fail(){ echo "FAIL: $*" >&2; exit 1; }
[ -d "$REPO/src" ] || fail "找不到 repo src/($REPO)"
[ -f "$DATA/mapx20" ] || fail "找不到資料 mapx20($DATA)── 需自備 Ultima II 資料"
[ -n "$FONT" ] || fail "找不到 wqy-zenhei.ttc 字型"

echo "== 1) 建置 =="
cmake -S "$REPO" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1 || fail "cmake 設定失敗"
cmake --build "$BUILD" --target u2_game -j >/dev/null 2>&1 || fail "編譯失敗"
GAME="$BUILD/u2_game"

echo "== 2) 取得 tileset =="
TS="${U2_TILESET:-}"
if [ -z "$TS" ] && python3 -c "import PIL" 2>/dev/null; then
  python3 "$REPO/tools/decode_u2upgrade_tiles.py" "$REPO/tileset/EGATILES" /tmp/u2town_ega.png ega strip >/dev/null 2>&1 \
    && TS=/tmp/u2town_ega.png
fi
[ -n "$TS" ] && [ -f "$TS" ] || fail "無可用 tileset(設 U2_TILESET 或裝 python3-pil)"
UI_TSV="$REPO/translations/exe_translatable_strings.tsv"
[ -f "$UI_TSV" ] || fail "找不到 UI 字串表 $UI_TSV"

rm -rf "$OUT"; mkdir -p "$OUT"

# run <名稱> <script> → 回傳 stdout(headless 訊息流)
run(){ "$GAME" "$DATA/mapx20" "$FONT" "$TS" "$UI_TSV" --script "$2" "$OUT/$1_" 2>/dev/null; }
# assert_grep <log> <樣式> <情境名>
assert_grep(){ echo "$1" | grep -q "$2" || fail "$3:stdout 未含「$2」"; echo "  ✓ $3"; }

echo "== 3) 城鎮正典情境 =="

# 情境 A:ENILNO 金幣閘門 — 金300(<500)持戒求劍 → 回絕,不扣金
A="$(run gate_lo "OR\$Z9")"
assert_grep "$A" "需獻上 500 黃金" "A 金幣閘門回絕(金<500)"

# 情境 B:ENILNO 金幣閘門 — 金600(≥500)→ 賜劍
B="$(run gate_hi "OR\$\$Z9")"
assert_grep "$B" "收下 500 金貢禮" "B 金幣閘門賜劍(金≥500)"

# 情境 C:對話蒐線索 clue 鏈兩段
C="$(run clue "OTT")"
assert_grep "$C" "居民低語" "C clue 第一段(居民低語)"
assert_grep "$C" "老者說"   "C clue 第二段(老者說)"

# 情境 D:YELL 市民台詞(任一市民均可)
D="$(run yell "OY")"
assert_grep "$D" "你大喊一聲" "D YELL 市民反應"

# 情境 E:ALAKAZAM 慷慨市民(clue 鏈後多次交談,固定 seed 必觸發)
E="$(run alakazam "OTTTTTTTTTTTTTTTTTTTT")"
assert_grep "$E" "阿拉卡贊" "E ALAKAZAM 慷慨市民"

# 情境 F:STEAL 行竊(oracle FUN_00409660)— 固定 seed 下 10 次必同時出現成功與失敗
F="$(run steal "OFFFFFFFFFF")"
assert_grep "$F" "沒得手"        "F STEAL 失敗(守衛逮到)"
assert_grep "$F" "摸走\|偷得"    "F STEAL 成功(白拿武/防/食)"

echo "== 4) 判定 =="
echo "PASS: 城鎮正典玩法 6 情境全數通過(輸出 $OUT)"
