#!/usr/bin/env bash
# 移動驗證:抽 tile → Docker build → 跑移動序列輸出連續 PNG (headless)
# 用法: ./build_demo.sh <mapxNN> <moves> [prefix] [palette] [sx sy]
#   例: ./build_demo.sh ../dos-original/ultima2/mapx20 EEEESSSSWWWWWWWW move blue 31 33
set -euo pipefail
cd "$(dirname "$0")"

MAP="${1:?需指定 mapxNN}"; MOVES="${2:?需指定移動序列如 EEESSWW}"
PREFIX="${3:-build/move}"; PAL="${4:-blue}"; SX="${5:-}"; SY="${6:-}"
IMG=u2cht-build
FONT=/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc

MAP_ABS="$(readlink -f "$MAP")"; DATA="$(dirname "$MAP_ABS")"; NAME="$(basename "$MAP_ABS")"
EXE="$DATA/ultimaii.exe"
mkdir -p build
# 優先 U2 Upgrade EGATILES (正確);找不到 fallback EXE @0x7C42 (PoC)
EGATILES="${U2UP_TILES:-}"
[[ -z "$EGATILES" && -f tileset/EGATILES ]] && EGATILES="tileset/EGATILES"
[[ -z "$EGATILES" && -f "$DATA/EGATILES" ]] && EGATILES="$DATA/EGATILES"
[[ -z "$EGATILES" && -f build/u2up/EGATILES ]] && EGATILES="build/u2up/EGATILES"
if [[ -n "$EGATILES" && -f "$EGATILES" ]]; then
    echo "tileset: U2 Upgrade EGATILES ($EGATILES)"
    /usr/bin/python3 tools/decode_u2upgrade_tiles.py "$EGATILES" build/tileset.png ega strip
else
    echo "⚠️ 無 EGATILES,fallback EXE @0x7C42 (PoC)"
    /usr/bin/python3 tools/extract_tiles.py "$EXE" build/tileset.png "$PAL"
fi
docker build -q -t "$IMG" docker/ >/dev/null
docker run --rm -v "$PWD":/work -v "$DATA":/data:ro "$IMG" bash -c "
    set -e
    cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null
    cmake --build /work/build -j >/dev/null
    /work/build/u2_demo /data/$NAME $FONT /work/$PREFIX /work/build/tileset.png $MOVES $SX $SY
"