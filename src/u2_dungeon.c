#include "u2_dungeon.h"
#include <stdio.h>
#include <math.h>

U2Dungeon u2_dungeon_load(const char *path)
{
    U2Dungeon d = {0};
    FILE *f = fopen(path, "rb");
    if (!f) return d;
    /* 地牢檔為 64×66 raw;取左上 16×16 當一層(每列 stride 64) */
    unsigned char row[64];
    for (int y = 0; y < U2_DNG_N; y++) {
        if (fread(row, 1, 64, f) != 64) { fclose(f); return d; }
        for (int x = 0; x < U2_DNG_N; x++)
            d.cell[y][x] = row[x];
    }
    fclose(f);
    d.ok = 1;
    return d;
}

int u2_dungeon_is_wall(const U2Dungeon *d, int x, int y)
{
    if (x < 0 || y < 0 || x >= U2_DNG_N || y >= U2_DNG_N)
        return 1;                    /* 邊界外當牆 */
    unsigned char c = d->cell[y][x];
    /* 門(0xC0)/梯(0xE0)雖 &0x80 但是通道;實心牆 = 0x80 */
    if (c == 0xC0 || c == 0xE0) return 0;
    return (c & 0x80) ? 1 : 0;
}

/* surface 畫線 (Bresenham) */
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
static void rect(SDL_Surface *s, int l, int t, int r, int b, Uint32 col)
{
    line(s, l, t, r, t, col); line(s, r, t, r, b, col);
    line(s, r, b, l, b, col); line(s, l, b, l, t, col);
}

/* 朝向 dir 的前進向量與「左手邊」向量 */
static void vecs(int dir, int *fx, int *fy, int *lx, int *ly)
{
    /* 0=N(-y) 1=E(+x) 2=S(+y) 3=W(-x) */
    int FX[4] = { 0, 1, 0, -1 }, FY[4] = { -1, 0, 1, 0 };
    *fx = FX[dir]; *fy = FY[dir];
    *lx = FX[(dir + 3) & 3]; *ly = FY[(dir + 3) & 3];   /* 左轉 90° */
}

int u2_dungeon_render(SDL_Surface *surf, const U2Dungeon *d,
                      int px, int py, int dir, int ox, int oy, int w, int h)
{
    Uint32 bg   = SDL_MapRGB(surf->format, 6, 8, 14);
    Uint32 wall = SDL_MapRGB(surf->format, 120, 200, 130);   /* 線框綠(仿 CRT) */
    Uint32 dim  = SDL_MapRGB(surf->format, 40, 90, 55);      /* 地板/天花板暗線 */
    Uint32 back = SDL_MapRGB(surf->format, 70, 130, 85);     /* 背牆 */
    Uint32 door = SDL_MapRGB(surf->format, 230, 200, 90);    /* 門/梯 標記 */

    SDL_Rect vr = { ox, oy, w, h };
    SDL_FillRect(surf, &vr, bg);

    int cx = ox + w / 2, cy = oy + h / 2;
    int fx, fy, lx, ly; vecs(dir, &fx, &fy, &lx, &ly);

    const int MAXD = 5;
    const double SHRINK = 0.60;
    double half0 = (w < h ? w : h) * 0.5 - 4;

    /* 各深度方框半徑 */
    double hs[MAXD + 1];
    for (int dpt = 0; dpt <= MAXD; dpt++) hs[dpt] = half0 * pow(SHRINK, dpt);

    /* 往前掃,逐格畫;遇牆畫背牆停止 */
    int depth = 0;
    for (; depth < MAXD; depth++) {
        int cxp = px + fx * depth, cyp = py + fy * depth;
        /* 當前格若是牆 → 背牆,停 */
        if (depth > 0 && u2_dungeon_is_wall(d, cxp, cyp)) break;

        int nl = (int)hs[depth], nf = (int)hs[depth + 1];
        int nL = cx - nl, nR = cx + nl, nT = cy - nl, nB = cy + nl;  /* near frame */
        int fL = cx - nf, fR = cx + nf, fT = cy - nf, fB = cy + nf;  /* far frame */

        /* 地板/天花板透視線(四角連線) */
        line(surf, nL, nT, fL, fT, dim); line(surf, nR, nT, fR, fT, dim);
        line(surf, nL, nB, fL, fB, dim); line(surf, nR, nB, fR, fB, dim);

        /* 左側:看左鄰格 */
        int lwx = cxp + lx, lwy = cyp + ly;
        if (u2_dungeon_is_wall(d, lwx, lwy)) {
            /* 實心左牆:畫梯形(near 左緣 → far 左緣) */
            line(surf, nL, nT, nL, nB, wall);     /* near 左豎邊 */
            line(surf, fL, fT, fL, fB, wall);     /* far 左豎邊 */
        } else {
            /* 左開口:畫側通道框(門/梯則標色) */
            unsigned char c = (lwx>=0&&lwy>=0&&lwx<16&&lwy<16) ? d->cell[lwy][lwx] : 0;
            Uint32 oc = (c==0xC0||c==0xE0) ? door : dim;
            line(surf, nL, nT, fL, fT, oc); line(surf, nL, nB, fL, fB, oc);
            line(surf, fL, fT, fL, fB, oc);
        }
        /* 右側:看右鄰格 */
        int rwx = cxp - lx, rwy = cyp - ly;
        if (u2_dungeon_is_wall(d, rwx, rwy)) {
            line(surf, nR, nT, nR, nB, wall);
            line(surf, fR, fT, fR, fB, wall);
        } else {
            unsigned char c = (rwx>=0&&rwy>=0&&rwx<16&&rwy<16) ? d->cell[rwy][rwx] : 0;
            Uint32 oc = (c==0xC0||c==0xE0) ? door : dim;
            line(surf, nR, nT, fR, fT, oc); line(surf, nR, nB, fR, fB, oc);
            line(surf, fR, fT, fR, fB, oc);
        }
    }

    /* 背牆:填滿最深處方框 */
    int bf = (int)hs[depth];
    SDL_Rect br = { cx - bf, cy - bf, 2 * bf, 2 * bf };
    SDL_FillRect(surf, &br, back);
    rect(surf, cx - bf, cy - bf, cx + bf, cy + bf, wall);
    /* 背牆若是門/梯,中央標記 */
    {
        int bx = px + fx * depth, by = py + fy * depth;
        unsigned char c = (bx>=0&&by>=0&&bx<16&&by<16) ? d->cell[by][bx] : 0x80;
        if (c == 0xC0 || c == 0xE0) {
            int q = bf / 2;
            SDL_Rect dr = { cx - q, cy - q, 2 * q, 2 * q };
            SDL_FillRect(surf, &dr, door);
        }
    }
    /* 視區外框 */
    rect(surf, ox, oy, ox + w - 1, oy + h - 1, dim);
    return depth;
}
