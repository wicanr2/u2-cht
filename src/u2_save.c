#include "u2_save.h"
#include <stdio.h>

U2Save u2_save_load(const char *path)
{
    U2Save s = {0};
    FILE *f = fopen(path, "rb");
    if (!f)
        return s;
    unsigned char buf[U2_SAVE_SIZE];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    if (n < U2_SAVE_SIZE)
        return s;

    for (int i = 0; i < U2_REC_SIZE; i++)
        s.rec[i] = buf[i];
    s.marker = buf[0x100];
    s.has_character = (buf[0] != 0);
    s.ok = 1;
    return s;
}
