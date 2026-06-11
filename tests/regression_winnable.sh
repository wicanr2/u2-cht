#!/usr/bin/env bash
# 破關回歸測試 — 確定性 headless 走完整主線鏈到結局,grep 破關訊號判 pass/fail。
#
# 鎖住「建角存檔→商店補給→地牢→時空旅行→關鍵道具→傳說時代→米娜克斯對決→結局」
# 端到端整合不回退。需自備 Ultima II 資料(版權,gitignore)。
#
# 用法:在 u2cht-build / u2cht-pkg 容器內,或本機具 SDL2+PIL 環境執行:
#   tests/regression_winnable.sh <repo根> <資料目錄(含 mapx00..40 等)>
# 預設:
#   repo = 當前目錄  ·  資料 = $U2_DATA 或 ../dos-original/ultima2
set -uo pipefail
REPO="${1:-$(pwd)}"
DATA="${2:-${U2_DATA:-$REPO/../dos-original/ultima2}}"
BUILD=/tmp/u2reg_build
OUT=/tmp/u2reg_out
FONT="$(ls /usr/share/fonts/truetype/wqy/wqy-zenhei.ttc 2>/dev/null | head -1)"
export SDL_VIDEODRIVER=dummy

fail(){ echo "FAIL: $*" >&2; exit 1; }
[ -d "$REPO/src" ] || fail "找不到 repo src/($REPO)"
[ -f "$DATA/mapx20" ] || fail "找不到資料 mapx20($DATA)── 需自備 Ultima II 資料"
[ -f "$DATA/mapx00" ] || fail "找不到 mapx00(傳說時代 overworld)── 破關鏈需要"
[ -n "$FONT" ] || fail "找不到 wqy-zenhei.ttc 字型"

echo "== 1) 建置 =="
cmake -S "$REPO" -B "$BUILD" -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1 || fail "cmake 設定失敗"
cmake --build "$BUILD" --target u2_game -j >/dev/null 2>&1 || fail "編譯失敗"
GAME="$BUILD/u2_game"

echo "== 2) 取得 tileset =="
# 優先 U2_TILESET 環境變數(免 PIL),否則用 PIL 現生 strip
TS="${U2_TILESET:-}"
if [ -z "$TS" ] && python3 -c "import PIL" 2>/dev/null; then
  python3 "$REPO/tools/decode_u2upgrade_tiles.py" "$REPO/tileset/EGATILES" /tmp/u2reg_ega.png ega strip >/dev/null 2>&1 \
    && TS=/tmp/u2reg_ega.png
fi
[ -n "$TS" ] && [ -f "$TS" ] || fail "無可用 tileset(設 U2_TILESET 或裝 python3-pil)"

UI_TSV="$REPO/translations/exe_translatable_strings.tsv"   # argv[4] 必填(漏了會吃掉 --script)
[ -f "$UI_TSV" ] || fail "找不到 UI 字串表 $UI_TSV"

echo "== 3) 破關走法(確定性 headless)=="
# O 進城 · Z 開店 · 22211 升防具×3/武器×2(提高對決存活)· Z 關店 · X 離城
# I 取得關鍵道具(含力場之戒 + 迅捷之劍 ENILNO)· D 地牢進 · J 下樓 · K 上樓 · X 離開
# P×3 時空旅行 mapx20→30→40→00(傳說時代)· M 米娜克斯對決 → 結局
SCRIPT="OZ22211ZXIDJKXPPPM"
rm -rf "$OUT"; mkdir -p "$OUT"
LOG="$("$GAME" "$DATA/mapx20" "$FONT" "$TS" "$UI_TSV" --script "$SCRIPT" "$OUT/reg" 2>&1)"
echo "$LOG"

echo "== 4) 判定 =="
echo "$LOG" | grep -q "GAME WON" || fail "未觸發破關訊號(主線鏈有回退?)"
echo "$LOG" | grep -q "破關狀態:WON" || fail "結束狀態非 WON"
LAST="$(ls "$OUT"/reg*.png 2>/dev/null | sort | tail -1)"
[ -f "$LAST" ] || fail "無輸出幀"
echo "PASS: 主線端到端可破關(最後幀:$LAST)"
