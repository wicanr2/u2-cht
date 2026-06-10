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

# 可切換 tileset:產生 CGA / EGA / EGA-ALT / EGA-C64 strip(有哪個算哪個)
# 主要來源 tileset/(repo 內 CGATILES/EGATILES);EGATHEME 變體需 u2upgrade-2.1.zip。
mkdir -p build/ts build/u2up
gen() { /usr/bin/python3 tools/decode_u2upgrade_tiles.py "$1" "$2" "$3" strip >/dev/null 2>&1 && echo "$2"; }
declare -a STRIPS
[[ -f tileset/EGATILES ]] && STRIPS+=("$(gen tileset/EGATILES build/ts/ega.png ega)")
# 真實 FM Towns 地形(模擬器抽出)覆寫到 EGA strip 對應 id
if [[ -f build/ts/ega.png && -d tileset/fmtowns ]]; then
    /usr/bin/python3 tools/build_fmtowns_strip.py build/ts/ega.png tileset/fmtowns build/ts/fmtowns.png >/dev/null 2>&1 \
      && STRIPS+=("build/ts/fmtowns.png")
fi
[[ -f tileset/EGATILES ]] && STRIPS+=("$(gen tileset/EGATILES build/ts/vivid.png egafmt)")  # 鮮豔配色(FM Towns 風近似)
[[ -f tileset/CGATILES ]] && STRIPS+=("$(gen tileset/CGATILES build/ts/cga.png cga)")
# EGATHEME 變體:優先 repo 內 tileset/(已收錄),否則從 u2upgrade zip 解
ALT=""; C64=""
[[ -f tileset/EGATHEME.ALT/EGATILES ]] && ALT="tileset/EGATHEME.ALT/EGATILES"
[[ -f tileset/EGATHEME.C64/EGATILES ]] && C64="tileset/EGATHEME.C64/EGATILES"
if [[ -z "$ALT" || -z "$C64" ]]; then
    ZIP="$(ls ../u2upgrade-2.1.zip 2>/dev/null | head -1 || true)"
    if [[ -n "${ZIP:-}" && -f "$ZIP" ]]; then
        ( cd build/u2up && unzip -o "$(readlink -f "$OLDPWD/$ZIP")" 'EGATHEME.ALT/*' 'EGATHEME.C64/*' >/dev/null 2>&1 ) || true
        [[ -z "$ALT" && -f build/u2up/EGATHEME.ALT/EGATILES ]] && ALT="build/u2up/EGATHEME.ALT/EGATILES"
        [[ -z "$C64" && -f build/u2up/EGATHEME.C64/EGATILES ]] && C64="build/u2up/EGATHEME.C64/EGATILES"
    fi
fi
[[ -n "$ALT" ]] && STRIPS+=("$(gen "$ALT" build/ts/ega_alt.png ega)")
[[ -n "$C64" ]] && STRIPS+=("$(gen "$C64" build/ts/ega_c64.png ega)")
# 串成逗號清單(容器內路徑 /work/...)
TILES=""
for s in "${STRIPS[@]}"; do [[ -n "$s" ]] && TILES="${TILES:+$TILES,}/work/$s"; done
echo "tilesets: $TILES"

docker build -q -t "$IMG" docker/ >/dev/null
docker run --rm \
    -v "$PWD":/work \
    -v "$DATA_DIR":/data:ro \
    "$IMG" bash -c "
        set -e
        cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release >/dev/null
        cmake --build /work/build --target u2_game -j >/dev/null
        SPL=""; [ -f /work/build/splash.png ] && SPL="--splash /work/build/splash.png"
        TTL=""; [ -f /work/docs/screenshots/fmtowns_title_decoded.png ] && TTL="--title /work/docs/screenshots/fmtowns_title_decoded.png"
        /work/build/u2_game /data/$MAP_NAME $FONT $TILES /work/translations/exe_translatable_strings.tsv /work/tests/fixtures/player_sample_abcd \$TTL \$SPL --script $MOVES /work/$PREFIX
    "
echo "→ ${PREFIX}NN.png"
