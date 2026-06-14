#!/usr/bin/env bash
# 在 u2cht-android 容器內:組裝 SDL2 Android 專案 + 引擎源 + assets → 出 APK。
# 掛載:/work(repo) /data(Ultima II 資料,唯讀) /out(輸出)
set -euxo pipefail

WORK=/work
DATA=/data
OUT=/out
SDLV=2.30.9
TTFV=2.22.0
IMGV=2.8.2
PKG=tw.lairware.ultima2cht
BUILD=/tmp/andbuild
SRC=/tmp/andsrc

rm -rf "$BUILD" "$SRC"; mkdir -p "$BUILD" "$SRC"

# ---------- 1) 下載 SDL 系列原始碼(/cache 重用,加速反覆建置)----------
CACHE=/cache; mkdir -p "$CACHE"
cd "$CACHE"
[ -f "SDL2-$SDLV.tar.gz" ]     || wget -q "https://github.com/libsdl-org/SDL/releases/download/release-$SDLV/SDL2-$SDLV.tar.gz"
[ -f "SDL2_ttf-$TTFV.tar.gz" ] || wget -q "https://github.com/libsdl-org/SDL_ttf/releases/download/release-$TTFV/SDL2_ttf-$TTFV.tar.gz"
[ -f "SDL2_image-$IMGV.tar.gz" ] || wget -q "https://github.com/libsdl-org/SDL_image/releases/download/release-$IMGV/SDL2_image-$IMGV.tar.gz"
cd "$SRC"
tar xzf "$CACHE/SDL2-$SDLV.tar.gz"
tar xzf "$CACHE/SDL2_ttf-$TTFV.tar.gz"
tar xzf "$CACHE/SDL2_image-$IMGV.tar.gz"

# ---------- 2) 以 SDL 的 android-project 為範本 ----------
cp -r "SDL2-$SDLV/android-project/." "$BUILD/"
mkdir -p "$BUILD/app/jni"
cp -r "SDL2-$SDLV"            "$BUILD/app/jni/SDL"
cp -r "SDL2_ttf-$TTFV"        "$BUILD/app/jni/SDL2_ttf"
cp -r "SDL2_image-$IMGV"      "$BUILD/app/jni/SDL2_image"
# SDL 的 Java 類別(org.libsdl.app.*)隨範本帶入;確認存在
test -d "$BUILD/app/src/main/java/org/libsdl/app"

