/* game_main — 整合式 Ultima II 引擎主迴圈(地面 ↔ 地牢 ↔ 角色表)
 *
 * 模式狀態機:
 *   - 地面(WORLD):玩家置中走路;踩到地牢入口(tile id 9)自動進地牢。
 *   - 地牢(DUNGEON):第一人稱線框;N=前進 S=後退 W=左轉 E=右轉 X=離開。
 *   - 角色表:任何模式按 C 疊加繁中角色資料(讀 player 存檔)。
 *
 * 兩種執行:
 *   1. 互動:開 SDL 視窗(方向鍵 / WASD;C 角色表;X 離開地牢;Q/Esc 結束)。
 *   2. headless 腳本:--script <CMDS> <out_prefix>,逐步套用並存 PNG。
 *      CMDS 字元:N/S/E/W(或 w/a/s/d)依模式;C 角色表;X 離地牢;D 強制進地牢。
 *
 * 用法:
 *   u2_game <worldmap> <font.ttf> <tileset.png> <ui_tsv> [player_save] [--script CMDS prefix]
 */
#include <SDL.h>
#include <SDL_image.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include "u2_map.h"
#include "u2_mon.h"
#include "u2_play.h"
#include "u2_render.h"
#include "u2_strings.h"
#include "u2_tileset.h"
#include "u2_text.h"
#include "u2_dungeon.h"
#include "u2_save.h"
#include "u2_talk.h"
#include "u2_i18n.h"

/* 引擎硬編訊息:外部多語字典(translations/ui_strings.tsv;欄 zh=key, en, ja…)。
 * tr(zh) 依 u2_lang 回對應語言;空或查無 fallback zh。加語言只需 TSV 加欄,無需改碼。 */
#define DICT_MAX 512
#define DICT_LANG 4
#define DICT_STR 200
static struct { char t[DICT_LANG][DICT_STR]; } DICT[DICT_MAX];   /* t[0]=zh(key) t[1]=en … */
static int NDICT = 0;
/* 載入字典 TSV(首行 header 決定語言欄數 → u2_nlang)。回 1=成功 */
static int load_ui_dict(const char *path)
{
    FILE *f=fopen(path,"rb"); if(!f) return 0;
    char line[1200]; int row=0, ncol=0;
    while (fgets(line,sizeof line,f)){
        size_t L=strlen(line); while(L&&(line[L-1]=='\n'||line[L-1]=='\r')) line[--L]=0;
        char *cols[DICT_LANG]={0}; int nc=0; char *p=line; cols[nc++]=p;
        for (; *p && nc<DICT_LANG; p++) if(*p=='\t'){ *p=0; cols[nc++]=p+1; }
        if (row++==0){ ncol=nc; u2_nlang=ncol; continue; }       /* header:語言欄數 */
        if (NDICT>=DICT_MAX) break;
        if (!cols[0] || !cols[0][0]) continue;
        for (int c=0;c<DICT_LANG;c++) snprintf(DICT[NDICT].t[c],DICT_STR,"%s",(c<ncol&&cols[c])?cols[c]:"");
        NDICT++;
    }
    fclose(f);
    return NDICT>0;
}
static const char *tr(const char *zh)
{
    if (u2_lang==U2_ZH || (int)u2_lang>=u2_nlang) return zh;
    for (int i=0;i<NDICT;i++) if(!strcmp(DICT[i].t[0],zh))
        return DICT[i].t[u2_lang][0] ? DICT[i].t[u2_lang] : zh;
    return zh;
}
/* 目前語言標籤(切換時顯示)*/
static const char *lang_label(void)
{
    static const char *N[]={"語系:繁體中文","Language: English","言語:日本語","Language"};
    return N[(u2_lang<3)?u2_lang:3];
}

#define CANVAS_W   960
#define CANVAS_H   600
#define HDR_H      30
#define TILE_PX    48
#define VIEW_COLS  19
#define VIEW_ROWS  8
#define MOB_MAX    16   /* 同場怪物上限(oracle 原版 32;受視野 19×8 與可玩性折中取 16)*/
#define MAP_OX     24
#define MAP_OY     (HDR_H + 8)
#define PLAYER_TILE 16
#define WORLD_DUNGEON_TILE 9      /* 地面上的地牢入口 tile id */
#define WORLD_TOWN_TILE 5         /* 地面上的城鎮入口 tile id */
#define DVIEW 520                 /* 地牢線框視區邊長 */

enum Mode { MODE_WORLD, MODE_DUNGEON, MODE_SPACE };

typedef struct {
    enum Mode mode;
    int show_sheet;
    int show_help;                /* F1 指令表疊加 */
    int show_shop;                /* 城鎮商店疊加 */
    int weapon, armour;           /* 裝備等級(0 起;商店升級)*/
    int won;                      /* 已擊敗 Minax(結局)*/
    U2Map map; U2Mon mon;         /* 地面(overworld) */
    U2Player player;
    /* 城鎮 */
    int in_town;
    char loc_kind;                /* 目前 tile-map 地點類型('v'/'t'/'c'),供標題顯示 */
    int dg_tower;                 /* 地牢場景為「塔」(倒置:KLIMB 上=深入) */
    U2Map town; U2Mon tmon; U2Talk talk; int town_ok;
    int tret_x, tret_y;           /* 進城前的地面座標 */
    char town_path[512];
    char data_dir[512];           /* mapxNN 所在目錄(動態組地點檔路徑) */
    char world_num[8];            /* 目前 overworld 編號(如 "20"),供地點登記表查詢 */
    int td_x, td_y;               /* 時間之門座標(overworld;-1=消散中) */
    int td_timer;                 /* 時間之門可見/隱沒週期計數(回合) */
    char town_loaded[8];          /* 目前已載入的城鎮編號(偵測換城重載) */
    /* 地牢 */
    U2Dungeon dg; int dg_ok;
    int dx, dy, ddir, dlevel;     /* 地牢內位置 / 朝向 / 樓層 */
    int ret_x, ret_y;             /* 進地牢前的座標 */
    char dungeon_path[512];
    char dg_loaded[8];            /* 目前已載入的地牢編號(換地牢重載) */
    /* 翻譯 / 存檔 */
    U2Strings ui; U2Strings tr;   /* ui=狀態標籤;tr=對話譯文 */
    U2Save save;
    /* 可切換 tileset(參考 u3-cht 多平台 tileset 清單) */
    SDL_Surface *tset[8]; char tname[8][24]; int ntset, curset;
    /* 地面隨機遭遇 / 戰鬥(U2 overworld 怪物動態生成) */
    struct { int x, y, hp, maxhp, atk; unsigned char tile; const char *name; } mob[MOB_MAX];
    int nmob, php, turn;
    unsigned int rng;             /* 簡易 LCG,determinism 供 headless 驗證 */
    int vehicle;                  /* oracle 0x7390:0 步行 1 馬 2 船 3 飛機 4 火箭 */
    unsigned int items;           /* 物品旗標 bitset(見 ITEM_*) */
    int planet;                   /* 太空中目前軌道行星 index(見 PLANETS) */
    char ret_world[8];            /* 發射前的 overworld 編號(降落回原星用) */
    int spells[9];                /* 各法術持有數(SPELLBOOK 索引;消耗制) */
    int spell_light;              /* 魔法照明剩餘回合(地牢 HUD) */
    int spells_given;             /* 起始法術已依職業發放(一次性) */
    int level;                    /* 等級(自 EXP 結算;不進存檔每次重算) */
    int maxhp;                    /* HP 上限(隨等級提升;取代硬編 400) */
    int sleep_t, arms_t, legs_t;  /* 狀態計時器(oracle 0x73a0/0x7398/0x739c):睡眠/臂麻/腿麻 */
    int show_view;                /* VIEW 鳥瞰疊加(需魔法頭盔) */
    char msg[200];
} Game;
/* 載具(oracle this+0x7390) */
enum { VEH_WALK=0, VEH_HORSE=1, VEH_SHIP=2, VEH_PLANE=3, VEH_ROCKET=4 };
/* 載具地圖 tile id(oracle:HORSE 0x11 / SHIP 0x12 / PLANE 0x13 / ROCKET 0x14) */
#define HORSE_TILE 17
#define SHIP_TILE  18
#define PLANE_TILE 19
#define ROCKET_TILE 20
/* 物品旗標(對應 oracle save offset) */
#define ITEM_ANKH        (1u<<0)   /* 0x140:火箭登艦 */
#define ITEM_SKULL_KEY   (1u<<1)   /* 0x148:飛機登艦 */
#define ITEM_BRASS_BUTTON (1u<<2)  /* 0x150:飛機起飛 */
#define ITEM_BLUE_TASSLE (1u<<3)   /* 0x154:船登艦 */
#define ITEM_TRI_LITHIUM (1u<<4)   /* 0x160:火箭發射燃料 */
#define ITEM_RING        (1u<<5)   /* 戒指:破 Minax 力場(Father Antos 賜)*/
#define ITEM_QUICKSWORD  (1u<<6)   /* ENILNO 迅捷之劍:唯一能殺 Minax */
#define ITEM_BOOTS       (1u<<7)   /* 0x130 魔法長靴:擋腿麻痺(oracle SAVED BY MAGICAL BOOTS)*/
#define ITEM_CLOAK       (1u<<8)   /* 0x134 魔法斗篷:擋臂麻痺 */
#define ITEM_IDOL        (1u<<9)   /* 0x15c 綠色神像:擋睡眠(oracle SAVED BY IDOL)*/
#define ITEM_HELM        (1u<<10)  /* 0x138 魔法頭盔:VIEW 鳥瞰城鎮/星球(oracle)*/

/* 簡易 LCG(同 oracle 風格,seed 固定 → headless 可重現) */
static unsigned int rng_next(Game *g)
{
    g->rng = g->rng * 0x343fd + 0x269ec3;
    return (g->rng >> 16) & 0x7fff;
}

/* 目前作用中的地圖 / 實體層(城鎮 or 地面) */
static U2Map *amap(Game *g) { return g->in_town ? &g->town : &g->map; }
static U2Mon *amon(Game *g) { return g->in_town ? &g->tmon : &g->mon; }

static void clampi(int *v, int lo, int hi) { if (*v<lo)*v=lo; if (*v>hi)*v=hi; }

/* 地點登記表查詢(forward 宣告) */
static const char *loc_dest(const char *world, unsigned char tile);
static const char *kind_name(char k);
static const char *race_nm(int r);
/* 法術(forward 宣告;render_dungeon HUD 用) */
static const char *spell_name(int i);
static int spell_class_ok(const Game *g, int i);
static const char *class_nm(int k);
static const char *stat_nm(int i);

static void find_start(const U2Map *m, U2Player *p, const char *world)
{
    /* 優先:落在城鎮 landmark 旁、且周圍最開闊(陸路鄰格最多)的可通行格;
     * 避免落在水域孤島被困(一開場陸路走不動,只能登船)。*/
    {
        int bx=-1, by=-1, bestopen=-1;
        for (int y=0; y<U2_MAP_H; y++)
            for (int x=0; x<U2_MAP_W; x++){
                if (!loc_dest(world, u2_map_tile(m,x,y))) continue;
                int NX[4]={0,0,1,-1}, NY[4]={1,-1,0,0};
                for (int k=0;k<4;k++){
                    int ax=x+NX[k], ay=y+NY[k];
                    if (ax<0||ay<0||ax>=U2_MAP_W||ay>=U2_MAP_H) continue;
                    unsigned char at=u2_map_tile(m,ax,ay);
                    if (!u2_passable(at) || loc_dest(world, at)) continue;
                    int open=0;                               /* 8 鄰陸路可通行數 = 開闊度 */
                    for (int oy=-1;oy<=1;oy++) for (int ox=-1;ox<=1;ox++){
                        if(!ox&&!oy) continue;
                        int nx=ax+ox, ny=ay+oy;
                        if(nx<0||ny<0||nx>=U2_MAP_W||ny>=U2_MAP_H) continue;
                        if(u2_passable(u2_map_tile(m,nx,ny))) open++;
                    }
                    if (open>bestopen){ bestopen=open; bx=ax; by=ay; }
                }
            }
        if (bx>=0){ p->x=bx; p->y=by; p->tile=PLAYER_TILE; return; }
    }
    /* 後備:地圖中心向外找第一個可通行格 */
    int cx = U2_MAP_W/2, cy = U2_MAP_H/2;
    for (int r=0; r<U2_MAP_W; r++)
        for (int dy=-r; dy<=r; dy++)
            for (int dx=-r; dx<=r; dx++) {
                if (dx>-r&&dx<r&&dy>-r&&dy<r) continue;
                int x=cx+dx, y=cy+dy;
                if (x<0||y<0||x>=U2_MAP_W||y>=U2_MAP_H) continue;
                if (u2_passable(u2_map_tile(m,x,y))) { p->x=x;p->y=y;p->tile=PLAYER_TILE; return; }
            }
    p->x=cx; p->y=cy; p->tile=PLAYER_TILE;
}

/* 地牢某層挑前方可見深度最大的開放格當入口 */
static void dungeon_entry(const U2Dungeon *d, int level, int *ox, int *oy, int *odir)
{
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};
    int best=-1; *ox=*oy=*odir=0;
    for (int y=0; y<U2_DNG_N; y++)
        for (int x=0; x<U2_DNG_N; x++) {
            if (u2_dungeon_is_wall(d,level,x,y)) continue;
            for (int dd=0; dd<4; dd++) {
                int n=0;
                for (int k=1;k<=5;k++){
                    int nx=x+FX[dd]*k, ny=y+FY[dd]*k;
                    if (u2_dungeon_is_wall(d,level,nx,ny)) break;
                    n++;
                }
                if (n>best){best=n;*ox=x;*oy=y;*odir=dd;}
            }
        }
}

/* 狀態列:標籤查翻譯表,數值取自真實 player 存檔(4 位,仿 U2 0400 格式)。
   無角色時數值顯示 ----。 */
static void draw_status_panel(SDL_Surface *cv, U2Text *body, const U2Strings *ui,
                              const U2Save *sv, int x0, int y0)
{
    int has = sv && sv->ok && sv->has_character;
    struct { const char *orig; int val; } st[] = {
        { "H.P.=",     has ? sv->hp   : -1 },
        { "FOOD=",     has ? sv->food : -1 },
        { "EXP.=%.4d", has ? sv->exp  : -1 },
        { "GOLD=%.4d", has ? sv->gold : -1 },
    };
    for (int i=0;i<4;i++){
        const char *zh = ui ? u2_strings_lookup(ui, st[i].orig) : NULL;
        const char *src = zh ? zh : st[i].orig;   /* EN 時回 NULL → 用原文 */
        char label[64]; size_t k=0;
        for(const char*q=src;*q&&*q!='='&&k<sizeof label-1;q++)label[k++]=*q; label[k]=0;
        char val[16]; if (st[i].val>=0) snprintf(val,sizeof val,"%04d",st[i].val);
        else snprintf(val,sizeof val,"----");
        char line[96]; snprintf(line,sizeof line,"%s  %s",label,val);
        u2_text_draw(cv, body, line, x0, y0, 235,225,150); y0+=30;
    }
}

/* 環形世界:把世界座標 (wx,wy) 轉成以玩家為中心的螢幕格 (sx,sy);
 * 回 1 = 在視窗內。delta 以 & 0x3f 環繞取最近距離(-32..31)。 */
static int wrap_screen(int wx, int wy, int px, int py, int *sx, int *sy)
{
    int dx=((wx-px+32)&(U2_WORLD_DIM-1))-32;
    int dy=((wy-py+32)&(U2_WORLD_DIM-1))-32;
    *sx=VIEW_COLS/2+dx; *sy=VIEW_ROWS/2+dy;
    return *sx>=0 && *sx<VIEW_COLS && *sy>=0 && *sy<VIEW_ROWS;
}

