#include "u2_strings.h"
#include "u2_i18n.h"
#include <stdio.h>
#include <string.h>

U2Lang u2_lang = U2_ZH;   /* 全域語系(F4 切換) */

/* 把字面 "\r" (反斜線+r) 還原為 0x0d,就地改寫。 */
static void unescape_cr(char *s)
{
    char *d = s;
    for (char *p = s; *p; p++) {
        if (p[0] == '\\' && p[1] == 'r') {
            *d++ = '\r';
            p++;
        } else {
            *d++ = *p;
        }
    }
    *d = 0;
}

/* 取第 col 個 tab 欄 (0-based) 到 out。 */
static int field(const char *line, int col, char *out, size_t cap)
{
    const char *p = line;
    for (int i = 0; i < col; i++) {
        p = strchr(p, '\t');
        if (!p)
            return 0;
        p++;
    }
    const char *e = strchr(p, '\t');
    size_t len = e ? (size_t)(e - p) : strlen(p);
    if (len >= cap)
        len = cap - 1;
    memcpy(out, p, len);
    out[len] = 0;
    return 1;
}

U2Strings u2_strings_load(const char *tsv_path, int orig_col, int zh_col)
{
    U2Strings s = {0};
    FILE *f = fopen(tsv_path, "r");
    if (!f)
        return s;
    char line[512];
    int first = 1;
    while (fgets(line, sizeof line, f) && s.count < U2_STR_MAX) {
        line[strcspn(line, "\r\n")] = 0;
        if (first) { /* 跳表頭 */
            first = 0;
            continue;
        }
        char o[U2_STR_LEN], z[U2_STR_LEN];
        if (!field(line, orig_col, o, sizeof o) ||
            !field(line, zh_col, z, sizeof z))
            continue;
        if (!z[0])
            continue; /* 無譯文 */
        unescape_cr(o);
        unescape_cr(z);
        snprintf(s.orig[s.count], U2_STR_LEN, "%s", o);
        snprintf(s.zh[s.count], U2_STR_LEN, "%s", z);
        s.count++;
    }
    fclose(f);
    return s;
}

const char *u2_strings_lookup(const U2Strings *s, const char *orig)
{
    if (u2_lang == U2_EN) return NULL;   /* 英文:回 NULL → 呼叫端用原文 */
    for (int i = 0; i < s->count; i++)
        if (strcmp(s->orig[i], orig) == 0)
            return s->zh[i];
    return NULL;
}
