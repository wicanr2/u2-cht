#include "u2_mon.h"
#include <stdio.h>

U2Mon u2_mon_load(const char *path)
{
    U2Mon m = {0};
    FILE *f = fopen(path, "rb");
    if (!f)
        return m;
    unsigned char buf[384];
    size_t n = fread(buf, 1, sizeof buf, f);
    fclose(f);
    if (n < sizeof buf)
        return m;

    for (int i = 0; i < U2_MON_SLOTS; i++) {
        unsigned char t = buf[0x60 + i];
        if (t == 0)
            continue; /* 空 slot */
        U2Entity *e = &m.ent[m.count++];
        e->x = buf[0x00 + i];
        e->y = buf[0x20 + i];
        e->status = buf[0x40 + i];
        e->tile = t / 4;
        e->dlg = buf[0xA0 + i];
    }
    return m;
}
