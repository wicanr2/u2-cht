/* u2_play — 玩家狀態與移動 (隊伍在世界地圖上行走)
 *
 * U2 慣例:玩家恆置於畫面中央,地圖在腳下捲動。移動受 tile 可通行性限制。
 * passable 表為 PoC 推定 (water/mountain/wall 不可通行);後續對照 oracle 校正。
 */
#ifndef U2_PLAY_H
#define U2_PLAY_H

#include "u2_map.h"

typedef struct {
    int x, y;          /* 玩家在地圖上的座標 */
    int tile;          /* 玩家 sprite 的 tile id */
} U2Player;

/* tile 是否可步行通過。 */
int u2_passable(unsigned char tile_id);

/* 嘗試往 dir ('N'/'S'/'E'/'W') 移動一步。
 * 回傳 1=成功移動,0=被擋(撞牆/水/地圖邊界)。 */
int u2_player_move(U2Player *p, const U2Map *m, char dir);

#endif
