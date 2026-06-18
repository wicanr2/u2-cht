# 編譯與打包(Building)

跨平台編譯、打包與發佈的完整流程與「踩過的雷」。全程 Docker first;版權遊戲資料一律自備、不進 repo。

## 0. 心智模型:引擎 / 資料分離

- **引擎包**(可公開、進 GitHub Release):引擎二進位 + 中文字型(Noto/WQY)+ 繁中翻譯 + 公開的 EGA/CGA tileset(mcmagi U2 Upgrade)。**不含** `mapx/monx/tlkx`、FM Towns 美術/音樂/音效、原版執行檔。
- **完整可玩版**(私人留存):引擎包 + 自備的版權資料。
- 製法:`ENGINE_ONLY=1` + 空 `/data` → 引擎包;掛真實 `/data` → 完整版。

## 1. Docker 映像

| 映像 | 用途 | 來源 |
|---|---|---|
| `u2cht-build` | 編譯 + 跑回歸測試(SDL2 + SDL2_ttf/image/mixer + wqy 字型) | `docker/` |
| `u2cht-pkg` | 打包 AppImage / Windows(PIL / mingw / linuxdeploy / appimagetool) | `docker/` |
| `u2cht-android` | Android NDK r25 + SDK 34 + Gradle | `android/Dockerfile` |
| `dwdos` | 跑真 DOS `ultimaii.exe`(dosbox + xvfb-run + imagemagick + xdotool)做行為對照 | 共用 |
| `u2cht-ghidra` / `u2cht-re` | 反編 oracle / capstone 反組譯 | RE 用 |

## 2. 本地編譯 / 測試

```bash
# 編譯互動執行檔
docker run --rm -v "$PWD":/work -v <資料>:/data:ro u2cht-build bash -c '
  cmake -S /work -B /work/build -DCMAKE_BUILD_TYPE=Release
  cmake --build /work/build --target u2_game -j'

# headless 回歸(決定性;--script 模式強制速度/生成 100/100)
docker run --rm -v "$PWD":/work -v <資料>:/data:ro u2cht-build \
  bash -c '/work/build/u2_game /data/mapx20 <font> <tileset> <ui_tsv> /data/player --script "WWSS" /tmp/o/'
```

`u2_game` 參數:`<worldmap> <font.ttf> <tileset.png(逗號分隔多張)> <ui_tsv> [player_save]`
旗標:`--title --town-tiles --music --sfx --speed N --spawn N --script CMDS prefix`。

## 3. 桌面打包(`build_release.sh`)

```bash
docker run --rm [-e ENGINE_ONLY=1] -v "$PWD":/work -v <資料或空>:/data:ro -v <out>:/out \
  u2cht-pkg bash /work/build_release.sh {appimage|windows|mac|all}
```

- **AppImage**:linuxdeploy + appimagetool;自訂 AppRun 帶資料路徑。
- **Windows**:mingw 跨編(`x86_64-w64-mingw32-gcc -Dmain=SDL_main ... -mwindows`)+ 收 SDL2 runtime DLL + 「玩遊戲.bat」。
- **mac**(此腳本)：原始碼 + 一鍵編譯包;**原生 .app 走 CI**(見 §5)。

## 4. Android(`android/`)

兩條路:本地 docker 一次到位,或 CI 編引擎 + 本地注入(較快)。

```bash
# (a) 本地 docker:引擎 + 資料一次出可玩 APK(慢:4 ABI 從零編 ~30 分)
docker run --rm -v "$PWD":/work:ro -v <資料>:/data:ro -v <out>:/out \
  -v <ndk快取>:/opt/android-sdk/ndk -v <sdl快取>:/cache -v <gradle快取>:/root/.gradle \
  u2cht-android bash /work/android/build_apk.sh

# (b) CI 編引擎 APK(快、快取)→ 本地注入資料 + 重簽(秒級)
ENGINE_APK=<CI下載的引擎.apk> docker run --rm -v "$PWD":/work -v <資料>:/data -v <out>:/out \
  u2cht-android bash /work/android/inject_data.sh
```

`build_apk.sh` 路徑(`WORK/DATA/OUT/CACHE`)可由環境變數覆寫,docker 與 CI 共用同一腳本;`DATA` 空 → 引擎 APK。
`inject_data.sh`:解 APK → 注入 `mapx/monx/tlkx` + FM Towns tileset/音樂/音效 → 重建 `assets/filelist.txt` → `zipalign` + `apksigner`(debug key)重簽。

## 5. CI(GitHub Actions)