static void render_world(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body)
{
    U2Map *m=amap(g); U2Mon *mon=amon(g);
    SDL_Surface *tiles = g->ntset ? g->tset[g->curset] : NULL;
    SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format,0,0,0));
    SDL_Rect hdr={0,0,CANVAS_W,HDR_H};
    SDL_FillRect(cv,&hdr,SDL_MapRGB(cv->format,36,44,110));
    char hdr_t[64];
    if (g->in_town) snprintf(hdr_t,sizeof hdr_t,tr("Ultima II — %s(T 交談 · X 離開)"),kind_name(g->loc_kind));
    else snprintf(hdr_t,sizeof hdr_t,tr("Ultima II:女巫的復仇 — 繁體中文"));
    u2_text_draw(cv,title, hdr_t, 10,4,235,235,245);

    int wrap = !g->in_town;   /* overworld 環形;城鎮不環繞 */
    int cam_x=g->player.x-VIEW_COLS/2, cam_y=g->player.y-VIEW_ROWS/2;
    if (wrap){
        /* 玩家恆置中,tile 以 64×64 環繞 */
        for (int ty=0;ty<VIEW_ROWS;ty++) for (int tx=0;tx<VIEW_COLS;tx++){
            unsigned char id=u2_map_tile_wrap(m,cam_x+tx,cam_y+ty);
            int dx=MAP_OX+tx*TILE_PX, dy=MAP_OY+ty*TILE_PX;
            if (tiles) u2_tileset_blit(cv,tiles,id,dx,dy,TILE_PX);
        }
        /* 地面靜態實體(monxNN)*/
        for (int i=0;i<mon->count;i++){
            int sx,sy; if (!mon->ent[i].tile) continue;
            if (wrap_screen(mon->ent[i].x,mon->ent[i].y,g->player.x,g->player.y,&sx,&sy) && tiles)
                u2_tileset_blit(cv,tiles,mon->ent[i].tile,MAP_OX+sx*TILE_PX,MAP_OY+sy*TILE_PX,TILE_PX);
        }
    } else {
        clampi(&cam_x,0,U2_MAP_W-VIEW_COLS); clampi(&cam_y,0,U2_MAP_H-VIEW_ROWS);
        u2_render_viewport(cv,m,tiles,cam_x,cam_y,VIEW_COLS,VIEW_ROWS,TILE_PX,MAP_OX,MAP_OY);
        u2_render_entities(cv,mon,tiles,cam_x,cam_y,VIEW_COLS,VIEW_ROWS,TILE_PX,MAP_OX,MAP_OY);
    }

    /* 動態怪物(地面遭遇,環繞)*/
    if (wrap && tiles)
        for (int i=0;i<g->nmob;i++){
            int sx,sy;
            if (wrap_screen(g->mob[i].x,g->mob[i].y,g->player.x,g->player.y,&sx,&sy))
                u2_tileset_blit(cv,tiles,g->mob[i].tile,MAP_OX+sx*TILE_PX,MAP_OY+sy*TILE_PX,TILE_PX);
        }

    /* 時間之門標記(青底紫框,踏入即穿越)*/
    if (wrap && g->td_x>=0){
        int sx,sy;
        if (wrap_screen(g->td_x,g->td_y,g->player.x,g->player.y,&sx,&sy)){
            int gx=MAP_OX+sx*TILE_PX, gy=MAP_OY+sy*TILE_PX;
            SDL_Rect in={gx+6,gy+6,TILE_PX-12,TILE_PX-12};
            SDL_FillRect(cv,&in,SDL_MapRGB(cv->format,40,230,230));
            Uint32 fr=SDL_MapRGB(cv->format,230,60,230);
            SDL_Rect e1={gx+4,gy+4,TILE_PX-8,3},e2={gx+4,gy+TILE_PX-7,TILE_PX-8,3},
                     e3={gx+4,gy+4,3,TILE_PX-8},e4={gx+TILE_PX-7,gy+4,3,TILE_PX-8};
            SDL_FillRect(cv,&e1,fr);SDL_FillRect(cv,&e2,fr);SDL_FillRect(cv,&e3,fr);SDL_FillRect(cv,&e4,fr);
        }
    }

    int px,py;
    if (wrap){ px=MAP_OX+(VIEW_COLS/2)*TILE_PX; py=MAP_OY+(VIEW_ROWS/2)*TILE_PX; }
    else { px=MAP_OX+(g->player.x-cam_x)*TILE_PX; py=MAP_OY+(g->player.y-cam_y)*TILE_PX; }
    if (tiles) u2_tileset_blit(cv,tiles,g->player.tile,px,py,TILE_PX);
    Uint32 col=SDL_MapRGB(cv->format,250,240,90);
    SDL_Rect t={px,py,TILE_PX,2},b={px,py+TILE_PX-2,TILE_PX,2},
             l={px,py,2,TILE_PX},rr={px+TILE_PX-2,py,2,TILE_PX};
    SDL_FillRect(cv,&t,col);SDL_FillRect(cv,&b,col);SDL_FillRect(cv,&l,col);SDL_FillRect(cv,&rr,col);

    int by=MAP_OY+VIEW_ROWS*TILE_PX+10;
    u2_text_draw(cv,body, g->in_town ? tr("方向鍵/WASD 移動 · T 交談 · Z 商店 · C 角色表 · X 離開 · F1 指令表")
                                     : tr("方向鍵/WASD 移動 · B 登船 · P 時間門 · G 畫風 · F1 指令表 · Q"),
                 MAP_OX,by,150,175,205);
    u2_text_draw(cv,body,g->msg,MAP_OX,by+30,210,225,205);
    char pos[96]; snprintf(pos,sizeof pos,tr("座標 (%d, %d)  地形 id=%d  圖塊 %s(G 切換)"),
        g->player.x,g->player.y,u2_map_tile(m,g->player.x,g->player.y),
        g->ntset?g->tname[g->curset]:"-");
    u2_text_draw(cv,body,pos,MAP_OX,by+60,150,165,150);
    if (g->save.has_character) g->save.hp = g->php;   /* 顯示運行時生命 */
    draw_status_panel(cv,body,&g->ui,&g->save,640,by);
}

static const char *DIR_ZH[4]={"北 N","東 E","南 S","西 W"};

static void render_dungeon(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body, U2Text *small)
{
    SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format,10,12,18));
    SDL_Rect hdr={0,0,CANVAS_W,HDR_H};
    SDL_FillRect(cv,&hdr,SDL_MapRGB(cv->format,36,44,110));
    u2_text_draw(cv,title, g->dg_tower?tr("塔 — 第一人稱線框"):tr("地牢 — 第一人稱線框"),10,4,235,235,245);

    int depth=u2_dungeon_render(cv,&g->dg,g->dlevel,g->dx,g->dy,g->ddir,24,MAP_OY,DVIEW,DVIEW,g->curset);

    int en=(u2_lang==U2_EN);
    static const char *DIR_EN[4]={"N","E","S","W"};
    int rx=24+DVIEW+24, ry=MAP_OY+6; char ln[96];
    u2_text_draw(cv,body,tr("狀態"),rx,ry,150,175,205); ry+=34;
    snprintf(ln,sizeof ln,"%s (%d, %d)",tr("座標:"),g->dx,g->dy);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=30;
    snprintf(ln,sizeof ln,"%s %s",tr("朝向:"),en?DIR_EN[g->ddir]:DIR_ZH[g->ddir]);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=30;
    snprintf(ln,sizeof ln,tr("樓層: %d / 共 %d 層"),g->dlevel+1,g->dg.levels);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=30;
    snprintf(ln,sizeof ln,"%s %d",tr("前方可見深度:"),depth);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=40;
    draw_status_panel(cv,body,&g->ui,&g->save,rx,ry); ry+=4*30+10;

    /* 法術欄(僅顯示職業可用;1-9 施放)*/
    {
        int any=0;
        for(int i=0;i<9;i++) if(spell_class_ok(g,i)){any=1;break;}
        if(any){
            u2_text_draw(cv,body,tr("法術 (1-9)"),rx,ry,150,175,205); ry+=26;
            for(int i=0;i<9;i++){
                if(!spell_class_ok(g,i)) continue;
                snprintf(ln,sizeof ln,"%d %s x%d",i+1,spell_name(i),g->spells[i]);
                int v = g->spells[i]>0 ? 225:110;
                u2_text_draw(cv,small,ln,rx,ry,v,v,v); ry+=22;
            }
            if(g->spell_light>0){ snprintf(ln,sizeof ln,tr("(照明 %d)"),g->spell_light);
                u2_text_draw(cv,small,ln,rx,ry,200,200,120); ry+=22; }
        }
    }

    int by=MAP_OY+DVIEW+12;
    u2_text_draw(cv,small,tr("N 前進 · S 後退 · W/E 轉向 · J 下樓 · K 上樓 · 1-9 施法 · X 離開"),
        24,by,150,170,200);
    u2_text_draw(cv,small,g->msg,24,by+26,210,225,205);
}

/* F1 指令表疊加面板(置中) */
static void render_help_overlay(SDL_Surface *cv, U2Text *body, U2Text *small)
{
    static const char *ROWS_ZH[] = {
        "方向鍵 / WASD   移動 / 攻擊(朝怪物移動即攻擊)",
        "B               登載 / 下載具(馬/船/飛機/火箭)",
        "Y               火箭發射 / 太空降落  ·  Z 城鎮商店",
        "P / 踏入青紫門   穿越時間之門(切換時代)",
        "走上城堡圖塊     進入城鎮",
        "走上地牢圖塊     進入地牢",
        "T               城鎮中與 NPC 交談",
        "V               鳥瞰地圖(需魔法頭盔) · Y 城鎮中發洩",
        "J / K           地牢中 下樓 / 上樓",
        "1-9 (地牢)      施放法術(光明/穿牆/返地表/擊殺…)",
        "X               離開城鎮 / 地牢",
        "C               角色資料表",
        "G               切換畫風(EGA / FM Towns…)",
        "F4              切換語系(繁中 / English / 日本語)",
        "F1              顯示 / 關閉本指令表",
        "Q / Esc         離開遊戲(自動存檔)",
    };
    static const char *ROWS_EN[] = {
        "Arrows / WASD   Move / attack (move into a monster)",
        "B               Board / disembark (horse/ship/plane/rocket)",
        "Y               Rocket launch / land in space  ·  Z shop",
        "P / step in door  Enter time door (change era)",
        "Step on castle  Enter town",
        "Step on dungeon Enter dungeon",
        "T               Talk to NPC in town",
        "V               Bird's-eye view (needs helm) · Y yell in town",
        "J / K           Descend / ascend in dungeon",
        "1-9 (dungeon)   Cast spell (Light/Passwall/Surface/Kill...)",
        "X               Leave town / dungeon",
        "C               Character sheet",
        "G               Switch theme (EGA / FM Towns...)",
        "F4              Switch language (中 / EN / 日)",
        "F1              Show / hide this list",
        "Q / Esc         Quit game (autosave)",
    };
    const char **ROWS = (u2_lang==U2_EN)?ROWS_EN:ROWS_ZH;
    int nrow = (int)(sizeof ROWS_ZH / sizeof ROWS_ZH[0]);
    int pw=560, ph=70+nrow*30, x=(CANVAS_W-pw)/2, y=(CANVAS_H-ph)/2;
    SDL_Rect bg={x,y,pw,ph}; SDL_FillRect(cv,&bg,SDL_MapRGB(cv->format,18,22,40));
    SDL_Rect bar={x,y,pw,40}; SDL_FillRect(cv,&bar,SDL_MapRGB(cv->format,40,50,120));
    Uint32 fr=SDL_MapRGB(cv->format,120,140,200);
    SDL_Rect e1={x,y,pw,2},e2={x,y+ph-2,pw,2},e3={x,y,2,ph},e4={x+pw-2,y,2,ph};
    SDL_FillRect(cv,&e1,fr);SDL_FillRect(cv,&e2,fr);SDL_FillRect(cv,&e3,fr);SDL_FillRect(cv,&e4,fr);
    u2_text_draw(cv,body,tr("指令表(F1 關閉)"),x+16,y+8,235,235,245);
    int iy=y+52;
    for (int i=0;i<nrow;i++){ u2_text_draw(cv,small,ROWS[i],x+24,iy,215,220,230); iy+=30; }
}

/* ---- 城鎮商店(M3)---- */
static const char *WEAPON_ZH[9]={"匕首","錘矛","斧","弓","劍","巨劍","光劍","相位槍","迅捷之劍"};
static const char *WEAPON_EN[9]={"Dagger","Mace","Ax","Bow","Sword","Greatsword","Light Sword","Phaser","Quicksword"};
static const char *ARMOUR_ZH[6]={"布甲","皮甲","鎖甲","板甲","反射甲","動力甲"};
static const char *ARMOUR_EN[6]={"Cloth","Leather","Chain","Plate","Reflect","Power"};
static const char *weapon_nm(int w){ if(w<0||w>8)return"-"; return (u2_lang==U2_EN)?WEAPON_EN[w]:WEAPON_ZH[w]; }
static const char *armour_nm(int a){ if(a<0||a>5)return"-"; return (u2_lang==U2_EN)?ARMOUR_EN[a]:ARMOUR_ZH[a]; }
/* 商店品項:kind 0=武器升級 1=防具升級 2=食物 3=道具旗標 */
/* ---- 法術書(手冊 MAGIC SPELLS;9 法術,限地牢/塔施放)----
 * 索引對齊手冊清單;cls=可用職業 bitmask(1<<klass):牧師 bit1(0x2)、巫師 bit2(0x4)。
 * 光明/上下梯雙修(0x6);穿牆/返地表/祈禱牧師(0x2);飛彈/瞬移/擊殺巫師(0x4)。
 * 施法消耗一張(手冊:即使失敗也消耗);建角依職業給起始量、商店補充。 */
enum { SP_LIGHT, SP_DDOWN, SP_DUP, SP_PASS, SP_SURF, SP_PRAY, SP_MISSILE, SP_BLINK, SP_KILL };
static const struct { const char *zh, *en; unsigned char cls; } SPELLBOOK[9] = {
    {"光明",     "Light",         0x6},
    {"下梯",     "Ladder Down",   0x6},
    {"上梯",     "Ladder Up",     0x6},
    {"穿牆",     "Passwall",      0x2},
    {"返地表",   "Surface",       0x2},
    {"祈禱",     "Prayer",        0x2},
    {"魔法飛彈", "Magic Missile", 0x4},
    {"瞬移",     "Blink",         0x4},
    {"擊殺",     "Kill",          0x4},
};
static const char *spell_name(int i){ return (u2_lang==U2_EN)?SPELLBOOK[i].en:SPELLBOOK[i].zh; }
static int spell_class_ok(const Game *g, int i){
    int k = g->save.has_character ? g->save.klass : 2;   /* 無角色預設巫師(headless 測試)*/
    return (SPELLBOOK[i].cls & (1u<<k)) != 0;
}
/* 起始法術:依職業給可用法術各 n 張(法術不進存檔,每次啟動重給)。 */
static void grant_starting_spells(Game *g, int n){
    for(int i=0;i<9;i++) if(spell_class_ok(g,i) && g->spells[i]<n) g->spells[i]=n;
}

/* ---- 升級系統(EXP→等級→HP 上限)----
 * U2 等級偏隱晦(原版靠 Lord British 結算 HP);此處用階梯閾值自動結算,讓 EXP 即時有意義。*/
static int level_for_exp(int exp){
    static const int TH[9]={0,20,50,100,200,400,800,1600,3200};  /* 9 級閾值 */
    int lv=1; for(int i=1;i<9;i++) if(exp>=TH[i]) lv=i+1; return lv;
}
static int max_hp_for(int level){ return 400 + (level-1)*50; }   /* 1 級=400(對齊舊預設)*/
static void init_level(Game *g){
    g->level = g->save.has_character ? level_for_exp(g->save.exp) : 1;
    g->maxhp = max_hp_for(g->level);
}
/* 戰鬥/法術得 EXP 後呼叫:結算升級(HP 上限提升 + 回滿 + 提示)。*/
static void check_levelup(Game *g){
    if (!g->save.has_character) return;
    int lv = level_for_exp(g->save.exp);
    if (lv > g->level){
        g->level = lv; g->maxhp = max_hp_for(lv); g->php = g->maxhp;   /* 升級回滿 */
        snprintf(g->msg,sizeof g->msg,
            tr("升級!你已達 %d 級(HP 上限 %d)。"),
            lv,g->maxhp);
    }
}

