/* game_main — 互動式 Ultima II 引擎切片:玩家置中 + 鍵盤移動 + CJK 狀態列
 *
 * U2 慣例:玩家恆在畫面中央,地圖在腳下捲動;移動受 tile 可通行性限制。
 * 兩種執行模式:
 *   1. 互動:開 SDL 視窗,方向鍵 / WASD 走路,Q / Esc 離開。
 *   2. headless 腳本(Docker 驗證用):--script <moves> <out_prefix>
 *      moves 為 N/S/E/W 字串,逐步套用並存 <out_prefix>NN.png(免顯示器)。
 *
 * 用法:
 *   u2_game <mapxNN> <font.ttf> <tileset.png> <ui_tsv> [--script NNEE out/step_]
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include "u2_map.h"
#include "u2_mon.h"
#include "u2_play.h"
#include "u2_render.h"
#include "u2_strings.h"
#include "u2_tileset.h"
#include "u2_text.h"

#define CANVAS_W   960
#define CANVAS_H   600
#define HDR_H      30
#define TILE_PX    48           /* 16 × 3 */
#define VIEW_COLS  19
#define VIEW_ROWS  8
#define MAP_OX     24
#define MAP_OY     (HDR_H + 8)
#define PLAYER_TILE 16          /* U2 Upgrade tileset: 16 = person */

static void clampi(int *v, int lo, int hi)
{
    if (*v < lo) *v = lo;
    if (*v > hi) *v = hi;
}

/* 從地圖中心螺旋向外找第一個可通行 tile 當出生點。 */
static void find_start(const U2Map *m, U2Player *p)
{
    int cx = U2_MAP_W / 2, cy = U2_MAP_H / 2;
    for (int r = 0; r < U2_MAP_W; r++) {
        for (int dy = -r; dy <= r; dy++) {
            for (int dx = -r; dx <= r; dx++) {
                if (dx > -r && dx < r && dy > -r && dy < r) continue; /* 只走外環 */
                int x = cx + dx, y = cy + dy;
                if (x < 0 || y < 0 || x >= U2_MAP_W || y >= U2_MAP_H) continue;
                if (u2_passable(u2_map_tile(m, x, y))) {
                    p->x = x; p->y = y; p->tile = PLAYER_TILE;
                    return;
                }
            }
        }
    }
    p->x = cx; p->y = cy; p->tile = PLAYER_TILE; /* 全不通行的保底 */
}

