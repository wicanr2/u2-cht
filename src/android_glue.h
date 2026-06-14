#ifndef U2_ANDROID_GLUE_H
#define U2_ANDROID_GLUE_H
/* Android 專用:把 APK assets 解壓到 app 內部儲存,並合成 main() 的 argv。
 * 桌面建置不編此檔。 */
#ifdef __ANDROID__
/* 解壓 assets(依 assets/filelist.txt)到內部儲存,並填好 *argc/argv(指向內部儲存路徑)。
 * argv 內字串為靜態緩衝,呼叫端可直接覆寫 main 的 argc/argv。 */
void android_bootstrap(char **argv, int *argc);
#endif
#endif
