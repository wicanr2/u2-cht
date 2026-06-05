#!/usr/bin/env bash
# 互動式引擎 u2_game:Docker build + headless 腳本驗證(走一段路,逐步存 PNG)。
# 互動視窗需在有 X 的環境直接跑 build/u2_game(見 README);此腳本走 --script 模式驗證。
# 用法: ./build_game.sh <mapxNN 路徑> [moves] [out_prefix]
#   ./build_game.sh ../dos-original/ultima2/mapx20 EEESSSWWWNNN build/game_step_
set -euo pipefail
cd "$(dirname "$0")"

MAP="${1:?需指定 mapxNN 路徑 (自備合法 Ultima II 資料)}"
MOVES="${2:-EEEESSSSWWWWNNNN}"
PREFIX="${3:-build/game_step_}"
IMG=u2cht-build
FONT=/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc

MAP_ABS="$(readlink -f "$MAP")"
DATA_DIR="$(dirname "$MAP_ABS")"
MAP_NAME="$(basename "$MAP_ABS")"
mkdir -p build

# 引擎 tileset(同 build_poc:優先 U2 Upgrade EGATILES)
EGATILES="${U2UP_TILES:-}"
[[ -z "$EGATILES" && -f tileset/EGATILES ]] && EGATILES="tileset/EGATILES"
[[ -z "$EGATILES" && -f "$DATA_DIR/EGATILES" ]] && EGATILES="$DATA_DIR/EGATILES"
if [[ -n "$EGATILES" && -f "$EGATILES" ]]; then
    echo "tileset: U2 Upgrade EGATILES ($EGATILES)"
    /usr/bin/python3 tools/decode_u2upgrade_tiles.py "$EGATILES" build/tileset.png ega strip
    TILES=/work/build/tileset.png
else
    echo "找不到 EGATILES,改用色塊 fallback"
    TILES=/work/build/tileset.png
fi

docker build -q -t "$IMG" docker/ >/dev/null
docker run --rm \
    -v "$PWD":/work \
    -v "$DATA_DIR":/data:ro \
    "$IMG" bash -c "
        set -e
        cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null
        cmake --build /work/build --target u2_game -j >/dev/null
        /work/build/u2_game /data/$MAP_NAME $FONT $TILES /work/translations/exe_translatable_strings.tsv --script $MOVES /work/$PREFIX
    "
echo "→ ${PREFIX}NN.png"