static const struct { const char *zh,*en; int price,kind,arg; } SHOP[] = {
    {"升級武器","Upgrade weapon",   100,0,0},
    {"升級防具","Upgrade armour",    80,1,0},
    {"食物 +200","Food +200",        50,2,200},
    {"藍流蘇(船)","Blue Tassle (ship)", 60,3,ITEM_BLUE_TASSLE},
    {"骷髏鑰(飛機)","Skull Key (plane)", 80,3,ITEM_SKULL_KEY},
    {"黃銅鈕扣(起飛)","Brass Button",     80,3,ITEM_BRASS_BUTTON},
    {"生命符(火箭)","Ankh (rocket)",   120,3,ITEM_ANKH},
    {"三鋰(燃料)","Tri-Lithium",      200,3,ITEM_TRI_LITHIUM},
    {"獻金給國王(屬性+1)","Tribute to King (stat +1)",150,4,0},
    {"習得法術(可用各+5)","Learn spells (+5 each)",120,5,5},
};
#define NSHOP ((int)(sizeof SHOP/sizeof SHOP[0]))
static void shop_buy(Game *g, int idx)
{
    if (idx<0||idx>=NSHOP) return;
    int en=(u2_lang==U2_EN);
    if (g->save.gold < SHOP[idx].price){ snprintf(g->msg,sizeof g->msg,tr("黃金不足。")); return; }
    switch (SHOP[idx].kind){
        case 0: if(g->weapon>=8){snprintf(g->msg,sizeof g->msg,tr("武器已是最強。"));return;} g->weapon++; break;
        case 1: if(g->armour>=5){snprintf(g->msg,sizeof g->msg,tr("防具已是最強。"));return;} g->armour++; break;
        case 2: g->save.food += SHOP[idx].arg; if(g->save.food>9999)g->save.food=9999; break;
        case 3: g->items |= (unsigned)SHOP[idx].arg; break;
        case 4: { g->save.gold -= SHOP[idx].price;
                  g->php=g->maxhp;   /* 國王為你療傷(回滿生命)*/
                  /* 持有力場之戒者,國王賜予迅捷之劍 ENILNO(任務主鏈)*/
                  if ((g->items&ITEM_RING) && !(g->items&ITEM_QUICKSWORD)){
                      g->items|=ITEM_QUICKSWORD;
                      snprintf(g->msg,sizeof g->msg,tr("國王見你持有力場之戒,賜予迅捷之劍 ENILNO!")); return;
                  }
                  int lo=0; for(int i=1;i<U2_NUM_STATS;i++) if(g->save.stats[i]<g->save.stats[lo])lo=i;
                  if(g->save.stats[lo]<99) g->save.stats[lo]++;
                  snprintf(g->msg,sizeof g->msg,tr("國王為你療傷,並提升了你的%s。"),
                           u2_lang==U2_EN?u2_save_stat_name(lo):u2_save_stat_zh(lo)); return; }
        case 5: {  /* 習得法術:職業可用法術各 +arg(不會魔法則不扣錢)*/
                  int k=g->save.has_character?g->save.klass:2, got=0;
                  for(int i=0;i<9;i++) if(SPELLBOOK[i].cls&(1u<<k)){ g->spells[i]+=SHOP[idx].arg; got++; }
                  if(!got){ snprintf(g->msg,sizeof g->msg,tr("你的職業不會魔法。")); return; }
                  g->save.gold -= SHOP[idx].price;
                  snprintf(g->msg,sizeof g->msg,tr("習得 %d 種法術(各+%d)。"),got,SHOP[idx].arg); return; }
    }
    g->save.gold -= SHOP[idx].price;
    snprintf(g->msg,sizeof g->msg,tr("買了:%s"),en?SHOP[idx].en:SHOP[idx].zh);
}
static void render_shop_overlay(SDL_Surface *cv, Game *g, U2Text *body, U2Text *small)
{
    int en=(u2_lang==U2_EN);
    int pw=520, ph=120+NSHOP*30, x=(CANVAS_W-pw)/2, y=(CANVAS_H-ph)/2;
    SDL_Rect bg={x,y,pw,ph}; SDL_FillRect(cv,&bg,SDL_MapRGB(cv->format,18,22,40));
    SDL_Rect bar={x,y,pw,40}; SDL_FillRect(cv,&bar,SDL_MapRGB(cv->format,40,50,120));
    Uint32 fr=SDL_MapRGB(cv->format,120,140,200);
    SDL_Rect e1={x,y,pw,2},e2={x,y+ph-2,pw,2},e3={x,y,2,ph},e4={x+pw-2,y,2,ph};
    SDL_FillRect(cv,&e1,fr);SDL_FillRect(cv,&e2,fr);SDL_FillRect(cv,&e3,fr);SDL_FillRect(cv,&e4,fr);
    char ln[96];
    snprintf(ln,sizeof ln,tr("商店 — 黃金 %d (按 1-9 購買,Z 關閉)"),g->save.gold);
    u2_text_draw(cv,body,ln,x+16,y+8,235,235,245);
    int iy=y+52;
    for (int i=0;i<NSHOP;i++){
        snprintf(ln,sizeof ln,"%d. %s  —  %d G",i+1,en?SHOP[i].en:SHOP[i].zh,SHOP[i].price);
        u2_text_draw(cv,small,ln,x+24,iy,215,220,230); iy+=30;
    }
    snprintf(ln,sizeof ln,tr("武器:%s   防具:%s"),weapon_nm(g->weapon),armour_nm(g->armour));
    u2_text_draw(cv,small,ln,x+24,iy+6,180,200,160);
}

/* 目前任務目標(依持有道具推進) */
static const char *quest_hint(const Game *g)
{
    if (g->won) return tr("任務:已完成 ── 你拯救了宇宙!");
    if (!(g->items & ITEM_RING)) return tr("任務:尋找安托斯神父(盤古大陸的城堡)取得力場之戒。");
    if (!(g->items & ITEM_QUICKSWORD)) return tr("任務:帶著戒指去見國王,取得迅捷之劍 ENILNO。");
    return tr("任務:前往傳說時代,於巢穴擊敗米娜克斯!");
}

/* 角色表疊加面板(置中) */
static void render_sheet_overlay(SDL_Surface *cv, Game *g, U2Text *body, U2Text *small)
{
    int pw=560, ph=485, x=(CANVAS_W-pw)/2, y=(CANVAS_H-ph)/2;
    SDL_Rect bg={x,y,pw,ph}; SDL_FillRect(cv,&bg,SDL_MapRGB(cv->format,18,22,40));
    SDL_Rect bar={x,y,pw,40}; SDL_FillRect(cv,&bar,SDL_MapRGB(cv->format,40,50,120));
    /* 邊框 */
    Uint32 fr=SDL_MapRGB(cv->format,120,140,200);
    SDL_Rect e1={x,y,pw,2},e2={x,y+ph-2,pw,2},e3={x,y,2,ph},e4={x+pw-2,y,2,ph};
    SDL_FillRect(cv,&e1,fr);SDL_FillRect(cv,&e2,fr);SDL_FillRect(cv,&e3,fr);SDL_FillRect(cv,&e4,fr);

    int en=(u2_lang==U2_EN);
    u2_text_draw(cv,body,tr("角色資料(C 關閉)"),x+16,y+8,235,235,245);
    U2Save *s=&g->save; int ix=x+28, iy=y+56, lh=30; char ln[96];
    if (!s->ok || !s->has_character){
        u2_text_draw(cv,body,tr("(無 player 存檔可顯示)"),ix,iy,200,180,120);
        return;
    }
    snprintf(ln,sizeof ln,"%s %s",tr("姓名:"),s->name); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh;
    snprintf(ln,sizeof ln,"%s %s",tr("性別:"),s->sex=='F'?tr("女"):tr("男")); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh;
    snprintf(ln,sizeof ln,"%s %s",tr("種族:"),race_nm(s->race)); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh;
    snprintf(ln,sizeof ln,"%s %s",tr("職業:"),class_nm(s->klass)); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh;
    snprintf(ln,sizeof ln,tr("等級:%d   HP 上限:%d"),g->level,g->maxhp);
    u2_text_draw(cv,body,ln,ix,iy,200,225,190); iy+=lh;
    snprintf(ln,sizeof ln,tr("武器:%s  防具:%s"),weapon_nm(g->weapon),armour_nm(g->armour));
    u2_text_draw(cv,small,ln,ix,iy,200,205,165); iy+=lh-6;
    { char it[200]=""; struct{unsigned f;const char*z,*e;}IT[]={{ITEM_BLUE_TASSLE,"藍流蘇","Tassle"},
        {ITEM_SKULL_KEY,"骷髏鑰","SkullKey"},{ITEM_BRASS_BUTTON,"黃銅鈕扣","Brass"},
        {ITEM_ANKH,"生命符","Ankh"},{ITEM_TRI_LITHIUM,"三鋰","TriLith"},
        {ITEM_RING,"力場之戒","Ring"},{ITEM_QUICKSWORD,"迅捷之劍","Enilno"},
        {ITEM_BOOTS,"魔法長靴","Boots"},{ITEM_CLOAK,"魔法斗篷","Cloak"},{ITEM_IDOL,"綠色神像","Idol"},
        {ITEM_HELM,"魔法頭盔","Helm"}};
      for(int i=0;i<11;i++) if(g->items&IT[i].f){ if(it[0])strcat(it," ·"); strcat(it," "); strcat(it,en?IT[i].e:IT[i].z); }
      snprintf(ln,sizeof ln,tr("道具:%s"),it[0]?it:tr(" 無"));
      u2_text_draw(cv,small,ln,ix,iy,180,195,160); iy+=lh+2; }
    u2_text_draw(cv,small,tr("屬性(BCD 解碼)"),ix,iy,150,170,205); iy+=26;
    for (int i=0;i<U2_NUM_STATS;i++){
        char lab[64]; snprintf(lab,sizeof lab,en?"%s":"%s %s",stat_nm(i),u2_save_stat_name(i));
        u2_text_draw(cv,body,lab,ix,iy,210,215,225);
        char v[8]; snprintf(v,sizeof v,"%2d",s->stats[i]);
        u2_text_draw(cv,body,v,ix+170,iy,245,225,150); iy+=lh;
    }
    iy+=4; u2_text_draw(cv,small,quest_hint(g),ix,iy,250,220,140);   /* 任務目標 */
}

static void render_space(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body, U2Text *small);
static void render_ending(SDL_Surface *cv, U2Text *title, U2Text *body);
static void revive_if_dead(Game *g);
/* VIEW 鳥瞰(手冊 V)ew):縮略整張當前地圖(城鎮/overworld),玩家位置標紅。需魔法頭盔。*/
static void render_view_overlay(SDL_Surface *cv, Game *g, U2Text *body)
{
    U2Map *m = amap(g);
    int dim=64, cell=7, gw=dim*cell, gh=dim*cell;
    int ox=(CANVAS_W-gw)/2, oy=(CANVAS_H-gh)/2 + 10;
    SDL_Rect bg={ox-10,oy-10,gw+20,gh+20};
    SDL_FillRect(cv,&bg,SDL_MapRGB(cv->format,12,14,24));
    Uint32 cwater=SDL_MapRGB(cv->format,40,60,150), cland=SDL_MapRGB(cv->format,40,110,55);
    Uint32 cmark =SDL_MapRGB(cv->format,230,200,90), cwall=SDL_MapRGB(cv->format,95,72,72);
    for(int y=0;y<dim;y++) for(int x=0;x<dim;x++){
        unsigned char t=u2_map_tile(m,x,y);
        Uint32 col = (!g->in_town && loc_dest(g->world_num,t)) ? cmark
                   : u2_passable(t) ? cland : (t==0 ? cwater : cwall);
        SDL_Rect r={ox+x*cell, oy+y*cell, cell-1, cell-1};
        SDL_FillRect(cv,&r,col);
    }
    int px=g->player.x, py=g->player.y;
    if(px>=0&&px<dim&&py>=0&&py<dim){
        SDL_Rect pr={ox+px*cell-1, oy+py*cell-1, cell+1, cell+1};
        SDL_FillRect(cv,&pr,SDL_MapRGB(cv->format,240,60,60));
    }
    u2_text_draw(cv,body,tr("鳥瞰 VIEW(V 關閉)"),ox,oy-40,235,230,200);
}

static void render_all(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body, U2Text *small)
{
    if (g->won){ render_ending(cv,title,body); return; }   /* 結局蓋過一切 */
    if (!g->spells_given){ g->spells_given=1; grant_starting_spells(g,8); init_level(g); }  /* 起始法術+等級 */
    revive_if_dead(g);                                     /* HP 歸零 → 復活 */
    if (g->mode==MODE_SPACE)      render_space(cv,g,title,body,small);
    else if (g->mode==MODE_WORLD) render_world(cv,g,title,body);
    else                          render_dungeon(cv,g,title,body,small);
    if (g->show_view)        render_view_overlay(cv,g,body);
    if (g->show_sheet)       render_sheet_overlay(cv,g,body,small);
    if (g->show_help)        render_help_overlay(cv,body,small);
    if (g->show_shop)        render_shop_overlay(cv,g,body,small);
}

/* 進地牢:載入地牢檔,設定入口 */
/* 進地牢/塔:依登記表 dest 載入對應地牢圖(per-location;換地牢重載) */
static void enter_dungeon_at(Game *g, const char *num, int tower)
{
    if (!num) num = "15";
    if (!g->dg_ok || strcmp(g->dg_loaded, num) != 0){
        snprintf(g->dungeon_path,sizeof g->dungeon_path,"%s/mapx%s",g->data_dir,num);
        g->dg = u2_dungeon_load(g->dungeon_path);
        g->dg_ok = g->dg.ok;
        if (g->dg_ok) snprintf(g->dg_loaded,sizeof g->dg_loaded,"%s",num);
    }
    if (!g->dg_ok){ snprintf(g->msg,sizeof g->msg,tr("找不到地牢資料,無法進入。")); return; }
    g->dg_tower=tower;
    g->ret_x=g->player.x; g->ret_y=g->player.y;
    g->dlevel=0;
    dungeon_entry(&g->dg,g->dlevel,&g->dx,&g->dy,&g->ddir);
    g->mode=MODE_DUNGEON;
    snprintf(g->msg,sizeof g->msg, tower?tr("你踏入了高聳的塔…"):tr("你踏入了黑暗的地牢…"));
}
static void enter_dungeon(Game *g){ enter_dungeon_at(g,"15",0); }

/* 下樓:站在下梯(&0x20)才生效 */
static void dungeon_descend(Game *g)
{
    if (g->mode!=MODE_DUNGEON) return;
    if (u2_dungeon_ladder(&g->dg,g->dlevel,g->dx,g->dy)!=+1){
        snprintf(g->msg,sizeof g->msg,tr("腳下沒有向下的樓梯。")); return; }
    if (g->dlevel+1>=g->dg.levels){ snprintf(g->msg,sizeof g->msg,tr("已是最底層。")); return; }
    g->dlevel++;
    if (u2_dungeon_is_wall(&g->dg,g->dlevel,g->dx,g->dy))
        dungeon_entry(&g->dg,g->dlevel,&g->dx,&g->dy,&g->ddir);
    snprintf(g->msg,sizeof g->msg,tr("你沿樓梯往下,來到第 %d 層。"),g->dlevel+1);
}

/* 上樓:站在上梯(&0x10)才生效;最頂層再上則離開地牢 */
static void dungeon_ascend(Game *g)
{
    if (g->mode!=MODE_DUNGEON) return;
    if (u2_dungeon_ladder(&g->dg,g->dlevel,g->dx,g->dy)!=-1){
        snprintf(g->msg,sizeof g->msg,tr("腳下沒有向上的樓梯。")); return; }
    if (g->dlevel==0){
        g->mode=MODE_WORLD; g->player.x=g->ret_x; g->player.y=g->ret_y;
        snprintf(g->msg,sizeof g->msg,tr("你沿樓梯回到了地面。")); return;
    }
    g->dlevel--;
    if (u2_dungeon_is_wall(&g->dg,g->dlevel,g->dx,g->dy))
        dungeon_entry(&g->dg,g->dlevel,&g->dx,&g->dy,&g->ddir);
    snprintf(g->msg,sizeof g->msg,tr("你沿樓梯往上,來到第 %d 層。"),g->dlevel+1);
}

static void exit_dungeon(Game *g)
{
    g->mode=MODE_WORLD;
    g->player.x=g->ret_x; g->player.y=g->ret_y;
    snprintf(g->msg,sizeof g->msg,tr("你回到了地面。"));
}

/* 地點登記表(world 編號 + landmark tile → 目的地 + 類型)。
 * world-aware:不同世代 overworld 的 landmark 對到該世代的地點。
 * provisional:精確對應/類型待 oracle/Codex 校正(見 docs/MAP_REGISTRY.md)。
 * kind(oracle ENTER 類型):'v'村莊 't'城鎮 'c'城堡 'T'塔(倒置地牢)'d'地牢。 */
typedef struct { const char *world; unsigned char tile; const char *dest; char kind; } LocReg;
static const LocReg LOC_REG[] = {
    /* 5 個地球時代 overworld(oracle 首位數字定);landmark → 地點(kind/dest 為 provisional)*/
    {"00",8,"03",'t'},{"00",10,"92",'c'},{"00",9,"15",'d'},                 /* Legends */
    {"10",5,"11",'v'},{"10",10,"93",'c'},{"10",9,"15",'d'},                 /* 9,000,000 B.C. */
    {"20",5,"21",'t'},{"20",6,"22",'v'},{"20",7,"23",'c'},{"20",8,"31",'t'},{"20",10,"32",'t'},{"20",9,"15",'d'}, /* 1423 B.C. */
    {"30",5,"33",'t'},{"30",6,"41",'v'},{"30",7,"61",'c'},{"30",8,"15",'T'},{"30",10,"81",'t'},{"30",9,"15",'d'}, /* 1990 A.D.(8=塔示範,用 15 地牢格式)*/
    {"40",5,"82",'t'},{"40",10,"61",'c'},{"40",9,"15",'d'},                 /* 2112 A.D. */
};
static const LocReg *loc_lookup(const char *world, unsigned char tile)
{
    for (size_t i=0;i<sizeof LOC_REG/sizeof LOC_REG[0];i++)
        if (!strcmp(LOC_REG[i].world,world) && LOC_REG[i].tile==tile)
            return &LOC_REG[i];
    return NULL;
}
/* 進入用地圖編號(村莊/城鎮/城堡;地牢/塔走 dungeon)。回 NULL = 非 tile-map 地點。 */
static const char *loc_dest(const char *world, unsigned char tile)
{
    const LocReg *L=loc_lookup(world,tile);
    return (L && (L->kind=='v'||L->kind=='t'||L->kind=='c')) ? L->dest : NULL;
}
/* 種族/職業/屬性名:lang-aware(EN 用 u2_save_*_name,ZH 用 *_zh) */
static const char *race_nm(int r){ return u2_lang==U2_EN?u2_save_race_name(r):u2_save_race_zh(r); }
static const char *class_nm(int k){ return u2_lang==U2_EN?u2_save_class_name(k):u2_save_class_zh(k); }
static const char *stat_nm(int i){ return u2_lang==U2_EN?u2_save_stat_name(i):u2_save_stat_zh(i); }

