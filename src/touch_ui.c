/* 觸控疊層實作 — 見 touch_ui.h。 */
#include "touch_ui.h"
#include <string.h>

int touch_ui_enabled = 0;

static int CW = 960, CH = 600;
static U2Text *g_font = NULL;
static int g_more = 0;          /* 「≡ More」次要指令面板開合 */

typedef struct { SDL_Rect r; SDL_Keycode key; const char *label; int kind; } Btn;
/* kind: 0=一般 1=方向(較大) 2=面板切換(≡) 3=面板內按鈕 */

#define MAXB 48
static Btn g_btn[MAXB];
static int g_nb = 0;

static void add(int x, int y, int w, int h, SDL_Keycode key, const char *label, int kind)
{
    if (g_nb >= MAXB) return;
    g_btn[g_nb].r = (SDL_Rect){ x, y, w, h };
    g_btn[g_nb].key = key;
    g_btn[g_nb].label = label;
    g_btn[g_nb].kind = kind;
    g_nb++;
}

void touch_ui_set_canvas(int w, int h) { CW = w; CH = h; }
void touch_ui_set_font(U2Text *font) { g_font = font; }

/* 數字列 1..0(商店買賣 / 地牢法術)。 */
static void add_number_row(void)
{
    int bw = 70, bh = 56, gap = 6;
    int total = 10 * bw + 9 * gap;
    int x0 = (CW - total) / 2, y = CH - bh - 8;
    for (int i = 0; i < 10; i++) {
        SDL_Keycode k = (i == 9) ? SDLK_0 : (SDLK_1 + i);
        static const char *lab[10] = {"1","2","3","4","5","6","7","8","9","0"};
        add(x0 + i * (bw + gap), y, bw, bh, k, lab[i], 0);
    }
}

void touch_ui_layout(int ctx)
{
    g_nb = 0;
    if (!touch_ui_enabled) return;

    if (ctx & TUI_QUIT) {
        add(CW/2 - 160, CH/2 + 60, 150, 64, SDLK_y, "是 Y", 0);
        add(CW/2 + 10,  CH/2 + 60, 150, 64, SDLK_n, "否 N", 0);
        return;
    }

    if (ctx & TUI_MENU) {
        add(40, CH - 230, 120, 64, SDLK_UP,     "↑", 1);
        add(40, CH - 150, 120, 64, SDLK_DOWN,   "↓", 1);
        add(40, CH -  70, 120, 64, SDLK_RETURN, "確 ↵", 0);
        return;
    }

    if (ctx & TUI_CREATE) {
        /* 方向選擇 + 確認;姓名輸入交給 Android 軟鍵盤(SDL_StartTextInput)。 */
        add(40,  CH - 150, 90, 64, SDLK_LEFT,   "←", 1);
        add(140, CH - 150, 90, 64, SDLK_RIGHT,  "→", 1);
        add(40,  CH - 230, 90, 64, SDLK_UP,     "↑", 1);
        add(140, CH - 230, 90, 64, SDLK_DOWN,   "↓", 1);
        add(40,  CH -  70, 190, 64, SDLK_RETURN, "確認 ↵", 0);
        return;
    }

    /* ---- 主遊戲 ---- */
    if (ctx & TUI_GAME) {
        /* 左下 D-pad(十字) */
        int s = 78, cx = 130, cy = CH - 150;
        add(cx,        cy - s,   s, s, SDLK_UP,    "↑", 1);
        add(cx,        cy + s,   s, s, SDLK_DOWN,  "↓", 1);
        add(cx - s,    cy,       s, s, SDLK_LEFT,  "←", 1);
        add(cx + s,    cy,       s, s, SDLK_RIGHT, "→", 1);
        add(cx,        cy,       s, s, SDLK_RETURN,"↵", 0);   /* 中央:確認/通過 */

        /* 右側主要動作鈕(2 欄) */
        int bw = 92, bh = 58, gap = 8;
        int rx = CW - 2*bw - gap - 12, ry = CH - 4*bh - 3*gap - 12;
        struct { SDL_Keycode k; const char *l; } act[] = {
            {SDLK_t,"談 T"}, {SDLK_z,"店 Z"},
            {SDLK_b,"乘 B"}, {SDLK_x,"出 X"},
            {SDLK_p,"門 P"}, {SDLK_c,"表 C"},
            {SDLK_y,"喊/射 Y"}, {0,"≡"},
        };
        for (int i = 0; i < 8; i++) {
            int col = i & 1, row = i / 2;
            int x = rx + col * (bw + gap), y = ry + row * (bh + gap);
            if (act[i].k == 0) add(x, y, bw, bh, 0, "≡", 2);   /* More 切換 */
            else add(x, y, bw, bh, act[i].k, act[i].l, 0);
        }

        /* 右上系統鈕 */
        add(CW - 70,  10, 58, 48, SDLK_F10, "離", 0);
        add(CW - 134, 10, 58, 48, SDLK_F1,  "?",  0);
        add(CW - 198, 10, 58, 48, SDLK_F4,  "語", 0);
        add(CW - 262, 10, 58, 48, SDLK_g,   "畫", 0);
    }

    if (ctx & TUI_DUNGEON) {
        add(12, 70,  70, 56, SDLK_j, "下層", 0);
        add(12, 134, 70, 56, SDLK_k, "上層", 0);
        add_number_row();   /* 法術 1..0 */
    }
    if (ctx & TUI_SHOP) add_number_row();   /* 買賣 1..0 */

    /* ---- More 面板:次要指令 ---- */
    if (g_more && (ctx & TUI_GAME)) {
        struct { SDL_Keycode k; const char *l; } more[] = {
            {SDLK_f,"竊 F"}, {SDLK_v,"瞰 V"}, {SDLK_n,"凝 N"},
            {SDLK_i,"道具"}, {SDLK_ESCAPE,"消 ESC"},
        };
        int bw = 150, bh = 56, gap = 10;
        int n = (int)(sizeof more / sizeof more[0]);
        int total = n * bh + (n - 1) * gap;
        int y0 = (CH - total) / 2, x = CW/2 - bw/2;
        for (int i = 0; i < n; i++)
            add(x, y0 + i * (bh + gap), bw, bh, more[i].k, more[i].l, 3);
    }
}

