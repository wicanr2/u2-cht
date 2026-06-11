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
        /* 偵測非地牢層:有效地牢層頂行為邊界牆(0x80);頂行牆 < 12 = 尾部非地牢資料,停止。
         * (mapx15 = 15 層 16×16 地牢 + 384B 尾部;避免把尾部當第 16 層乱碼)*/
        int topwall = 0;
        for (int x = 0; x < U2_DNG_N; x++) if (blk[x] == 0x80) topwall++;
        if (topwall < 12) break;
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

/* 地牢線框配色盤(隨畫風 style 切換;順序對齊 tileset:ega/fmtowns/vivid/cga/ega_alt/c64)。
 * 每組:bg 背景 / wall 近牆 / dim 遠牆 / back 側牆 / door 門。 */
static const struct { Uint8 bg[3], wall[3], dim[3], back[3], door[3]; } DPAL[] = {
    {{6,8,14},   {120,200,130},{40,90,55},  {70,130,85},  {230,200,90}},   /* 0 EGA 綠 */
    {{8,10,22},  {110,180,235},{42,72,115}, {72,122,175}, {240,210,120}},  /* 1 FM Towns 青藍 */
    {{16,8,18},  {220,130,210},{100,52,96}, {165,82,152}, {245,225,130}},  /* 2 Vivid 洋紅 */
    {{0,0,0},    {85,255,255}, {0,128,128}, {45,205,205}, {255,85,255}},    /* 3 CGA 青/洋紅 */
    {{14,9,4},   {232,182,72}, {112,82,32}, {172,132,52}, {250,232,160}},   /* 4 EGA-alt 琥珀 */
    {{8,8,26},   {132,150,242},{52,56,112}, {92,102,182}, {220,210,150}},   /* 5 C64 藍紫 */
};
#define U2_NDPAL ((int)(sizeof DPAL / sizeof DPAL[0]))

/* 怪物類型 tile → 身影顏色(對齊 oracle 低 nibble 怪物 index 多樣外觀)。
 * tile 對應 mob_type:12 蜥蜴/13 幽靈/14 魔鬼/15 炎魔/60 哥布林/61 盜賊/62 惡魔/63 海蛇。 */
static void mob_tile_color(unsigned char tile, Uint8 *r, Uint8 *g, Uint8 *b)
{
    switch (tile) {
        case 12: *r=110; *g=200; *b=90;  break;  /* 蜥蜴人:綠 */
        case 13: *r=200; *g=210; *b=230; break;  /* 幽靈:慘白 */
        case 14: *r=230; *g=70;  *b=70;  break;  /* 魔鬼:紅 */
        case 15: *r=250; *g=140; *b=40;  break;  /* 炎魔:橙火 */
        case 60: *r=150; *g=170; *b=80;  break;  /* 哥布林:黃綠 */
        case 61: *r=120; *g=120; *b=180; break;  /* 盜賊:藍灰 */
        case 62: *r=200; *g=80;  *b=200; break;  /* 惡魔:紫紅 */
        case 63: *r=80;  *g=200; *b=200; break;  /* 海蛇:青 */
        default: *r=230; *g=70;  *b=70;  break;  /* 預設紅 */
    }
}

int u2_dungeon_render(SDL_Surface *surf, const U2Dungeon *d, int level,
                      int px, int py, int dir, int ox, int oy, int w, int h, int style,
                      int ent_depth, char ent_kind, unsigned char ent_tile, int maxd)
{
    const int s = ((style % U2_NDPAL) + U2_NDPAL) % U2_NDPAL;
    Uint32 bg   = SDL_MapRGB(surf->format, DPAL[s].bg[0],   DPAL[s].bg[1],   DPAL[s].bg[2]);
    Uint32 wall = SDL_MapRGB(surf->format, DPAL[s].wall[0], DPAL[s].wall[1], DPAL[s].wall[2]);
    Uint32 dim  = SDL_MapRGB(surf->format, DPAL[s].dim[0],  DPAL[s].dim[1],  DPAL[s].dim[2]);
    Uint32 back = SDL_MapRGB(surf->format, DPAL[s].back[0], DPAL[s].back[1], DPAL[s].back[2]);
    Uint32 door = SDL_MapRGB(surf->format, DPAL[s].door[0], DPAL[s].door[1], DPAL[s].door[2]);

    SDL_Rect vr = { ox, oy, w, h };
    SDL_FillRect(surf, &vr, bg);

    int cx = ox + w / 2, cy = oy + h / 2;
    int fx, fy, lx, ly; vecs(dir, &fx, &fy, &lx, &ly);

    int MAXD = maxd; if (MAXD < 1) MAXD = 1; if (MAXD > 5) MAXD = 5;  /* 光照決定視野深度 */
    const double SHRINK = 0.60;
    double half0 = (w < h ? w : h) * 0.5 - 4;
    double hs[6];
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
    /* 前方實體(怪物/寶箱)按深度透視繪製(對齊 oracle:正前方最近怪物畫 sprite,depthIndex 控縮放)*/
    if (ent_depth >= 1 && ent_depth <= depth) {
        double eh = half0 * pow(SHRINK, ent_depth);
        int es = (int)(eh * 0.7); if (es < 4) es = 4;
        if (ent_kind == 'C') {                       /* 寶箱:黃色方箱 */
            SDL_Rect cr = { cx - es/2, cy - es/3, es, (es*2)/3 };
            SDL_FillRect(surf, &cr, door);
            rectln(surf, cr.x, cr.y, cr.x+cr.w, cr.y+cr.h, wall);
        } else {                                     /* 怪物:依類型上色的身影 + 眼睛 */
            Uint8 mr_=230,mg_=70,mb_=70; mob_tile_color(ent_tile,&mr_,&mg_,&mb_);
            Uint32 mc = SDL_MapRGB(surf->format, mr_, mg_, mb_);
            SDL_Rect mr = { cx - es/2, cy - es/2, es, es };
            SDL_FillRect(surf, &mr, mc);
            int e = es/6; if (e < 1) e = 1;
            SDL_Rect e1 = { cx - es/4, cy - es/6, e, e }, e2 = { cx + es/4 - e, cy - es/6, e, e };
            SDL_FillRect(surf, &e1, bg); SDL_FillRect(surf, &e2, bg);
        }
    }
    rectln(surf, ox, oy, ox + w - 1, oy + h - 1, dim);
    return depth;
}