static const char *kind_name(char k)
{
    if (u2_lang==U2_EN)
        switch (k){ case 'v':return "Village"; case 't':return "Town"; case 'c':return "Castle";
                    case 'T':return "Tower"; case 'd':return "Dungeon"; default:return "Place"; }
    switch (k){ case 'v':return "村莊"; case 't':return "城鎮"; case 'c':return "城堡";
                case 'T':return "塔"; case 'd':return "地牢"; }
    return "地點";
}

/* 進城:依世界圖 landmark tile 載入對應城鎮地圖 + 實體 + 對話 */
static void enter_town_tile(Game *g, unsigned char wtile)
{
    const LocReg *L = loc_lookup(g->world_num, wtile);
    g->loc_kind = L ? L->kind : 't';
    const char *num = loc_dest(g->world_num, wtile);
    if (!num) num = "21";   /* 後備:'O' 強制進城或未知 landmark */
    /* 換城(或首次)→ 重載 */
    if (!g->town_ok || strcmp(g->town_loaded, num) != 0){
        snprintf(g->town_path,sizeof g->town_path,"%s/mapx%s",g->data_dir,num);
        g->town = u2_map_load(g->town_path);
        char mp[512]; snprintf(mp,sizeof mp,"%s/monx%s",g->data_dir,num);
        g->tmon = u2_mon_load(mp);
        char tp[512]; snprintf(tp,sizeof tp,"%s/tlkx%s",g->data_dir,num);
        g->talk = u2_talk_load(tp);
        g->town_ok = g->town.ok;
        if (g->town_ok) snprintf(g->town_loaded,sizeof g->town_loaded,"%s",num);
    }
    if (!g->town_ok){ snprintf(g->msg,sizeof g->msg,tr("找不到城鎮資料,無法進入。")); return; }
    g->tret_x=g->player.x; g->tret_y=g->player.y;
    /* 落點:站在第一個「可交談」NPC 旁的可通行格;否則實體群中心 */
    int sx=U2_MAP_W/2, sy=U2_MAP_H/2, placed=0;
    for (int i=0;i<g->tmon.count && !placed;i++){
        U2Entity *e=&g->tmon.ent[i];
        if (!e->tile || !(e->dlg & 0x80)) continue;
        int NX[4]={0,0,1,-1}, NY[4]={1,-1,0,0};
        for (int k=0;k<4;k++){
            int x=e->x+NX[k], y=e->y+NY[k];
            if (x<0||y<0||x>=U2_MAP_W||y>=U2_MAP_H) continue;
            if (u2_passable(u2_map_tile(&g->town,x,y))){ sx=x; sy=y; placed=1; break; }
        }
    }
    g->player.x=sx; g->player.y=sy;
    g->in_town=1;
    snprintf(g->msg,sizeof g->msg,tr("你進入了%s。"),kind_name(g->loc_kind));
}

static void exit_town(Game *g)
{
    g->in_town=0;
    g->player.x=g->tret_x; g->player.y=g->tret_y;
    snprintf(g->msg,sizeof g->msg,tr("你離開了城鎮。"));
}

/* VIEW 鳥瞰(手冊 V)ew):需魔法頭盔;地牢/塔中無效。 */
static void do_view(Game *g)
{
    if (g->mode==MODE_DUNGEON){ snprintf(g->msg,sizeof g->msg,tr("鳥瞰在地牢/塔中無效。")); return; }
    if (!(g->items & ITEM_HELM)){ snprintf(g->msg,sizeof g->msg,tr("你需要魔法頭盔才能鳥瞰四周。")); return; }
    g->show_view = !g->show_view;
    if (g->show_view) snprintf(g->msg,sizeof g->msg,tr("你戴上魔法頭盔,俯瞰四周。"));
}
/* YELL 發洩(手冊 Y)ell):純情緒宣洩,不影響遊戲(原版梗)。 */
static void do_yell(Game *g)
{
    snprintf(g->msg,sizeof g->msg,tr("你放聲大喊,發洩了情緒……但什麼也沒改變。"));
}

/* 交談:鄰格若有 NPC 實體,顯示其 tlkx 對話(查翻譯覆蓋層) */
static void do_talk(Game *g)
{
    if (!g->in_town){ snprintf(g->msg,sizeof g->msg,tr("這裡沒有人可以交談。")); return; }
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};
    for (int d=0; d<4; d++){
        int nx=g->player.x+FX[d], ny=g->player.y+FY[d];
        for (int i=0;i<g->tmon.count;i++){
            U2Entity *e=&g->tmon.ent[i];
            if (!e->tile || e->x!=nx || e->y!=ny) continue;
            /* dlg & 0x80 = 可交談;行索引 = (dlg&0x7f)-1 (1-based 進 tlkx) */
            if (!(e->dlg & 0x80)){ snprintf(g->msg,sizeof g->msg,tr("對方沉默不語。")); return; }
            int k=(e->dlg & 0x7f) - 1;
            if (k<0 || k>=g->talk.count){ snprintf(g->msg,sizeof g->msg,tr("對方欲言又止。")); return; }
            /* Father Antos(mapx93)賜力場之戒(任務主鏈)*/
            if (!strcmp(g->town_loaded,"93") && !(g->items & ITEM_RING)){
                g->items |= ITEM_RING;
                snprintf(g->msg,sizeof g->msg,tr("安托斯神父祝福你,賜予了力場之戒。")); return;
            }
            const char *zh=u2_strings_lookup(&g->tr, g->talk.line[k]);
            const char *disp=zh?zh:g->talk.line[k];
            char one[180]; size_t j=0;
            for (const char *p=disp; *p && j<sizeof one-1; p++) one[j++]=(*p=='\r')?' ':*p;
            one[j]=0;
            snprintf(g->msg,sizeof g->msg,"「%s」",one);
            return;
        }
    }
    snprintf(g->msg,sizeof g->msg,tr("附近沒有人可以交談。"));
}

/* ---- 地面隨機遭遇 / 戰鬥(oracle 公式) ---- */
/* 怪物型別:tile → 中文名 / HP(bestiary)/ 攻擊力 */
static void mob_type(unsigned char tile, const char **name, int *hp, int *atk)
{
    switch (tile) {
        case 12: *name="蜥蜴人"; *hp=16;  *atk=4;  break;  /* Orc 級 */
        case 13: *name="幽靈";   *hp=49;  *atk=6;  break;  /* Ghost */
        case 14: *name="魔鬼";   *hp=64;  *atk=8;  break;  /* Devil(縮放) */
        case 15: *name="炎魔";   *hp=80;  *atk=10; break;  /* Balron(縮放) */
        case 60: *name="哥布林"; *hp=12;  *atk=3;  break;
        case 61: *name="盜賊";   *hp=32;  *atk=5;  break;
        case 62: *name="惡魔";   *hp=64;  *atk=7;  break;
        case 63: *name="海蛇";   *hp=48;  *atk=6;  break;
        default: *name="怪物";   *hp=20;  *atk=4;  break;
    }
}
/* 玩家命中技能(oracle:rng%0x50 >= skill → MISS;cap 0x50=80)。以 AGI 估。 */
static int hit_skill(Game *g)
{
    int agi = g->save.has_character ? g->save.stats[1] : 20;
    int s = 44 + agi; if (s>0x50) s=0x50; return s;   /* ~64-71 → 命中率 ~80-89% */
}
/* 玩家近戰傷害(oracle 地面:(力量 + 武器*8)>>2;手搏=武器0,加亂數使可玩) */
static int player_dmg(Game *g)
{
    int str = g->save.has_character ? g->save.stats[0] : 20;
    int wpn = g->weapon + 1;   /* 1..9(oracle 武器序號)*/
    return ((str + 8*wpn) >> 2) + (rng_next(g) % 8) + 4;  /* 武器越高傷害越高 */
}
/* 視野邊緣可通行格生成怪物(~28%/回合) */
static void spawn_mob(Game *g)
{
    if (g->nmob >= MOB_MAX || g->in_town || g->mode != MODE_WORLD) return;
    if (rng_next(g) % 8 >= 7) return;   /* oracle FUN_0040c350:每次移動 7/8≈87.5% 嘗試 spawn */
    static const unsigned char mt[8] = {12,13,14,15,60,61,62,63};
    for (int t = 0; t < 8; t++) {
        int dx = (rng_next(g)%VIEW_COLS) - VIEW_COLS/2;
        int dy = (rng_next(g)%VIEW_ROWS) - VIEW_ROWS/2;
        int x = g->player.x+dx, y = g->player.y+dy;
        if (x<0||y<0||x>=U2_MAP_W||y>=U2_MAP_H) continue;
        if ((x==g->player.x&&y==g->player.y) || !u2_passable(u2_map_tile(&g->map,x,y))) continue;
        int occ=0; for(int i=0;i<g->nmob;i++) if(g->mob[i].x==x&&g->mob[i].y==y) occ=1;
        if (occ) continue;
        int m=g->nmob++;
        unsigned char tile=mt[rng_next(g)%8];
        const char *nm; int hp,atk; mob_type(tile,&nm,&hp,&atk);
        g->mob[m].x=x; g->mob[m].y=y; g->mob[m].tile=tile;
        g->mob[m].hp=g->mob[m].maxhp=hp; g->mob[m].atk=atk; g->mob[m].name=nm;
        snprintf(g->msg,sizeof g->msg,tr("%s出現了!"),tr(nm));
        return;
    }
}
/* 怪物朝玩家移動;貼身則以各自攻擊力打玩家 */
/* 怪物特殊攻擊(oracle FUN_0040c610):遠程狀態(~12.5%)+ 偷竊(~12.5%),防護道具減免。*/
static void apply_status_attack(Game *g)
{
    unsigned int sr = rng_next(g) & 0xff;
    int en=(u2_lang==U2_EN); (void)en;
    if (sr < 0x20){                            /* 遠程狀態攻擊:腿麻 / 臂麻 / 睡眠 */
        int kind = sr % 3;
        if (kind==0){
            if(g->items&ITEM_BOOTS) snprintf(g->msg,sizeof g->msg,tr("魔法長靴擋下了腿麻!"));
            else { g->legs_t=(rng_next(g)&7)+2; snprintf(g->msg,sizeof g->msg,tr("你的雙腿被麻痺了!")); }
        } else if (kind==1){
            if(g->items&ITEM_CLOAK) snprintf(g->msg,sizeof g->msg,tr("魔法斗篷擋下了臂麻!"));
            else { g->arms_t=(rng_next(g)&7)+2; snprintf(g->msg,sizeof g->msg,tr("你的手臂被麻痺了!")); }
        } else {
            if(g->items&ITEM_IDOL) snprintf(g->msg,sizeof g->msg,tr("綠色神像擋下了睡眠!"));
            else { g->sleep_t=(rng_next(g)&7)+2; snprintf(g->msg,sizeof g->msg,tr("你陷入了沉睡!")); }
        }
    } else if (sr < 0x40){                      /* 偷竊:哥布林偷食物 / 盜賊偷金 */
        if (sr&1){ int f=50+(rng_next(g)%50); g->save.food-=f; if(g->save.food<0)g->save.food=0;
                   snprintf(g->msg,sizeof g->msg,tr("哥布林偷走了 %d 份食物!"),f); }
        else { int gd=10+(rng_next(g)%40); g->save.gold-=gd; if(g->save.gold<0)g->save.gold=0;
               snprintf(g->msg,sizeof g->msg,tr("盜賊偷走了 %d 金幣!"),gd); }
    }
}
static void step_mobs(Game *g)
{
    for (int i=0;i<g->nmob;i++){
        int dx=g->player.x-g->mob[i].x, dy=g->player.y-g->mob[i].y;
        if (abs(dx)+abs(dy)==1){
            int dmg=g->mob[i].atk + (rng_next(g)%4) - g->armour*2;  /* 防具減傷 */
            if (dmg<1) dmg=1;
            g->php-=dmg; if(g->php<0)g->php=0;
            snprintf(g->msg,sizeof g->msg,tr("%s攻擊你!失去 %d 點生命。"),tr(g->mob[i].name),dmg);
            apply_status_attack(g);            /* oracle:遠程狀態 / 偷竊 */
            continue;
        }
        int sx=dx>0?1:dx<0?-1:0, sy=dy>0?1:dy<0?-1:0;
        int nx=g->mob[i].x, ny=g->mob[i].y;
        if (abs(dx)>=abs(dy) && sx) nx+=sx; else if (sy) ny+=sy; else if(sx) nx+=sx;
        if (nx==g->player.x&&ny==g->player.y) continue;
        int occ=0; for(int j=0;j<g->nmob;j++) if(j!=i&&g->mob[j].x==nx&&g->mob[j].y==ny) occ=1;
        if (!occ && u2_passable(u2_map_tile(&g->map,nx,ny))){ g->mob[i].x=nx; g->mob[i].y=ny; }
    }
}
/* 玩家攻擊 (nx,ny) 的怪物(oracle:命中判定 rng%0x50>=技能→MISS,地面傷害公式)。
 * 回傳 1=該方向有怪物(取代移動) */
static int attack_mob(Game *g, int nx, int ny)
{
    for (int i=0;i<g->nmob;i++){
        if (g->mob[i].x==nx && g->mob[i].y==ny){
            if (g->arms_t>0){ g->arms_t--;            /* 臂麻:無法攻擊 */
                snprintf(g->msg,sizeof g->msg,tr("手臂麻痺,無法攻擊!(剩 %d)"),g->arms_t); return 1; }
            if ((int)(rng_next(g) % 0x50) >= hit_skill(g)){
                snprintf(g->msg,sizeof g->msg,tr("你攻擊%s,但沒打中。"),tr(g->mob[i].name));
                return 1;
            }
            int dmg=player_dmg(g); g->mob[i].hp-=dmg;
            if (g->mob[i].hp<=0){
                int xp=(rng_next(g)&3)+1;            /* oracle 地面 EXP +(rng&3)+1 */
                g->save.exp += xp;
                snprintf(g->msg,sizeof g->msg,tr("你擊敗了%s!(+%d 經驗)"),tr(g->mob[i].name),xp);
                check_levelup(g);
                g->mob[i]=g->mob[--g->nmob];
            } else snprintf(g->msg,sizeof g->msg,tr("你擊中%s,造成 %d 傷害(剩 %d)。"),
                            tr(g->mob[i].name),dmg,g->mob[i].hp);
            return 1;
        }
    }
    return 0;
}

/* 在玩家附近的水域放一艘船供登船示範;不移動玩家(保留城旁起點)。
 * 優先放在玩家相鄰水格(可直接 B 登船),否則放最近的水格(玩家走過去)。 */
static void place_ship(Game *g)
{
    /* 向外找最近水格放船(距玩家 ≥2 格,不佔玩家四鄰,避免一開場被載具圍困)*/
    for (int r=2;r<24;r++)
        for (int dy=-r;dy<=r;dy++) for (int dx=-r;dx<=r;dx++){
            if (abs(dx)<r && abs(dy)<r) continue;
            int x=g->player.x+dx, y=g->player.y+dy;
            if (x<1||y<1||x>=U2_MAP_W-1||y>=U2_MAP_H-1) continue;
            if (u2_map_tile(&g->map,x,y)==0){ g->map.tile[y][x]=SHIP_TILE; return; }
        }
}

static int veh_for_tile(unsigned char t);   /* forward */
/* 在玩家附近陸地放馬/飛機/火箭(demo);避開 landmark / 門 / 既有載具 */
static void place_land_vehicles(Game *g)
{
    unsigned char want[3]={HORSE_TILE,PLANE_TILE,ROCKET_TILE}; int placed=0;
    for (int r=2;r<24 && placed<3;r++)   /* r≥2:不佔玩家四鄰,避免圍困 */
        for (int dy=-r;dy<=r && placed<3;dy++) for (int dx=-r;dx<=r && placed<3;dx++){
            if (abs(dx)<r && abs(dy)<r) continue;
            int x=(g->player.x+dx)&(U2_WORLD_DIM-1), y=(g->player.y+dy)&(U2_WORLD_DIM-1);
            if (x==g->player.x && y==g->player.y) continue;
            if (x==g->td_x && y==g->td_y) continue;
            unsigned char t=u2_map_tile(&g->map,x,y);
            if (t==2 && !loc_lookup(g->world_num,t) && veh_for_tile(t)<0){   /* 純草地 */
                g->map.tile[y][x]=want[placed++];
            }
        }
}
/* 載具 tile → 載具型別(非載具回 -1) */
static int veh_for_tile(unsigned char t)
{
    switch (t){ case HORSE_TILE:return VEH_HORSE; case SHIP_TILE:return VEH_SHIP;
                case PLANE_TILE:return VEH_PLANE; case ROCKET_TILE:return VEH_ROCKET; }
    return -1;
}
static unsigned char veh_tile(int v)
{
    switch (v){ case VEH_HORSE:return HORSE_TILE; case VEH_SHIP:return SHIP_TILE;
                case VEH_PLANE:return PLANE_TILE; case VEH_ROCKET:return ROCKET_TILE; }
    return PLAYER_TILE;
}
/* 登載門檻物品(oracle):船=藍流蘇、飛機=骷髏鑰、火箭=生命符;馬無 */
static unsigned int veh_item(int v)
{
    switch (v){ case VEH_SHIP:return ITEM_BLUE_TASSLE; case VEH_PLANE:return ITEM_SKULL_KEY;
                case VEH_ROCKET:return ITEM_ANKH; }
    return 0;
}
static const char *veh_refuse(int v)
{
    switch (v){
        case VEH_SHIP:  return tr("船員不讓你登船(需藍流蘇)。");
        case VEH_PLANE: return tr("奇怪,你進不去(需骷髏鑰)。");
        case VEH_ROCKET:return tr("金屬之聲喝令:你必須擁有生命符。");
    }
    return tr("無法登載。");
}
static const char *veh_board_msg(int v)
{
    switch (v){
        case VEH_HORSE: return tr("你騎上了馬。");
        case VEH_SHIP:  return tr("你登上了船,可在水上航行。");
        case VEH_PLANE: return tr("你登上了飛機。");
        case VEH_ROCKET:return tr("你進入了火箭。");
    }
    return "";
}

