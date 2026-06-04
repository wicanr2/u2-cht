#!/usr/bin/env bash
# 一鍵:Docker build 環境 → 編譯 PoC → 跑出 PNG (headless)
# 用法: ./build_poc.sh <mapxNN 路徑> [out.png]
#   原版資料檔請自備 (本 repo 不含)。範例:
#   ./build_poc.sh ../dos-original/ultima2/mapx21 build/poc_out.png
set -euo pipefail
cd "$(dirname "$0")"

MAP="${1:?需指定 mapxNN 路徑 (自備合法 Ultima II 資料)}"
OUT="${2:-build/poc_out.png}"
IMG=u2cht-build
FONT=/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc

docker build -q -t "$IMG" docker/ >/dev/null

mkdir -p build
# 把 repo 與資料檔掛進容器;資料檔路徑換算成容器內路徑
MAP_ABS="$(readlink -f "$MAP")"
docker run --rm \
    -v "$PWD":/work \
    -v "$MAP_ABS":/data/map:ro \
    "$IMG" bash -c "
        set -e
        cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null
        cmake --build /work/build -j >/dev/null
        /work/build/u2_poc /data/map $FONT /work/$OUT
    "
echo "→ $OUT"
