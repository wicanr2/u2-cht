/* u2_save — player 存檔 (角色記錄) 解析
 *
 * 格式 (oracle FUN at 0x401xxx 載入碼 + 本機驗證):
 *   - player 檔 384 byte;前 **256 byte (0x100) = 角色記錄**,載入時讀進物件。
 *   - 角色記錄 byte 0 != 0 表示「已建角色」(0 = 無角色 / fresh)。
 *   - offset 0x100 處有 `0x1a` 標記(與 monxNN 同;用途未定)。
 *   ⚠️ 記錄內各 stat(HP/食物/黃金/屬性…)之精確 byte offset **尚未對應**
 *      —— 需一份「已建角色」的真實存檔才能逐欄驗證;bundled player 為空。
 */
#ifndef U2_SAVE_H
#define U2_SAVE_H

#define U2_SAVE_SIZE   384
#define U2_REC_SIZE    256

typedef struct {
    int ok;                       /* 檔案讀取成功且大小正確 */
    int has_character;            /* rec[0] != 0 = 已建角色 */
    unsigned char rec[U2_REC_SIZE]; /* 256-byte 角色記錄 */
    unsigned char marker;         /* offset 0x100 的標記值 (空檔=0x1a) */
} U2Save;

/* 載入 player 檔。失敗回傳 .ok = 0。 */
U2Save u2_save_load(const char *path);

#endif
