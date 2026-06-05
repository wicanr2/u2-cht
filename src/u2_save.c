#include "u2_save.h"
#include <stdio.h>

/* BCD byte -> 十進位 (0x15 -> 15)。非法 nibble 容錯成個位數。 */
static int bcd_to_int(unsigned char b)
{
    return (b >> 4) * 10 + (b & 0x0f);
}

static const char *CLASS_NAMES[4] = { "FIGHTER", "CLERIC", "WIZARD", "THIEF" };
static const char *RACE_NAMES[4]  = { "HUMAN", "ELF", "DWARF", "HOBBIT" };
static const char *STAT_NAMES[U2_NUM_STATS] =
    { "STR", "AGI", "STA", "CHA", "WIS", "INT" };

const char *u2_save_class_name(int k)
{
    return (k >= 0 && k < 4) ? CLASS_NAMES[k] : "?";
}
const char *u2_save_race_name(int r)
{
    return (r >= 0 && r < 4) ? RACE_NAMES[r] : "?";
}
const char *u2_save_stat_name(int i)
{
    return (i >= 0 && i < U2_NUM_STATS) ? STAT_NAMES[i] : "?";
}

static const char *CLASS_ZH[4] = { "戰士", "牧師", "巫師", "盜賊" };
static const char *RACE_ZH[4]  = { "人類", "精靈", "矮人", "哈比人" };
static const char *STAT_ZH[U2_NUM_STATS] =
    { "力量", "敏捷", "體力", "魅力", "智慧", "智力" };

const char *u2_save_class_zh(int k)
{
    return (k >= 0 && k < 4) ? CLASS_ZH[k] : "?";
}
const char *u2_save_race_zh(int r)
{
    return (r >= 0 && r < 4) ? RACE_ZH[r] : "?";
}
const char *u2_save_stat_zh(int i)
{
    return (i >= 0 && i < U2_NUM_STATS) ? STAT_ZH[i] : "?";
}

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

    if (s.has_character) {
        int j = 0;
        for (int i = 0; i < U2_NAME_LEN; i++) {
            unsigned char c = buf[U2_OFF_NAME + i];
            if (c == 0)
                break;
            s.name[j++] = (char)c;
        }
        s.name[j] = '\0';
        s.sex   = (char)buf[U2_OFF_SEX];
        s.klass = buf[U2_OFF_CLASS];
        s.race  = buf[U2_OFF_RACE];
        for (int i = 0; i < U2_NUM_STATS; i++)
            s.stats[i] = bcd_to_int(buf[U2_OFF_STATS + i]);
    }
    return s;
}
