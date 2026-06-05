/* u2_dungeon — 地牢資料 + first-person 線框繪製
 *
 * 資料(docs/DATA_FORMATS.md + oracle FUN_0040d000):
 *   - 地牢 cell 用 **raw byte**(非地圖 ÷4):0x80=牆、0x00=走廊、0xC0=門、0xE0=梯、
 *     0x08/0x0C 等=特殊;可走集合 oracle 列 0x00/0x10/0x20/0x30/0x40/0xC0/0xE0。
 *   - 座標 0..15(16×16 一層);本模組讀地牢檔左上 16×16 當一層(多層堆疊待驗證)。
 *   - 朝向 0/1/2/3 = N/E/S/W(oracle this+0x74c0)。
 * 繪製:沿朝向往前掃深度,逐格畫透視方框 + 側牆/側開口 + 背牆(線框),
 *   對應 oracle 的程式化畫線(FUN_0040dd90 LineTo),非 tile bitmap。
 */
#ifndef U2_DUNGEON_H
#define U2_DUNGEON_H

#include <SDL.h>

#define U2_DNG_N 16

typedef struct {
    unsigned char cell[U2_DNG_N][U2_DNG_N]; /* [y][x] raw byte */
    int ok;
} U2Dungeon;

/* 從地牢檔(mapxN4/N5)載入左上 16×16 當一層。失敗回 .ok=0。 */
U2Dungeon u2_dungeon_load(const char *path);

/* cell 是否為實心牆(畫面要擋住)。門/梯/走廊視為可見通道。 */
int u2_dungeon_is_wall(const U2Dungeon *d, int x, int y);

/* 在 surf 的 (ox,oy,w,h) 方形視區內,畫出站在 (px,py) 面向 dir(0=N/1=E/2=S/3=W)
 * 的 first-person 線框地牢。回傳前方可見通道深度(到背牆為止)。 */
int u2_dungeon_render(SDL_Surface *surf, const U2Dungeon *d,
                      int px, int py, int dir,
                      int ox, int oy, int w, int h);

#endif
