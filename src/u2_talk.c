#include "u2_talk.h"
#include <stdio.h>
#include <string.h>

static int is_junk(const char *s)
{
    return strstr(s, "PRBYTE") || strstr(s, "PRINT") || strstr(s, "$00");
}

static int has_3upper(const char *s)
{
    int run = 0;
    for (const char *p = s; *p; p++) {
        if (*p >= 'A' && *p <= 'Z') {
            if (++run >= 3)
                return 1;
        } else {
            run = 0;
        }
    }
    return 0;
}

U2Talk u2_talk_load(const char *path)
{
    U2Talk t = {0};
    FILE *f = fopen(path, "rb");
    if (!f)
        return t;
    unsigned char buf[512];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);

    char seg[U2_TALK_LEN];
    size_t si = 0;
    for (size_t i = 0; i <= n && t.count < U2_TALK_MAX; i++) {
        unsigned char b = (i < n) ? (buf[i] & 0x7f) : 0;
        if (b == 0) { /* 段尾 */
            seg[si] = 0;
            /* trim 前導空白 */
            char *s = seg;
            while (*s == ' ')
                s++;
            if (strlen(s) >= 4 && has_3upper(s) && !is_junk(s)) {
                strncpy(t.line[t.count], s, U2_TALK_LEN - 1);
                t.line[t.count][U2_TALK_LEN - 1] = 0;
                t.count++;
            }
            si = 0;
        } else if (si < U2_TALK_LEN - 1) {
            seg[si++] = (char)b;
        }
    }
    return t;
}
