#ifndef U2_TOUCH_UI_H
#define U2_TOUCH_UI_H
#include <SDL.h>
#include "u2_text.h"

/* 觸控疊層:把螢幕邊緣的虛擬按鈕,於觸控時合成對應 SDL_KEYDOWN 事件,
 * 讓既有(鍵盤驅動的)選單/建角/主迴圈/商店無須改寫即可手機操作。
 * 桌面預設停用;Android 開啟。畫面座標一律用 canvas(CANVAS_W×CANVAS_H)。 */

/* context 旗標:決定顯示哪組按鈕。 */
enum {
    TUI_MENU    = 1,    /* 開場選單:上/下/Enter */
    TUI_CREATE  = 2,    /* 建角:上/下/左/右/Enter + 軟鍵盤(姓名) */
    TUI_GAME    = 4,    /* 主遊戲:D-pad + 動作鈕 */
    TUI_SHOP    = 8,    /* 商店開啟:數字列 1..0 */
    TUI_DUNGEON = 16,   /* 地牢:數字列(法術)+ 上/下層 */
    TUI_QUIT    = 32    /* 離開確認:Y/N */
};

extern int touch_ui_enabled;          /* 0=停用(桌面預設) 1=啟用(Android) */

void touch_ui_set_canvas(int w, int h);          /* 告知 canvas 尺寸(座標換算用) */
void touch_ui_set_font(U2Text *font);            /* 標籤字型 */
void touch_ui_layout(int ctx);                   /* 依 context 重建目前按鈕配置 */
void touch_ui_draw(SDL_Surface *cv);             /* 把目前配置畫到 canvas */
/* 觸控/滑鼠按下:nx,ny 為視窗 normalized(0..1)。命中按鈕則合成 keydown 並回傳 1。 */
int  touch_ui_finger(float nx, float ny);

#endif
