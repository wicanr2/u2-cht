# ADR 0001 — 渲染解析度與 CJK glyph 策略

> 狀態:已接受(2026-06-04)
> 脈絡:中文化需要在原版 320×200 的小畫面塞入清晰的中文字;使用者提案放大解析度(例 640×480)讓 16×16 CJK glyph 好排版,原始像素圖用 pixel scaling 放大。

## 決策

採**雙層渲染 + 內外解析度解耦**:

1. **像素圖層(tile/sprite)**:從原版 16×16 tile 來源,**整數倍放大**到內部 render target。預設 **nearest-neighbor**(銳利、忠實、對齊 u3-cht/u6-cht);提供 **bilinear/bicubic 平滑**為選項 toggle(Alderson exe 本就有 render/palette 切換,延續其精神)。
2. **文字圖層(CJK/UI)**:在內部 render target 的**原生高解析度直接繪製** CJK glyph(16×16 或 24×24),**永不**由 8×8 點陣放大 → 中文字恆為銳利,品質與像素圖縮放完全脫鉤。
3. **內外解耦**:
   - **內部 render 解析度** = 320×200 × N,N 決定 CJK glyph 大小(N=2→16px、N=3→24px)。
   - **視窗/輸出解析度** 自由(640×480、1280×960、全螢幕),最後 `SDL_RenderCopy` 一次把內部 target 縮放呈現,filter 由使用者選。

## 為何不「整畫面一起縮放」(被否決的 Option A)

若把 tiles+text 全畫進 320×200 再整體放大,CJK 只能在 320×200 內以 8×8 繪製再被放大 → 模糊;或在 320×200 內畫 16×16 CJK → 一個字吃掉 2×2 tile,文字面板塞不下。**故文字層必須在高解析度原生繪製**,不能跟著像素圖被縮放。

## 解析度數學(base 320×200,16×16 tile)

| 內部 N | 內部解析度 | tile 放大後 | 建議 CJK glyph | 備註 |
|---|---|---|---|---|
| 2× | 640×400 | 32×32 | 16×16 | 16:10,最省;CJK=半 tile |
| **3×** ⭐ | **960×600** | 48×48 | **24×24** | 可讀性最佳,推薦預設 |
| 4× | 1280×800 | 64×64 | 32×32 | 高 DPI 螢幕 |

## 關於 640×480(使用者提到的數字)

- 640×480 是 **4:3**,而 320×200×2 = 640×**400**(16:10),Y 不是整數倍(480/200=2.4)。
- 兩種正解:
  - **(a) 內部 640×400 → 呈現時 letterbox 進 640×480**(上下黑邊,像素層維持整數倍,最銳利)。
  - **(b) 內部 640×400 → 最終 stretch 到 640×480(Y×1.2)**:這正好**還原 CGA 320×200 在 4:3 CRT 的非方形像素比(~1.2)**,是「CRT 原汁」做法。代價是像素層最後一次非整數縮放會有輕微 shimmer(只在 present 階段,可接受)。
- 結論:640×480 可行且 (b) 有「CRT 正統」加分;但**內部 render 仍建議用整數倍(960×600 或 640×400)**,640×480 只當最終 present 視窗尺寸。

## 關於 bicubic(使用者提到)

- 像素圖:**nearest 預設**(retro 銳利,u3/u6 慣例)。bicubic/bilinear 會柔化像素邊緣 → 提供為**選項**而非預設。
- SDL2 實作:bilinear 用 `SDL_SetTextureScaleMode(LINEAR)` 即可;**bicubic 需自寫 shader**(SDL_gpu / GLSL),工作量較高 → 先上 nearest + bilinear 兩檔,bicubic 視需求再加。
- CJK 文字層**不套任何縮放 filter**(原生繪製),不受此影響。

## 影響

- `render/` 模組需求:① 內部 framebuffer(可設 N)② tile blit(整數放大,filter 可選)③ CJK 文字層(SDL_ttf / BDF atlas,複用 u6-cht Big5 pipeline)④ present 縮放(視窗自由 + aspect 處理)。
- 文字換行/寬度邏輯改 **CJK-aware**(全形 2 倍寬),對照 oracle 的 `RewrapString` 等價邏輯。
- glyph 大小 24×24 → 直接用 u6-cht `tools/build-big5-font-wqysharp.py` 產字(或可商用 TTF)。
