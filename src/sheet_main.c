/* sheet_main — 繁中角色資料表:載入真實 player 存檔 → CJK 渲染
 *
 * 佐證 player 存檔破解(u2_save)端到端:從實機建角存檔解出
 * 姓名 / 性別 / 種族 / 職業 / 六屬性(BCD),以繁體中文排版繪成 PNG。
 *
 * 用法: u2_sheet <player_file> <font.ttf> <out.png>
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include "u2_save.h"
#include "u2_text.h"

#define W 600
#define H 560

int main(int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "用法: %s <player_file> <font.ttf> <out.png>\n", argv[0]);
        return 2;
    }
    if (SDL_Init(0) != 0 || IMG_Init(IMG_INIT_PNG) == 0) {
        fprintf(stderr, "SDL init: %s\n", SDL_GetError());
        return 1;
    }
    U2Save s = u2_save_load(argv[1]);
    if (!s.ok) { fprintf(stderr, "無法載入存檔: %s\n", argv[1]); return 1; }

    SDL_Surface *cv = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format, 12, 14, 28));
    /* 標題列 */
    SDL_Rect hdr = { 0, 0, W, 44 };
    SDL_FillRect(cv, &hdr, SDL_MapRGB(cv->format, 36, 44, 110));

    U2Text title = u2_text_open(argv[2], 24);
    U2Text body = u2_text_open(argv[2], 24);
    U2Text small = u2_text_open(argv[2], 18);
    if (!title.font || !body.font) {
        fprintf(stderr, "字型載入失敗: %s\n", TTF_GetError());
        return 1;
    }

    u2_text_draw(cv, &title, "角色資料 — 創世紀 II", 16, 8, 235, 235, 245);

    if (!s.has_character) {
        u2_text_draw(cv, &body, "(此存檔尚無角色)", 24, 80, 200, 180, 120);
    } else {
        int x = 32, y = 64, lh = 34;
        char ln[128];
        snprintf(ln, sizeof ln, "姓名: %s", s.name);
        u2_text_draw(cv, &body, ln, x, y, 230, 230, 235); y += lh;
        snprintf(ln, sizeof ln, "性別: %s", s.sex == 'F' ? "女" : "男");
        u2_text_draw(cv, &body, ln, x, y, 230, 230, 235); y += lh;
        snprintf(ln, sizeof ln, "種族: %s", u2_save_race_zh(s.race));
        u2_text_draw(cv, &body, ln, x, y, 230, 230, 235); y += lh;
        snprintf(ln, sizeof ln, "職業: %s", u2_save_class_zh(s.klass));
        u2_text_draw(cv, &body, ln, x, y, 230, 230, 235); y += lh + 8;

        /* 分隔線 */
        SDL_Rect sep = { x, y, W - 2 * x, 1 };
        SDL_FillRect(cv, &sep, SDL_MapRGB(cv->format, 80, 90, 130)); y += 14;

        u2_text_draw(cv, &small, "屬性(由 BCD 解碼)", x, y, 150, 170, 205); y += 28;
        for (int i = 0; i < U2_NUM_STATS; i++) {
            char lab[64];
            snprintf(lab, sizeof lab, "%s %s", u2_save_stat_zh(i), u2_save_stat_name(i));
            u2_text_draw(cv, &body, lab, x, y, 210, 215, 225);
            char val[16];
            snprintf(val, sizeof val, "%2d", s.stats[i]);
            u2_text_draw(cv, &body, val, x + 180, y, 245, 225, 150);
            y += lh;
        }
    }
    u2_text_draw(cv, &small,
        "資料來源:DOSBox 實機建角存檔 → u2_save 解析(姓名/性別/種族/職業/六屬性 BCD)",
        16, H - 28, 130, 145, 175);

    if (IMG_SavePNG(cv, argv[3]) != 0) {
        fprintf(stderr, "存檔失敗: %s\n", IMG_GetError());
        return 1;
    }
    printf("角色資料表 → %s (%s)\n", argv[3], s.has_character ? s.name : "無角色");

    u2_text_close(&title); u2_text_close(&body); u2_text_close(&small);
    SDL_FreeSurface(cv);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
    return 0;
}