/* B:登載(腳下/相鄰載具 tile,檢查門檻)或下載具(→相鄰可站格) */
static void board_vehicle(Game *g)
{
    if (g->mode!=MODE_WORLD || g->in_town){ snprintf(g->msg,sizeof g->msg,tr("這裡無法登載。")); return; }
    int NX[4]={0,1,0,-1}, NY[4]={-1,0,1,0};
    if (g->vehicle!=VEH_WALK){                                /* 下載具 */
        int cur=g->vehicle;
        for (int d=0;d<4;d++){
            int x=g->player.x+NX[d], y=g->player.y+NY[d];
            if (x<0||y<0||x>=U2_MAP_W||y>=U2_MAP_H) continue;
            unsigned char t=u2_map_tile(&g->map,x,y);
            int ok = (cur==VEH_SHIP) ? (t!=0 && t!=1 && u2_passable(t)) : u2_passable(t);
            if (ok){
                g->map.tile[g->player.y][g->player.x]=veh_tile(cur); /* 留載具在原地 */
                g->player.x=x; g->player.y=y; g->vehicle=VEH_WALK; g->player.tile=PLAYER_TILE;
                snprintf(g->msg,sizeof g->msg,tr("你下了載具。")); return;
            }
        }
        snprintf(g->msg,sizeof g->msg,tr("附近沒有可下載具的地方。")); return;
    }
    /* 登載:腳下或相鄰是載具 tile */
    int bx=g->player.x, by=g->player.y, v=veh_for_tile(u2_map_tile(&g->map,bx,by));
    if (v<0)
        for (int d=0;d<4;d++){
            int x=g->player.x+NX[d], y=g->player.y+NY[d];
            if (x<0||y<0||x>=U2_MAP_W||y>=U2_MAP_H) continue;
            int vv=veh_for_tile(u2_map_tile(&g->map,x,y));
            if (vv>=0){ v=vv; bx=x; by=y; break; }
        }
    if (v<0){ snprintf(g->msg,sizeof g->msg,tr("附近沒有載具。")); return; }
    unsigned int need=veh_item(v);
    if (need && !(g->items & need)){ snprintf(g->msg,sizeof g->msg,"%s",veh_refuse(v)); return; }
    g->map.tile[by][bx]=(v==VEH_SHIP)?0:2;   /* 移除載具 tile(船下露水,陸上露草地;避開 landmark 8)*/
    g->player.x=bx; g->player.y=by; g->vehicle=v; g->player.tile=veh_tile(v);
    snprintf(g->msg,sizeof g->msg,"%s",veh_board_msg(v));
}

/* ---- 時間之門(時間旅行雛形)---- */
/* 時代招牌(oracle FUN_0040c270 已釘死):map 檔名首位數字 → 時代。
 * 0=Legends 1=9,000,000 B.C. 2=1423 B.C. 3=1990 A.D. 4=2112 A.D. */
static const char *era_name(const char *world)
{
    if (u2_lang==U2_EN)
        switch (world[0]){
            case '0': return "LEGENDS"; case '1': return "9,000,000 B.C.";
            case '2': return "1423 B.C."; case '3': return "1990 A.D.";
            case '4': return "2112 A.D. (Aftermath)"; default: return "UNKNOWN";
        }
    switch (world[0]){
        case '0': return "傳說時代(Legends)";
        case '1': return "盤古大陸(9,000,000 B.C.)";
        case '2': return "西元前 1423 年";
        case '3': return "西元 1990 年";
        case '4': return "浩劫餘生(2112 A.D.)";
    }
    return "未知時代";
}
/* 時間門循環:5 個時代 overworld(mapx00/10/20/30/40)依序。 */
static const char *next_era_world(const char *world)
{
    switch (world[0]){
        case '0': return "10";
        case '1': return "20";
        case '2': return "30";
        case '3': return "40";
        case '4': return "00";
    }
    return "20";
}

/* 時間之門可見週期(回合;手冊:定時升起、很快消散)。 */
#define TD_VIS 14   /* 升起後可見回合數 */
#define TD_HID 6    /* 消散後隱沒回合數 */

/* 在玩家附近的可通行陸地放一個時間之門(非 landmark、非玩家腳下;wrap-aware) */
static void place_time_door(Game *g)
{
    g->td_x=g->td_y=-1;
    for (int r=2;r<22 && g->td_x<0;r++)
        for (int dy=-r;dy<=r && g->td_x<0;dy++) for (int dx=-r;dx<=r;dx++){
            if (abs(dx)<r && abs(dy)<r) continue;
            int x=(g->player.x+dx)&(U2_WORLD_DIM-1), y=(g->player.y+dy)&(U2_WORLD_DIM-1);
            if (x==g->player.x && y==g->player.y) continue;       /* 勿在玩家腳下重現 */
            unsigned char t=u2_map_tile(&g->map,x,y);
            if (t!=0 && u2_passable(t) && !loc_dest(g->world_num,t) && t!=WORLD_DUNGEON_TILE){
                g->td_x=x; g->td_y=y; break;
            }
        }
    g->td_timer=0;
}

/* 每回合推進時間之門:可見 TD_VIS 回合 → 消散 → 隱沒 TD_HID 回合 → 在新位置升起。 */
static void tick_time_door(Game *g)
{
    g->td_timer++;
    if (g->td_x>=0){
        if (g->td_timer>=TD_VIS){ g->td_x=g->td_y=-1; g->td_timer=0;
            snprintf(g->msg,sizeof g->msg,tr("時間之門化作藍霧消散了……")); }
    } else {
        if (g->td_timer>=TD_HID){ place_time_door(g);
            if (g->td_x>=0) snprintf(g->msg,sizeof g->msg,tr("一道時間之門如藍霧般在附近升起。")); }
    }
}

/* 穿越時間之門:切到下個時代 overworld,座標保留,重載地圖/實體/門/船 */
static void time_travel(Game *g)
{
    if (g->in_town || g->mode!=MODE_WORLD){ snprintf(g->msg,sizeof g->msg,tr("這裡沒有時間之門。")); return; }
    const char *nw=next_era_world(g->world_num);
    char mp[600];
    snprintf(mp,sizeof mp,"%s/mapx%s",g->data_dir,nw);
    U2Map nm=u2_map_load(mp);
    if (!nm.ok){ snprintf(g->msg,sizeof g->msg,tr("時間之門連向虛無(找不到 mapx%s)。"),nw); return; }
    g->map=nm;
    snprintf(mp,sizeof mp,"%s/monx%s",g->data_dir,nw); g->mon=u2_mon_load(mp);
    snprintf(g->world_num,sizeof g->world_num,"%s",nw);
    /* 座標保留;若落在不可通行格則就近找可通行 */
    if (!u2_passable(u2_map_tile(&g->map,g->player.x,g->player.y)))
        find_start(&g->map,&g->player,g->world_num);
    g->nmob=0;
    place_ship(g);
    place_time_door(g);
    snprintf(g->msg,sizeof g->msg,tr("時間之門開啟……招牌寫著:ANOS %s"),era_name(g->world_num));
}

/* ---- 太空飛行(M2 Step 2)---- */
/* 行星表(手冊 GALACTIC MAP;Xeno/Yako/Zabo 座標)。mapnum 為 provisional 行星表面圖。 */
static const struct { const char *zh, *en, *mapnum; int xe, ya, za; } PLANETS[] = {
    {"地球 Earth",  "Earth",   "20", 6,6,6},
    {"水星 Mercury","Mercury", "50", 5,4,5},
    {"金星 Venus",  "Venus",   "60", 3,3,4},
    {"火星 Mars",   "Mars",    "70", 6,2,3},
    {"木星 Jupiter","Jupiter", "80", 1,3,4},
    {"土星 Saturn", "Saturn",  "90", 2,8,5},
    {"天王星 Uranus","Uranus", "85", 9,4,6},
    {"海王星 Neptune","Neptune","82", 4,0,5},
    {"冥王星 Pluto","Pluto",   "45", 0,1,4},
};
#define NPLANET ((int)(sizeof PLANETS/sizeof PLANETS[0]))
static const char *planet_name(int i){ return (u2_lang==U2_EN)?PLANETS[i].en:PLANETS[i].zh; }

/* 火箭發射:在火箭上 + 有三鋰燃料 → 進入太空(目前行星軌道) */
static void launch_rocket(Game *g)
{
    if (g->vehicle!=VEH_ROCKET){ snprintf(g->msg,sizeof g->msg,tr("只有火箭能發射升空。")); return; }
    if (!(g->items & ITEM_TRI_LITHIUM)){ snprintf(g->msg,sizeof g->msg,tr("金屬之聲:火箭無法發射(需三鋰)。")); return; }
    snprintf(g->ret_world,sizeof g->ret_world,"%s",g->world_num);
    /* 目前所在 overworld 對應的行星(預設地球);否則由 mapnum 反查 */
    g->planet=0;
    for (int i=0;i<NPLANET;i++) if (!strcmp(PLANETS[i].mapnum,g->world_num)){ g->planet=i; break; }
    g->mode=MODE_SPACE;
    snprintf(g->msg,sizeof g->msg,tr("準備發射!你進入了%s 的軌道。"),planet_name(g->planet));
}
/* HYPERWARP:循環切換目標行星軌道 */
static void hyperwarp(Game *g)
{
    if (g->mode!=MODE_SPACE) return;
    /* 太陽危險(manual:Sun 4,4,4 / "YOU HIT THE SUN"):躍遷時偶爾擦過太陽 */
    if ((rng_next(g)&0xf)==0){
        int dmg=20+(rng_next(g)%30); g->php-=dmg; if(g->php<0)g->php=0;
        snprintf(g->msg,sizeof g->msg,tr("你的火箭擦過太陽,船身受損!失去 %d 點生命。"),dmg);
        return;
    }
    g->planet=(g->planet+1)%NPLANET;
    snprintf(g->msg,sizeof g->msg,tr("HYPERWARP……你來到了%s 的軌道。"),planet_name(g->planet));
}
/* 降落:載入目前軌道行星的表面圖,回 overworld(火箭仍在身上,可再發射) */
static void land_planet(Game *g)
{
    if (g->mode!=MODE_SPACE) return;
    const char *num=PLANETS[g->planet].mapnum;
    char mp[600]; snprintf(mp,sizeof mp,"%s/mapx%s",g->data_dir,num);
    U2Map nm=u2_map_load(mp);
    if (!nm.ok){ snprintf(g->msg,sizeof g->msg,tr("該行星沒有可降落的地表(mapx%s)。"),num); return; }
    g->map=nm;
    snprintf(mp,sizeof mp,"%s/monx%s",g->data_dir,num); g->mon=u2_mon_load(mp);
    snprintf(g->world_num,sizeof g->world_num,"%s",num);
    g->mode=MODE_WORLD; g->nmob=0;
    find_start(&g->map,&g->player,g->world_num);
    g->vehicle=VEH_ROCKET; g->player.tile=ROCKET_TILE;   /* 降落時人還在火箭上 */
    g->td_x=g->td_y=-1; place_time_door(g);
    snprintf(g->msg,sizeof g->msg,tr("你降落在%s 的地表。"),planet_name(g->planet));
}

/* 太空畫面:星空 + 目前軌道行星 + 行星清單 + 操作提示 */
static void render_space(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body, U2Text *small)
{
    int en=(u2_lang==U2_EN); (void)en;
    SDL_FillRect(cv,NULL,SDL_MapRGB(cv->format,4,4,14));
    SDL_Rect hdr={0,0,CANVAS_W,HDR_H}; SDL_FillRect(cv,&hdr,SDL_MapRGB(cv->format,30,30,70));
    u2_text_draw(cv,title,tr("深太空"),10,4,235,235,245);
    /* 簡易星空(determinism:依座標雜湊)*/
    for (int i=0;i<260;i++){
        unsigned int h=(unsigned)(i*2654435761u); int sx=h%CANVAS_W, sy=HDR_H+(h/CANVAS_W)%(CANVAS_H-HDR_H-120);
        Uint8 b=120+(h%120); SDL_Rect p={sx,sy,2,2}; SDL_FillRect(cv,&p,SDL_MapRGB(cv->format,b,b,b));
    }
    /* 中央行星圓(用顏色塊代表)*/
    int cx=CANVAS_W/2, cy=240, rad=70;
    Uint32 pc=SDL_MapRGB(cv->format,80+30*(g->planet%3),120+15*(g->planet%4),200);
    for (int yy=-rad;yy<=rad;yy++){ int w=(int)(0.999*rad*rad-yy*yy); if(w<0)continue; int hw=(int)(sqrt((double)w));
        SDL_Rect r={cx-hw,cy+yy,2*hw,1}; SDL_FillRect(cv,&r,pc); }
    char ln[96];
    snprintf(ln,sizeof ln,tr("目前軌道:%s"),planet_name(g->planet));
    u2_text_draw(cv,body,ln,cx-150,cy+rad+16,245,235,160);
    snprintf(ln,sizeof ln,"Xeno %d  Yako %d  Zabo %d",PLANETS[g->planet].xe,PLANETS[g->planet].ya,PLANETS[g->planet].za);
    u2_text_draw(cv,small,ln,cx-150,cy+rad+48,170,190,220);
    /* 行星清單(右側)*/
    int lx=CANVAS_W-260, ly=HDR_H+20;
    u2_text_draw(cv,small,tr("行星(E/W 躍遷)"),lx,ly,160,180,210); ly+=28;
    for (int i=0;i<NPLANET;i++){
        u2_text_draw(cv,small,planet_name(i),lx,ly,(i==g->planet)?250:180,(i==g->planet)?235:185,(i==g->planet)?150:195);
        ly+=24;
    }
    int by=CANVAS_H-90;
    u2_text_draw(cv,body,tr("E/W HYPERWARP 躍遷 · Y 降落 · F1 指令表"),24,by,150,175,205);
    u2_text_draw(cv,body,g->msg,24,by+30,210,225,205);
}

/* 勝利結局畫面(oracle FUN_0040eb60) */
static void render_ending(SDL_Surface *cv, U2Text *title, U2Text *body)
{
    int en=(u2_lang==U2_EN);
    SDL_FillRect(cv,NULL,SDL_MapRGB(cv->format,6,4,18));
    for (int i=0;i<200;i++){ unsigned int h=(unsigned)(i*2654435761u);
        SDL_Rect p={(int)(h%CANVAS_W),(int)((h/CANVAS_W)%CANVAS_H),2,2};
        SDL_FillRect(cv,&p,SDL_MapRGB(cv->format,180,170,90)); }
    const char *L_ZH[]={"米娜克斯死了!","她的一切邪惡都將消亡。","你拯救了宇宙,",
        "並完成了《創世紀 II》。","接著去征服邪惡的 EXODUS ──","就在《創世紀 III》之中。","","── 感謝遊玩(試玩版)──"};
    const char *L_EN[]={"MINAX IS DEAD!","ALL HER WORKS SHALL DIE.","YOU HAVE SAVED THE UNIVERSE,",
        "AND COMPLETED ULTIMA II.","SEEK NOW TO CONQUER WICKED EXODUS,","FOUND IN ULTIMA III.","","-- Thanks for playing (demo) --"};
    const char **Lz = en?L_EN:L_ZH;
    u2_text_draw(cv,title,tr("勝 利"),CANVAS_W/2-60,80,250,240,140);
    for (int i=0;i<8;i++) u2_text_draw(cv,body,Lz[i],CANVAS_W/2-260,180+i*44,235,230,180);
}

