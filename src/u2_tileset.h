/* u2_tileset — 真實 tile 美術 (從 ultimaii.exe 抽出的 PNG strip)
 *
 * strip 格式:1024×16,64 個 16×16 tile 橫排;tile id → src x = id×16。
 * 由 tools/extract_tiles.py 產生 (EA 版權,不散布;玩家自備)。
 */
#ifndef U2_TILESET_H
#define U2_TILESET_H

#include <SDL.h>

#define U2_TILE_SRC 16

/* 載入 tileset PNG。失敗回傳 NULL (呼叫端可 fallback 到色塊)。 */
SDL_Surface *u2_tileset_load(const char *png_path);

/* 把 tile id 以 nearest 放大 blit 到 dst 的 (dx,dy),邊長 size。 */
void u2_tileset_blit(SDL_Surface *dst, SDL_Surface *tiles, int id,
                     int dx, int dy, int size);

#endif
