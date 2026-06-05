#!/usr/bin/env bash
# 一鍵:抽真 tile → Docker build → 跑出 PoC PNG (headless)
# 用法: ./build_poc.sh <mapxNN 路徑> [out.png] [palette]
#   原版資料檔請自備 (本 repo 不含)。範例:
#   ./build_poc.sh ../dos-original/ultima2/mapx21 build/poc_out.png original
set -euo pipefail
cd "$(dirname "$0")"

MAP="${1:?需指定 mapxNN 路徑 (自備合法 Ultima II 資料)}"
OUT="${2:-build/poc_out.png}"
PAL="${3:-original}"
IMG=u2cht-build
FONT=/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc

MAP_ABS="$(readlink -f "$MAP")"
DATA_DIR="$(dirname "$MAP_ABS")"
MAP_NAME="$(basename "$MAP_ABS")"
EXE="$DATA_DIR/ultimaii.exe"
mkdir -p build

# 1) 產引擎 tileset → build/tileset.png
#    優先用 U2 Upgrade EGATILES (正確 ground truth);找不到才 fallback EXE @0x7C42 (PoC,錯誤)
EGATILES="${U2UP_TILES:-}"
[[ -z "$EGATILES" && -f "$DATA_DIR/EGATILES" ]] && EGATILES="$DATA_DIR/EGATILES"
[[ -z "$EGATILES" && -f build/u2up/EGATILES ]] && EGATILES="build/u2up/EGATILES"
if [[ -n "$EGATILES" && -f "$EGATILES" ]]; then
    echo "tileset: U2 Upgrade EGATILES ($EGATILES)"
    /usr/bin/python3 tools/decode_u2upgrade_tiles.py "$EGATILES" build/tileset.png ega strip
    TILES=/work/build/tileset.png
elif [[ -f "$EXE" ]]; then
    echo "⚠️ 無 EGATILES,fallback EXE @0x7C42 tile (PoC,非正確 tileset)"
    /usr/bin/python3 tools/extract_tiles.py "$EXE" build/tileset.png "$PAL"
    TILES=/work/build/tileset.png
else
    echo "找不到 tileset 來源,改用色塊 fallback"
    TILES=""
fi

# 2) Docker build + 跑
docker build -q -t "$IMG" docker/ >/dev/null
# 掛整個資料目錄 (唯讀) → mon/tlk 同目錄檔可一併讀到
docker run --rm \
    -v "$PWD":/work \
    -v "$DATA_DIR":/data:ro \
    "$IMG" bash -c "
        set -e
        cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null
        cmake --build /work/build -j >/dev/null
        /work/build/u2_poc /data/$MAP_NAME $FONT /work/$OUT $TILES /work/translations/talk_dialogue.tsv
    "
echo "→ $OUT"
