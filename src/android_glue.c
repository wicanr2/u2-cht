/* Android 啟動引導:APK assets → 內部儲存解壓 + 合成 argv。
 * 桌面建置不編此檔(Android.mk 才列入)。 */
#ifdef __ANDROID__
#include <SDL.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

/* 內部儲存基底路徑(SDL 提供,通常 /data/data/<pkg>/files)。 */
static char g_base[1024];

/* 確保 path 的各層父目錄存在(path 為「base/相對」完整路徑)。 */
static void ensure_parent_dirs(const char *path)
{
    char tmp[1024];
    snprintf(tmp, sizeof tmp, "%s", path);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    }
}

/* 從 assets 讀 rel(相對路徑;SDL 在 Android 會解析到 APK assets),寫到 base/rel。
 * 回傳 1 成功。已存在且非強制則略過。 */
static int extract_one(const char *rel, int force)
{
    char dst[1024];
    snprintf(dst, sizeof dst, "%s/%s", g_base, rel);
    if (!force) {
        struct stat st;
        if (stat(dst, &st) == 0 && st.st_size > 0) return 1;   /* 已解壓 */
    }
    SDL_RWops *in = SDL_RWFromFile(rel, "rb");                  /* 相對 → APK assets */
    if (!in) { SDL_Log("asset 缺失: %s", rel); return 0; }
    Sint64 sz = SDL_RWsize(in);
    if (sz < 0) sz = 0;
    void *buf = SDL_malloc((size_t)sz);
    if (sz && SDL_RWread(in, buf, 1, (size_t)sz) != (size_t)sz) {
        SDL_RWclose(in); SDL_free(buf); SDL_Log("asset 讀取失敗: %s", rel); return 0;
    }
    SDL_RWclose(in);
    ensure_parent_dirs(dst);
    FILE *out = fopen(dst, "wb");
    if (!out) { SDL_free(buf); SDL_Log("無法寫入: %s", dst); return 0; }
    if (sz) fwrite(buf, 1, (size_t)sz, out);
    fclose(out);
    SDL_free(buf);
    return 1;
}

/* 解壓 assets/filelist.txt 列出的所有檔。回傳成功數。 */
static int extract_all(int force)
{
    SDL_RWops *lst = SDL_RWFromFile("filelist.txt", "rb");
    if (!lst) { SDL_Log("找不到 assets/filelist.txt"); return 0; }
    Sint64 sz = SDL_RWsize(lst);
    char *text = (char *)SDL_malloc((size_t)sz + 1);
    SDL_RWread(lst, text, 1, (size_t)sz);
    text[sz] = 0;
    SDL_RWclose(lst);
    int ok = 0;
    char *save = NULL;
    for (char *line = strtok_r(text, "\r\n", &save); line; line = strtok_r(NULL, "\r\n", &save)) {
        if (!*line) continue;
        if (extract_one(line, force)) ok++;
    }
    SDL_free(text);
    return ok;
}

/* 靜態緩衝:合成的 argv 字串需在 main 期間長存。 */
static char ts_list[2048];
static char p_map[1024], p_font[1024], p_ui[1024], p_player[1024],
            p_title[1024], p_town[2048], p_music[1024], p_sfx[1024];

static void mkpath(char *out, size_t n, const char *rel) { snprintf(out, n, "%s/%s", g_base, rel); }

void android_bootstrap(char **argv, int *argc)
{
    const char *base = SDL_AndroidGetInternalStoragePath();
    snprintf(g_base, sizeof g_base, "%s", base ? base : "/data/local/tmp");

    /* 版本標記:不同版號 → 強制重新解壓(避免舊資料殘留)。 */
    char marker[1024];
    snprintf(marker, sizeof marker, "%s/.assets_v3", g_base);
    struct stat stm;
    int force = (stat(marker, &stm) != 0);
    int n = extract_all(force);
    SDL_Log("Ultima2: 解壓 %d 個 asset(force=%d)到 %s", n, force, g_base);
    if (force) { FILE *m = fopen(marker, "wb"); if (m) { fputc('1', m); fclose(m); } }

    /* tileset 清單(對齊桌面 AppRun 順序:ega 預設 + fmtowns 畫風)。 */
    char te[1024], tf[1024], tv[1024], tc[1024];
    mkpath(te, sizeof te, "tileset/ega.png");
    mkpath(tf, sizeof tf, "tileset/fmtowns.png");
    mkpath(tv, sizeof tv, "tileset/vivid.png");
    mkpath(tc, sizeof tc, "tileset/cga.png");
    snprintf(ts_list, sizeof ts_list, "%s,%s,%s,%s", te, tf, tv, tc);

    mkpath(p_map,    sizeof p_map,    "data/mapx20");
    mkpath(p_font,   sizeof p_font,   "font/wqy-zenhei.ttc");
    mkpath(p_ui,     sizeof p_ui,     "translations/exe_translatable_strings.tsv");
    mkpath(p_player, sizeof p_player, "data/player");
    mkpath(p_title,  sizeof p_title,  "title.png");

    char twn[1024]; mkpath(twn, sizeof twn, "tileset/fmtowns_town.png");
    snprintf(p_town, sizeof p_town, "-,%s,-,-,-,-", twn);
    mkpath(p_music, sizeof p_music, "music");
    mkpath(p_sfx,   sizeof p_sfx,   "sfx");

    int i = 0;
    argv[i++] = (char *)"ultima2";
    argv[i++] = p_map;
    argv[i++] = p_font;
    argv[i++] = ts_list;
    argv[i++] = p_ui;
    argv[i++] = p_player;
    argv[i++] = (char *)"--title";      argv[i++] = p_title;
    argv[i++] = (char *)"--town-tiles"; argv[i++] = p_town;
    argv[i++] = (char *)"--music";      argv[i++] = p_music;
    argv[i++] = (char *)"--sfx";        argv[i++] = p_sfx;
    argv[i] = NULL;
    *argc = i;
}
#endif /* __ANDROID__ */
