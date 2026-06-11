/* u2_dungeon — 地牢資料(多層)+ first-person 線框繪製
 *
 * 資料(docs/DATA_FORMATS.md + oracle):地牢檔為多層平鋪,
 *   **index = level*256 + Y*16 + X**(每層 16×16=256 byte,row stride 16)。
 *   cell 用 raw byte:0x80=牆、0x00=走廊、0xC0=門;樓梯以位元判定:
 *   `& 0x10`=上梯(KLIMB up)、`& 0x20`=下梯(DESCEND down)(0xE0 含 0x20=下梯)。
 *   全 0xFF 的 block 視為結尾(未使用層)。
 *   朝向 0/1/2/3 = N/E/S/W(oracle this+0x74c0)。
 * 繪製:沿朝向往前掃深度,逐格畫透視方框 + 側牆/側開口 + 背牆(線框),
 *   對應 oracle FUN_0040d000 的程式化畫線。
 */
#ifndef U2_DUNGEON_H
#define U2_DUNGEON_H

#include <SDL.h>

#define U2_DNG_N    16
#define U2_DNG_MAXL 16

typedef struct {
    unsigned char cell[U2_DNG_MAXL][U2_DNG_N][U2_DNG_N]; /* [level][y][x] raw byte */
    int levels;   /* 有效層數 */
    int ok;
} U2Dungeon;

/* 從地牢檔(mapxN4/N5)載入所有層(stride 16)。失敗回 .ok=0。 */
U2Dungeon u2_dungeon_load(const char *path);

/* (level,x,y) 是否為實心牆。門/梯/走廊視為通道。 */
int u2_dungeon_is_wall(const U2Dungeon *d, int level, int x, int y);

/* 樓梯方向:+1=可下樓(&0x20)、-1=可上樓(&0x10)、0=非梯;同時上下回 +1。 */
int u2_dungeon_ladder(const U2Dungeon *d, int level, int x, int y);

/* 在 surf 的 (ox,oy,w,h) 方形視區畫 (level) 上站在 (px,py) 面向 dir 的線框。
 * style = 畫風 index(隨 G 鍵 curset 切換地牢線框配色)。
 * ent_depth = 正前方最近實體的深度(1..可見深度;-1=無);ent_kind = 'M'怪物 / 'C'寶箱;
 *   ent_tile = 怪物類型 tile(對齊 oracle 低 nibble 怪物 index → 不同外觀;寶箱忽略)。
 *   對齊 oracle:掃描後往正前方找最近怪物畫 sprite(depthIndex 控縮放、index 決定外觀)。
 * 回傳前方可見通道深度。 */
int u2_dungeon_render(SDL_Surface *surf, const U2Dungeon *d, int level,
                      int px, int py, int dir, int ox, int oy, int w, int h, int style,
                      int ent_depth, char ent_kind, unsigned char ent_tile);

#endif