static void fill(SDL_Surface *cv, SDL_Rect r, Uint8 cr, Uint8 cg, Uint8 cb, Uint8 a)
{
    /* 半透明填色(手動 alpha blend,因目標 surface 無 blend) */
    if (a == 255) { SDL_FillRect(cv, &r, SDL_MapRGB(cv->format, cr, cg, cb)); return; }
    SDL_Surface *t = SDL_CreateRGBSurfaceWithFormat(0, r.w, r.h, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_FillRect(t, NULL, SDL_MapRGBA(t->format, cr, cg, cb, a));
    SDL_SetSurfaceBlendMode(t, SDL_BLENDMODE_BLEND);
    SDL_BlitSurface(t, NULL, cv, &r);
    SDL_FreeSurface(t);
}

void touch_ui_draw(SDL_Surface *cv)
{
    if (!touch_ui_enabled || !g_nb) return;
    for (int i = 0; i < g_nb; i++) {
        Btn *b = &g_btn[i];
        int dir = (b->kind == 1);
        int panel = (b->kind == 3);
        /* 底色 */
        fill(cv, b->r, dir ? 40 : (panel ? 60 : 30), dir ? 60 : 50, dir ? 90 : 70, 170);
        /* 邊框 */
        SDL_Rect top = { b->r.x, b->r.y, b->r.w, 2 };
        SDL_Rect bot = { b->r.x, b->r.y + b->r.h - 2, b->r.w, 2 };
        SDL_Rect lft = { b->r.x, b->r.y, 2, b->r.h };
        SDL_Rect rgt = { b->r.x + b->r.w - 2, b->r.y, 2, b->r.h };
        Uint32 brc = SDL_MapRGB(cv->format, 150, 160, 200);
        SDL_FillRect(cv, &top, brc); SDL_FillRect(cv, &bot, brc);
        SDL_FillRect(cv, &lft, brc); SDL_FillRect(cv, &rgt, brc);
        /* 標籤(置中) */
        if (g_font && g_font->font && b->label) {
            int tw = 0, th = 0;
            TTF_SizeUTF8(g_font->font, b->label, &tw, &th);
            int tx = b->r.x + (b->r.w - tw) / 2;
            int ty = b->r.y + (b->r.h - th) / 2;
            u2_text_draw(cv, g_font, b->label, tx, ty, 235, 235, 200);
        }
    }
}

static void push_key(SDL_Keycode k)
{
    SDL_Event e;
    SDL_zero(e);
    e.type = SDL_KEYDOWN;
    e.key.state = SDL_PRESSED;
    e.key.keysym.sym = k;
    e.key.keysym.scancode = SDL_GetScancodeFromKey(k);
    SDL_PushEvent(&e);
    e.type = SDL_KEYUP;
    e.key.state = SDL_RELEASED;
    SDL_PushEvent(&e);
}

int touch_ui_finger(float nx, float ny)
{
    if (!touch_ui_enabled || !g_nb) return 0;
    int x = (int)(nx * CW), y = (int)(ny * CH);
    for (int i = 0; i < g_nb; i++) {
        Btn *b = &g_btn[i];
        if (x < b->r.x || y < b->r.y || x >= b->r.x + b->r.w || y >= b->r.y + b->r.h) continue;
        if (b->kind == 2) { g_more = !g_more; return 1; }   /* ≡ 切換面板 */
        if (b->kind == 3) g_more = 0;                       /* 面板按鈕:按後關面板 */
        push_key(b->key);
        return 1;
    }
    return 0;
}
