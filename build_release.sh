#!/usr/bin/env bash
# 打包 u2-cht 互動遊戲:Linux AppImage(Ubuntu 22.04+)+ Windows zip。
# 在 u2cht-pkg 容器內執行;掛載 /work(repo)、/data(自備 Ultima II 資料)、/out(輸出)。
# 內含能動的遊戲 + 資料(地圖/字型/tileset/翻譯/角色),供使用者自行保存。
set -euxo pipefail
WORK=/work; DATA=/data; OUT=/out
FONT=/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc
cd "$WORK"

# 工具路徑:優先用環境變數,否則在 image 內自動尋找(容忍有無 .AppImage 副檔名)。
find_tool(){  # $1=env值 $2,$3...=候選路徑
  local v="$1"; shift
  [ -n "$v" ] && [ -x "$v" ] && { echo "$v"; return; }
  for c in "$@"; do [ -x "$c" ] && { echo "$c"; return; }; done
  echo ""   # 找不到 → 空字串,呼叫端自行處理
}
LINUXDEPLOY="$(find_tool "${LINUXDEPLOY:-}" /opt/linuxdeploy /opt/linuxdeploy-x86_64.AppImage)"
APPIMAGETOOL="$(find_tool "${APPIMAGETOOL:-}" /opt/appimagetool /opt/appimagetool-x86_64.AppImage)"

# ---------- 0) 產生 tileset strips ----------
mkdir -p /tmp/ts
gen(){ python3 tools/decode_u2upgrade_tiles.py "$1" "$2" "$3" strip >/dev/null 2>&1; }
gen tileset/EGATILES /tmp/ts/ega.png ega
gen tileset/CGATILES /tmp/ts/cga.png cga
[ -f tileset/EGATHEME.ALT/EGATILES ] && gen tileset/EGATHEME.ALT/EGATILES /tmp/ts/ega_alt.png ega || true
[ -f tileset/EGATHEME.C64/EGATILES ] && gen tileset/EGATHEME.C64/EGATILES /tmp/ts/ega_c64.png ega || true
gen tileset/EGATILES /tmp/ts/vivid.png egafmt
# FM Towns:優先用預建完整 tileset(build_fmtowns_tileset.py 產;含 sprite/地形 + 城鎮變體),
# 否則 fallback 舊 5-tile strip。使用者先用 crop+build 工具產生 build/fmtowns_full.png。
if [ -f build/fmtowns_full.png ]; then
  cp build/fmtowns_full.png /tmp/ts/fmtowns.png
  [ -f build/fmtowns_full_town.png ] && cp build/fmtowns_full_town.png /tmp/ts/fmtowns_town.png || true
else
  python3 tools/build_fmtowns_strip.py /tmp/ts/ega.png tileset/fmtowns /tmp/ts/fmtowns.png || true
fi