/* 原版 FM Towns 開場標題畫面(整張等比放大置中 + 提示) */
static void render_title(SDL_Surface *cv, SDL_Surface *img, U2Text *title, U2Text *body)
{
    SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format,0,0,0));
    if (img){
        double s=(double)CANVAS_W/img->w;
        if (img->h*s > CANVAS_H-44) s=(double)(CANVAS_H-44)/img->h;
        int w=(int)(img->w*s), h=(int)(img->h*s);
        SDL_Rect dst={(CANVAS_W-w)/2,(CANVAS_H-h)/2-8,w,h};
        SDL_BlitScaled(img,NULL,cv,&dst);
    }
    u2_text_draw(cv,title,"Ultima II:女巫的復仇",16,8,210,205,150);
    u2_text_draw(cv,body,"繁體中文化(試玩版) ── 按任意鍵繼續",CANVAS_W/2-180,CANVAS_H-34,200,210,235);
}

/* 開場 splash:全家福 + 試玩版標註 */
static void render_splash(SDL_Surface *cv, SDL_Surface *photo, U2Text *title, U2Text *body)
{
    SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format,12,14,28));
    SDL_Rect hdr={0,0,CANVAS_W,HDR_H}; SDL_FillRect(cv,&hdr,SDL_MapRGB(cv->format,36,44,110));
    u2_text_draw(cv,title,"Ultima II:女巫的復仇 — 繁體中文化(試玩版)",10,4,250,240,140);
    if (photo){
        int maxh=CANVAS_H-150, maxw=CANVAS_W-80;
        double s=(double)maxh/photo->h; if (photo->w*s>maxw) s=(double)maxw/photo->w;
        int w=(int)(photo->w*s), h=(int)(photo->h*s);
        SDL_Rect dst={(CANVAS_W-w)/2,52,w,h};
        SDL_BlitScaled(photo,NULL,cv,&dst);
        Uint32 fr=SDL_MapRGB(cv->format,250,240,90);
        SDL_Rect e1={dst.x-3,dst.y-3,w+6,3},e2={dst.x-3,dst.y+h,w+6,3},
                 e3={dst.x-3,dst.y-3,3,h+6},e4={dst.x+w,dst.y-3,3,h+6};
        SDL_FillRect(cv,&e1,fr);SDL_FillRect(cv,&e2,fr);SDL_FillRect(cv,&e3,fr);SDL_FillRect(cv,&e4,fr);
    }
    u2_text_draw(cv,body,"※ 試玩版(demo):核心引擎與在地化展示,非完整遊戲。",60,CANVAS_H-84,210,210,220);
    u2_text_draw(cv,body,"感謝遊玩 ── 獻給我的家人。  按任意鍵開始。",60,CANVAS_H-52,180,205,235);
}

/* 地牢遭遇:前進時隨機觸發寶箱或怪物(自動戰鬥)。最深層寶箱給三鋰(火箭燃料);深層寶箱可得防護具。 */
static void dungeon_event(Game *g)
{
    unsigned int r = rng_next(g) % 100;
    if (r < 22){                                  /* 寶箱 */
        unsigned want = (~g->items) & (ITEM_BOOTS|ITEM_CLOAK|ITEM_IDOL|ITEM_HELM);  /* 尚缺的魔法道具 */
        if (g->dlevel>=13 && !(g->items & ITEM_TRI_LITHIUM)){
            g->items |= ITEM_TRI_LITHIUM;
            snprintf(g->msg,sizeof g->msg,tr("寶箱中閃耀著三鋰!(火箭燃料)"));
        } else if (g->dlevel>=4 && want && (rng_next(g)%3==0)){           /* 深層概率給魔法道具 */
            unsigned pick = (want&ITEM_BOOTS)?ITEM_BOOTS:(want&ITEM_CLOAK)?ITEM_CLOAK
                          : (want&ITEM_IDOL)?ITEM_IDOL:ITEM_HELM;
            g->items |= pick;
            const char *nm = (pick==ITEM_BOOTS)?tr("魔法長靴(擋腿麻)")
                           : (pick==ITEM_CLOAK)?tr("魔法斗篷(擋臂麻)")
                           : (pick==ITEM_IDOL)?tr("綠色神像(擋睡眠)"):tr("魔法頭盔(鳥瞰)");
            snprintf(g->msg,sizeof g->msg,tr("寶箱中是%s!"),nm);
        } else {
            int gold=10+(rng_next(g)%40); g->save.gold+=gold; if(g->save.gold>9999)g->save.gold=9999;
            snprintf(g->msg,sizeof g->msg,tr("你找到一個寶箱:+%d 黃金。"),gold);
        }
    } else if (r < 44){                           /* 怪物:自動戰鬥(玩家先攻)*/
        unsigned char tile = (unsigned char)(12 + (g->dlevel % 4));   /* 越深越強 */
        const char *nm; int hp,atk; mob_type(tile,&nm,&hp,&atk);
        int php=g->php, guard=0;
        while (hp>0 && php>0 && guard++<30){
            hp -= player_dmg(g);
            if (hp<=0) break;
            int d=atk + (rng_next(g)%4) - g->armour*2; if(d<1)d=1; php-=d;
        }
        g->php = php<0?0:php;
        if (hp<=0){
            int xp=(rng_next(g)&3)+2, gold=5+(rng_next(g)%20);
            g->save.exp+=xp; g->save.gold+=gold;
            snprintf(g->msg,sizeof g->msg,tr("地牢中%s擋路!你擊敗了它(+%d 經驗,+%d 金)。"),tr(nm),xp,gold);
            check_levelup(g);
        } else {
            snprintf(g->msg,sizeof g->msg,tr("%s 在地牢重創了你!"),tr(nm));
        }
    }
}

/* 施放法術 idx(僅地牢/塔內;手冊:C)ast only in dungeons and towers)。 */
static void cast_spell(Game *g, int idx)
{
    if (idx<0 || idx>=9) return;
    if (g->mode != MODE_DUNGEON){
        snprintf(g->msg,sizeof g->msg,tr("只能在地牢或塔中施法。")); return;
    }
    if (!spell_class_ok(g,idx)){
        snprintf(g->msg,sizeof g->msg,tr("你的職業無法施展「%s」。"),spell_name(idx)); return;
    }
    if (g->spells[idx] <= 0){
        snprintf(g->msg,sizeof g->msg,tr("你沒有「%s」法術(可在城鎮商店習得)。"),spell_name(idx)); return;
    }
    g->spells[idx]--;                                    /* 消耗(成敗皆扣)*/
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};              /* N E S W */
    int fx=g->dx+FX[g->ddir], fy=g->dy+FY[g->ddir];      /* 前方格 */
    switch (idx){
        case SP_LIGHT:
            g->spell_light = 40;
            snprintf(g->msg,sizeof g->msg,tr("魔法照明亮起,地牢豁然開朗。")); break;
        case SP_DDOWN:  dungeon_descend(g); break;
        case SP_DUP:    dungeon_ascend(g);  break;
        case SP_PASS:
            if (u2_dungeon_is_wall(&g->dg,g->dlevel,fx,fy)){
                g->dx=fx; g->dy=fy;                      /* 打通並穿過前方牆 */
                snprintf(g->msg,sizeof g->msg,tr("穿牆術轟開前方石壁,你穿了過去。"));
            } else snprintf(g->msg,sizeof g->msg,tr("前方沒有牆可穿。"));
            break;
        case SP_SURF:
            snprintf(g->msg,sizeof g->msg,tr("返地表法術將你傳回地面。")); exit_dungeon(g); break;
        case SP_PRAY:
            g->php = g->maxhp;                           /* 神聖干預:治癒(results simulate reality)*/
            snprintf(g->msg,sizeof g->msg,tr("你祈禱,神聖之力湧入,傷勢盡復。")); break;
        case SP_MISSILE: {
            int xp=(rng_next(g)&3)+2, gold=4+(rng_next(g)%16);
            g->save.exp+=xp; g->save.gold+=gold; if(g->save.gold>9999)g->save.gold=9999;
            snprintf(g->msg,sizeof g->msg,tr("魔法飛彈轟向前方,擊潰擋路之敵(+%d 經驗)。"),xp); check_levelup(g); break;
        }
        case SP_BLINK: {
            for (int t=0;t<16;t++){
                int rx=(rng_next(g)%17)-8, ry=(rng_next(g)%17)-8;
                int nx=g->dx+rx, ny=g->dy+ry;
                if (!u2_dungeon_is_wall(&g->dg,g->dlevel,nx,ny)){ g->dx=nx; g->dy=ny; break; }
            }
            snprintf(g->msg,sizeof g->msg,tr("瞬移!你閃現到地牢另一處。")); break;
        }
        case SP_KILL: {
            int xp=(rng_next(g)&7)+4, gold=8+(rng_next(g)%24);
            g->save.exp+=xp; g->save.gold+=gold; if(g->save.gold>9999)g->save.gold=9999;
            snprintf(g->msg,sizeof g->msg,tr("擊殺術迸發,前方之敵灰飛煙滅(+%d 經驗,+%d 金)。"),xp,gold); check_levelup(g); break;
        }
    }
}

/* Minax 對決(傳說時代 Legends);oracle:力場 1000 傷、RING 免疫、ENILNO 殺。 */
static void minax_encounter(Game *g)
{
    if (g->world_num[0] != '0' || g->in_town || g->mode!=MODE_WORLD){
        snprintf(g->msg,sizeof g->msg,tr("米娜克斯只存在於傳說時代。")); return;
    }
    if (!(g->items & ITEM_RING)){
        g->php=0; snprintf(g->msg,sizeof g->msg,tr("米娜克斯的力場造成 1000 點傷害!你被消滅了。")); return;
    }
    if (!(g->items & ITEM_QUICKSWORD)){
        snprintf(g->msg,sizeof g->msg,tr("戒指擋下了力場!但唯有迅捷之劍 ENILNO 能殺死她。")); return;
    }
    /* 真實對決:Minax 高血 + DIE FOOL 反擊(oracle);戒指免力場,仍需撐過近身戰 */
    int mhp=200, php=g->php, rounds=0;
    while (mhp>0 && php>0 && rounds++<20){
        mhp -= player_dmg(g) + 50;          /* 迅捷之劍 ENILNO 重擊 */
        if (mhp<=0) break;
        int d=90 - g->armour*3; if(d<40)d=40; php-=d;   /* MINAX CRIES: DIE FOOL! */
    }
    g->php = php<0?0:php;
    if (mhp<=0){
        g->won=1;
        snprintf(g->msg,sizeof g->msg,tr("你以迅捷之劍 ENILNO 擊穿了米娜克斯!"));
    } else {
        snprintf(g->msg,sizeof g->msg,tr("米娜克斯尖叫「去死吧,蠢貨!」你倒下了……(回城補給再來)"));
    }
}

/* 死亡 → 復活(HP 歸零時不列顛王在城堡復活;回 overworld、滿血、失半數黃金)*/
static void revive_if_dead(Game *g)
{
    if (g->won || !g->save.has_character || g->php>0) return;
    g->php=g->maxhp; g->save.hp=g->maxhp; g->save.gold/=2;
    g->mode=MODE_WORLD; g->in_town=0; g->vehicle=VEH_WALK; g->player.tile=PLAYER_TILE; g->nmob=0;
    find_start(&g->map,&g->player,g->world_num);
    snprintf(g->msg,sizeof g->msg,tr("你倒下了……不列顛王將你復活,但失去了半數黃金。"));
}

/* 依模式處理一個方向鍵(dir ∈ N/S/E/W) */
static void handle_dir(Game *g, char dir)
{
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};  /* N E S W */
    if (g->mode!=MODE_SPACE){                /* 狀態效果:睡眠/腿麻阻止行動,每回合遞減自解 */
        if (g->sleep_t>0){ g->sleep_t--; snprintf(g->msg,sizeof g->msg,tr("你在沉睡中,動彈不得……(剩 %d)"),g->sleep_t); return; }
        if (g->legs_t>0){ g->legs_t--; snprintf(g->msg,sizeof g->msg,tr("雙腿麻痺,無法移動……(剩 %d)"),g->legs_t); return; }
    }
    if (g->mode==MODE_SPACE){                /* 太空:E/W HYPERWARP 切換行星軌道 */
        if (dir=='E') hyperwarp(g);
        else if (dir=='W'){ g->planet=(g->planet+NPLANET-1)%NPLANET;
            snprintf(g->msg,sizeof g->msg,tr("HYPERWARP……你來到了%s 的軌道。"),planet_name(g->planet)); }
        return;
    }
    if (g->mode==MODE_WORLD){
        /* 朝向怪物移動 = 攻擊(不移動);否則正常走 + 觸發怪物回合 */
        int di = (dir=='N')?0:(dir=='E')?1:(dir=='S')?2:3;
        int tx=g->player.x+FX[di], ty=g->player.y+FY[di];
        if (!g->in_town){ tx&=(U2_WORLD_DIM-1); ty&=(U2_WORLD_DIM-1); }  /* overworld 環形 */
        if (!g->in_town && attack_mob(g,tx,ty)){ step_mobs(g); return; }
        /* 載具感知移動:船=水域可走,步行=陸地;城鎮一律步行 */
        U2Map *am=amap(g);
        int inb = tx>=0&&ty>=0&&tx<U2_MAP_W&&ty<U2_MAP_H;
        unsigned char tt = inb ? u2_map_tile(am,tx,ty) : 0;
        /* 飛機需黃銅鈕扣才起飛(manual:PLANES NEED BRASS BUTTONS)*/
        if (!g->in_town && g->vehicle==VEH_PLANE && !(g->items & ITEM_BRASS_BUTTON)){
            snprintf(g->msg,sizeof g->msg,tr("這架飛機少了黃銅鈕扣,飛不起來。")); return;
        }
        int pass;
        if (g->in_town) pass = inb && u2_passable(tt);                    /* 城鎮:步行 */
        else switch (g->vehicle){
            case VEH_SHIP:  pass = inb && (tt==0||tt==1); break;          /* 船:水域 */
            case VEH_PLANE: pass = inb; break;                           /* 飛機:飛越任意地形 */
            default:        pass = inb && u2_passable(tt); break;         /* 步行/馬/火箭:陸地 */
        }
        if (pass){
            g->player.x=tx; g->player.y=ty;
            snprintf(g->msg,sizeof g->msg, g->vehicle?tr("航行 %c。"):tr("往 %c 移動。"),dir);
            unsigned char t=u2_map_tile(am,g->player.x,g->player.y);
            const LocReg *L = (!g->in_town) ? loc_lookup(g->world_num,t) : NULL;
            if (!g->in_town && g->td_x==g->player.x && g->td_y==g->player.y) { time_travel(g); }
            else if (!g->in_town && g->world_num[0]=='0' && t==8) { minax_encounter(g); }  /* 傳說時代 Minax 巢穴 */
            else if (L && (L->kind=='d'||L->kind=='T')) { g->nmob=0; enter_dungeon_at(g, L->dest, L->kind=='T'); }
            else if (L) { g->nmob=0; enter_town_tile(g,t); }
            else if (!g->in_town){ g->turn++; step_mobs(g); spawn_mob(g); tick_time_door(g);
                /* 食物每回合消耗(manual);耗盡則飢餓扣血 */
                if (g->save.has_character){
                    if (g->save.food>0) g->save.food--;
                    else { g->php-=2; if(g->php<0)g->php=0; snprintf(g->msg,sizeof g->msg,tr("你飢餓難耐,生命流逝……")); }
                }
            }
        } else snprintf(g->msg,sizeof g->msg,
                        (!g->in_town&&g->vehicle==VEH_SHIP&&tt!=0&&tt!=1)?tr("船無法駛上陸地(B 下船)。"):tr("%c 方向被擋住。"),dir);
    } else { /* DUNGEON: N前進 S後退 W左轉 E右轉 */
        if (dir=='W'){ g->ddir=(g->ddir+3)&3; snprintf(g->msg,sizeof g->msg,tr("左轉。")); }
        else if (dir=='E'){ g->ddir=(g->ddir+1)&3; snprintf(g->msg,sizeof g->msg,tr("右轉。")); }
        else {
            int s=(dir=='N')?1:-1;
            int di=g->ddir; int nx=g->dx+FX[di]*s, ny=g->dy+FY[di]*s;
            if (!u2_dungeon_is_wall(&g->dg,g->dlevel,nx,ny)){ g->dx=nx; g->dy=ny;
                int lad=u2_dungeon_ladder(&g->dg,g->dlevel,nx,ny);
                if (lad>0) snprintf(g->msg,sizeof g->msg,tr("腳下有向下的樓梯(J 下樓)。"));
                else if (lad<0) snprintf(g->msg,sizeof g->msg,tr("腳下有向上的樓梯(K 上樓)。"));
                else { snprintf(g->msg,sizeof g->msg, s>0?tr("前進。"):tr("後退。"));
                       dungeon_event(g); }   /* 前進時隨機遭遇寶箱/怪物 */
            }
            else snprintf(g->msg,sizeof g->msg,tr("前方是牆。"));
        }
    }
}

static char norm_dir(char c)
{
    switch (c){ case 'w':case 'N': return 'N'; case 's':case 'S': return 'S';
                case 'a':case 'W': return 'W'; case 'd':case 'E': return 'E'; }
    return 0;
}

/* ===================== 開場選單 + 原版建角流程 ===================== */

