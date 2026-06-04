/* u2_render — 像素圖層:把 tile 地圖畫到離屏 SDL_Surface
 *
 * PoC 階段 tile 美術用 id→RGB 調色盤 (placeholder);真 tile 抽取見 task #5。
 * 縮放策略 (ADR 0001):整數倍放大 (tile_px = 16 × N),色塊本身無 filter。
 */
#ifndef U2_RENDER_H
#define U2_RENDER_H

#include <SDL.h>
#include "u2_map.h"
#include "u2_mon.h"

/* 取得 tile_id 的 placeholder RGB 色 (無 tileset 時 fallback)。 */
void u2_tile_color(unsigned char tile_id, Uint8 *r, Uint8 *g, Uint8 *b);

/* 把 map 的一個 viewport 畫到 surf。
 *   tiles      :真實 tileset surface;NULL 則 fallback 到色塊
 *   cam_x/cam_y:viewport 左上角的 map 座標
 *   cols/rows  :viewport 顯示幾個 tile
 *   tile_px    :每 tile 放大後像素 (16×N)
 *   ox/oy      :畫到 surf 的左上角像素位移
 */
void u2_render_viewport(SDL_Surface *surf, const U2Map *m, SDL_Surface *tiles,
                        int cam_x, int cam_y, int cols, int rows,
                        int tile_px, int ox, int oy);

/* 在地形之上疊繪 monxNN 實體 (落在 viewport 內者)。tiles 為真 tileset。 */
void u2_render_entities(SDL_Surface *surf, const U2Mon *mon, SDL_Surface *tiles,
                        int cam_x, int cam_y, int cols, int rows,
                        int tile_px, int ox, int oy);

#endif
