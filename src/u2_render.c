#include "u2_render.h"
#include "u2_tileset.h"

/* PoC placeholder 調色盤:低 id 給接近原版的地形色,其餘程序化分散色,
 * 目的是讓地圖「結構」可見以驗證資料讀取正確,非最終美術。
 * 真實 tile_id→地形對照表待 task #5 補。 */
void u2_tile_color(unsigned char id, Uint8 *r, Uint8 *g, Uint8 *b)
{
    static const struct { Uint8 r, g, b; } base[] = {
        {  30,  60, 170},  /* 0  水 (深藍) */
        {  60, 150,  60},  /* 1  草原 (綠) */
        {  30, 100,  40},  /* 2  森林 (深綠) */
        { 120, 110, 100},  /* 3  山 (灰褐) */
        { 150, 130,  90},  /* 4  丘陵/沙 */
        { 180,  70,  60},  /* 5  城鎮 (磚紅) */
        { 200, 200, 210},  /* 6  城堡 (亮灰) */
        {  90,  70, 120},  /* 7  地牢入口 (紫) */
    };
    if (id < sizeof base / sizeof base[0]) {
        *r = base[id].r; *g = base[id].g; *b = base[id].b;
        return;
    }
    /* 其餘 id:程序化分散色 (HSV-ish via 質數打散),確保彼此可辨。 */
    *r = (Uint8)(40 + (id * 53) % 200);
    *g = (Uint8)(40 + (id * 97) % 200);
    *b = (Uint8)(40 + (id * 29) % 200);
}

static void fill_rect(SDL_Surface *s, int x, int y, int w, int h,
                      Uint8 r, Uint8 g, Uint8 b)
{
    SDL_Rect rc = { x, y, w, h };
    SDL_FillRect(s, &rc, SDL_MapRGB(s->format, r, g, b));
}

void u2_render_viewport(SDL_Surface *surf, const U2Map *m, SDL_Surface *tiles,
                        int cam_x, int cam_y, int cols, int rows,
                        int tile_px, int ox, int oy)
{
    for (int ty = 0; ty < rows; ty++) {
        for (int tx = 0; tx < cols; tx++) {
            unsigned char id = u2_map_tile(m, cam_x + tx, cam_y + ty);
            int dx = ox + tx * tile_px, dy = oy + ty * tile_px;
            if (tiles) {
                u2_tileset_blit(surf, tiles, id, dx, dy, tile_px);
            } else {
                Uint8 r, g, b;
                u2_tile_color(id, &r, &g, &b);
                fill_rect(surf, dx, dy, tile_px, tile_px, r, g, b);
            }
        }
    }
}

void u2_render_entities(SDL_Surface *surf, const U2Mon *mon, SDL_Surface *tiles,
                        int cam_x, int cam_y, int cols, int rows,
                        int tile_px, int ox, int oy)
{
    for (int i = 0; i < mon->count; i++) {
        const U2Entity *e = &mon->ent[i];
        int tx = e->x - cam_x, ty = e->y - cam_y;
        if (tx < 0 || tx >= cols || ty < 0 || ty >= rows)
            continue; /* 不在 viewport */
        int dx = ox + tx * tile_px, dy = oy + ty * tile_px;
        if (tiles)
            u2_tileset_blit(surf, tiles, e->tile, dx, dy, tile_px);
        else
            fill_rect(surf, dx, dy, tile_px, tile_px, 255, 60, 60);
    }
}
