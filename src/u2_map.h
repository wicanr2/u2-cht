/* u2_map — Ultima II 地圖檔 (mapxNN) 解析
 *
 * 格式 (docs/DATA_FORMATS.md):64×66 tile array,4224 byte,無 header。
 * 每 byte 為 tile_id × 4,讀取要 ÷4。tile_id 範圍 0..63。
 * 介面收斂:載入 → 查 tile。呼叫端不需知道 ×4 quirk。
 */
#ifndef U2_MAP_H_INCLUDED
#define U2_MAP_H_INCLUDED

#define U2_MAP_W 64
#define U2_MAP_H 66

typedef struct {
    unsigned char tile[U2_MAP_H][U2_MAP_W]; /* 已 ÷4 還原的 tile_id */
    int ok;
} U2Map;

/* 從 mapxNN 檔載入。失敗回傳 .ok = 0。 */
U2Map u2_map_load(const char *path);

/* 取得 (x,y) 的 tile_id;越界回傳 0。 */
unsigned char u2_map_tile(const U2Map *m, int x, int y);

#endif