/* 畫一張完整畫面到 canvas(地圖 viewport 跟隨玩家 + 實體 + 玩家 + 狀態/訊息)。 */
static void render_frame(SDL_Surface *canvas, const U2Map *map, const U2Mon *mon,
                         SDL_Surface *tiles, U2Text *title, U2Text *body,
                         const U2Player *p, const U2Strings *ui, const char *msg)
{
    SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 0, 0, 0));

    /* 標題列 */
    SDL_Rect hdr = { 0, 0, CANVAS_W, HDR_H };
    SDL_FillRect(canvas, &hdr, SDL_MapRGB(canvas->format, 36, 44, 110));
    u2_text_draw(canvas, title, "Ultima II:女巫的復仇 — 繁體中文", 10, 4, 235, 235, 245);

    /* 相機跟隨玩家(玩家置中,邊界 clamp) */
    int cam_x = p->x - VIEW_COLS / 2;
    int cam_y = p->y - VIEW_ROWS / 2;
    clampi(&cam_x, 0, U2_MAP_W - VIEW_COLS);
    clampi(&cam_y, 0, U2_MAP_H - VIEW_ROWS);

    u2_render_viewport(canvas, map, tiles, cam_x, cam_y,
                       VIEW_COLS, VIEW_ROWS, TILE_PX, MAP_OX, MAP_OY);
    u2_render_entities(canvas, mon, tiles, cam_x, cam_y,
                       VIEW_COLS, VIEW_ROWS, TILE_PX, MAP_OX, MAP_OY);

    /* 玩家 sprite(畫在玩家實際格在 viewport 內的位置) */
    int px = MAP_OX + (p->x - cam_x) * TILE_PX;
    int py = MAP_OY + (p->y - cam_y) * TILE_PX;
    if (tiles)
        u2_tileset_blit(canvas, tiles, p->tile, px, py, TILE_PX);
    /* 玩家高亮外框 */
    SDL_Rect mk = { px, py, TILE_PX, TILE_PX };
    Uint32 col = SDL_MapRGB(canvas->format, 250, 240, 90);
    SDL_Rect t = { mk.x, mk.y, mk.w, 2 }; SDL_FillRect(canvas, &t, col);
    SDL_Rect b = { mk.x, mk.y + mk.h - 2, mk.w, 2 }; SDL_FillRect(canvas, &b, col);
    SDL_Rect l = { mk.x, mk.y, 2, mk.h }; SDL_FillRect(canvas, &l, col);
    SDL_Rect rr = { mk.x + mk.w - 2, mk.y, 2, mk.h }; SDL_FillRect(canvas, &rr, col);

    int bottom_y = MAP_OY + VIEW_ROWS * TILE_PX + 10;

    /* 左:操作提示 + 訊息列 */
    u2_text_draw(canvas, body, "方向鍵 / WASD 移動  ·  Q 離開",
                 MAP_OX, bottom_y, 150, 175, 205);
    u2_text_draw(canvas, body, msg, MAP_OX, bottom_y + 30, 210, 225, 205);
    char pos[64];
    snprintf(pos, sizeof pos, "座標 (%d, %d)  地形 id=%d",
             p->x, p->y, u2_map_tile(map, p->x, p->y));
    u2_text_draw(canvas, body, pos, MAP_OX, bottom_y + 60, 150, 165, 150);

    /* 右:狀態欄,標籤從 exe UI 翻譯表查 */
    static const struct { const char *orig; const char *val; } st[] = {
        { "H.P.=", "0343" }, { "FOOD=", "0184" },
        { "EXP.=%.4d", "0018" }, { "GOLD=%.4d", "0067" },
    };
    int sx0 = 640, sy0 = bottom_y;
    for (int i = 0; i < 4; i++) {
        const char *zh = ui ? u2_strings_lookup(ui, st[i].orig) : NULL;
        char label[64];
        if (zh) {
            size_t k = 0;
            for (const char *q = zh; *q && *q != '=' && k < sizeof label - 1; q++)
                label[k++] = *q;
            label[k] = 0;
        } else {
            snprintf(label, sizeof label, "%s", st[i].orig);
        }
        char line[96];
        snprintf(line, sizeof line, "%s  %s", label, st[i].val);
        u2_text_draw(canvas, body, line, sx0, sy0, 235, 225, 150);
        sy0 += 30;
    }
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr,
            "用法: %s <mapxNN> <font.ttf> <tileset.png> <ui_tsv> "
            "[--script MOVES out_prefix]\n", argv[0]);
        return 2;
    }
    const char *map_path = argv[1];
    const char *font_path = argv[2];
    const char *tiles_path = argv[3];
    const char *ui_tsv_path = argv[4];

    const char *script = NULL, *out_prefix = NULL;
    for (int i = 5; i < argc; i++) {
        if (strcmp(argv[i], "--script") == 0 && i + 2 < argc) {
            script = argv[i + 1];
            out_prefix = argv[i + 2];
            i += 2;
        }
    }
    int headless = (script != NULL);

    Uint32 sdl_flags = headless ? 0 : SDL_INIT_VIDEO;
    if (SDL_Init(sdl_flags) != 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return 1;
    }

    U2Map map = u2_map_load(map_path);
    if (!map.ok) { fprintf(stderr, "無法載入地圖: %s\n", map_path); return 1; }

    /* mon / ui 路徑同 PoC 慣例 */
    char mon_path[512];
    snprintf(mon_path, sizeof mon_path, "%s", map_path);
    char *bn = strrchr(mon_path, '/'); bn = bn ? bn + 1 : mon_path;
    char *mp = strstr(bn, "map"); if (mp) memcpy(mp, "mon", 3);
    U2Mon mon = u2_mon_load(mon_path);

    U2Strings ui = ui_tsv_path ? u2_strings_load(ui_tsv_path, 2, 3) : (U2Strings){0};
    SDL_Surface *tiles = u2_tileset_load(tiles_path);

    SDL_Surface *canvas = SDL_CreateRGBSurfaceWithFormat(
        0, CANVAS_W, CANVAS_H, 32, SDL_PIXELFORMAT_RGBA32);
    U2Text title = u2_text_open(font_path, 20);
    U2Text body = u2_text_open(font_path, 22);
    if (!title.font || !body.font) {
        fprintf(stderr, "字型載入失敗: %s (%s)\n", font_path, TTF_GetError());
        return 1;
    }

    U2Player player;
    find_start(&map, &player);
    char msg[128];
    snprintf(msg, sizeof msg, "歡迎來到 Sosaria,冒險者。");

    if (headless) {
        /* 腳本模式:起始畫面 + 每步存 PNG */
        int step = 0;
        char out[600];
        render_frame(canvas, &map, &mon, tiles, &title, &body, &player, &ui, msg);
        snprintf(out, sizeof out, "%s%02d.png", out_prefix, step++);
        IMG_SavePNG(canvas, out);
        for (const char *s = script; *s; s++) {
            char dir = *s;
            if (dir == 'w') dir = 'N';
            else if (dir == 's') dir = 'S';
            else if (dir == 'a') dir = 'W';
            else if (dir == 'd') dir = 'E';
            int moved = u2_player_move(&player, &map, dir);
            snprintf(msg, sizeof msg, moved ? "往 %c 移動。" : "%c 方向被擋住。", dir);
            render_frame(canvas, &map, &mon, tiles, &title, &body, &player, &ui, msg);
            snprintf(out, sizeof out, "%s%02d.png", out_prefix, step++);
            IMG_SavePNG(canvas, out);
        }
        printf("腳本完成:%d 步,輸出 %s00..%02d.png\n", step - 1, out_prefix, step - 1);
    } else {
        /* 互動模式:視窗 + 事件迴圈 */
        SDL_Window *win = SDL_CreateWindow(
            "Ultima II 繁中", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            CANVAS_W, CANVAS_H, 0);
        SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        int running = 1;
        while (running) {
            SDL_Event e;
            int dirty = 0;
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) running = 0;
                else if (e.type == SDL_KEYDOWN) {
                    char dir = 0;
                    switch (e.key.keysym.sym) {
                        case SDLK_UP: case SDLK_w: dir = 'N'; break;
                        case SDLK_DOWN: case SDLK_s: dir = 'S'; break;
                        case SDLK_LEFT: case SDLK_a: dir = 'W'; break;
                        case SDLK_RIGHT: case SDLK_d: dir = 'E'; break;
                        case SDLK_q: case SDLK_ESCAPE: running = 0; break;
                    }
                    if (dir) {
                        int moved = u2_player_move(&player, &map, dir);
                        snprintf(msg, sizeof msg,
                                 moved ? "往 %c 移動。" : "%c 方向被擋住。", dir);
                        dirty = 1;
                    }
                }
            }
            if (dirty || 1) {
                render_frame(canvas, &map, &mon, tiles, &title, &body,
                             &player, &ui, msg);
                SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, canvas);
                SDL_RenderClear(ren);
                SDL_RenderCopy(ren, tex, NULL, NULL);
                SDL_RenderPresent(ren);
                SDL_DestroyTexture(tex);
            }
            SDL_Delay(16);
        }
        SDL_DestroyRenderer(ren);
        SDL_DestroyWindow(win);
    }

    u2_text_close(&title);
    u2_text_close(&body);
    if (tiles) SDL_FreeSurface(tiles);
    SDL_FreeSurface(canvas);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
