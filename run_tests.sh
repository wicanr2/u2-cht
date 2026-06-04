#!/usr/bin/env bash
# 跑 data 層自動化測試 (headless,Docker)。需自備合法 U2 資料目錄。
# 用法: ./run_tests.sh [ultima2_dir]
set -euo pipefail
cd "$(dirname "$0")"

DATA="${1:-../dos-original/ultima2}"
DATA_ABS="$(readlink -f "$DATA")"
IMG=u2cht-build

docker build -q -t "$IMG" docker/ >/dev/null
docker run --rm -v "$PWD":/work -v "$DATA_ABS":/data:ro "$IMG" bash -c "
    set -e
    cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null
    cmake --build /work/build --target u2_test -j >/dev/null
    /work/build/u2_test /data /work/translations
"