# ---------- 3) 放入引擎 C 源 + Android.mk ----------
JNISRC="$BUILD/app/jni/src"
mkdir -p "$JNISRC"
cp "$WORK"/src/game_main.c "$WORK"/src/u2_map.c "$WORK"/src/u2_mon.c "$WORK"/src/u2_play.c \
   "$WORK"/src/u2_render.c "$WORK"/src/u2_save.c "$WORK"/src/u2_strings.c "$WORK"/src/u2_talk.c \
   "$WORK"/src/u2_tileset.c "$WORK"/src/u2_dungeon.c "$WORK"/src/u2_text.c "$WORK"/src/touch_ui.c \
   "$WORK"/src/android_glue.c "$WORK"/src/*.h "$JNISRC/"

cat > "$JNISRC/Android.mk" <<'MK'
LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := main
SDL_PATH := ../SDL
# tr()/i18n 回傳執行期字串作 snprintf 格式 → 關閉 NDK 預設的 format-security Werror
LOCAL_CFLAGS := -Wno-error=format-security -Wno-format-security -Wno-format-nonliteral
LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/$(SDL_PATH)/include \
    $(LOCAL_PATH)/../SDL2_ttf \
    $(LOCAL_PATH)/../SDL2_image \
    $(LOCAL_PATH)
LOCAL_SRC_FILES := \
    game_main.c u2_map.c u2_mon.c u2_play.c u2_render.c u2_save.c \
    u2_strings.c u2_talk.c u2_tileset.c u2_dungeon.c u2_text.c touch_ui.c android_glue.c
LOCAL_SHARED_LIBRARIES := SDL2 SDL2_ttf SDL2_image
LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -lOpenSLES -llog -landroid
include $(BUILD_SHARED_LIBRARY)
MK

# Application.mk:ABI + 平台 + 全域關閉 format-security Werror(tr() 動態格式字串)
cat > "$BUILD/app/jni/Application.mk" <<'MK'
APP_ABI := arm64-v8a armeabi-v7a
APP_PLATFORM := android-21
APP_STL := c++_shared
APP_CFLAGS := -Wno-error=format-security -Wno-format-security
MK

# ---------- 4) 套用設定:applicationId / 名稱 / 橫向 ----------
# applicationId = 安裝身分;namespace 維持 org.libsdl.app 以對齊 SDL Java 類別。
# (若改 namespace,manifest 的 android:name="SDLActivity" 會解析到不存在的類別 → 開機崩潰)
grep -q "applicationId" "$BUILD/app/build.gradle" || \
  sed -i "s/        minSdkVersion 19/        minSdkVersion 19\n        applicationId \"$PKG\"/" "$BUILD/app/build.gradle"
# NDK 版本:沿用範本預設(25.1.8937393);NDK 26 的 fortify 會擋某些函式庫對 readlinkat 取址。
# 由 /opt/android-sdk/ndk 持久化掛載重用,避免每次重抓。
# app 名稱
mkdir -p "$BUILD/app/src/main/res/values"
cat > "$BUILD/app/src/main/res/values/strings.xml" <<'XML'
<?xml version="1.0" encoding="utf-8"?>
<resources>
    <string name="app_name">Ultima II 繁中</string>
</resources>
XML

# AndroidManifest:只鎖橫向(不改套件名 → activity 類別維持 org.libsdl.app.SDLActivity)
MAN="$BUILD/app/src/main/AndroidManifest.xml"
grep -q 'screenOrientation' "$MAN" || \
  sed -i 's/<activity android:name="SDLActivity"/<activity android:name="SDLActivity" android:screenOrientation="sensorLandscape"/' "$MAN"

# ---------- 5) 組 assets(data + tileset + font + translations + title + filelist) ----------
A="$BUILD/app/src/main/assets"
rm -rf "$A"; mkdir -p "$A/data" "$A/tileset" "$A/font" "$A/translations"
for pat in mapx monx tlkx; do
  for f in "$DATA/$pat"*; do [ -f "$f" ] && cp "$f" "$A/data/"; done
done
# tileset strips(容器內現產)
gen(){ python3 "$WORK"/tools/decode_u2upgrade_tiles.py "$1" "$2" "$3" strip >/dev/null 2>&1 || true; }
gen "$WORK"/tileset/EGATILES "$A/tileset/ega.png" ega
gen "$WORK"/tileset/CGATILES "$A/tileset/cga.png" cga
gen "$WORK"/tileset/EGATILES "$A/tileset/vivid.png" egafmt
[ -f "$WORK"/build/fmtowns_full.png ] && cp "$WORK"/build/fmtowns_full.png "$A/tileset/fmtowns.png" || true
[ -f "$WORK"/build/fmtowns_full_town.png ] && cp "$WORK"/build/fmtowns_full_town.png "$A/tileset/fmtowns_town.png" || true
cp "$WORK"/translations/exe_translatable_strings.tsv "$WORK"/translations/ui_strings.tsv "$A/translations/"
[ -f "$WORK"/docs/screenshots/fmtowns_title_decoded.png ] && cp "$WORK"/docs/screenshots/fmtowns_title_decoded.png "$A/title.png" || true
# 中文字型:Noto Sans CJK(穩定來源;快取)
[ -f "$CACHE/NotoSansCJKsc-Regular.otf" ] || curl -fsSL -A "Mozilla/5.0" -o "$CACHE/NotoSansCJKsc-Regular.otf" \
  "https://github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf"
cp "$CACHE/NotoSansCJKsc-Regular.otf" "$A/font/wqy-zenhei.ttc"
# filelist.txt(相對 assets 根;android_glue 逐檔解壓用)
( cd "$A" && find . -type f ! -name filelist.txt | sed 's#^\./##' | sort > filelist.txt )
wc -l "$A/filelist.txt"

# ---------- 6) Gradle 出 APK ----------
cd "$BUILD"
export ANDROID_HOME ANDROID_SDK_ROOT ANDROID_NDK_HOME
chmod +x gradlew || true
./gradlew --no-daemon assembleDebug

APK=$(find "$BUILD" -name "*.apk" -path "*debug*" | head -1)
echo "APK = $APK"
cp "$APK" "$OUT/Ultima2-繁中-android.apk"
ls -la "$OUT/Ultima2-繁中-android.apk"
