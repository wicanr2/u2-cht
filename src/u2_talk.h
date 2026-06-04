/* u2_talk — 原始對話檔 (tlkxNN) 解碼
 *
 * 格式 (docs/DATA_FORMATS.md):high-bit-set ASCII (每 byte OR 0x80,清 bit7),
 * \x00 分段,\r 換行。檔尾組譯殘渣 (PRBYTE/PRINT/$00) 過濾。
 */
#ifndef U2_TALK_H
#define U2_TALK_H

#define U2_TALK_MAX 16
#define U2_TALK_LEN 128

typedef struct {
    char line[U2_TALK_MAX][U2_TALK_LEN]; /* 還原的 ASCII 對話 (\r 保留) */
    int count;
} U2Talk;

/* 載入並解碼 tlkxNN。失敗回傳 .count = 0。 */
U2Talk u2_talk_load(const char *path);

#endif
