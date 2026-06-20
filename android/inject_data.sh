#!/usr/bin/env bash
# 把版權遊戲資料注入「引擎 APK」(CI 產)→ 可玩私人 APK,並重新簽章。
# 在 u2cht-android 容器內跑(需 build-tools 的 zipalign/apksigner + keytool)。
# 掛載:/work(repo) /data(Ultima II 資料) /out(輸出);引擎 APK 路徑由 $ENGINE_APK 指定。
set -euxo pipefail

WORK="${WORK:-/work}"
DATA="${DATA:-/data}"
OUT="${OUT:-/out}"
ENGINE_APK="${ENGINE_APK:-$OUT/Ultima2-cht-engine-android.apk}"
BT="$ANDROID_HOME/build-tools/34.0.0"
W=/tmp/apkinject
rm -rf "$W"; mkdir -p "$W/x"

cd "$W/x"
unzip -q "$ENGINE_APK"
A=assets
mkdir -p "$A/data" "$A/tileset"   # 空目錄常不入 zip,解開後需重建

# ---- 注入版權資料 ----
for pat in mapx monx tlkx; do
  for f in "$DATA/$pat"*; do [ -f "$f" ] && cp "$f" "$A/data/"; done
done
# FM Towns tileset / 音樂 / 音效(版權物,僅私人版)
[ -f "$WORK/build/fmtowns_full.png" ] && cp "$WORK/build/fmtowns_full.png" "$A/tileset/fmtowns.png" || true
[ -f "$WORK/build/fmtowns_full_town.png" ] && cp "$WORK/build/fmtowns_full_town.png" "$A/tileset/fmtowns_town.png" || true
if ls "$WORK"/build/music/*.ogg >/dev/null 2>&1; then mkdir -p "$A/music"; cp "$WORK"/build/music/*.ogg "$A/music/"; fi
if ls "$WORK"/build/sfx/*.wav  >/dev/null 2>&1; then mkdir -p "$A/sfx";   cp "$WORK"/build/sfx/*.wav  "$A/sfx/"; fi
# 重建 filelist.txt(android_glue 解壓用)
( cd "$A" && find . -type f ! -name filelist.txt | sed 's#^\./##' | sort > filelist.txt )

# ---- 移除舊簽章 ----
rm -rf META-INF

# ---- 重新打包 + 對齊 + 簽章(image 無 zip,用 python3 zipfile)----
rm -f "$W/unsigned.apk" "$W/aligned.apk"
( cd "$W/x" && python3 - "$W/unsigned.apk" <<'PY'
import zipfile, os, sys
out = sys.argv[1]
z = zipfile.ZipFile(out, 'w', zipfile.ZIP_DEFLATED)
for root, _, files in os.walk('.'):
    for f in files:
        p = os.path.join(root, f)
        z.write(p, os.path.relpath(p, '.'))
z.close()
PY
)
"$BT/zipalign" -f 4 "$W/unsigned.apk" "$W/aligned.apk"
# debug keystore(無則生成;sideload 用)
KS="$HOME/.android/debug.keystore"
if [ ! -f "$KS" ]; then
  mkdir -p "$HOME/.android"
  keytool -genkeypair -keystore "$KS" -storepass android -keypass android \
    -alias androiddebugkey -dname "CN=Android Debug,O=Android,C=US" \
    -keyalg RSA -keysize 2048 -validity 10000 >/dev/null 2>&1
fi
"$BT/apksigner" sign --ks "$KS" --ks-pass pass:android --key-pass pass:android \
  --out "$OUT/Ultima2-繁中-android.apk" "$W/aligned.apk"
echo "可玩 APK：$OUT/Ultima2-繁中-android.apk"
echo "資料檔數：$(cd "$W/x/$A/data" && ls | grep -cE 'map|mon|tlk')"
