#include "u2_play.h"

/* PoC 可通行表:海洋(0) 不可步行(需船);其餘暫可通行。
 * TODO 對照 oracle 校正山脈/牆等;目前以海洋碰撞示範移動。 */
int u2_passable(unsigned char tile_id)
{
    if (tile_id == 0)   /* 海洋/深水 */
        return 0;
    return 1;
}

int u2_player_move(U2Player *p, const U2Map *m, char dir)
{
    int nx = p->x, ny = p->y;
    switch (dir) {
    case 'N': ny--; break;
    case 'S': ny++; break;
    case 'E': nx++; break;
    case 'W': nx--; break;
    default: return 0;
    }
    if (nx < 0 || nx >= U2_MAP_W || ny < 0 || ny >= U2_MAP_H)
        return 0; /* 地圖邊界 */
    if (!u2_passable(u2_map_tile(m, nx, ny)))
        return 0; /* 撞牆/水 */
    p->x = nx;
    p->y = ny;
    return 1;
}