| Workflow | Runner | 產物 |
|---|---|---|
| `build-mac.yml` | `macos-14`(arm64)+ `macos-13`(Intel)matrix | 原生 `.app`(引擎;無資料) |
| `build-android.yml` | `ubuntu-latest` | 引擎 APK(4 ABI;無資料) |

兩者 `workflow_dispatch` 手動 + 打 `v*` tag 自動觸發;tag 時 `action-gh-release` 附到 Release。
完整可玩版 = 下載 CI 引擎 artifact → 本地注入資料(Mac:塞 `Resources/share/data/`;Android:`inject_data.sh`)。

## 6. 發佈 Release

```bash
gh release create vX.Y.Z -R wicanr2/u2-cht --title "..." --notes-file notes.md <引擎包...>
gh release upload vX.Y.Z <檔> --clobber      # 補/換資產
```

- **資產檔名用 ASCII**(GitHub 會把中文檔名換成點點)。
- 只放引擎包(不含版權資料);notes 寫明「自備合法資料 + 放哪」。
- 強推 tag 會被擋 → 用 `gh workflow run --ref main` 跑最新碼 → `gh run download` → `gh release upload`。

## 7. 踩過的雷(務必記得)

| 平台 | 症狀 | 解法 |
|---|---|---|
| **Windows** | 進不了城鎮、「找不到資料」 | `.bat` 傳反斜線路徑,`strrchr('/')` 解析失敗 → `main` 開頭把 argv `\`→`/` |
| **Windows** | `undefined reference touch_ui_finger` | `build_release.sh` 的 mingw 源碼清單漏了 `src/touch_ui.c`(cmake 有、手寫清單要同步) |
| **Android** | `-Werror=format-security` 擋 `tr()` 動態格式字串 | 原始碼頂 `#pragma clang diagnostic ignored "-Wformat-security"`(壓得過 AGP 最後追加的旗標;`LOCAL/APP_CFLAGS` 排序輸掉) |
| **Android** | NDK r26 fortify 擋 `&readlinkat` | 用 **NDK r25.1.8937393**(SDL 範本預設) |
| **Android** | 開機 `ClassNotFoundException` 崩潰 | `namespace` 維持 `org.libsdl.app`(對齊 SDL Java 類別),只設 `applicationId`;勿全域改套件名 |
| **Android** | `fopen` 讀不到 APK assets | 啟動引導 `android_glue.c` 依 `filelist.txt` 解壓到內部儲存,再合成 argv |
| **macOS CI** | `ld: library 'SDL2' not found` | Homebrew SDL2 在 `/opt/homebrew/lib`(非預設搜尋路徑)→ CMakeLists 加 `target_link_directories(${SDL2*_LIBRARY_DIRS})`(Linux /usr/lib 無害) |
| **macOS CI** | 字型 cask 不存在 / raw URL 404 | 改 curl **Noto Sans CJK**(`github.com/googlefonts/noto-cjk/raw/main/Sans/OTF/SimplifiedChinese/NotoSansCJKsc-Regular.otf`) |
| **macOS** | Intel 版 | GitHub Intel(macos-13)runner 長期配置不到;arm64 可用,Intel 暫缺 |
| **AppImage** | AppRun 被建成 symlink、`cat` 寫穿覆蓋真 ELF | 先 `rm -f AppDir/AppRun` 再寫自訂 AppRun |
| **打包** | 引擎包誤含 FM Towns 美術 | 引擎包前先把 `build/fmtowns_full*.png` 暫移走(只留公開 EGA/CGA) |
| **回歸測試** | 加 RNG 呼叫破壞 headless 決定性 | 新機率閘門用短路(`spawn_pct<100 && rng...`)→ 100 時不消耗 RNG;headless 強制 speed/spawn=100 |

## 8. 對照真 DOS(行為驗證)

`dwdos` 映像跑真 `ultimaii.exe` 抓圖、讀座標:
```bash
cp -r build/dosgame /tmp/dosrun   # 可改 /tmp/dosrun/player 的 0x24-0x25(=X,Y 座標)
docker run --rm -v /tmp/dosrun:/game -v /tmp/out:/out dwdos bash -c '
  export DISPLAY=:99; Xvfb :99 -screen 0 640x480x16 & sleep 3
  printf "[dosbox]\nmachine=cga\n[autoexec]\nmount c /game\nc:\nultimaii.exe\n" > /tmp/db.conf
  dosbox -conf /tmp/db.conf & sleep 7
  xdotool key p; sleep 5; import -window root /out/dos.png'   # P=Play 載入存檔
```
（DOS CGA 配色:洋紅=海、青=陸、灰=山。詳見 [`START-POSITION-VERIFICATION.md`](START-POSITION-VERIFICATION.md)。）
