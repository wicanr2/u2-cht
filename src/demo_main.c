/* demo_main — 隊伍移動 headless 驗證:跑一段移動序列,每步輸出一張 PNG。
 * 玩家恆置中,地圖捲動跟隨,海洋擋路。驗證 移動/鏡頭跟隨/碰撞。
 *
 * 用法: u2_demo <mapxNN> <font.ttf> <out_prefix> <tileset.png> <moves NSEW...> [sx sy]
 *        例: ... build/move "EEENNNWWSS"   → build/move_00.png ...
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include "u2_map.h"
#include "u2_mon.h"
#include "u2_play.h"
#include "u2_render.h"
#include "u2_tileset.h"
#include "u2_text.h"

#define CANVAS_W  840
#define CANVAS_H  600
#define HDR_H     30
#define TILE_PX   48
#define VIEW_COLS 15           /* 720 px,中央格 = (7,4) */
#define VIEW_ROWS 9            /* 432 px */
#define MAP_OX    24
#define MAP_OY    (HDR_H + 8)
#define PLAYER_TILE 25         /* 人形 sprite */

static const char *dir_zh(char d)
{
    switch (d) {
    case 'N': return "北"; case 'S': return "南";
    case 'E': return "東"; case 'W': return "西";
    }
    return "?";
}

int main(int argc, char **argv)
{
    if (argc < 6) {
        fprintf(stderr, "用法: %s <map> <font> <out_prefix> <tileset> <moves> [sx sy]\n", argv[0]);
        return 2;
    }
    const char *map_path = argv[1], *font_path = argv[2];
    const char *out_prefix = argv[3], *tiles_path = argv[4];
    const char *moves = argv[5];

    SDL_Init(0);
    IMG_Init(IMG_INIT_PNG);
    U2Map map = u2_map_load(map_path);
    if (!map.ok) { fprintf(stderr, "map 載入失敗\n"); return 1; }

    /* 實體層 */
    char mon_path[512];
    snprintf(mon_path, sizeof mon_path, "%s", map_path);
    char *bn = strrchr(mon_path, '/'); bn = bn ? bn + 1 : mon_path;
    char *mp = strstr(bn, "map"); if (mp) memcpy(mp, "mon", 3);
    U2Mon mon = u2_mon_load(mon_path);

    SDL_Surface *tiles = u2_tileset_load(tiles_path);
    U2Text title = u2_text_open(font_path, 20);
    U2Text body = u2_text_open(font_path, 22);

    /* 起點:給定 or 自地圖中心向外找第一塊陸地 */
    U2Player pl = { U2_MAP_W / 2, U2_MAP_H / 2, PLAYER_TILE };
    if (argc >= 8) { pl.x = atoi(argv[6]); pl.y = atoi(argv[7]); }
    else {
        for (int r = 0; r < 40 && !u2_passable(u2_map_tile(&map, pl.x, pl.y)); r++) {
            pl.x = U2_MAP_W / 2 + r; /* 簡單向東掃 */
        }
    }

    int nsteps = (int)strlen(moves);
    char msg[64] = "開始";
    for (int step = 0; step <= nsteps; step++) {
        if (step > 0) {
            char d = moves[step - 1];
            int ok = u2_player_move(&pl, &map, d);
            snprintf(msg, sizeof msg, "指令:%s%s", dir_zh(d), ok ? "" : "──無法移動!");
        }

        SDL_Surface *cv = SDL_CreateRGBSurfaceWithFormat(0, CANVAS_W, CANVAS_H, 32,
                                                         SDL_PIXELFORMAT_RGBA32);
        SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format, 0, 0, 0));
        SDL_Rect hdr = { 0, 0, CANVAS_W, HDR_H };
        SDL_FillRect(cv, &hdr, SDL_MapRGB(cv->format, 36, 44, 110));
        u2_text_draw(cv, &title, "Ultima II:女巫的復仇 — 移動驗證", 10, 4, 235, 235, 245);

        /* camera 讓玩家置中 (不 clamp,邊界顯示海洋) */
        int cam_x = pl.x - VIEW_COLS / 2;
        int cam_y = pl.y - VIEW_ROWS / 2;
        u2_render_viewport(cv, &map, tiles, cam_x, cam_y,
                           VIEW_COLS, VIEW_ROWS, TILE_PX, MAP_OX, MAP_OY);
        u2_render_entities(cv, &mon, tiles, cam_x, cam_y,
                           VIEW_COLS, VIEW_ROWS, TILE_PX, MAP_OX, MAP_OY);
        /* 玩家恆置於中央格 */
        u2_tileset_blit(cv, tiles, pl.tile,
                        MAP_OX + (VIEW_COLS / 2) * TILE_PX,
                        MAP_OY + (VIEW_ROWS / 2) * TILE_PX, TILE_PX);

        /* 底部:訊息列 + 座標 */
        int by = MAP_OY + VIEW_ROWS * TILE_PX + 12;
        u2_text_draw(cv, &body, msg, MAP_OX, by, 200, 220, 200);
        char pos[64];
        snprintf(pos, sizeof pos, "座標 (%d,%d)   步 %d/%d", pl.x, pl.y, step, nsteps);
        u2_text_draw(cv, &body, pos, MAP_OX, by + 32, 235, 225, 150);

        char out[600];
        snprintf(out, sizeof out, "%s_%02d.png", out_prefix, step);
        IMG_SavePNG(cv, out);
        SDL_FreeSurface(cv);
    }
    printf("移動驗證:%d 幀 → %s_NN.png (終點 %d,%d)\n", nsteps + 1, out_prefix, pl.x, pl.y);

    u2_text_close(&title); u2_text_close(&body);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
    return 0;
}
