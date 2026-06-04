#include "u2_text.h"

U2Text u2_text_open(const char *ttf_path, int px)
{
    U2Text t = { NULL, px };
    if (TTF_WasInit() == 0)
        TTF_Init();
    t.font = TTF_OpenFont(ttf_path, px);
    return t;
}

void u2_text_close(U2Text *t)
{
    if (t->font) {
        TTF_CloseFont(t->font);
        t->font = NULL;
    }
}

int u2_text_draw(SDL_Surface *surf, U2Text *t, const char *utf8,
                 int x, int y, Uint8 r, Uint8 g, Uint8 b)
{
    if (!t->font || !utf8 || !utf8[0])
        return 0;
    SDL_Color col = { r, g, b, 255 };
    SDL_Surface *glyph = TTF_RenderUTF8_Blended(t->font, utf8, col);
    if (!glyph)
        return 0;
    SDL_Rect dst = { x, y, glyph->w, glyph->h };
    SDL_BlitSurface(glyph, NULL, surf, &dst);
    int w = glyph->w;
    SDL_FreeSurface(glyph);
    return w;
}