# ---------- 1) 共用:組裝資料夾 ----------
stage_data(){  # $1 = 目標 share 目錄
  local S="$1"; mkdir -p "$S/data" "$S/font" "$S/tileset" "$S/translations"
  for f in mapx20 monx20 mapx21 monx21 tlkx21 mapx15 monx15; do
    [ -f "$DATA/$f" ] && cp "$DATA/$f" "$S/data/" || true
  done
  cp tests/fixtures/player_sample_abcd "$S/data/player" || true
  cp "$FONT" "$S/font/wqy-zenhei.ttc"
  cp /tmp/ts/*.png "$S/tileset/"
  # FM Towns CDDA 音樂(預抽的 build/music/*.ogg;extract_fmtowns_cdda.py 產;版權,使用者自備)
  if ls build/music/*.ogg >/dev/null 2>&1; then mkdir -p "$S/music"; cp build/music/*.ogg "$S/music/"; fi
  cp translations/exe_translatable_strings.tsv translations/talk_dialogue.tsv translations/ui_strings.tsv "$S/translations/"
# (開場全家福已移除:不打包 splash)
  [ -f docs/screenshots/fmtowns_title_decoded.png ] && cp docs/screenshots/fmtowns_title_decoded.png "$S/title.png" || true  # 原版開場標題
}

# ---------- 2) Linux AppImage ----------
build_appimage(){
  if [ -z "$LINUXDEPLOY" ] || [ -z "$APPIMAGETOOL" ]; then
    echo "錯誤:找不到 linuxdeploy / appimagetool(請設 LINUXDEPLOY / APPIMAGETOOL 或放到 /opt/)" >&2
    exit 1
  fi
  rm -rf /tmp/AppDir; mkdir -p /tmp/AppDir/usr/bin /tmp/AppDir/usr/share/u2cht
  cmake -S "$WORK" -B /tmp/lbuild -DCMAKE_BUILD_TYPE=Release >/dev/null
  cmake --build /tmp/lbuild --target u2_game -j >/dev/null
  cp /tmp/lbuild/u2_game /tmp/AppDir/usr/bin/u2cht_bin   # 真 ELF,獨特名避免與 AppRun 撞名
  stage_data /tmp/AppDir/usr/share/u2cht
  # icon (簡單方塊)
  python3 - <<'PY'
from PIL import Image,ImageDraw
im=Image.new("RGB",(256,256),(20,24,60)); d=ImageDraw.Draw(im)
d.rectangle([20,20,236,236],outline=(250,240,90),width=6)
d.text((60,110),"U2",fill=(250,240,90))
im.save("/tmp/AppDir/u2cht.png")
PY
  cat > /tmp/AppDir/u2cht.desktop <<EOF
[Desktop Entry]
Type=Application
Name=Ultima II 繁中
Exec=u2cht_bin
Icon=u2cht
Categories=Game;
EOF
  # linuxdeploy 打包 SDL2 等依賴(patch rpath)
  ARCH=x86_64 "$LINUXDEPLOY" --appimage-extract-and-run \
    --appdir /tmp/AppDir --executable /tmp/AppDir/usr/bin/u2cht_bin \
    --desktop-file /tmp/AppDir/u2cht.desktop --icon-file /tmp/AppDir/u2cht.png || true
  # 自訂 AppRun:帶入打包資料路徑開互動視窗(呼叫真 ELF u2cht_bin)
  # linuxdeploy 會把 AppRun 建成指向 usr/bin/u2cht_bin 的 symlink;
  # 先 rm 斷開,否則 cat 會寫穿 symlink 覆寫掉真 ELF(自我遞迴)。
  rm -f /tmp/AppDir/AppRun
  cat > /tmp/AppDir/AppRun <<'EOF'
#!/bin/bash
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
S="$HERE/usr/share/u2cht"
TS="$S/tileset/ega.png,$S/tileset/fmtowns.png,$S/tileset/vivid.png,$S/tileset/cga.png,$S/tileset/ega_alt.png,$S/tileset/ega_c64.png"
SPL=""   # 開場全家福已移除
TTL=""; [ -f "$S/title.png" ] && TTL="--title $S/title.png"
# FM Towns 城鎮變體(slot1);其餘畫風 '-' fallback 主 tileset
TWN=""; [ -f "$S/tileset/fmtowns_town.png" ] && TWN="--town-tiles -,$S/tileset/fmtowns_town.png,-,-,-,-"
MUS=""; [ -d "$S/music" ] && MUS="--music $S/music"   # FM Towns CDDA BGM
exec "$HERE/usr/bin/u2cht_bin" "$S/data/mapx20" "$S/font/wqy-zenhei.ttc" "$TS" \
     "$S/translations/exe_translatable_strings.tsv" "$S/data/player" $TTL $SPL $TWN $MUS "$@"
EOF
  chmod +x /tmp/AppDir/AppRun
  ARCH=x86_64 "$APPIMAGETOOL" --appimage-extract-and-run /tmp/AppDir "$OUT/Ultima2-繁中試玩版-x86_64.AppImage"
}

# ---------- 3) Windows zip(mingw cross + SDL2 mingw devel) ----------
build_windows(){
  local M=/tmp/win; rm -rf "$M"; mkdir -p "$M/sdl"
  cd "$M/sdl"
  SDLV=2.30.9; TTFV=2.22.0; IMGV=2.8.2
  wget -q "https://github.com/libsdl-org/SDL/releases/download/release-$SDLV/SDL2-devel-$SDLV-mingw.tar.gz"
  wget -q "https://github.com/libsdl-org/SDL_ttf/releases/download/release-$TTFV/SDL2_ttf-devel-$TTFV-mingw.tar.gz"
  wget -q "https://github.com/libsdl-org/SDL_image/releases/download/release-$IMGV/SDL2_image-devel-$IMGV-mingw.tar.gz"
  for t in *.tar.gz; do tar xzf "$t"; done
  local P=x86_64-w64-mingw32 SDLR="$M/sdl"
  local INC="-I $SDLR/SDL2-$SDLV/$P/include/SDL2 -I $SDLR/SDL2_ttf-$TTFV/$P/include/SDL2 -I $SDLR/SDL2_image-$IMGV/$P/include/SDL2"
  local LIB="-L $SDLR/SDL2-$SDLV/$P/lib -L $SDLR/SDL2_ttf-$TTFV/$P/lib -L $SDLR/SDL2_image-$IMGV/$P/lib"
  cd "$WORK"
  mkdir -p "$M/pkg"
  x86_64-w64-mingw32-gcc -O2 -o "$M/pkg/Ultima2-cht.exe" \
    -Dmain=SDL_main \
    src/game_main.c src/u2_map.c src/u2_mon.c src/u2_play.c src/u2_render.c src/u2_save.c \
    src/u2_strings.c src/u2_talk.c src/u2_tileset.c src/u2_dungeon.c src/u2_text.c \
    -I src $INC $LIB -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -lSDL2_image -lm -mwindows
  # 收 runtime DLL
  cp "$M/sdl/SDL2-$SDLV/$P/bin/SDL2.dll" "$M/pkg/"
  cp "$M/sdl/SDL2_ttf-$TTFV/$P/bin/"*.dll "$M/pkg/"
  cp "$M/sdl/SDL2_image-$IMGV/$P/bin/"*.dll "$M/pkg/"
  stage_data "$M/pkg/share"
  cat > "$M/pkg/玩遊戲.bat" <<'EOF'
@echo off
cd /d "%~dp0"
set TS=share\tileset\ega.png,share\tileset\fmtowns.png,share\tileset\vivid.png,share\tileset\cga.png,share\tileset\ega_alt.png,share\tileset\ega_c64.png
Ultima2-cht.exe share\data\mapx20 share\font\wqy-zenhei.ttc %TS% share\translations\exe_translatable_strings.tsv share\data\player --title share\title.png
EOF
  cat > "$M/pkg/README.txt" <<'EOF'
Ultima II: 女巫的復仇 — 繁體中文化(C/SDL2 重寫引擎)【試玩版 / DEMO】

※ 這是試玩版(demo),用於展示重寫引擎與中文化成果,非完整遊戲。
   部分系統(完整劇情、結局、所有城鎮/地牢)仍在開發中。

雙擊「玩遊戲.bat」開始。

開場:原版標題 → 選單(新遊戲 / 繼續)。
  新遊戲:建立角色(姓名 → 性別 → 種族 → 職業 → 屬性分配)。
  繼續:載入上次存檔(離開遊戲時自動存檔)。
  存檔位置:%APPDATA%\LairWare-cht\Ultima2\player

操作:
  移動/攻擊  方向鍵 / WASD(朝怪物移動即攻擊)
  載具       B 登載/下載(馬/船/飛機/火箭) · Y 火箭發射 / 太空降落
  時空旅行   P 或踏入青紫時間之門 → 切換五大時代
  進入地點   走上城堡圖塊進城 · 走上地牢圖塊入地牢 · X 離開
  城鎮       T 與 NPC 交談 · Z 商店(補給/升級裝備/買關鍵道具)
  介面       C 角色表(含任務提示) · G 切換畫風 · F4 切換語系(繁中/EN/日)
  系統       F1 指令表 · Q 離開(自動存檔)

遊戲目標(試玩版可走通的主線):
  建角 → 商店補給/取得載具 → 時空旅行各時代 → 行星拜訪 Father Antos 取得力場之戒
  → 晉見不列顛王取得迅捷之劍 Enilno → 前往「傳說時代」巢穴擊敗女巫米娜克斯 → 結局。
  (角色表 C 會依進度顯示下一步任務提示;陣亡會被不列顛王復活,失去半數黃金。)
EOF
  cd "$M/pkg" && zip -qr "$OUT/Ultima2-繁中試玩版-windows.zip" . && cd "$WORK"
}

case "${1:-all}" in
  appimage) build_appimage ;;
  windows)  build_windows ;;
  *)        build_appimage; build_windows ;;
esac
ls -la "$OUT"
