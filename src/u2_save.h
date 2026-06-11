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
 *   | 0x1B..0x1C  | H.P.        | **2-byte BCD**(4 位,高位在前);起始 0400        |
 *   | 0x1D..0x1E  | 食物 FOOD   | 2-byte BCD;起始 0400                            |
 *   | 0x1F..0x20  | 經驗 EXP    | 2-byte BCD;起始 0000                            |
 *   | 0x22..0x23  | 黃金 GOLD   | 2-byte BCD;起始 0400(0x21 為 00,用途未定)     |
 *   | 0x100       | 標記        | 0x1a                                            |
 *   證據:還原 HERO 存檔進遊戲,狀態列顯示 H.P.=0400/FOOD=0400/EXP.=0000/GOLD=0400,
 *        對應存檔三個「04 00」+ 一個「00 00」。⚠️ 因起始 HP/FOOD/GOLD 皆 400,
 *        HP/FOOD/GOLD 三者的欄位順序按「顯示序」假設,尚未以差異值逐欄消歧。
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
#define U2_OFF_HP      0x1B   /* 2-byte BCD (順序假設,見上) */
#define U2_OFF_FOOD    0x1D
#define U2_OFF_EXP     0x1F
#define U2_OFF_GOLD    0x22
/* 註:道具旗標(oracle this+0x140…)是執行時物件 offset,非存檔 rec offset;
 * 存檔檔案是否在對應位置存 inventory 尚未驗證 → 暫不持久化 g.items(避免損壞存檔)。 */

typedef struct {
    int ok;                       /* 檔案讀取成功且大小正確 */
    int has_character;            /* rec[0] != 0 = 已建角色 */
    unsigned char rec[U2_REC_SIZE]; /* 256-byte 角色記錄 */
    unsigned char raw[U2_SAVE_SIZE];/* 完整 384-byte 原始映像 (寫回基底,保留未解析欄位) */
    unsigned char marker;         /* offset 0x100 的標記值 (空檔=0x1a) */

    /* 解析後欄位 (僅 has_character 時有意義) */
    char name[U2_NAME_LEN + 1];   /* NUL 結尾名字 */
    char sex;                     /* 'M' / 'F' */
    int  klass;                   /* 0..3 職業 (見上表) */
    int  race;                    /* 0..3 種族 (見上表) */
    int  stats[U2_NUM_STATS];     /* STR,AGI,STA,CHA,WIS,INT (BCD 解碼後十進位) */
    int  hp, food, exp, gold;     /* 4-digit BCD 解碼後十進位 (順序假設,見上) */
} U2Save;

/* 載入 player 檔。失敗回傳 .ok = 0。 */
U2Save u2_save_load(const char *path);

/* 把 s 的執行時欄位 (hp/food/exp/gold/stats) 重新編碼成 BCD 寫回 raw[],
 * 並輸出完整 384-byte 檔到 path。未解析欄位沿用原始映像。
 * 數值會 clamp:4 位欄位 0..9999、屬性 0..99。回傳 1=成功。 */
int u2_save_store(const U2Save *s, const char *path);

/* 職業 / 種族 / 屬性的英文名 (index 對應上表;越界回 "?") */
const char *u2_save_class_name(int klass);
const char *u2_save_race_name(int race);
const char *u2_save_stat_name(int i);

/* 繁中對照 (同 index;越界回 "?") */
const char *u2_save_class_zh(int klass);
const char *u2_save_race_zh(int race);
const char *u2_save_stat_zh(int i);

#endif
