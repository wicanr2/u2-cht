# Ultima II 繁中 — Android(觸控)版

C/SDL2 引擎以 SDL2 官方 Android 後端編成 APK,內含遊戲資料,觸控可玩。

## 建置

```bash
# 1) 工具鏈映像(NDK r25 + SDK 34 + Gradle,一次性)
docker build -t u2cht-android android/

# 2) 出 APK(掛載:repo / 資料 / 輸出 / 快取)
docker run --rm \
  -v "$PWD":/work:ro \
  -v /path/to/ultima2-data:/data:ro \
  -v "$PWD/../release_out":/out \
  -v "$PWD/../.andcache":/cache \
  -v "$PWD/../.andgradle":/root/.gradle \
  -v "$PWD/../.andndk":/opt/android-sdk/ndk \
  u2cht-android bash /work/android/build_apk.sh
# → /out/Ultima2-繁中-android.apk
```

產物為 **debug 簽章**(自簽),可直接 sideload;非 Play 商店上架版。

## 安裝(sideload)

1. 手機開啟「允許安裝未知來源 / 安裝未知應用程式」。
2. 把 `Ultima2-繁中-android.apk` 傳到手機,用檔案管理員點開安裝;
   或用 adb:`adb install -r Ultima2-繁中-android.apk`。
3. 首次啟動會把 APK 內的遊戲資料解壓到 app 內部儲存(約一兩秒)。

支援 ABI:arm64-v8a / armeabi-v7a / x86_64 / x86(涵蓋實機與模擬器)。橫向。

## 觸控操作

- 左下 **十字 D-pad**:移動 / 朝怪攻擊;中央 `↵` = 確認 / 通過。
- 右側動作鈕:`談 T` `店 Z` `乘 B` `出 X` `門 P`(時間之門) `表 C` `喊/射 Y`、`≡`(更多)。
- `≡` 面板:`竊 F` `瞰 V` `凝 N` `道具` `消 ESC`。
- 右上系統鈕:`離`(F10 離開) `?`(F1 指令) `語`(F4 切語系) `畫`(G 換畫風)。
- 商店 / 地牢開啟時,底部出現數字列 `1..0`(買賣 / 施法)。
- 選單、建角畫面也有對應方向 / 確認鈕。

> 建角的「姓名輸入」目前需外接鍵盤;其餘流程觸控即可。軟鍵盤輸入為後續改進項。

## 設計

- `src/android_glue.c`:啟動時依 `assets/filelist.txt` 把資料解壓到內部儲存,合成 `main()` 的 argv。
- `src/touch_ui.c`:觸控疊層;觸控命中虛擬鈕 → `SDL_PushEvent` 合成對應 `SDL_KEYDOWN`,
  讓既有(鍵盤驅動的)選單 / 建角 / 主迴圈 / 商店無須改寫即可手機操作。
- 命名空間維持 `org.libsdl.app`(對齊 SDL Java 類別),`applicationId` 才設為 `tw.lairware.ultima2cht`。
- NDK r25(範本預設);NDK r26 的 fortify 會擋某些函式庫對 `readlinkat` 取址。
- 目前無音效 / 音樂(未含 SDL2_mixer Android 後端;待後續)。

## 限制

實機觸控 / 遊玩需在 Android 裝置上驗證;第一版觸控版面可能需依回饋微調。
