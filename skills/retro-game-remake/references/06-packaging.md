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
