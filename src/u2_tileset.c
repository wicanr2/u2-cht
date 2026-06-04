#include "u2_tileset.h"
#include <SDL_image.h>

SDL_Surface *u2_tileset_load(const char *png_path)
{
    SDL_Surface *raw = IMG_Load(png_path);
    if (!raw)
        return NULL;
    /* 轉成已知格式方便 blit */
    SDL_Surface *s = SDL_ConvertSurfaceFormat(raw, SDL_PIXELFORMAT_RGBA32, 0);
    SDL_FreeSurface(raw);
    return s;
}

void u2_tileset_blit(SDL_Surface *dst, SDL_Surface *tiles, int id,
                     int dx, int dy, int size)
{
    if (!tiles)
        return;
    SDL_Rect src = { id * U2_TILE_SRC, 0, U2_TILE_SRC, U2_TILE_SRC };
    SDL_Rect d = { dx, dy, size, size };
    /* nearest 放大 (pixel art);SDL_BlitScaled 對 surface 為 nearest stretch */
    SDL_BlitScaled(tiles, &src, dst, &d);
}
