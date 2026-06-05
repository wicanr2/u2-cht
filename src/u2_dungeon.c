#include "u2_dungeon.h"
#include <stdio.h>
#include <math.h>

U2Dungeon u2_dungeon_load(const char *path)
{
    U2Dungeon d = {0};
    FILE *f = fopen(path, "rb");
    if (!f) return d;
    unsigned char blk[256];
    for (int lv = 0; lv < U2_DNG_MAXL; lv++) {
        if (fread(blk, 1, 256, f) != 256) break;
        /* 全 0xFF 的 block = 結尾(未使用層) */
        int allff = 1;
        for (int i = 0; i < 256; i++) if (blk[i] != 0xFF) { allff = 0; break; }
        if (allff) break;
        for (int y = 0; y < U2_DNG_N; y++)
            for (int x = 0; x < U2_DNG_N; x++)
                d.cell[lv][y][x] = blk[y * U2_DNG_N + x];
        d.levels = lv + 1;
    }
    fclose(f);
    d.ok = (d.levels > 0);
    return d;
}

int u2_dungeon_is_wall(const U2Dungeon *d, int level, int x, int y)
{
    if (level < 0 || level >= d->levels) return 1;
    if (x < 0 || y < 0 || x >= U2_DNG_N || y >= U2_DNG_N) return 1;
    unsigned char c = d->cell[level][y][x];
    if (c == 0xC0 || c == 0xE0) return 0;   /* 門 / 下梯 = 通道 */
    return (c & 0x80) ? 1 : 0;
}

int u2_dungeon_ladder(const U2Dungeon *d, int level, int x, int y)
{
    if (level < 0 || level >= d->levels) return 0;
    if (x < 0 || y < 0 || x >= U2_DNG_N || y >= U2_DNG_N) return 0;
    unsigned char c = d->cell[level][y][x];
    if (c & 0x20) return +1;   /* 下梯 */
    if (c & 0x10) return -1;   /* 上梯 */
    return 0;
}

/* ---- surface 畫線 ---- */
static void put(SDL_Surface *s, int x, int y, Uint32 col)
{
    if (x < 0 || y < 0 || x >= s->w || y >= s->h) return;
    ((Uint32 *)((Uint8 *)s->pixels + y * s->pitch))[x] = col;
}
static void line(SDL_Surface *s, int x0, int y0, int x1, int y1, Uint32 col)
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        put(s, x0, y0, col);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}
static void rectln(SDL_Surface *s, int l, int t, int r, int b, Uint32 col)
{
    line(s, l, t, r, t, col); line(s, r, t, r, b, col);
    line(s, r, b, l, b, col); line(s, l, b, l, t, col);
}

static void vecs(int dir, int *fx, int *fy, int *lx, int *ly)
{
    int FX[4] = { 0, 1, 0, -1 }, FY[4] = { -1, 0, 1, 0 };
    *fx = FX[dir]; *fy = FY[dir];
    *lx = FX[(dir + 3) & 3]; *ly = FY[(dir + 3) & 3];
}

int u2_dungeon_render(SDL_Surface *surf, const U2Dungeon *d, int level,
                      int px, int py, int dir, int ox, int oy, int w, int h)
{
    Uint32 bg   = SDL_MapRGB(surf->format, 6, 8, 14);
    Uint32 wall = SDL_MapRGB(surf->format, 120, 200, 130);
    Uint32 dim  = SDL_MapRGB(surf->format, 40, 90, 55);
    Uint32 back = SDL_MapRGB(surf->format, 70, 130, 85);
    Uint32 door = SDL_MapRGB(surf->format, 230, 200, 90);

    SDL_Rect vr = { ox, oy, w, h };
    SDL_FillRect(surf, &vr, bg);

    int cx = ox + w / 2, cy = oy + h / 2;
    int fx, fy, lx, ly; vecs(dir, &fx, &fy, &lx, &ly);

    const int MAXD = 5;
    const double SHRINK = 0.60;
    double half0 = (w < h ? w : h) * 0.5 - 4;
    double hs[MAXD + 1];
    for (int dpt = 0; dpt <= MAXD; dpt++) hs[dpt] = half0 * pow(SHRINK, dpt);

    int depth = 0;
    for (; depth < MAXD; depth++) {
        int cxp = px + fx * depth, cyp = py + fy * depth;
        if (depth > 0 && u2_dungeon_is_wall(d, level, cxp, cyp)) break;

        int nl = (int)hs[depth], nf = (int)hs[depth + 1];
        int nL = cx - nl, nR = cx + nl, nT = cy - nl, nB = cy + nl;
        int fL = cx - nf, fR = cx + nf, fT = cy - nf, fB = cy + nf;

        line(surf, nL, nT, fL, fT, dim); line(surf, nR, nT, fR, fT, dim);
        line(surf, nL, nB, fL, fB, dim); line(surf, nR, nB, fR, fB, dim);

        int lwx = cxp + lx, lwy = cyp + ly;
        if (u2_dungeon_is_wall(d, level, lwx, lwy)) {
            line(surf, nL, nT, nL, nB, wall); line(surf, fL, fT, fL, fB, wall);
        } else {
            unsigned char c = (lwx>=0&&lwy>=0&&lwx<16&&lwy<16)?d->cell[level][lwy][lwx]:0;
            Uint32 oc = (c==0xC0||c==0xE0)?door:dim;
            line(surf, nL, nT, fL, fT, oc); line(surf, nL, nB, fL, fB, oc);
            line(surf, fL, fT, fL, fB, oc);
        }
        int rwx = cxp - lx, rwy = cyp - ly;
        if (u2_dungeon_is_wall(d, level, rwx, rwy)) {
            line(surf, nR, nT, nR, nB, wall); line(surf, fR, fT, fR, fB, wall);
        } else {
            unsigned char c = (rwx>=0&&rwy>=0&&rwx<16&&rwy<16)?d->cell[level][rwy][rwx]:0;
            Uint32 oc = (c==0xC0||c==0xE0)?door:dim;
            line(surf, nR, nT, fR, fT, oc); line(surf, nR, nB, fR, fB, oc);
            line(surf, fR, fT, fR, fB, oc);
        }
    }

    int bf = (int)hs[depth];
    SDL_Rect br = { cx - bf, cy - bf, 2 * bf, 2 * bf };
    SDL_FillRect(surf, &br, back);
    rectln(surf, cx - bf, cy - bf, cx + bf, cy + bf, wall);
    {
        int bx = px + fx * depth, by = py + fy * depth;
        unsigned char c = (bx>=0&&by>=0&&bx<16&&by<16)?d->cell[level][by][bx]:0x80;
        if (c == 0xC0 || c == 0xE0) {
            int q = bf / 2; SDL_Rect dr = { cx - q, cy - q, 2 * q, 2 * q };
            SDL_FillRect(surf, &dr, door);
        }
    }
    rectln(surf, ox, oy, ox + w - 1, oy + h - 1, dim);
    return depth;
}
