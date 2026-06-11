#include "u2_map.h"
#include <stdio.h>

U2Map u2_map_load(const char *path)
{
    U2Map m = {0};
    FILE *f = fopen(path, "rb");
    if (!f)
        return m;

    unsigned char raw[U2_MAP_W * U2_MAP_H];
    size_t n = fread(raw, 1, sizeof raw, f);
    fclose(f);
    if (n < sizeof raw)
        return m; /* 檔太小,非合法 mapxNN */

    for (int y = 0; y < U2_MAP_H; y++)
        for (int x = 0; x < U2_MAP_W; x++)
            m.tile[y][x] = raw[y * U2_MAP_W + x] / 4; /* ×4 quirk → ÷4 */

    m.ok = 1;
    return m;
}

unsigned char u2_map_tile(const U2Map *m, int x, int y)
{
    if (x < 0 || x >= U2_MAP_W || y < 0 || y >= U2_MAP_H)
        return 0;
    return m->tile[y][x];
}

/* 環形(toroidal)存取:overworld 為 64×64,座標 & 0x3f 環繞(oracle 確認)。 */
unsigned char u2_map_tile_wrap(const U2Map *m, int x, int y)
{
    return m->tile[y & (U2_WORLD_DIM - 1)][x & (U2_WORLD_DIM - 1)];
}