/* 各種族基礎屬性 STR,AGI,STA,CHA,WIS,INT(demo 指派:均衡/智敏/壯碩/靈巧) */
static const int RACE_BASE[4][6] = {
    {15,15,15,15,15,15},   /* 人類 */
    {12,17,12,15,15,18},   /* 精靈 */
    {18,12,18,12,12,12},   /* 矮人 */
    {12,18,12,18,15,15},   /* 哈比人 */
};
#define CREATE_POOL 25       /* 可分配點數 */
#define STAT_CAP    50       /* 建角時單項上限 */

enum CreateStep { CS_NAME, CS_SEX, CS_RACE, CS_CLASS, CS_STATS, CS_DONE };

typedef struct {
    int step;
    char name[16]; int nlen;
    int sex, race, klass;     /* sex 0=M 1=F */
    int stats[6], pool, cur;
} Create;

static void create_init(Create *c)
{
    memset(c,0,sizeof *c);
    c->step=CS_NAME;
    for (int i=0;i<6;i++) c->stats[i]=RACE_BASE[0][i];
    c->pool=CREATE_POOL;
}

/* 選種族時重設屬性基底 */
static void create_apply_race(Create *c)
{
    for (int i=0;i<6;i++) c->stats[i]=RACE_BASE[c->race][i];
    c->pool=CREATE_POOL; c->cur=0;
}

