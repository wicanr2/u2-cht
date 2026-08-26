#!/usr/bin/env bash
# 組裝本機自用的 macOS 完整版；不建置 Mach-O，只接受已驗證的 binary。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${1:?用法: package_macos_full.sh <u2_game Mach-O> <版本> [輸出根目錄]}"
VERSION="${2:?缺少版本}"
OUT_ROOT="${3:-$ROOT/dist-all}"
FONT="${FONT:-/usr/share/fonts/truetype/wqy/wqy-zenhei.ttc}"
SOURCE_COMMIT="${SOURCE_COMMIT:-unknown}"
MACOS_ARCH="${MACOS_ARCH:-arm64}"
OUT="$OUT_ROOT/$VERSION/full/macos-$MACOS_ARCH"
APP="$OUT/Ultima2.app"
SHARE="$APP/Contents/Resources/share"

test -f "$BIN"
test -f "$FONT"
test -f "$ROOT/build/dosgame/mapx20"

# 只替換這個版本、這個平台的輸出，不碰 dist-all 其他交付物。
rm -rf "$OUT"
mkdir -p "$APP/Contents/MacOS" "$SHARE/data" "$SHARE/font" \
         "$SHARE/tileset" "$SHARE/translations" "$SHARE/music" "$SHARE/sfx"

cp "$BIN" "$APP/Contents/MacOS/u2_game"
chmod 0755 "$APP/Contents/MacOS/u2_game"

# 完整原版資料；故意不帶 player，讓正常玩家路徑從建角開始。
for pat in mapx monx tlkx; do
    for f in "$ROOT/build/dosgame/$pat"*; do
        test -f "$f" && cp "$f" "$SHARE/data/"
    done
done

cp "$FONT" "$SHARE/font/wqy-zenhei.ttc"
cp "$ROOT/build/ts/ega.png" "$ROOT/build/ts/cga.png" \
   "$ROOT/build/ts/vivid.png" "$ROOT/build/ts/ega_alt.png" \
   "$ROOT/build/ts/ega_c64.png" "$SHARE/tileset/"
cp "$ROOT/build/fmtowns_full.png" "$SHARE/tileset/fmtowns.png"
cp "$ROOT/build/fmtowns_full_town.png" "$SHARE/tileset/fmtowns_town.png"
cp "$ROOT/translations/exe_translatable_strings.tsv" \
   "$ROOT/translations/talk_dialogue.tsv" \
   "$ROOT/translations/ui_strings.tsv" "$SHARE/translations/"
cp "$ROOT/build/music/"*.ogg "$SHARE/music/"
cp "$ROOT/build/sfx/"*.wav "$SHARE/sfx/"
cp "$ROOT/docs/screenshots/fmtowns_title_decoded.png" "$SHARE/title.png"

cat > "$APP/Contents/Info.plist" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
  <key>CFBundleName</key><string>Ultima2-cht</string>
  <key>CFBundleDisplayName</key><string>創世紀 II：女巫的復仇</string>
  <key>CFBundleExecutable</key><string>launch</string>
  <key>CFBundleIdentifier</key><string>tw.lairware.ultima2cht</string>
  <key>CFBundleVersion</key><string>${VERSION}</string>
  <key>CFBundleShortVersionString</key><string>${VERSION}</string>
  <key>CFBundlePackageType</key><string>APPL</string>
  <key>LSMinimumSystemVersion</key><string>11.0</string>
  <key>NSHighResolutionCapable</key><true/>
</dict></plist>
PLIST

cat > "$APP/Contents/MacOS/launch" <<'LAUNCH'
#!/bin/sh
HERE="$(cd "$(dirname "$0")" && pwd)"
D="$(cd "$HERE/../Resources/share" && pwd)"
TS="$D/tileset/ega.png,$D/tileset/fmtowns.png,$D/tileset/vivid.png,$D/tileset/cga.png,$D/tileset/ega_alt.png,$D/tileset/ega_c64.png"
TWN="--town-tiles -,$D/tileset/fmtowns_town.png,-,-,-,-"
exec "$HERE/u2_game" "$D/data/mapx20" "$D/font/wqy-zenhei.ttc" "$TS" \
     "$D/translations/exe_translatable_strings.tsv" \
     --title "$D/title.png" $TWN --music "$D/music" --sfx "$D/sfx" "$@"
LAUNCH
chmod 0755 "$APP/Contents/MacOS/launch"

cat > "$OUT/README-完整版.txt" <<EOF
創世紀 II：女巫的復仇——繁體中文 macOS ${MACOS_ARCH} 完整版
版本：${VERSION}

1. 解壓後以右鍵選擇「打開」Ultima2.app。
2. 本包含原版遊戲資料及 FM Towns 音樂／音效，只供合法持有者本機自用，請勿公開散布。
3. 存檔位於 macOS 使用者偏好資料夾，不會寫回 App 內的原版資料。

此包由 Linux 上的 osxcross + macOS 15.5 SDK 交叉建置，已通過 Mach-O 靜態驗收；
Linux 無法執行 macOS 程式，因此仍需在 Apple Silicon Mac 上做實機啟動與遊玩驗收。
EOF

cat > "$OUT/BUILD-METADATA.txt" <<EOF
version=${VERSION}
architecture=${MACOS_ARCH}
minimum_macos=11.0
sdk=macOS 15.5
sdl=2.30.9
sdl_ttf=2.22.0
sdl_image=2.8.2
sdl_mixer=2.8.0
packaging=full-private
source_commit=${SOURCE_COMMIT}
EOF

(cd "$OUT" && zip -qry -y "Ultima2-繁中-macos-${MACOS_ARCH}-full.zip" "Ultima2.app" \
    "README-完整版.txt" "BUILD-METADATA.txt")
sha256sum "$OUT/Ultima2-繁中-macos-${MACOS_ARCH}-full.zip" > \
    "$OUT/Ultima2-繁中-macos-${MACOS_ARCH}-full.zip.sha256"

echo "$OUT/Ultima2-繁中-macos-${MACOS_ARCH}-full.zip"
