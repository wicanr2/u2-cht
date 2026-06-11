/* u2_i18n — 多語系切換(F4)。
 * 引擎硬編訊息:外部字典檔 translations/ui_strings.tsv(欄:zh=key, en, ja, …),
 *   啟動載入,tr(zh) 依 u2_lang 回對應語言(空則 fallback zh)。
 * 資料字串(UI/對話)經 u2_strings_lookup:非 ZH 時回 NULL → 呼叫端 fallback 原文(英文)。
 * 加新語言 = ui_strings.tsv 加一欄 + 填譯文,無需改程式。 */
#ifndef U2_I18N_H
#define U2_I18N_H

typedef enum { U2_ZH=0, U2_EN=1, U2_JA=2 } U2Lang;
extern U2Lang u2_lang;   /* 目前語言 index(定義於 u2_strings.c)*/
extern int u2_nlang;     /* 已載入字典的語言欄數(F4 循環範圍;預設 2=zh/en)*/

#endif
