# 06 · 打包(Docker first + 跨平台 + 引擎/資料分離)

## 鐵則:引擎與版權資料分離
- **公開**(MIT):自寫引擎 `src/`、工具 `tools/`、文件 `docs/`、反編 oracle(史保存)。
- **不散布、gitignore**:原版 binary、遊戲資料(map/mon/tlk)、各版本美術/音樂/音效。玩家自備合法副本。
- **公開 Release = 引擎包**(`ENGINE_ONLY=1` 跳過版權音訊;空 /data → 不帶遊戲資料)。**本機完整版**(含資料、可直接玩)只私人留存。

## Docker first(三個常用 image)
- `*-build`:編譯 + SDL2 + wqy 中文字型 + SDL_mixer。跑回歸測試。
- `*-pkg`:打包(PIL/mingw/linuxdeploy/appimagetool)。
- `*-test`:Xvfb/ffmpeg/objdump headless 測試。
- RE 用:`*-ghidra`、`*-re`(capstone)、`eupmini`(EUP 算繪)。Dockerfile 都收進 repo `docker/`。

## 三平台
- **Linux AppImage**:linuxdeploy + appimagetool;自訂 AppRun 帶資料路徑。注意 linuxdeploy 會把 AppRun 建成 symlink → 先 `rm` 再寫,否則寫穿覆蓋真 ELF。
- **Windows zip**:mingw cross(`x86_64-w64-mingw32-gcc -Dmain=SDL_main ... -mwindows`)+ 收 SDL2 runtime DLL + `玩遊戲.bat`。
- **macOS**:Linux **無法可靠跨編 Mach-O** → 用 **GitHub Actions macOS runner 原生編譯** `.app`。
  - `.github/workflows/build-mac.yml`:brew 裝 SDL2 系列 → cmake → 組 .app(launch wrapper + Info.plist + ad-hoc codesign);`workflow_dispatch` + push `v*` tag 觸發、tag 時 `action-gh-release` 附到 Release。
  - **Mac CI 兩個必踩雷**:① `ld: library 'SDL2' not found` ── Homebrew 在 `/opt/homebrew/lib`(非預設搜尋路徑),CMakeLists 要加 **`target_link_directories(${SDL2*_LIBRARY_DIRS})`**(Linux /usr/lib 無害)。② 字型 cask `font-wqy-zenhei` 不存在、多數 raw URL 404 ── 改 curl **Noto Sans CJK**(`github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf`,含中日韓,SDL_ttf 可載 OTF)。

