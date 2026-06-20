/* dungeon_main — 地牢 first-person 線框 demo（繁中 HUD + 小地圖）
 *
 * 載入真實地牢檔(mapxN4/N5)左上 16×16 一層,自動挑「前方可見深度最大」的
 * 位置 + 朝向,渲染 oracle FUN_0040d000 風格的線框地牢,右側附繁中狀態與小地圖。
 *
 * 用法: u2_dungeon_demo <dungeon_map> <font.ttf> <out.png> [px py dir]
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include "u2_dungeon.h"
#include "u2_text.h"

#define W 960
#define H 600
#define VIEW 560

static const char *DIR_ZH[4] = { "北 N", "東 E", "南 S", "西 W" };

/* 沿 dir 數前方連續可走格數(到牆為止) */
static int fwd_depth(const U2Dungeon *d, int level, int px, int py, int dir)
{
    int FX[4] = { 0, 1, 0, -1 }, FY[4] = { -1, 0, 1, 0 };
    int n = 0;
    for (int k = 1; k <= 5; k++) {
        int x = px + FX[dir] * k, y = py + FY[dir] * k;
        if (u2_dungeon_is_wall(d, level, x, y)) break;
        n++;
    }
    return n;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "用法: %s <dungeon_map> <font.ttf> <out.png> [px py dir]\n", argv[0]);
        return 2;
    }
    if (SDL_Init(0) != 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return 1;
    }
    U2Dungeon dg = u2_dungeon_load(argv[1]);
    if (!dg.ok) { fprintf(stderr, "無法載入地牢: %s\n", argv[1]); return 1; }

    int level = 0;
    /* 起點:給定或自動挑前方深度最大者 */
    int px, py, dir;
    if (argc >= 7) {
        px = atoi(argv[4]); py = atoi(argv[5]); dir = atoi(argv[6]) & 3;
    } else {
        px = py = dir = 0; int best = -1;
        for (int y = 0; y < U2_DNG_N; y++)
            for (int x = 0; x < U2_DNG_N; x++) {
                if (u2_dungeon_is_wall(&dg, level, x, y)) continue;
                for (int dd = 0; dd < 4; dd++) {
                    int v = fwd_depth(&dg, level, x, y, dd);
                    if (v > best) { best = v; px = x; py = y; dir = dd; }
                }
            }
    }

    SDL_Surface *cv = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format, 12, 14, 22));
    SDL_Rect hdr = { 0, 0, W, 36 };
    SDL_FillRect(cv, &hdr, SDL_MapRGB(cv->format, 36, 44, 110));

    U2Text title = u2_text_open(argv[2], 22);
    U2Text body = u2_text_open(argv[2], 22);
    U2Text small = u2_text_open(argv[2], 17);
    if (!title.font) { fprintf(stderr, "字型失敗: %s\n", TTF_GetError()); return 1; }
    u2_text_draw(cv, &title, "地牢 — 第一人稱線框(Ultima II)", 12, 5, 235, 235, 245);

    /* 左:3D 線框視區(style 0 線框;無實體 ent_depth=-1;maxd 5 全視野)*/
    int depth = u2_dungeon_render(cv, &dg, level, px, py, dir, 16, 50, VIEW, VIEW, 0, -1, 0, 0, 5);

    /* 右:繁中 HUD */
    int rx = 16 + VIEW + 24, ry = 60;
    char ln[96];
    u2_text_draw(cv, &body, "狀態", rx, ry, 150, 175, 205); ry += 34;
    snprintf(ln, sizeof ln, "座標: (%d, %d)", px, py);
    u2_text_draw(cv, &body, ln, rx, ry, 225, 225, 230); ry += 30;
    snprintf(ln, sizeof ln, "朝向: %s", DIR_ZH[dir]);
    u2_text_draw(cv, &body, ln, rx, ry, 225, 225, 230); ry += 30;
    snprintf(ln, sizeof ln, "前方可見深度: %d", depth);
    u2_text_draw(cv, &body, ln, rx, ry, 225, 225, 230); ry += 30;
    snprintf(ln, sizeof ln, "樓層: %d / 共 %d 層", level + 1, dg.levels);
    u2_text_draw(cv, &body, ln, rx, ry, 225, 225, 230); ry += 42;

    /* 小地圖 (16×16,每格 18px) */
    u2_text_draw(cv, &small, "本層地圖(■牆 ·走廊 門/梯)", rx, ry, 150, 170, 200); ry += 26;
    int cell = 18, mx = rx, my = ry;
    for (int y = 0; y < U2_DNG_N; y++)
        for (int x = 0; x < U2_DNG_N; x++) {
            unsigned char c = dg.cell[level][y][x];
            Uint8 r=18,g=20,b=28;
            if (u2_dungeon_is_wall(&dg, level, x, y)) { r=70;g=78;b=95; }
            else if (c==0xC0||c==0xE0) { r=200;g=170;b=70; }   /* 門/梯 */
            else { r=30;g=44;b=36; }                            /* 走廊 */
            SDL_Rect rc = { mx + x*cell, my + y*cell, cell-1, cell-1 };
            SDL_FillRect(cv, &rc, SDL_MapRGB(cv->format, r,g,b));
        }
    /* 玩家箭頭(用方塊 + 朝向小點) */
    SDL_Rect pr = { mx + px*cell, my + py*cell, cell-1, cell-1 };
    SDL_FillRect(cv, &pr, SDL_MapRGB(cv->format, 240, 90, 90));
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};
    SDL_Rect dp = { mx + px*cell + (cell/2) + FX[dir]*5 - 2,
                    my + py*cell + (cell/2) + FY[dir]*5 - 2, 4, 4 };
    SDL_FillRect(cv, &dp, SDL_MapRGB(cv->format, 250, 240, 90));

    u2_text_draw(cv, &small,
        "線框畫法源自 oracle FUN_0040d000(程式化畫線,非 tile);資料為真實地牢 raw byte(0x80 牆)",
        16, H - 26, 130, 145, 175);

    if (IMG_SavePNG(cv, argv[3]) != 0) {
        fprintf(stderr, "存檔失敗: %s\n", IMG_GetError()); return 1;
    }
    printf("地牢線框 → %s (起點 %d,%d dir=%d 深度=%d)\n", argv[3], px, py, dir, depth);

    u2_text_close(&title); u2_text_close(&body); u2_text_close(&small);
    SDL_FreeSurface(cv);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
    return 0;
}