/* 餵一個輸入給建角狀態機。key=SDL keycode,ch=可列印字元(0 表無)。回傳 1=已完成 */
static int create_feed(Create *c, int key, char ch)
{
    switch (c->step){
    case CS_NAME:
        if ((ch>='A'&&ch<='Z')||(ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch==' '){
            if (c->nlen<15){ char u=(ch>='a'&&ch<='z')?ch-32:ch; c->name[c->nlen++]=u; c->name[c->nlen]=0; }
        } else if (key==SDLK_BACKSPACE){ if (c->nlen>0) c->name[--c->nlen]=0; }
        else if (key==SDLK_RETURN){ if (c->nlen>0) c->step=CS_SEX; }
        break;
    case CS_SEX:
        if (key==SDLK_LEFT||key==SDLK_RIGHT||key==SDLK_UP||key==SDLK_DOWN) c->sex^=1;
        else if (key==SDLK_RETURN) c->step=CS_RACE;
        break;
    case CS_RACE:
        if (key==SDLK_LEFT||key==SDLK_UP){ c->race=(c->race+3)&3; create_apply_race(c); }
        else if (key==SDLK_RIGHT||key==SDLK_DOWN){ c->race=(c->race+1)&3; create_apply_race(c); }
        else if (key==SDLK_RETURN) c->step=CS_CLASS;
        break;
    case CS_CLASS:
        if (key==SDLK_LEFT||key==SDLK_UP) c->klass=(c->klass+3)&3;
        else if (key==SDLK_RIGHT||key==SDLK_DOWN) c->klass=(c->klass+1)&3;
        else if (key==SDLK_RETURN) c->step=CS_STATS;
        break;
    case CS_STATS:
        if (key==SDLK_UP) c->cur=(c->cur+5)%6;
        else if (key==SDLK_DOWN) c->cur=(c->cur+1)%6;
        else if (key==SDLK_RIGHT){ if (c->pool>0 && c->stats[c->cur]<STAT_CAP){ c->stats[c->cur]++; c->pool--; } }
        else if (key==SDLK_LEFT){ if (c->stats[c->cur]>RACE_BASE[c->race][c->cur]){ c->stats[c->cur]--; c->pool++; } }
        else if (key==SDLK_RETURN){ c->step=CS_DONE; return 1; }
        break;
    default: return 1;
    }
    return c->step==CS_DONE;
}

/* 由完成的 Create 產生 U2Save(寫 raw[] + 解析欄位;數值在 store 時編碼成 BCD) */
static U2Save make_character(const Create *c)
{
    U2Save s; memset(&s,0,sizeof s);
    s.ok=1; s.has_character=1;
    for (int i=0;i<16;i++) s.raw[i]=(i<c->nlen)?(unsigned char)c->name[i]:0;
    memcpy(s.name,c->name,c->nlen); s.name[c->nlen]=0;
    s.raw[U2_OFF_SEX]=c->sex?'F':'M'; s.sex=s.raw[U2_OFF_SEX];
    s.raw[U2_OFF_CLASS]=(unsigned char)c->klass; s.klass=c->klass;
    s.raw[U2_OFF_RACE]=(unsigned char)c->race;   s.race=c->race;
    for (int i=0;i<6;i++) s.stats[i]=c->stats[i];
    s.hp=400; s.food=400; s.exp=0; s.gold=400;     /* 新角色起始(見 u2_save.h 起始值) */
    s.raw[0x100]=0x1a; s.marker=0x1a;
    memcpy(s.rec,s.raw,U2_REC_SIZE);
    return s;
}

/* 渲染建角當前步驟 */
static void render_create(SDL_Surface *cv, const Create *c, U2Text *title, U2Text *body, U2Text *small)
{
    SDL_FillRect(cv,NULL,SDL_MapRGB(cv->format,12,14,28));
    SDL_Rect hdr={0,0,CANVAS_W,HDR_H}; SDL_FillRect(cv,&hdr,SDL_MapRGB(cv->format,36,44,110));
    int en=(u2_lang==U2_EN);
    const char *sexnm = c->sex?tr("女"):tr("男");
    u2_text_draw(cv,title,tr("建立角色"),16,4,250,240,140);
    const char *steps_zh[]={"① 姓名","② 性別","③ 種族","④ 職業","⑤ 屬性分配"};
    const char *steps_en[]={"1.Name","2.Sex","3.Race","4.Class","5.Stats"};
    const char **steps = en?steps_en:steps_zh;
    char bar[160]=""; for (int i=0;i<5;i++){ strcat(bar,(i==c->step)?"[":" "); strcat(bar,steps[i]); strcat(bar,(i==c->step)?"] ":" "); }
    u2_text_draw(cv,small,bar,16,40,170,185,215);

    int x=60, y=110, lh=40;
    char ln[160];
    /* 已決定欄位摘要 */
    if (c->step>CS_NAME){ snprintf(ln,sizeof ln,"%s %s",tr("姓名:"),c->name); u2_text_draw(cv,body,ln,x,y,210,215,225);} y+=lh;
    if (c->step>CS_SEX){ snprintf(ln,sizeof ln,"%s %s",tr("性別:"),sexnm); u2_text_draw(cv,body,ln,x,y,210,215,225);} y+=lh;
    if (c->step>CS_RACE){ snprintf(ln,sizeof ln,"%s %s",tr("種族:"),race_nm(c->race)); u2_text_draw(cv,body,ln,x,y,210,215,225);} y+=lh;
    if (c->step>CS_CLASS){ snprintf(ln,sizeof ln,"%s %s",tr("職業:"),class_nm(c->klass)); u2_text_draw(cv,body,ln,x,y,210,215,225);} y+=lh;

    int py=320;
    switch (c->step){
    case CS_NAME:
        snprintf(ln,sizeof ln,"%s%s_",tr("輸入姓名:"),c->name);
        u2_text_draw(cv,body,ln,x,py,245,235,180);
        u2_text_draw(cv,small,tr("鍵入 A–Z / 0–9,Backspace 刪除,Enter 確認"),x,py+44,180,195,220);
        break;
    case CS_SEX:
        snprintf(ln,sizeof ln,"%s ◀ %s ▶",tr("性別:"),sexnm);
        u2_text_draw(cv,body,ln,x,py,245,235,180);
        u2_text_draw(cv,small,tr("←/→ 切換,Enter 確認"),x,py+44,180,195,220);
        break;
    case CS_RACE:
        snprintf(ln,sizeof ln,"%s ◀ %s ▶",tr("種族:"),race_nm(c->race));
        u2_text_draw(cv,body,ln,x,py,245,235,180);
        u2_text_draw(cv,small,tr("←/→ 選擇(人類/精靈/矮人/哈比人),Enter 確認"),x,py+44,180,195,220);
        break;
    case CS_CLASS:
        snprintf(ln,sizeof ln,"%s ◀ %s ▶",tr("職業:"),class_nm(c->klass));
        u2_text_draw(cv,body,ln,x,py,245,235,180);
        u2_text_draw(cv,small,tr("←/→ 選擇(戰士/牧師/巫師/盜賊),Enter 確認"),x,py+44,180,195,220);
        break;
    case CS_STATS: {
        snprintf(ln,sizeof ln,"%s%d",tr("屬性分配  剩餘點數:"),c->pool);
        u2_text_draw(cv,body,ln,x,260,245,235,180);
        for (int i=0;i<6;i++){
            int yy=300+i*30;
            char lab[80]; snprintf(lab,sizeof lab,en?"%s %s":"%s %s %s",(i==c->cur)?"▶":"  ",stat_nm(i),u2_save_stat_name(i));
            u2_text_draw(cv,body,lab,x,yy,(i==c->cur)?250:210,(i==c->cur)?235:215,(i==c->cur)?150:225);
            char v[8]; snprintf(v,sizeof v,"%d",c->stats[i]);
            u2_text_draw(cv,body,v,x+230,yy,245,225,150);
        }
        u2_text_draw(cv,small,tr("↑/↓ 選屬性,←/→ 增減,Enter 完成建角"),x,300+6*30+8,180,195,220);
        break; }
    default: break;
    }
}

/* 渲染開場選單 */
static void render_menu(SDL_Surface *cv, const char *opts[], int n, int sel,
                        SDL_Surface *titleimg, U2Text *title, U2Text *body)
{
    SDL_FillRect(cv,NULL,SDL_MapRGB(cv->format,8,10,22));
    if (titleimg){
        double s=(double)CANVAS_W/titleimg->w; if (titleimg->h*s>CANVAS_H-150) s=(double)(CANVAS_H-150)/titleimg->h;
        int w=(int)(titleimg->w*s), h=(int)(titleimg->h*s);
        SDL_Rect dst={(CANVAS_W-w)/2,20,w,h}; SDL_BlitScaled(titleimg,NULL,cv,&dst);
    }
    int by=CANVAS_H-210;
    for (int i=0;i<n;i++){
        int yy=by+i*44;
        if (i==sel){ SDL_Rect r={CANVAS_W/2-160,yy-4,320,40}; SDL_FillRect(cv,&r,SDL_MapRGB(cv->format,40,50,120)); }
        u2_text_draw(cv,body,tr(opts[i]),CANVAS_W/2-120,yy,(i==sel)?250:200,(i==sel)?235:205,(i==sel)?150:215);
    }
    u2_text_draw(cv,title,tr("↑/↓ 選擇 · Enter 確認"),CANVAS_W/2-150,CANVAS_H-28,170,185,215);
}

/* 裝備/道具 sidecar 持久化(存檔旁 .meta;不碰原存檔格式,避免損壞)。 */
static void meta_path(char *out, size_t n, const char *save_path){ snprintf(out,n,"%s.meta",save_path); }
static void load_meta(Game *g, const char *save_path){
    char mp[1100]; meta_path(mp,sizeof mp,save_path);
    FILE *f=fopen(mp,"rb"); if(!f) return;
    unsigned int v[3]={0,0,0}; if(fread(v,sizeof(unsigned int),3,f)==3){ g->items=v[0]; g->weapon=(int)v[1]; g->armour=(int)v[2]; }
    fclose(f);
}
static void save_meta(const Game *g, const char *save_path){
    char mp[1100]; meta_path(mp,sizeof mp,save_path);
    FILE *f=fopen(mp,"wb"); if(!f) return;
    unsigned int v[3]={g->items,(unsigned)g->weapon,(unsigned)g->armour}; fwrite(v,sizeof(unsigned int),3,f); fclose(f);
}

int main(int argc, char **argv)
{
    if (argc < 5){
        fprintf(stderr,"用法: %s <worldmap> <font.ttf> <tileset.png> <ui_tsv> "
            "[player_save] [--script CMDS prefix]\n",argv[0]);
        return 2;
    }
    const char *map_path=argv[1], *font_path=argv[2], *tiles_path=argv[3], *ui_tsv=argv[4];
    const char *player_save=NULL, *script=NULL, *out_prefix=NULL, *splash_path=NULL, *title_path=NULL;
    const char *save_override=NULL, *screens_prefix=NULL;
    for (int i=5;i<argc;i++){
        if (!strcmp(argv[i],"--script") && i+2<argc){ script=argv[i+1]; out_prefix=argv[i+2]; i+=2; }
        else if (!strcmp(argv[i],"--splash") && i+1<argc){ splash_path=argv[i+1]; i+=1; }
        else if (!strcmp(argv[i],"--title") && i+1<argc){ title_path=argv[i+1]; i+=1; }
        else if (!strcmp(argv[i],"--save") && i+1<argc){ save_override=argv[i+1]; i+=1; }
        else if (!strcmp(argv[i],"--screens") && i+1<argc){ screens_prefix=argv[i+1]; i+=1; }
        else if (!player_save) player_save=argv[i];
    }
    int headless=(script!=NULL || screens_prefix!=NULL);

    if (SDL_Init(headless?0:SDL_INIT_VIDEO)!=0 || IMG_Init(IMG_INIT_PNG)==0){
        fprintf(stderr,"SDL init: %s\n",SDL_GetError()); return 1;
    }
    Game g; memset(&g,0,sizeof g);
    g.map=u2_map_load(map_path);
    if (!g.map.ok){ fprintf(stderr,"無法載入地圖: %s\n",map_path); return 1; }

    char mon_path[512]; snprintf(mon_path,sizeof mon_path,"%s",map_path);
    char *bn=strrchr(mon_path,'/'); bn=bn?bn+1:mon_path;
    char *mp=strstr(bn,"map"); if (mp) memcpy(mp,"mon",3);
    g.mon=u2_mon_load(mon_path);

    /* 地牢檔路徑:與世界圖同目錄的 mapx15 */
    snprintf(g.dungeon_path,sizeof g.dungeon_path,"%s",map_path);
    char *db=strrchr(g.dungeon_path,'/'); db=db?db+1:g.dungeon_path;
    snprintf(db,sizeof g.dungeon_path-(db-g.dungeon_path),"mapx15");
    /* 地點檔目錄:取世界圖所在目錄,供動態組各 mapxNN 路徑 */
    snprintf(g.data_dir,sizeof g.data_dir,"%s",map_path);
    { char *sl=strrchr(g.data_dir,'/'); if(sl)*sl=0; else snprintf(g.data_dir,sizeof g.data_dir,"."); }
    /* 目前 overworld 編號:由 map_path basename "mapxNN" 取 "NN" */
    { const char *b=strrchr(map_path,'/'); b=b?b+1:map_path;
      const char *p=strstr(b,"mapx"); snprintf(g.world_num,sizeof g.world_num,"%s",p?p+4:"20"); }

    /* tileset:tiles_path 為逗號分隔的多張 strip(可切換) */
    {
        char buf[1024]; snprintf(buf,sizeof buf,"%s",tiles_path);
        for (char *tok=strtok(buf,","); tok && g.ntset<8; tok=strtok(NULL,",")){
            SDL_Surface *s=u2_tileset_load(tok);
            if (!s) continue;
            g.tset[g.ntset]=s;
            /* 名稱 = basename 去副檔名 */
            const char *bn2=strrchr(tok,'/'); bn2=bn2?bn2+1:tok;
            snprintf(g.tname[g.ntset],sizeof g.tname[0],"%s",bn2);
            char *dot=strrchr(g.tname[g.ntset],'.'); if(dot)*dot=0;
            g.ntset++;
        }
        g.curset=0;
    }

    g.ui = ui_tsv ? u2_strings_load(ui_tsv,2,3) : (U2Strings){0};
    /* 對話譯文 + 引擎多語字典:由 ui_tsv 同目錄推 */
    if (ui_tsv){
        char tt[512]; snprintf(tt,sizeof tt,"%s",ui_tsv);
        char *e=strrchr(tt,'/'); e=e?e+1:tt;
        snprintf(e,sizeof tt-(e-tt),"talk_dialogue.tsv");
        g.tr = u2_strings_load(tt,2,3);
        char ud[512]; snprintf(ud,sizeof ud,"%s",ui_tsv);
        char *e2=strrchr(ud,'/'); e2=e2?e2+1:ud;
        snprintf(e2,sizeof ud-(e2-ud),"ui_strings.tsv");
        load_ui_dict(ud);   /* 引擎硬編訊息字典(多語)*/
    }
    /* 可寫存檔路徑:--save 覆寫 > headless 用 player_save > SDL_GetPrefPath(跨平台可寫) */
    char save_path[1024];
    if (save_override) snprintf(save_path,sizeof save_path,"%s",save_override);
    else if (headless && player_save) snprintf(save_path,sizeof save_path,"%s",player_save);
    else {
        char *pref=SDL_GetPrefPath("LairWare-cht","Ultima2");   /* 自動建立目錄 */
        if (pref){ snprintf(save_path,sizeof save_path,"%splayer",pref); SDL_free(pref); }
        else snprintf(save_path,sizeof save_path,"u2cht_player");
    }
    U2Save wsave = u2_save_load(save_path);                       /* 可寫存檔(繼續用) */
    U2Save tmpl  = player_save ? u2_save_load(player_save) : (U2Save){0};  /* 打包範本(唯讀) */
    int have_continue = wsave.ok && wsave.has_character;
    /* headless / 預設:有可寫存檔用它,否則用範本 */
    g.save = have_continue ? wsave : tmpl;
    load_meta(&g, save_path);   /* 還原裝備/道具(continue)*/

    SDL_Surface *cv=SDL_CreateRGBSurfaceWithFormat(0,CANVAS_W,CANVAS_H,32,SDL_PIXELFORMAT_RGBA32);
    U2Text title=u2_text_open(font_path,20), body=u2_text_open(font_path,22), small=u2_text_open(font_path,17);
    if (!title.font||!body.font||!small.font){ fprintf(stderr,"字型失敗: %s\n",TTF_GetError()); return 1; }
    SDL_Surface *splash = splash_path ? IMG_Load(splash_path) : NULL;
    SDL_Surface *titleimg = title_path ? IMG_Load(title_path) : NULL;

    /* --screens:渲染選單 + 建角各步驟到 PNG(版面驗證),不進遊戲 */
    if (screens_prefix){
        char out[600];
        const char *opts[]={tr("繼續冒險"),tr("新遊戲(建立角色)"),tr("試玩範例角色"),tr("離開")};
        render_menu(cv,opts,4,1,titleimg,&title,&body);
        snprintf(out,sizeof out,"%smenu.png",screens_prefix); IMG_SavePNG(cv,out);
        Create c; create_init(&c);
        const char *seq="NAME|sex|race|class|stats";  /* 標記用 */
        (void)seq;
        /* 模擬一路填到各步驟 */
        const char *demo="HERO"; for (const char*p=demo;*p;p++) create_feed(&c,0,*p);
        render_create(cv,&c,&title,&body,&small); snprintf(out,sizeof out,"%s1name.png",screens_prefix); IMG_SavePNG(cv,out);
        create_feed(&c,SDLK_RETURN,0); /* →sex */
        render_create(cv,&c,&title,&body,&small); snprintf(out,sizeof out,"%s2sex.png",screens_prefix); IMG_SavePNG(cv,out);
        create_feed(&c,SDLK_RIGHT,0); create_feed(&c,SDLK_RETURN,0); /* 女→race */
        render_create(cv,&c,&title,&body,&small); snprintf(out,sizeof out,"%s3race.png",screens_prefix); IMG_SavePNG(cv,out);
        create_feed(&c,SDLK_RIGHT,0); create_feed(&c,SDLK_RETURN,0); /* 精靈→class */
        render_create(cv,&c,&title,&body,&small); snprintf(out,sizeof out,"%s4class.png",screens_prefix); IMG_SavePNG(cv,out);
        create_feed(&c,SDLK_RIGHT,0); create_feed(&c,SDLK_RETURN,0); /* 牧師→stats */
        create_feed(&c,SDLK_DOWN,0); create_feed(&c,SDLK_RIGHT,0); create_feed(&c,SDLK_RIGHT,0);
        render_create(cv,&c,&title,&body,&small); snprintf(out,sizeof out,"%s5stats.png",screens_prefix); IMG_SavePNG(cv,out);
        /* 新角色存檔 round-trip 驗證 */
        create_feed(&c,SDLK_RETURN,0);   /* 完成建角 */
        U2Save made=make_character(&c);
        char sp[600]; snprintf(sp,sizeof sp,"%snewchar",screens_prefix);
        if (u2_save_store(&made,sp)){
            U2Save rl=u2_save_load(sp);
            printf("建角存檔 round-trip:name=%s sex=%c race=%s class=%s STR=%d INT=%d hp=%d gold=%d\n",
                   rl.name,rl.sex,u2_save_race_name(rl.race),u2_save_class_name(rl.klass),
                   rl.stats[0],rl.stats[5],rl.hp,rl.gold);
        } else printf("建角存檔失敗\n");
        printf("screens 輸出:%smenu/1name/2sex/3race/4class/5stats.png\n",screens_prefix);
        return 0;
    }

    g.mode=MODE_WORLD;
    find_start(&g.map,&g.player,g.world_num);
    g.rng = 1;                                          /* 固定 seed → headless 可重現 */
    g.php = (g.save.ok && g.save.has_character) ? g.save.hp : 400;
    place_ship(&g);                                     /* 起點附近放一艘可登的船 */
    g.td_x=g.td_y=-1; place_time_door(&g);              /* 起點附近放一個時間之門 */
    place_land_vehicles(&g);                            /* 馬 / 飛機 / 火箭(demo)*/
    snprintf(g.msg,sizeof g.msg,tr("歡迎來到 Sosaria,冒險者。"));

    if (headless){
        int step=0; char out[600]; int won_announced=0;
        if (titleimg){ render_title(cv,titleimg,&title,&body);
            snprintf(out,sizeof out,"%stitle.png",out_prefix); IMG_SavePNG(cv,out); }
        if (splash){ render_splash(cv,splash,&title,&body);
            snprintf(out,sizeof out,"%ssplash.png",out_prefix); IMG_SavePNG(cv,out); }
        render_all(cv,&g,&title,&body,&small);
        snprintf(out,sizeof out,"%s%02d.png",out_prefix,step++); IMG_SavePNG(cv,out);
        for (const char *s=script;*s;s++){
            char c=*s, d=norm_dir(c);
            if (d) handle_dir(&g,d);
            else if (c=='C'||c=='c') g.show_sheet=!g.show_sheet;
            else if (c=='H'||c=='h') g.show_help=!g.show_help;   /* F1 指令表(headless) */
            else if (c=='T'||c=='t') do_talk(&g);
            else if (c=='J'||c=='j') dungeon_descend(&g);
            else if (c=='K'||c=='k') dungeon_ascend(&g);
            else if (c=='G'||c=='g'){ if(g.ntset){ g.curset=(g.curset+1)%g.ntset;
                snprintf(g.msg,sizeof g.msg,tr("切換圖塊:%s"),g.tname[g.curset]); } }
            else if (c=='X'||c=='x'){ if (g.mode==MODE_DUNGEON) exit_dungeon(&g); else if (g.in_town) exit_town(&g); }
            else if (c=='B'||c=='b') board_vehicle(&g);
            else if (c=='I'||c=='i'){ g.items=~0u; snprintf(g.msg,sizeof g.msg,tr("(除錯)取得所有關鍵道具。")); }
            else if (c=='Y'||c=='y'){ if (g.in_town) do_yell(&g); else if (g.mode==MODE_SPACE) land_planet(&g); else launch_rocket(&g); }
            else if (c=='V'||c=='v') do_view(&g);   /* VIEW 鳥瞰 */
            else if (c=='U'){ g.items=~0u; g.vehicle=VEH_ROCKET; launch_rocket(&g); }  /* 除錯:直接發射 */
            else if (c=='M') minax_encounter(&g);   /* Minax 對決(傳說時代)*/
            else if (c=='Z'||c=='z'){ if (g.in_town) g.show_shop=!g.show_shop; }
            else if (c>='1'&&c<='9'){ if (g.show_shop) shop_buy(&g,c-'1');
                                      else if (g.mode==MODE_DUNGEON) cast_spell(&g,c-'1'); }   /* 地牢:1-9 施放法術 */
            else if (c=='0'){ if (g.show_shop) shop_buy(&g,9); }   /* 商店第 10 項:習得法術 */
            else if (c=='D') enter_dungeon(&g);
            else if (c=='O') enter_town_tile(&g,5);   /* 強制進城(headless 測試用) */
            else if (c=='P'||c=='p') time_travel(&g); /* 穿越時間之門(快捷/headless) */
            else if (c=='L'||c=='l'){ u2_lang=(U2Lang)((u2_lang+1)%u2_nlang);
                snprintf(g.msg,sizeof g.msg,"%s",lang_label()); }   /* 循環語系(headless) */
            else continue;
            render_all(cv,&g,&title,&body,&small);
            snprintf(out,sizeof out,"%s%02d.png",out_prefix,step++); IMG_SavePNG(cv,out);
            if (g.won && !won_announced){      /* 破關確定性訊號(回歸測試 grep 用) */
                won_announced=1;
                printf("*** GAME WON (Minax defeated) step=%d ***\n",step-1);
            }
        }
        printf("腳本完成:%d 步,輸出 %s00..%02d.png\n",step-1,out_prefix,step-1);
        printf("破關狀態:%s\n", g.won ? "WON" : "NOT-WON");
    } else {
        SDL_Window *win=SDL_CreateWindow("Ultima II 繁中",
            SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,CANVAS_W,CANVAS_H,0);
        SDL_Renderer *ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
        int running=1;
        /* 開場序列:原版標題 → 全家福 splash(各按任意鍵或逾時) */
        if (titleimg){
            render_title(cv,titleimg,&title,&body);
            SDL_Texture *st=SDL_CreateTextureFromSurface(ren,cv);
            SDL_RenderClear(ren); SDL_RenderCopy(ren,st,NULL,NULL); SDL_RenderPresent(ren);
            SDL_DestroyTexture(st);
            Uint32 t0=SDL_GetTicks(); int go=0;
            while (!go && SDL_GetTicks()-t0<5000){
                SDL_Event e;
                while (SDL_PollEvent(&e)) if (e.type==SDL_KEYDOWN||e.type==SDL_QUIT||e.type==SDL_MOUSEBUTTONDOWN) go=1;
                SDL_Delay(16);
            }
        }
        if (splash){
            render_splash(cv,splash,&title,&body);
            SDL_Texture *st=SDL_CreateTextureFromSurface(ren,cv);
            SDL_RenderClear(ren); SDL_RenderCopy(ren,st,NULL,NULL); SDL_RenderPresent(ren);
            SDL_DestroyTexture(st);
            Uint32 t0=SDL_GetTicks(); int go=0;
            while (!go && SDL_GetTicks()-t0<4000){
                SDL_Event e;
                while (SDL_PollEvent(&e)) if (e.type==SDL_KEYDOWN||e.type==SDL_QUIT||e.type==SDL_MOUSEBUTTONDOWN) go=1;
                SDL_Delay(16);
            }
        }
        /* ---- 開場選單:繼續 / 新遊戲 / 試玩範例 / 離開 ---- */
        {
            const char *opts[4]; int code[4], n=0;
            if (have_continue){ opts[n]=tr("繼續冒險"); code[n++]=0; }
            opts[n]=tr("新遊戲(建立角色)"); code[n++]=1;
            if (tmpl.ok && tmpl.has_character){ opts[n]=tr("試玩範例角色"); code[n++]=2; }
            opts[n]=tr("離開"); code[n++]=3;
            int sel=0, chosen=-1;
            while (running && chosen<0){
                render_menu(cv,opts,n,sel,titleimg,&title,&body);
                SDL_Texture *t=SDL_CreateTextureFromSurface(ren,cv);
                SDL_RenderClear(ren); SDL_RenderCopy(ren,t,NULL,NULL); SDL_RenderPresent(ren);
                SDL_DestroyTexture(t);
                SDL_Event e;
                while (SDL_WaitEvent(&e)){
                    if (e.type==SDL_QUIT){ running=0; break; }
                    if (e.type==SDL_KEYDOWN){
                        SDL_Keycode k=e.key.keysym.sym;
                        if (k==SDLK_UP||k==SDLK_w) { sel=(sel+n-1)%n; break; }
                        if (k==SDLK_DOWN||k==SDLK_s){ sel=(sel+1)%n; break; }
                        if (k==SDLK_RETURN||k==SDLK_SPACE){ chosen=code[sel]; break; }
                        if (k==SDLK_ESCAPE){ chosen=3; break; }
                    }
                }
            }
            if (chosen==3) running=0;
            else if (chosen==0) g.save=wsave;
            else if (chosen==2) g.save=tmpl;
            else if (chosen==1){
                /* 原版建角流程 */
                Create c; create_init(&c);
                int done=0;
                while (running && !done){
                    render_create(cv,&c,&title,&body,&small);
                    SDL_Texture *t=SDL_CreateTextureFromSurface(ren,cv);
                    SDL_RenderClear(ren); SDL_RenderCopy(ren,t,NULL,NULL); SDL_RenderPresent(ren);
                    SDL_DestroyTexture(t);
                    SDL_Event e;
                    while (SDL_WaitEvent(&e)){
                        if (e.type==SDL_QUIT){ running=0; break; }
                        if (e.type==SDL_KEYDOWN){
                            SDL_Keycode k=e.key.keysym.sym;
                            char ch=0;
                            if (k>=SDLK_a&&k<=SDLK_z) ch='A'+(k-SDLK_a);
                            else if (k>=SDLK_0&&k<=SDLK_9) ch='0'+(k-SDLK_0);
                            else if (k==SDLK_SPACE) ch=' ';
                            done=create_feed(&c,k,ch);
                            break;
                        }
                    }
                }
                if (done){
                    g.save=make_character(&c);
                    g.items=0; g.weapon=0; g.armour=0;  /* 新角色:無裝備/道具 */
                    u2_save_store(&g.save,save_path);
                    save_meta(&g,save_path);            /* 同步初始 meta */
                }
            }
            g.php=(g.save.ok && g.save.has_character)?g.save.hp:400;
        }
        while (running){
            SDL_Event e;
            while (SDL_PollEvent(&e)){
                if (e.type==SDL_QUIT) running=0;
                else if (e.type==SDL_KEYDOWN){
                    SDL_Keycode k=e.key.keysym.sym; char d=0;
                    switch (k){
                        case SDLK_UP:case SDLK_w: d='N'; break;
                        case SDLK_DOWN:case SDLK_s: d='S'; break;
                        case SDLK_LEFT:case SDLK_a: d='W'; break;
                        case SDLK_RIGHT:case SDLK_d: d='E'; break;
                        case SDLK_c: g.show_sheet=!g.show_sheet; break;
                        case SDLK_F1: g.show_help=!g.show_help; break;
                        case SDLK_F4: u2_lang=(U2Lang)((u2_lang+1)%u2_nlang);
                            snprintf(g.msg,sizeof g.msg,"%s",lang_label()); break;  /* 循環語系 */
                        case SDLK_t: do_talk(&g); break;
                        case SDLK_j: dungeon_descend(&g); break;
                        case SDLK_k: dungeon_ascend(&g); break;
                        case SDLK_b: board_vehicle(&g); break;
                        case SDLK_i: g.items=~0u; snprintf(g.msg,sizeof g.msg,tr("(除錯)取得所有關鍵道具。")); break;
                        case SDLK_y: if (g.in_town) do_yell(&g); else if (g.mode==MODE_SPACE) land_planet(&g); else launch_rocket(&g); break;
                        case SDLK_v: do_view(&g); break;   /* VIEW 鳥瞰 */
                        case SDLK_m: minax_encounter(&g); break;
                        case SDLK_z: if (g.in_town) g.show_shop=!g.show_shop; break;
                        case SDLK_1:case SDLK_2:case SDLK_3:case SDLK_4:
                        case SDLK_5:case SDLK_6:case SDLK_7:case SDLK_8:case SDLK_9:
                            if (g.show_shop) shop_buy(&g,k-SDLK_1);
                            else if (g.mode==MODE_DUNGEON) cast_spell(&g,k-SDLK_1);   /* 地牢:1-9 施放法術 */
                            break;
                        case SDLK_0: if (g.show_shop) shop_buy(&g,9); break;          /* 商店第 10 項:習得法術 */
                        case SDLK_p: time_travel(&g); break;   /* 穿越時間之門快捷 */
                        case SDLK_g: if(g.ntset){ g.curset=(g.curset+1)%g.ntset;
                            snprintf(g.msg,sizeof g.msg,tr("切換圖塊:%s"),g.tname[g.curset]); } break;
                        case SDLK_x:
                            if (g.mode==MODE_DUNGEON) exit_dungeon(&g);
                            else if (g.in_town) exit_town(&g);
                            break;
                        case SDLK_q:case SDLK_ESCAPE: running=0; break;
                    }
                    if (d) handle_dir(&g,d);
                }
            }
            render_all(cv,&g,&title,&body,&small);
            SDL_Texture *tex=SDL_CreateTextureFromSurface(ren,cv);
            SDL_RenderClear(ren); SDL_RenderCopy(ren,tex,NULL,NULL); SDL_RenderPresent(ren);
            SDL_DestroyTexture(tex);
            SDL_Delay(16);
        }
        SDL_DestroyRenderer(ren); SDL_DestroyWindow(win);
    }

    /* 離開時把執行時狀態(HP/EXP/GOLD…)寫回可寫存檔 + 裝備/道具 meta */
    if (g.save.ok && g.save.has_character){
        g.save.hp = g.php;
        if (u2_save_store(&g.save, save_path))
            printf("已存檔:%s\n", save_path);
        else
            fprintf(stderr, "存檔失敗:%s(%s)\n", save_path, strerror(errno));
        save_meta(&g, save_path);
    }

    u2_text_close(&title); u2_text_close(&body); u2_text_close(&small);
    for (int i=0;i<g.ntset;i++) SDL_FreeSurface(g.tset[i]);
    if (splash) SDL_FreeSurface(splash);
    if (titleimg) SDL_FreeSurface(titleimg);
    SDL_FreeSurface(cv);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
    return 0;
}
