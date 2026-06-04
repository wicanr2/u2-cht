/* u2_strings — 翻譯覆蓋層 (UTF-8)
 *
 * 載入 translations 下的 .tsv,以「原文」為 key 查繁中譯文。
 * 原則 (CONTEXT.md):不寫回原始資料,載入時以外部 UTF-8 覆蓋。
 * TSV 欄:... \t original_text \t zh_hant (對話表為 file\tindex\torig\tzh)。
 */
#ifndef U2_STRINGS_H
#define U2_STRINGS_H

#define U2_STR_MAX 256
#define U2_STR_LEN 192

typedef struct {
    char orig[U2_STR_MAX][U2_STR_LEN];
    char zh[U2_STR_MAX][U2_STR_LEN];
    int count;
} U2Strings;

/* 載入翻譯表;orig_col/zh_col 為 0-based 欄索引。"\r" 還原為 0x0d。 */
U2Strings u2_strings_load(const char *tsv_path, int orig_col, int zh_col);

/* 以原文查譯文;查無或譯文空回傳 NULL。 */
const char *u2_strings_lookup(const U2Strings *s, const char *orig);

#endif
