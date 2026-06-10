#include "u2_save.h"
#include <stdio.h>

/* BCD byte -> 十進位 (0x15 -> 15)。非法 nibble 容錯成個位數。 */
static int bcd_to_int(unsigned char b)
{
    return (b >> 4) * 10 + (b & 0x0f);
}

/* 4 位 BCD,高位 byte 在前 (0x04,0x00 -> 400) */
static int bcd4(unsigned char hi, unsigned char lo)
{
    return bcd_to_int(hi) * 100 + bcd_to_int(lo);
}

/* 十進位 0..99 -> BCD byte (15 -> 0x15)。越界 clamp。 */
static unsigned char int_to_bcd(int v)
{
    if (v < 0) v = 0;
    if (v > 99) v = 99;
    return (unsigned char)(((v / 10) << 4) | (v % 10));
}

/* 十進位 0..9999 -> 2-byte BCD (高位在前;dst[0]=百位以上, dst[1]=個十位)。 */
static void int_to_bcd4(int v, unsigned char *dst)
{
    if (v < 0) v = 0;
    if (v > 9999) v = 9999;
    dst[0] = int_to_bcd(v / 100);
    dst[1] = int_to_bcd(v % 100);
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
    for (int i = 0; i < U2_SAVE_SIZE; i++)
        s.raw[i] = buf[i];
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
        s.hp   = bcd4(buf[U2_OFF_HP],   buf[U2_OFF_HP + 1]);
        s.food = bcd4(buf[U2_OFF_FOOD], buf[U2_OFF_FOOD + 1]);
        s.exp  = bcd4(buf[U2_OFF_EXP],  buf[U2_OFF_EXP + 1]);
        s.gold = bcd4(buf[U2_OFF_GOLD], buf[U2_OFF_GOLD + 1]);
    }
    return s;
}

int u2_save_store(const U2Save *s, const char *path)
{
    if (!s || !s->ok || !path)
        return 0;
    unsigned char buf[U2_SAVE_SIZE];
    for (int i = 0; i < U2_SAVE_SIZE; i++)
        buf[i] = s->raw[i];   /* 以原始映像為基底,只覆寫執行時欄位 */

    for (int i = 0; i < U2_NUM_STATS; i++)
        buf[U2_OFF_STATS + i] = int_to_bcd(s->stats[i]);
    int_to_bcd4(s->hp,   &buf[U2_OFF_HP]);
    int_to_bcd4(s->food, &buf[U2_OFF_FOOD]);
    int_to_bcd4(s->exp,  &buf[U2_OFF_EXP]);
    int_to_bcd4(s->gold, &buf[U2_OFF_GOLD]);

    FILE *f = fopen(path, "wb");
    if (!f)
        return 0;
    size_t n = fwrite(buf, 1, sizeof buf, f);
    fclose(f);
    return n == sizeof buf;
}
