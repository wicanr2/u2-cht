/* poc_main — 垂直切片 PoC,版面對齊真實 Ultima II (參考 ultima2.voyd.net):
 *   上:地圖 viewport (真 tile + monxNN 實體層)
 *   下:左=指令/訊息列,右=狀態 (生命/食物/經驗/黃金)
 * 一次驗證:繪圖換得掉 / 資料讀得到 / 中文畫得出。輸出 PNG (headless)。
 *
 * 用法: u2_poc <mapxNN> <font.ttf> <out.png> [tileset.png]
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include "u2_map.h"
#include "u2_mon.h"
#include "u2_render.h"
#include "u2_strings.h"
#include "u2_talk.h"
#include "u2_tileset.h"
#include "u2_text.h"

#define CANVAS_W   960
#define CANVAS_H   600
#define HDR_H      30           /* 仿視窗標題列 */
#define TILE_PX    48           /* 16 × 3 (ADR 內部 3×) */
#define VIEW_COLS  19           /* 912 px 寬 */
#define VIEW_ROWS  8            /* 384 px 高 */
#define MAP_OX     24
#define MAP_OY     (HDR_H + 8)

static void clamp(int *v, int lo, int hi)
{
    if (*v < lo) *v = lo;
    if (*v > hi) *v = hi;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "用法: %s <mapxNN> <font.ttf> <out.png> [tileset.png] [talk_dialogue.tsv]\n", argv[0]);
        return 2;
    }
    const char *map_path = argv[1];
    const char *font_path = argv[2];
    const char *out_path = argv[3];
    const char *tiles_path = (argc > 4) ? argv[4] : NULL;
    const char *tsv_path = (argc > 5) ? argv[5] : NULL;

    if (SDL_Init(0) != 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return 1;
    }

    U2Map map = u2_map_load(map_path);
    if (!map.ok) {
        fprintf(stderr, "無法載入地圖: %s\n", map_path);
        return 1;
    }

    /* sector 實體層:map 路徑 basename "map"→"mon" */
    char mon_path[512];
    snprintf(mon_path, sizeof mon_path, "%s", map_path);
    char *bn = strrchr(mon_path, '/');
    bn = bn ? bn + 1 : mon_path;
    char *mp = strstr(bn, "map");
    if (mp) memcpy(mp, "mon", 3);
    U2Mon mon = u2_mon_load(mon_path);

    /* 端到端在地化:載入原始對話 (tlkxNN) + 翻譯覆蓋層 */
    char tlk_path[512];
    snprintf(tlk_path, sizeof tlk_path, "%s", map_path);
    char *tb = strrchr(tlk_path, '/');
    tb = tb ? tb + 1 : tlk_path;
    char *tp = strstr(tb, "map");
    if (tp) memcpy(tp, "tlk", 3);
    U2Talk talk = u2_talk_load(tlk_path);
    U2Strings tr = tsv_path ? u2_strings_load(tsv_path, 2, 3) : (U2Strings){0};

    SDL_Surface *canvas = SDL_CreateRGBSurfaceWithFormat(
        0, CANVAS_W, CANVAS_H, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_FillRect(canvas, NULL, SDL_MapRGB(canvas->format, 0, 0, 0));

    U2Text title = u2_text_open(font_path, 20);
    U2Text body = u2_text_open(font_path, 22);
    if (!title.font || !body.font) {
        fprintf(stderr, "字型載入失敗: %s (%s)\n", font_path, TTF_GetError());
        return 1;
    }

    /* 仿視窗標題列 */
    SDL_Rect hdr = { 0, 0, CANVAS_W, HDR_H };
    SDL_FillRect(canvas, &hdr, SDL_MapRGB(canvas->format, 36, 44, 110));
    u2_text_draw(canvas, &title, "Ultima II:女巫的復仇 — 繁體中文",
                 10, 4, 235, 235, 245);

    /* 地圖 viewport,對準實體群 (空城→活城) */
    SDL_Surface *tiles = tiles_path ? u2_tileset_load(tiles_path) : NULL;
    int cam_x = (U2_MAP_W - VIEW_COLS) / 2;
    int cam_y = (U2_MAP_H - VIEW_ROWS) / 2;
    if (mon.count > 0) {
        long sx = 0, sy = 0;
        for (int i = 0; i < mon.count; i++) { sx += mon.ent[i].x; sy += mon.ent[i].y; }
        cam_x = (int)(sx / mon.count) - VIEW_COLS / 2;
        cam_y = (int)(sy / mon.count) - VIEW_ROWS / 2;
        clamp(&cam_x, 0, U2_MAP_W - VIEW_COLS);
        clamp(&cam_y, 0, U2_MAP_H - VIEW_ROWS);
    }
    u2_render_viewport(canvas, &map, tiles, cam_x, cam_y,
                       VIEW_COLS, VIEW_ROWS, TILE_PX, MAP_OX, MAP_OY);
    u2_render_entities(canvas, &mon, tiles, cam_x, cam_y,
                       VIEW_COLS, VIEW_ROWS, TILE_PX, MAP_OX, MAP_OY);

    /* 底部文字區起點 */
    int bottom_y = MAP_OY + VIEW_ROWS * TILE_PX + 10;

    /* 左:訊息列 — 端到端在地化展示。
       讀原始 tlkxNN 對話 → 翻譯覆蓋層查譯文 (查無 fallback 原文) → 繪 CJK。 */
    u2_text_draw(canvas, &body, "── NPC 對話 (tlkx → 翻譯覆蓋層) ──",
                 MAP_OX, bottom_y, 150, 175, 205);
    int ly = bottom_y + 30;
    int shown = 0;
    for (int i = 0; i < talk.count && shown < 3; i++) {
        const char *zh = u2_strings_lookup(&tr, talk.line[i]);
        const char *disp = zh ? zh : talk.line[i]; /* fallback 原文 */
        char one[256];
        size_t k = 0;
        for (const char *p = disp; *p && k < sizeof one - 1; p++)
            one[k++] = (*p == '\r') ? ' ' : *p; /* \r→空白,單行顯示 */
        one[k] = 0;
        int translated = (zh != NULL);
        u2_text_draw(canvas, &body, one, MAP_OX, ly,
                     translated ? 210 : 170, translated ? 225 : 170,
                     translated ? 205 : 120);
        ly += 30;
        shown++;
    }
    if (shown == 0)
        u2_text_draw(canvas, &body, "(此地圖無對話資料)", MAP_OX, ly, 150, 150, 150);

    /* 右:狀態欄 (對齊 U2 的 H.P./FOOD/EXP/GOLD) */
    static const char *stat_lines[] = {
        "生命  0343",
        "食物  0184",
        "經驗  0018",
        "黃金  0067",
    };
    int sx0 = 640, sy0 = bottom_y;
    for (int i = 0; i < 4; i++) {
        u2_text_draw(canvas, &body, stat_lines[i], sx0, sy0, 235, 225, 150);
        sy0 += 30;
    }

    if (IMG_SavePNG(canvas, out_path) != 0) {
        fprintf(stderr, "存檔失敗: %s\n", IMG_GetError());
        return 1;
    }
    printf("PoC 完成 → %s (實體層 monxNN: %d)\n", out_path, mon.count);

    u2_text_close(&title);
    u2_text_close(&body);
    SDL_FreeSurface(canvas);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    return 0;
}
