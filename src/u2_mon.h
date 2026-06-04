/* u2_mon — sector 實體層 (monxNN):NPC/怪物
 *
 * 格式 (docs/DATA_FORMATS.md,已破解):384 B = 32 格平行陣列
 *   X=[0x00+i] Y=[0x20+i] status=[0x40+i] tile=[0x60+i]×4
 *   tile==0 為空 slot。
 * 地圖只有地形;實體層由此疊上 → 空城變活城。
 */
#ifndef U2_MON_H
#define U2_MON_H

#define U2_MON_SLOTS 32

typedef struct {
    unsigned char x, y, status, tile; /* tile 已 ÷4 還原 */
} U2Entity;

typedef struct {
    U2Entity ent[U2_MON_SLOTS];
    int count;
} U2Mon;

/* 載入 monxNN。失敗回傳 .count = 0。 */
U2Mon u2_mon_load(const char *path);

#endif
