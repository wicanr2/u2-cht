/* test_data — data 層斷言 (對真實 DOS 資料 + 翻譯表)。
 * 把逆向發現編碼成可執行 regression 防線。headless,需自備合法 U2 資料。
 *
 * 用法: test_data <ultima2_dir> <translations_dir>
 */
#include <stdio.h>
#include <string.h>
#include "u2_map.h"
#include "u2_mon.h"
#include "u2_play.h"
#include "u2_talk.h"
#include "u2_strings.h"
#include "u2_save.h"

static int fails = 0, total = 0;
#define CHECK(cond, msg) do { \
    total++; \
    if (cond) { printf("  PASS %s\n", msg); } \
    else { printf("  FAIL %s\n", msg); fails++; } \
} while (0)

static void path(char *out, size_t n, const char *dir, const char *file)
{
    snprintf(out, n, "%s/%s", dir, file);
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "用法: %s <ultima2_dir> <translations_dir>\n", argv[0]);
        return 2;
    }
    const char *dd = argv[1], *td = argv[2];
    char p[600];

    printf("[u2_map] mapx21 (城鎮)\n");
    path(p, sizeof p, dd, "mapx21");
    U2Map map = u2_map_load(p);
    CHECK(map.ok, "mapx21 載入成功");
    /* monxNN 交叉驗證得知:id-24 在 (14,41)(16,10)(24,11)(24,37)(41,56)(45,12)(46,12)(56,45) */
    CHECK(u2_map_tile(&map, 14, 41) == 24, "tile(14,41)==24 (byte÷4)");
    CHECK(u2_map_tile(&map, 56, 45) == 24, "tile(56,45)==24");
    CHECK(u2_map_tile(&map, 999, 999) == 0, "越界回傳 0");

    printf("[u2_mon] monx21 實體層\n");
    path(p, sizeof p, dd, "monx21");
    U2Mon mon = u2_mon_load(p);
    CHECK(mon.count == 32, "32 個實體 slot");
    CHECK(mon.ent[0].x == 41 && mon.ent[0].y == 56, "ent0 座標 (41,56)");
    CHECK(mon.ent[0].tile == 24, "ent0 tile==24 (0x60+i ÷4)");
    CHECK(mon.ent[0].status == 0xff, "ent0 status==0xff (固定物)");

    printf("[u2_talk] tlkx21 對話解碼\n");
    path(p, sizeof p, dd, "tlkx21");
    U2Talk talk = u2_talk_load(p);
    CHECK(talk.count >= 4, "至少 4 句對話");
    CHECK(strstr(talk.line[0], "GRENDEL") != NULL, "line0 含 GRENDEL (high-bit 解碼)");

    printf("[u2_strings] 翻譯覆蓋層\n");
    path(p, sizeof p, td, "talk_dialogue.tsv");
    U2Strings tr = u2_strings_load(p, 2, 3);
    CHECK(tr.count > 0, "覆蓋層載入");
    const char *zh = u2_strings_lookup(&tr, talk.line[0]);
    CHECK(zh != NULL, "tlkx21 line0 查到譯文 (原文為 key)");
    if (zh) CHECK(strstr(zh, "格倫德") != NULL || strstr(zh, "謎題") != NULL,
                  "line0 譯文含預期繁中");

    printf("[u2_strings] exe UI 翻譯表 (第二來源)\n");
    path(p, sizeof p, td, "exe_translatable_strings.tsv");
    U2Strings ui = u2_strings_load(p, 2, 3);
    CHECK(ui.count > 0, "exe UI 翻譯表載入");
    const char *hp = u2_strings_lookup(&ui, "H.P.=");
    CHECK(hp && strstr(hp, "生命") != NULL, "H.P.= → 生命 (UI 標籤端到端)");

    printf("[u2_play] 可通行\n");
    CHECK(u2_passable(0) == 0, "海洋(0) 不可通行");
    CHECK(u2_passable(2) == 1, "陸地(2) 可通行");

    printf("[u2_save] player 存檔\n");
    path(p, sizeof p, dd, "player");
    U2Save sv = u2_save_load(p);
    CHECK(sv.ok, "player 載入成功 (384B)");
    CHECK(sv.has_character == 0, "bundled player 無角色 (rec[0]==0, fresh)");
    CHECK(sv.marker == 0x1a, "offset 0x100 標記 == 0x1a");

    /* 實機建角樣本 (DOSBox 差分驗證);fixtures 目錄為選填 argv[3] */
    const char *fx = (argc >= 4) ? argv[3] : "tests/fixtures";
    printf("[u2_save] 建角樣本 HERO (human/fighter/male, all 15)\n");
    path(p, sizeof p, fx, "player_sample_hero");
    U2Save h = u2_save_load(p);
    CHECK(h.ok && h.has_character, "HERO 樣本已建角色");
    CHECK(strcmp(h.name, "HERO") == 0, "name == HERO");
    CHECK(h.sex == 'M', "sex == 'M'");
    CHECK(h.klass == 0 && h.race == 0, "class=FIGHTER(0) race=HUMAN(0)");
    /* 螢幕值:STR35 AGI15 STA15 CHA15 WIS15 INT20 (含加成) */
    CHECK(h.stats[0] == 35 && h.stats[1] == 15 && h.stats[5] == 20,
          "HERO 屬性 BCD: STR35 AGI15 INT20");
    /* 起始 HP/FOOD/EXP/GOLD = 0400/0400/0000/0400 (進遊戲狀態列證實) */
    CHECK(h.hp == 400 && h.food == 400 && h.exp == 0 && h.gold == 400,
          "HERO HP/FOOD/EXP/GOLD = 400/400/0/400 (2-byte BCD)");

    printf("[u2_save] 建角樣本 ABCD (elf/wizard/female, distinct)\n");
    path(p, sizeof p, fx, "player_sample_abcd");
    U2Save a = u2_save_load(p);
    CHECK(a.ok && a.has_character, "ABCD 樣本已建角色");
    CHECK(strcmp(a.name, "ABCD") == 0, "name == ABCD");
    CHECK(a.sex == 'F', "sex == 'F'");
    CHECK(a.klass == 2 && a.race == 1, "class=WIZARD(2) race=ELF(1)");
    /* 螢幕值(含 ELF/WIZARD 加成):STR21 AGI16 STA12 CHA23 WIS14 INT29 */
    CHECK(a.stats[0] == 21 && a.stats[1] == 16 && a.stats[3] == 23 && a.stats[5] == 29,
          "ABCD 屬性 BCD 含加成: STR21 AGI16 CHA23 INT29");

    printf("\n結果: %d/%d 通過%s\n", total - fails, total, fails ? "  *** 有失敗 ***" : "");
    return fails ? 1 : 0;
}
