/* u2_save — player 存檔 (角色記錄) 解析
 *
 * 格式 (oracle 載入碼 + DOSBox 實機建角差分驗證,見 docs/DATA_FORMATS.md):
 *   - player 檔 384 byte;前 **256 byte (0x100) = 角色記錄**,載入時讀進物件。
 *   - 角色記錄 byte 0 != 0 表示「已建角色」(0 = 無角色 / fresh)。
 *   - offset 0x100 處有 `0x1a` 標記(與 monxNN 同;用途未定)。
 *
 * 已驗證欄位 (兩份實機建角樣本 diff,值逐一吻合建角畫面):
 *   | offset      | 欄位        | 編碼                                            |
 *   |-------------|-------------|-------------------------------------------------|
 *   | 0x00..0x0F  | 名字        | ASCII,NUL padding (16 byte)                     |
 *   | 0x10        | 性別        | ASCII 'M' / 'F'                                 |
 *   | 0x11        | 職業 class  | 0-indexed:0=FIGHTER 1=CLERIC 2=WIZARD 3=THIEF   |
 *   | 0x12        | 種族 race   | 0-indexed:0=HUMAN 1=ELF 2=DWARF 3=HOBBIT        |
 *   | 0x15..0x1A  | 六屬性      | **BCD**,順序 STR,AGI,STA,CHA,WIS,INT(含 race/class 加成後值) |
 *   | 0x100       | 標記        | 0x1a                                            |
 *   ⚠️ HP/食物/黃金/裝備等欄位(0x1b 之後的常數區)在兩樣本相同、尚未變動驗證,offset 待補。
 */
#ifndef U2_SAVE_H
#define U2_SAVE_H

#define U2_SAVE_SIZE   384
#define U2_REC_SIZE    256

/* 角色記錄欄位 offset (檔案/rec 共用,皆在前 256 byte 內) */
#define U2_OFF_NAME    0x00
#define U2_NAME_LEN    16
#define U2_OFF_SEX     0x10
#define U2_OFF_CLASS   0x11
#define U2_OFF_RACE    0x12
#define U2_OFF_STATS   0x15   /* 6 個 BCD byte: STR,AGI,STA,CHA,WIS,INT */
#define U2_NUM_STATS   6

typedef struct {
    int ok;                       /* 檔案讀取成功且大小正確 */
    int has_character;            /* rec[0] != 0 = 已建角色 */
    unsigned char rec[U2_REC_SIZE]; /* 256-byte 角色記錄 */
    unsigned char marker;         /* offset 0x100 的標記值 (空檔=0x1a) */

    /* 解析後欄位 (僅 has_character 時有意義) */
    char name[U2_NAME_LEN + 1];   /* NUL 結尾名字 */
    char sex;                     /* 'M' / 'F' */
    int  klass;                   /* 0..3 職業 (見上表) */
    int  race;                    /* 0..3 種族 (見上表) */
    int  stats[U2_NUM_STATS];     /* STR,AGI,STA,CHA,WIS,INT (BCD 解碼後十進位) */
} U2Save;

/* 載入 player 檔。失敗回傳 .ok = 0。 */
U2Save u2_save_load(const char *path);

/* 職業 / 種族 / 屬性的英文名 (index 對應上表;越界回 "?") */
const char *u2_save_class_name(int klass);
const char *u2_save_race_name(int race);
const char *u2_save_stat_name(int i);

/* 繁中對照 (同 index;越界回 "?") */
const char *u2_save_class_zh(int klass);
const char *u2_save_race_zh(int race);
const char *u2_save_stat_zh(int i);

#endif