## Android APK(觸控)
SDL2 有官方 Android 後端 → 用 SDL 源碼內 `android-project/` 當範本,引擎源放 `app/jni/src`,gradle 出 APK。Docker 工具鏈(NDK+SDK+Gradle)收進 `android/Dockerfile`、組裝/建置腳本 `android/build_apk.sh`。**踩雷**:
- **NDK 用 r25 不要 r26**:r26 的 fortify(`pass_object_size`)會擋某些函式庫對 `&readlinkat` 取址而編譯失敗。沿用範本預設 25.1.8937393。
- **`-Werror=format-security`**:NDK clang 預設開啟,`snprintf(buf,n,tr(...))`(i18n 回傳執行期字串)全爆。**在原始碼頂端 `#pragma clang diagnostic ignored "-Wformat-security"`**(pragma 勝過 AGP 最後追加的 -Werror;`LOCAL_CFLAGS`/`APP_CFLAGS`/gradle 旗標都因排序輸掉)。`#if defined(__clang__)` 包住 → 桌面 gcc 不受影響。
- **namespace vs applicationId**:**namespace 維持 `org.libsdl.app`**(對齊 SDL 的 Java 類別),只在 defaultConfig 設 `applicationId`。若全域改套件名,manifest 的 `android:name="SDLActivity"` 會解析到不存在的類別 → **開機 ClassNotFoundException 崩潰**(badging 看 launchable-activity 必須仍是 `org.libsdl.app.SDLActivity`)。
- **assets 讀取**:引擎用 `fopen` 讀不到 APK 內 assets。加 Android 啟動引導:依 `assets/filelist.txt`(打包時 `find` 產生)用 `SDL_RWFromFile`(相對路徑→APK assets)讀出、`fopen` 寫到 `SDL_AndroidGetInternalStoragePath()`,再合成 `main()` 的 argv 指向內部儲存。版本標記檔控制是否重解壓。
- **觸控操作層**:做一個疊層,`SDL_FINGERDOWN` 命中虛擬鈕 → `SDL_PushEvent` 合成對應 `SDL_KEYDOWN`。如此**選單/建角/主迴圈/商店等既有鍵盤驅動畫面全部無須改寫**即可手機操作。疊層畫進 canvas(隨 RenderCopy(NULL,NULL) 自動縮放),觸控 normalized 座標 ×canvas 尺寸做命中。桌面預設停用 → 零回歸。虛擬鈕要**對齊實際功能鍵**,且**勿把除錯/作弊鍵放成按鈕**。
- **文字輸入(姓名等)**:手機沒實體鍵盤 → 需輸入文字的步驟 `SDL_StartTextInput()` 叫出軟鍵盤、吃 `SDL_TEXTINPUT`(換行當 RETURN);非文字步驟 `SDL_StopTextInput()` 收起。字元只走 TEXTINPUT、KEYDOWN 只傳控制鍵 → 桌面不重複。
- **音訊(SDL2_mixer)**:預設沒連 → 靜音。`build_apk.sh` 加 SDL2_mixer 源 + 連結 + `-DHAVE_SDL_MIXER=1`;**關 `SUPPORT_WAVPACK`**(預設 true 但需外部 wavpack 模組 → `undefined modules: wavpack`)。WAV + OGG(stb_vorbis)為 single-header 預設可用、免外部 libvorbis。
- **CI 編 + 本地注入(省時)**:本地 `--rm` 容器從零編 4 ABI ~30 分。改用 **GitHub Actions(ubuntu runner,SDK/NDK 預裝 + 快取)編「引擎 APK」(DATA 空)→ 本地 `inject_data.sh` 注入版權資料 + `zipalign`+`apksigner` 重簽**(秒級)。注入雷:① 引擎 APK 空 `assets/data` 目錄不入 zip → 複製前 `mkdir -p`;② NDK image 常無 `zip` → 用 `python3` zipfile 打包。
- 產物:4 ABI universal(arm64-v8a/armeabi-v7a/x86_64/x86,涵蓋實機+模擬器)、鎖橫向、debug 自簽 sideload。資料一樣本地注入(不進 CI/repo)。驗證上限=「能編、結構正確(libmain NEEDED libSDL2/ttf/image、assets 齊、activity 正確)、可開機」;**實機觸控/遊玩需裝置驗證**。

## GitHub Release(gh)
- `gh release create v0.1.0 --notes-file ... <assets>`;更新資產用 `gh release upload v0.1.0 <f> --clobber`。
- 強推 tag 會被擋(破壞性);改 `gh workflow run --ref main`(workflow_dispatch)跑最新碼 → `gh run download` artifact → `gh release upload`。
- Release 連結要指 GitHub Releases(引擎包),Release notes 寫明「需自備合法原版資料 + 放哪」。

## 打包要帶「全部」資料(完整版)
stage_data 要 `cp $DATA/mapx* $DATA/monx* $DATA/tlkx*`(**全部**),不要只帶幾張 demo 圖(否則玩家進不了多數城鎮)。**不要打包測試角色存檔**(開局該走建角)。

## 文件分離
- `README.md` 玩家向(故事鉤子 + 立即下載/玩 + 操作表 + 截圖 + 狀態);工程深度移 `docs/ENGINEERING.md`。
- 繁中攻略 `docs/GUIDE.md`(怪物/世界地圖/迷宮/通關流程)。
- 開發環境重建包:git bundle + docker recipes + 私人資料 + Claude 對話 transcript/memory + REBUILD.md，tar.gz 私人留存。
