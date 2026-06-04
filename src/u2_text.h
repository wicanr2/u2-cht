/* u2_text — 文字圖層:CJK-aware UTF-8 文字繪製
 *
 * ADR 0001:文字層在內部高解析度「原生繪製」,不隨像素圖縮放 → 恆銳利。
 * PoC 用 SDL2_ttf + WQY TTF 直吃 UTF-8;production 可換 u6-cht 點陣字 atlas。
 */
#ifndef U2_TEXT_H
#define U2_TEXT_H

#include <SDL.h>
#include <SDL_ttf.h>

typedef struct {
    TTF_Font *font;
    int px;
} U2Text;

/* 載入字型 (TTF 路徑 + 像素高度)。失敗回傳 .font = NULL。 */
U2Text u2_text_open(const char *ttf_path, int px);
void u2_text_close(U2Text *t);

/* 在 surf 的 (x,y) 畫一行 UTF-8 文字 (左上對齊),回傳該行寬度像素。 */
int u2_text_draw(SDL_Surface *surf, U2Text *t, const char *utf8,
                 int x, int y, Uint8 r, Uint8 g, Uint8 b);

#endif
