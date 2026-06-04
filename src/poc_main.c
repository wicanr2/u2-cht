/* poc_main — 垂直切片 PoC:一次驗證三假設
 *   (1) 繪圖換得掉:SDL2 離屏渲染 tile 地圖
 *   (2) 資料讀得到:解析真實 mapxNN (tile÷4)
 *   (3) 中文畫得出:SDL2_ttf 畫真實 U2 字串的中文翻譯
 * 輸出一張 PNG (headless,不需顯示器)。
 *
 * 用法: u2_poc <mapxNN> <font.ttf> <out.png>
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include "u2_map.h"
#include "u2_render.h"
#include "u2_tileset.h"
#include "u2_text.h"

#define CANVAS_W 960
#define CANVAS_H 600
#define TILE_PX  48          /* 16 × 3 (ADR 內部 3×) */
#define VIEW_COLS 12
#define VIEW_ROWS 11

/* 右側面板:真實 U2 字串 → 中文翻譯 (demo 三來源:標題/UI/對話/結局) */
static const char *panel_lines[] = {
    "創世紀 II:女巫的復仇",
    "",
    "── 指令 (exe 內嵌) ──",
    "進入地牢",
    "攻擊──麻痺!",
    "超空間跳躍目標:",
    "",
    "── 對話 (tlkx) ──",
    "占星師宣稱:",
    "「有一顆行星叫 X!」",
    "",
    "── 結局 ──",
    "女巫米娜克斯已死!",
};

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "用法: %s <mapxNN> <font.ttf> <out.png> [tileset.png]\n", argv[0]);
        return 2;
    }
    const char *map_path = argv[1];
    const char *font_path = argv[2];
    const char *out_path = argv[3];
    const char *tiles_path = (argc > 4) ? argv[4] : NULL;

    if (SDL_Init(0) != 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    if (IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "IMG_Init: %s\n", IMG_GetError());
        return 1;
    }

    /* (2) 資料讀得到 */
    U2Map map = u2_map_load(map_path);
    if (!map.ok) {
        fprintf(stderr, "無法載入地圖: %s\n", map_path);
        return 1;
    }

    /* 離屏畫布 (免 video subsystem) */
    SDL_Surface *canvas = SDL_CreateRGBSurfaceWithFormat(
        0, CANVAS_W, CANVAS_H, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 18, 18, 24));

    /* 字型:標題 32px / 內文 24px (ADR:CJK 原生繪製) */
    U2Text title = u2_text_open(font_path, 30);
    U2Text body = u2_text_open(font_path, 22);
    if (!title.font || !body.font) {
        fprintf(stderr, "字型載入失敗: %s (%s)\n", font_path, TTF_GetError());
        return 1;
    }

    /* 標題列 (CJK) */
    u2_text_draw(canvas, &title, "Ultima II 繁中 PoC — 地圖 + 中文一次到位",
                 12, 14, 240, 220, 120);

    /* (1) 繪圖換得掉:把地圖中段 viewport 畫到左側 (真 tileset 或色塊 fallback) */
    SDL_Surface *tiles = tiles_path ? u2_tileset_load(tiles_path) : NULL;
    int cam_x = (U2_MAP_W - VIEW_COLS) / 2;
    int cam_y = (U2_MAP_H - VIEW_ROWS) / 2;
    int map_ox = 12, map_oy = 60;
    u2_render_viewport(canvas, &map, tiles, cam_x, cam_y,
                       VIEW_COLS, VIEW_ROWS, TILE_PX, map_ox, map_oy);

    /* map 來源標註 (證明讀的是真實檔) */
    char cap[128];
    snprintf(cap, sizeof cap, "map: %s  viewport (%d,%d) %dx%d  tile=byte/4",
             map_path, cam_x, cam_y, VIEW_COLS, VIEW_ROWS);
    u2_text_draw(canvas, &body, cap, 12,
                 map_oy + VIEW_ROWS * TILE_PX + 8, 150, 170, 200);

    /* (3) 中文畫得出:右側面板逐行 CJK */
    int px = map_ox + VIEW_COLS * TILE_PX + 20;
    int py = 60;
    int n = (int)(sizeof panel_lines / sizeof panel_lines[0]);
    for (int i = 0; i < n; i++) {
        if (panel_lines[i][0])
            u2_text_draw(canvas, &body, panel_lines[i], px, py, 230, 230, 235);
        py += 30;
    }

    if (IMG_SavePNG(canvas, out_path) != 0) {
        fprintf(stderr, "存檔失敗: %s\n", IMG_GetError());
        return 1;
    }
    printf("PoC 完成 → %s\n", out_path);

    u2_text_close(&title);
    u2_text_close(&body);
    SDL_FreeSurface(canvas);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
