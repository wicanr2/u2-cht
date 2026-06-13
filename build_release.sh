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
  # 完整遊戲資料(全部 mapx/monx/tlkx;否則進不了多數城鎮/時代)。ENGINE_ONLY 或空 /data 時自然帶 0 檔。
  for pat in mapx monx tlkx; do
    for f in "$DATA/$pat"*; do [ -f "$f" ] && cp "$f" "$S/data/"; done
  done
  # 不打包測試角色 → 開局走建角流程(對齊原版;避免「範例角色」滿狀態/卡住)。
  cp "$FONT" "$S/font/wqy-zenhei.ttc"
  cp /tmp/ts/*.png "$S/tileset/"
  # FM Towns CDDA 音樂 / 原版音效(版權物;ENGINE_ONLY=1 時排除,供公開引擎包)
  if [ "${ENGINE_ONLY:-0}" != "1" ]; then
    if ls build/music/*.ogg >/dev/null 2>&1; then mkdir -p "$S/music"; cp build/music/*.ogg "$S/music/"; fi
    if ls build/sfx/*.wav >/dev/null 2>&1; then mkdir -p "$S/sfx"; cp build/sfx/*.wav "$S/sfx/"; fi
  fi
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
SFX=""; [ -d "$S/sfx" ] && SFX="--sfx $S/sfx"         # FM Towns 原版音效
exec "$HERE/usr/bin/u2cht_bin" "$S/data/mapx20" "$S/font/wqy-zenhei.ttc" "$TS" \
     "$S/translations/exe_translatable_strings.tsv" "$S/data/player" $TTL $SPL $TWN $MUS $SFX "$@"
EOF
  chmod +x /tmp/AppDir/AppRun
  ARCH=x86_64 "$APPIMAGETOOL" --appimage-extract-and-run /tmp/AppDir "$OUT/Ultima2-繁中-x86_64.AppImage"
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
set AUD=
if exist share\music set AUD=%AUD% --music share\music
if exist share\sfx set AUD=%AUD% --sfx share\sfx
Ultima2-cht.exe share\data\mapx20 share\font\wqy-zenhei.ttc %TS% share\translations\exe_translatable_strings.tsv share\data\player --title share\title.png %AUD%
EOF
  cat > "$M/pkg/README.txt" <<'EOF'
Ultima II: 女巫的復仇 — 繁體中文化(C/SDL2 重寫引擎)

忠實對齊原版、可從建角一路玩到擊敗女巫米娜克斯的結局。

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
  系統       F1 指令表 · ESC 取消 · F10 離開(自動存檔)

遊戲目標(試玩版可走通的主線):
  建角 → 商店補給/取得載具 → 時空旅行各時代 → 行星拜訪 Father Antos 取得力場之戒
  → 晉見不列顛王取得迅捷之劍 Enilno → 前往「傳說時代」巢穴擊敗女巫米娜克斯 → 結局。
  (角色表 C 會依進度顯示下一步任務提示;陣亡會被不列顛王復活,失去半數黃金。)
EOF
  cd "$M/pkg" && zip -qr "$OUT/Ultima2-繁中-windows.zip" . && cd "$WORK"
}

# ---------- 4) macOS 原始碼包(在 Mac 上一鍵 build,因 Linux 無法可靠跨編 Mach-O)----------
build_mac() {
  local M=/tmp/mac; rm -rf "$M"; local P="$M/Ultima2-mac"; mkdir -p "$P/src"
  # 原始碼 + CMake(Mac 上用 brew 的 SDL2 + pkg-config 編譯)
  cp CMakeLists.txt "$P/"
  cp src/*.c src/*.h "$P/src/"
  stage_data "$P/share"
  # 一鍵 build 腳本(.command 在 macOS 可雙擊於 Terminal 執行)
  cat > "$P/build_mac.command" <<'EOF'
#!/bin/bash
# Ultima II 繁中重製 — macOS 一鍵編譯 + 執行
set -e
cd "$(dirname "$0")"
echo "== 1) 檢查相依(需 Homebrew)=="
if ! command -v brew >/dev/null 2>&1; then
  echo "找不到 Homebrew。請先安裝:https://brew.sh 然後重跑本檔。"; read -r _; exit 1
fi
for p in cmake pkg-config sdl2 sdl2_ttf sdl2_image sdl2_mixer; do
  brew list "$p" >/dev/null 2>&1 || brew install "$p"
done
echo "== 2) 編譯 =="
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target u2_game -j
echo "== 3) 組 Ultima2.app =="
APP="Ultima2.app"; rm -rf "$APP"
mkdir -p "$APP/Contents/MacOS" "$APP/Contents/Resources"
cp build/u2_game "$APP/Contents/MacOS/u2_game"
cp -R share "$APP/Contents/Resources/share"
cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>Ultima2-cht</string>
  <key>CFBundleExecutable</key><string>launch</string>
  <key>CFBundleIdentifier</key><string>tw.lairware.ultima2cht</string>
  <key>CFBundlePackageType</key><string>APPL</string>
</dict></plist>
PLIST
cat > "$APP/Contents/MacOS/launch" <<'LAUNCH'
#!/bin/bash
D="$(cd "$(dirname "$0")/../Resources/share" && pwd)"
TS="$D/tileset/ega.png,$D/tileset/fmtowns.png,$D/tileset/vivid.png,$D/tileset/cga.png,$D/tileset/ega_alt.png,$D/tileset/ega_c64.png"
TWN=""; [ -f "$D/tileset/fmtowns_town.png" ] && TWN="--town-tiles -,$D/tileset/fmtowns_town.png,-,-,-,-"
TTL=""; [ -f "$D/title.png" ] && TTL="--title $D/title.png"
MUS=""; [ -d "$D/music" ] && MUS="--music $D/music"
SFX=""; [ -d "$D/sfx" ] && SFX="--sfx $D/sfx"
exec "$(dirname "$0")/u2_game" "$D/data/mapx20" "$D/font/wqy-zenhei.ttc" "$TS" \
     "$D/translations/exe_translatable_strings.tsv" "$D/data/player" $TTL $TWN $MUS $SFX
LAUNCH
chmod +x "$APP/Contents/MacOS/launch"
echo "== 完成!雙擊 Ultima2.app 開始遊玩(或本視窗下方直接啟動)=="
open "$APP"
EOF
  chmod +x "$P/build_mac.command"
  cat > "$P/README.txt" <<'EOF'
Ultima II: 女巫的復仇 — 繁體中文化(C/SDL2 重寫引擎)· macOS 版

因 Linux 無法可靠跨編 macOS 二進位,本包為「原始碼 + 一鍵編譯」:

  1. 雙擊「build_mac.command」(若被 Gatekeeper 擋:右鍵 → 打開,或
     系統設定 → 隱私權與安全性 → 仍要打開)。
  2. 首次會用 Homebrew 自動安裝 SDL2 等相依(需先裝 Homebrew:https://brew.sh)。
  3. 編譯完成後自動產生並開啟 Ultima2.app。之後雙擊 Ultima2.app 即可遊玩。

操作:方向鍵/WASD 移動(朝怪移動=攻擊)· P 穿時間之門 · B 載具 · T 交談 · Z 商店
      · C 角色/任務 · G 換畫風 · F4 切語系 · F1 指令表 · ESC 取消 · F10 離開(自動存檔)。
含 FM Towns 原版音效;intro/結局 CDDA 音樂;遊玩中目前靜音(EUP 算繪待處理)。
EOF
  cd "$M" && zip -qr "$OUT/Ultima2-繁中-mac.zip" "Ultima2-mac" && cd "$WORK"
}

case "${1:-all}" in
  appimage) build_appimage ;;
  windows)  build_windows ;;
  mac)      build_mac ;;
  *)        build_appimage; build_windows; build_mac ;;
esac
ls -la "$OUT"
