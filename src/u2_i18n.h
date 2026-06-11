/* u2_i18n — 語系切換(F4)。
 * 資料字串(UI/對話)經 u2_strings_lookup:EN 時回 NULL → 呼叫端 fallback 原文(英文)。
 * 引擎硬編訊息經 game_main.c 的 tr():EN 時查中→英表。 */
#ifndef U2_I18N_H
#define U2_I18N_H

typedef enum { U2_ZH, U2_EN } U2Lang;
extern U2Lang u2_lang;   /* 定義於 u2_strings.c */

#endif
