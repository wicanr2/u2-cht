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

#define CANVAS_W   960
#define CANVAS_H   600
#define HDR_H      30
#define TILE_PX    48
#define VIEW_COLS  19
#define VIEW_ROWS  8
#define MAP_OX     24
#define MAP_OY     (HDR_H + 8)
#define PLAYER_TILE 16
#define WORLD_DUNGEON_TILE 9      /* 地面上的地牢入口 tile id */
#define WORLD_TOWN_TILE 5         /* 地面上的城鎮入口 tile id */
#define DVIEW 520                 /* 地牢線框視區邊長 */

enum Mode { MODE_WORLD, MODE_DUNGEON };

typedef struct {
    enum Mode mode;
    int show_sheet;
    int show_help;                /* F1 指令表疊加 */
    U2Map map; U2Mon mon;         /* 地面(overworld) */
    U2Player player;
    /* 城鎮 */
    int in_town;
    U2Map town; U2Mon tmon; U2Talk talk; int town_ok;
    int tret_x, tret_y;           /* 進城前的地面座標 */
    char town_path[512];
    char data_dir[512];           /* mapxNN 所在目錄(動態組地點檔路徑) */
    char town_loaded[8];          /* 目前已載入的城鎮編號(偵測換城重載) */
    /* 地牢 */
    U2Dungeon dg; int dg_ok;
    int dx, dy, ddir, dlevel;     /* 地牢內位置 / 朝向 / 樓層 */
    int ret_x, ret_y;             /* 進地牢前的座標 */
    char dungeon_path[512];
    /* 翻譯 / 存檔 */
    U2Strings ui; U2Strings tr;   /* ui=狀態標籤;tr=對話譯文 */
    U2Save save;
    /* 可切換 tileset(參考 u3-cht 多平台 tileset 清單) */
    SDL_Surface *tset[8]; char tname[8][24]; int ntset, curset;
    /* 地面隨機遭遇 / 戰鬥(U2 overworld 怪物動態生成) */
    struct { int x, y, hp, maxhp, atk; unsigned char tile; const char *name; } mob[8];
    int nmob, php, turn;
    unsigned int rng;             /* 簡易 LCG,determinism 供 headless 驗證 */
    int vehicle;                  /* 0=步行 1=船(frigate) */
    char msg[200];
} Game;
#define SHIP_TILE 18              /* 地圖上的船 tile id */

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

/* 世界圖 landmark 判定(forward 宣告於下方 town_map_num) */
static const char *town_map_num(unsigned char t);

static void find_start(const U2Map *m, U2Player *p)
{
    /* 優先:落在某個城鎮 landmark 旁的可通行格,讓玩家一開始就走得進城 */
    for (int y=0; y<U2_MAP_H; y++)
        for (int x=0; x<U2_MAP_W; x++){
            if (!town_map_num(u2_map_tile(m,x,y))) continue;
            int NX[4]={0,0,1,-1}, NY[4]={1,-1,0,0};
            for (int k=0;k<4;k++){
                int ax=x+NX[k], ay=y+NY[k];
                if (ax<0||ay<0||ax>=U2_MAP_W||ay>=U2_MAP_H) continue;
                unsigned char at=u2_map_tile(m,ax,ay);
                if (u2_passable(at) && !town_map_num(at)){
                    p->x=ax; p->y=ay; p->tile=PLAYER_TILE; return;
                }
            }
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
        char label[64];
        if (zh){ size_t k=0; for(const char*q=zh;*q&&*q!='='&&k<sizeof label-1;q++)label[k++]=*q; label[k]=0; }
        else snprintf(label,sizeof label,"%s",st[i].orig);
        char val[16]; if (st[i].val>=0) snprintf(val,sizeof val,"%04d",st[i].val);
        else snprintf(val,sizeof val,"----");
        char line[96]; snprintf(line,sizeof line,"%s  %s",label,val);
        u2_text_draw(cv, body, line, x0, y0, 235,225,150); y0+=30;
    }
}

static void render_world(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body)
{
    U2Map *m=amap(g); U2Mon *mon=amon(g);
    SDL_Surface *tiles = g->ntset ? g->tset[g->curset] : NULL;
    SDL_FillRect(cv, NULL, SDL_MapRGB(cv->format,0,0,0));
    SDL_Rect hdr={0,0,CANVAS_W,HDR_H};
    SDL_FillRect(cv,&hdr,SDL_MapRGB(cv->format,36,44,110));
    u2_text_draw(cv,title, g->in_town ? "Ultima II — 城鎮(T 交談 · X 離開)"
                                      : "Ultima II:女巫的復仇 — 繁體中文",
                 10,4,235,235,245);

    int cam_x=g->player.x-VIEW_COLS/2, cam_y=g->player.y-VIEW_ROWS/2;
    clampi(&cam_x,0,U2_MAP_W-VIEW_COLS); clampi(&cam_y,0,U2_MAP_H-VIEW_ROWS);
    u2_render_viewport(cv,m,tiles,cam_x,cam_y,VIEW_COLS,VIEW_ROWS,TILE_PX,MAP_OX,MAP_OY);
    u2_render_entities(cv,mon,tiles,cam_x,cam_y,VIEW_COLS,VIEW_ROWS,TILE_PX,MAP_OX,MAP_OY);

    /* 動態怪物(地面遭遇) */
    if (!g->in_town && tiles)
        for (int i=0;i<g->nmob;i++){
            int mx=g->mob[i].x-cam_x, my=g->mob[i].y-cam_y;
            if (mx<0||my<0||mx>=VIEW_COLS||my>=VIEW_ROWS) continue;
            u2_tileset_blit(cv,tiles,g->mob[i].tile,
                            MAP_OX+mx*TILE_PX, MAP_OY+my*TILE_PX, TILE_PX);
        }

    int px=MAP_OX+(g->player.x-cam_x)*TILE_PX, py=MAP_OY+(g->player.y-cam_y)*TILE_PX;
    if (tiles) u2_tileset_blit(cv,tiles,g->player.tile,px,py,TILE_PX);
    Uint32 col=SDL_MapRGB(cv->format,250,240,90);
    SDL_Rect t={px,py,TILE_PX,2},b={px,py+TILE_PX-2,TILE_PX,2},
             l={px,py,2,TILE_PX},rr={px+TILE_PX-2,py,2,TILE_PX};
    SDL_FillRect(cv,&t,col);SDL_FillRect(cv,&b,col);SDL_FillRect(cv,&l,col);SDL_FillRect(cv,&rr,col);

    int by=MAP_OY+VIEW_ROWS*TILE_PX+10;
    u2_text_draw(cv,body, g->in_town ? "方向鍵/WASD 移動 · T 交談 · C 角色表 · X 離開 · F1 指令表"
                                     : "方向鍵/WASD 移動 · B 登船 · G 畫風 · F1 指令表 · Q 離開",
                 MAP_OX,by,150,175,205);
    u2_text_draw(cv,body,g->msg,MAP_OX,by+30,210,225,205);
    char pos[96]; snprintf(pos,sizeof pos,"座標 (%d, %d)  地形 id=%d  圖塊 %s(G 切換)",
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
    u2_text_draw(cv,title,"地牢 — 第一人稱線框",10,4,235,235,245);

    int depth=u2_dungeon_render(cv,&g->dg,g->dlevel,g->dx,g->dy,g->ddir,24,MAP_OY,DVIEW,DVIEW);

    int rx=24+DVIEW+24, ry=MAP_OY+6; char ln[96];
    u2_text_draw(cv,body,"狀態",rx,ry,150,175,205); ry+=34;
    snprintf(ln,sizeof ln,"座標: (%d, %d)",g->dx,g->dy);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=30;
    snprintf(ln,sizeof ln,"朝向: %s",DIR_ZH[g->ddir]);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=30;
    snprintf(ln,sizeof ln,"樓層: %d / 共 %d 層",g->dlevel+1,g->dg.levels);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=30;
    snprintf(ln,sizeof ln,"前方可見深度: %d",depth);
    u2_text_draw(cv,body,ln,rx,ry,225,225,230); ry+=40;
    draw_status_panel(cv,body,&g->ui,&g->save,rx,ry); ry+=4*30+10;

    int by=MAP_OY+DVIEW+12;
    u2_text_draw(cv,small,"N 前進 · S 後退 · W 左轉 · E 右轉 · J 下樓 · K 上樓 · X 離開 · C 角色表",
        24,by,150,170,200);
    u2_text_draw(cv,small,g->msg,24,by+26,210,225,205);
}

/* F1 指令表疊加面板(置中) */
static void render_help_overlay(SDL_Surface *cv, U2Text *body, U2Text *small)
{
    static const char *ROWS[] = {
        "方向鍵 / WASD   移動 / 攻擊(朝怪物移動即攻擊)",
        "B               登船 / 下船(站在船旁)",
        "走上城堡圖塊     進入城鎮",
        "走上地牢圖塊     進入地牢",
        "T               城鎮中與 NPC 交談",
        "J / K           地牢中 下樓 / 上樓",
        "X               離開城鎮 / 地牢",
        "C               角色資料表",
        "G               切換畫風(EGA / FM Towns…)",
        "F1              顯示 / 關閉本指令表",
        "Q / Esc         離開遊戲(自動存檔)",
    };
    int nrow = (int)(sizeof ROWS / sizeof ROWS[0]);
    int pw=560, ph=70+nrow*30, x=(CANVAS_W-pw)/2, y=(CANVAS_H-ph)/2;
    SDL_Rect bg={x,y,pw,ph}; SDL_FillRect(cv,&bg,SDL_MapRGB(cv->format,18,22,40));
    SDL_Rect bar={x,y,pw,40}; SDL_FillRect(cv,&bar,SDL_MapRGB(cv->format,40,50,120));
    Uint32 fr=SDL_MapRGB(cv->format,120,140,200);
    SDL_Rect e1={x,y,pw,2},e2={x,y+ph-2,pw,2},e3={x,y,2,ph},e4={x+pw-2,y,2,ph};
    SDL_FillRect(cv,&e1,fr);SDL_FillRect(cv,&e2,fr);SDL_FillRect(cv,&e3,fr);SDL_FillRect(cv,&e4,fr);
    u2_text_draw(cv,body,"指令表(F1 關閉)",x+16,y+8,235,235,245);
    int iy=y+52;
    for (int i=0;i<nrow;i++){ u2_text_draw(cv,small,ROWS[i],x+24,iy,215,220,230); iy+=30; }
}

/* 角色表疊加面板(置中) */
static void render_sheet_overlay(SDL_Surface *cv, Game *g, U2Text *body, U2Text *small)
{
    int pw=440, ph=420, x=(CANVAS_W-pw)/2, y=(CANVAS_H-ph)/2;
    SDL_Rect bg={x,y,pw,ph}; SDL_FillRect(cv,&bg,SDL_MapRGB(cv->format,18,22,40));
    SDL_Rect bar={x,y,pw,40}; SDL_FillRect(cv,&bar,SDL_MapRGB(cv->format,40,50,120));
    /* 邊框 */
    Uint32 fr=SDL_MapRGB(cv->format,120,140,200);
    SDL_Rect e1={x,y,pw,2},e2={x,y+ph-2,pw,2},e3={x,y,2,ph},e4={x+pw-2,y,2,ph};
    SDL_FillRect(cv,&e1,fr);SDL_FillRect(cv,&e2,fr);SDL_FillRect(cv,&e3,fr);SDL_FillRect(cv,&e4,fr);

    u2_text_draw(cv,body,"角色資料(C 關閉)",x+16,y+8,235,235,245);
    U2Save *s=&g->save; int ix=x+28, iy=y+56, lh=30; char ln[96];
    if (!s->ok || !s->has_character){
        u2_text_draw(cv,body,"(無 player 存檔可顯示)",ix,iy,200,180,120);
        return;
    }
    snprintf(ln,sizeof ln,"姓名: %s",s->name); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh;
    snprintf(ln,sizeof ln,"性別: %s",s->sex=='F'?"女":"男"); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh;
    snprintf(ln,sizeof ln,"種族: %s",u2_save_race_zh(s->race)); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh;
    snprintf(ln,sizeof ln,"職業: %s",u2_save_class_zh(s->klass)); u2_text_draw(cv,body,ln,ix,iy,230,230,235); iy+=lh+6;
    u2_text_draw(cv,small,"屬性(BCD 解碼)",ix,iy,150,170,205); iy+=26;
    for (int i=0;i<U2_NUM_STATS;i++){
        char lab[64]; snprintf(lab,sizeof lab,"%s %s",u2_save_stat_zh(i),u2_save_stat_name(i));
        u2_text_draw(cv,body,lab,ix,iy,210,215,225);
        char v[8]; snprintf(v,sizeof v,"%2d",s->stats[i]);
        u2_text_draw(cv,body,v,ix+170,iy,245,225,150); iy+=lh;
    }
}

static void render_all(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body, U2Text *small)
{
    if (g->mode==MODE_WORLD) render_world(cv,g,title,body);
    else                     render_dungeon(cv,g,title,body,small);
    if (g->show_sheet)       render_sheet_overlay(cv,g,body,small);
    if (g->show_help)        render_help_overlay(cv,body,small);
}

/* 進地牢:載入地牢檔,設定入口 */
static void enter_dungeon(Game *g)
{
    if (!g->dg_ok){
        g->dg = u2_dungeon_load(g->dungeon_path);
        g->dg_ok = g->dg.ok;
    }
    if (!g->dg_ok){ snprintf(g->msg,sizeof g->msg,"找不到地牢資料,無法進入。"); return; }
    g->ret_x=g->player.x; g->ret_y=g->player.y;
    g->dlevel=0;
    dungeon_entry(&g->dg,g->dlevel,&g->dx,&g->dy,&g->ddir);
    g->mode=MODE_DUNGEON;
    snprintf(g->msg,sizeof g->msg,"你踏入了黑暗的地牢…");
}

/* 下樓:站在下梯(&0x20)才生效 */
static void dungeon_descend(Game *g)
{
    if (g->mode!=MODE_DUNGEON) return;
    if (u2_dungeon_ladder(&g->dg,g->dlevel,g->dx,g->dy)!=+1){
        snprintf(g->msg,sizeof g->msg,"腳下沒有向下的樓梯。"); return; }
    if (g->dlevel+1>=g->dg.levels){ snprintf(g->msg,sizeof g->msg,"已是最底層。"); return; }
    g->dlevel++;
    if (u2_dungeon_is_wall(&g->dg,g->dlevel,g->dx,g->dy))
        dungeon_entry(&g->dg,g->dlevel,&g->dx,&g->dy,&g->ddir);
    snprintf(g->msg,sizeof g->msg,"你沿樓梯往下,來到第 %d 層。",g->dlevel+1);
}

/* 上樓:站在上梯(&0x10)才生效;最頂層再上則離開地牢 */
static void dungeon_ascend(Game *g)
{
    if (g->mode!=MODE_DUNGEON) return;
    if (u2_dungeon_ladder(&g->dg,g->dlevel,g->dx,g->dy)!=-1){
        snprintf(g->msg,sizeof g->msg,"腳下沒有向上的樓梯。"); return; }
    if (g->dlevel==0){
        g->mode=MODE_WORLD; g->player.x=g->ret_x; g->player.y=g->ret_y;
        snprintf(g->msg,sizeof g->msg,"你沿樓梯回到了地面。"); return;
    }
    g->dlevel--;
    if (u2_dungeon_is_wall(&g->dg,g->dlevel,g->dx,g->dy))
        dungeon_entry(&g->dg,g->dlevel,&g->dx,&g->dy,&g->ddir);
    snprintf(g->msg,sizeof g->msg,"你沿樓梯往上,來到第 %d 層。",g->dlevel+1);
}

static void exit_dungeon(Game *g)
{
    g->mode=MODE_WORLD;
    g->player.x=g->ret_x; g->player.y=g->ret_y;
    snprintf(g->msg,sizeof g->msg,"你回到了地面。");
}

/* 世界圖 landmark tile → 地點地圖編號(均挑有 NPC + 對話 tlk 的城鎮)。
 * 註:U2 真實 landmark→map 對照需 oracle 校正;此處為 demo 指派,確保每個
 * 入口都通往一張有居民、可交談的城鎮。回 NULL = 非城鎮 landmark。 */
static const char *town_map_num(unsigned char t)
{
    switch (t){
        case 5:  return "21";
        case 6:  return "22";
        case 7:  return "23";
        case 8:  return "31";
        case 10: return "32";
    }
    return NULL;
}

/* 進城:依世界圖 landmark tile 載入對應城鎮地圖 + 實體 + 對話 */
static void enter_town_tile(Game *g, unsigned char wtile)
{
    const char *num = town_map_num(wtile);
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
    if (!g->town_ok){ snprintf(g->msg,sizeof g->msg,"找不到城鎮資料,無法進入。"); return; }
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
    snprintf(g->msg,sizeof g->msg,"你進入了城鎮。");
}

static void exit_town(Game *g)
{
    g->in_town=0;
    g->player.x=g->tret_x; g->player.y=g->tret_y;
    snprintf(g->msg,sizeof g->msg,"你離開了城鎮。");
}

/* 交談:鄰格若有 NPC 實體,顯示其 tlkx 對話(查翻譯覆蓋層) */
static void do_talk(Game *g)
{
    if (!g->in_town){ snprintf(g->msg,sizeof g->msg,"這裡沒有人可以交談。"); return; }
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};
    for (int d=0; d<4; d++){
        int nx=g->player.x+FX[d], ny=g->player.y+FY[d];
        for (int i=0;i<g->tmon.count;i++){
            U2Entity *e=&g->tmon.ent[i];
            if (!e->tile || e->x!=nx || e->y!=ny) continue;
            /* dlg & 0x80 = 可交談;行索引 = (dlg&0x7f)-1 (1-based 進 tlkx) */
            if (!(e->dlg & 0x80)){ snprintf(g->msg,sizeof g->msg,"對方沉默不語。"); return; }
            int k=(e->dlg & 0x7f) - 1;
            if (k<0 || k>=g->talk.count){ snprintf(g->msg,sizeof g->msg,"對方欲言又止。"); return; }
            const char *zh=u2_strings_lookup(&g->tr, g->talk.line[k]);
            const char *disp=zh?zh:g->talk.line[k];
            char one[180]; size_t j=0;
            for (const char *p=disp; *p && j<sizeof one-1; p++) one[j++]=(*p=='\r')?' ':*p;
            one[j]=0;
            snprintf(g->msg,sizeof g->msg,"「%s」",one);
            return;
        }
    }
    snprintf(g->msg,sizeof g->msg,"附近沒有人可以交談。");
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
    return ((str + 8*1) >> 2) + (rng_next(g) % 8) + 4;  /* ~13-21 */
}
/* 視野邊緣可通行格生成怪物(~28%/回合) */
static void spawn_mob(Game *g)
{
    if (g->nmob >= 8 || g->in_town || g->mode != MODE_WORLD) return;
    if (rng_next(g) % 100 >= 28) return;
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
        snprintf(g->msg,sizeof g->msg,"%s出現了!",nm);
        return;
    }
}
/* 怪物朝玩家移動;貼身則以各自攻擊力打玩家 */
static void step_mobs(Game *g)
{
    for (int i=0;i<g->nmob;i++){
        int dx=g->player.x-g->mob[i].x, dy=g->player.y-g->mob[i].y;
        if (abs(dx)+abs(dy)==1){
            int dmg=g->mob[i].atk + (rng_next(g)%4); g->php-=dmg; if(g->php<0)g->php=0;
            snprintf(g->msg,sizeof g->msg,"%s攻擊你!失去 %d 點生命。",g->mob[i].name,dmg);
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
            if ((int)(rng_next(g) % 0x50) >= hit_skill(g)){
                snprintf(g->msg,sizeof g->msg,"你攻擊%s,但沒打中。",g->mob[i].name);
                return 1;
            }
            int dmg=player_dmg(g); g->mob[i].hp-=dmg;
            if (g->mob[i].hp<=0){
                int xp=(rng_next(g)&3)+1;            /* oracle 地面 EXP +(rng&3)+1 */
                g->save.exp += xp;
                snprintf(g->msg,sizeof g->msg,"你擊敗了%s!(+%d 經驗)",g->mob[i].name,xp);
                g->mob[i]=g->mob[--g->nmob];
            } else snprintf(g->msg,sizeof g->msg,"你擊中%s,造成 %d 傷害(剩 %d)。",
                            g->mob[i].name,dmg,g->mob[i].hp);
            return 1;
        }
    }
    return 0;
}

/* 在玩家附近的水域放一艘船供登船示範;不移動玩家(保留城旁起點)。
 * 優先放在玩家相鄰水格(可直接 B 登船),否則放最近的水格(玩家走過去)。 */
static void place_ship(Game *g)
{
    int NX[4]={0,1,0,-1}, NY[4]={-1,0,1,0};
    /* 先試:玩家四鄰有水 → 直接放船 */
    for (int d=0;d<4;d++){
        int x=g->player.x+NX[d], y=g->player.y+NY[d];
        if (x<1||y<1||x>=U2_MAP_W-1||y>=U2_MAP_H-1) continue;
        if (u2_map_tile(&g->map,x,y)==0){ g->map.tile[y][x]=SHIP_TILE; return; }
    }
    /* 否則:向外找最近水格(玩家自行走到船邊) */
    for (int r=1;r<24;r++)
        for (int dy=-r;dy<=r;dy++) for (int dx=-r;dx<=r;dx++){
            if (abs(dx)<r && abs(dy)<r) continue;
            int x=g->player.x+dx, y=g->player.y+dy;
            if (x<1||y<1||x>=U2_MAP_W-1||y>=U2_MAP_H-1) continue;
            if (u2_map_tile(&g->map,x,y)==0){ g->map.tile[y][x]=SHIP_TILE; return; }
        }
}
/* B:登船(腳下/相鄰 ship tile)或下船(在船上→相鄰陸地) */
static void board_ship(Game *g)
{
    if (g->mode!=MODE_WORLD || g->in_town){ snprintf(g->msg,sizeof g->msg,"這裡無法登船。"); return; }
    int NX[4]={0,1,0,-1}, NY[4]={-1,0,1,0};
    if (g->vehicle==1){                                       /* 下船 */
        for (int d=0;d<4;d++){
            int x=g->player.x+NX[d], y=g->player.y+NY[d];
            if (x<0||y<0||x>=U2_MAP_W||y>=U2_MAP_H) continue;
            unsigned char t=u2_map_tile(&g->map,x,y);
            if (t!=0 && t!=1 && u2_passable(t)){
                g->map.tile[g->player.y][g->player.x]=SHIP_TILE;  /* 留船在水面 */
                g->player.x=x; g->player.y=y; g->vehicle=0; g->player.tile=PLAYER_TILE;
                snprintf(g->msg,sizeof g->msg,"你上岸了,船停在水邊。"); return;
            }
        }
        snprintf(g->msg,sizeof g->msg,"附近沒有陸地可上岸。"); return;
    }
    /* 登船:腳下是船 */
    if (u2_map_tile(&g->map,g->player.x,g->player.y)==SHIP_TILE){
        g->vehicle=1; g->player.tile=SHIP_TILE;
        snprintf(g->msg,sizeof g->msg,"你登上了船,可在水上航行。"); return;
    }
    /* 相鄰是船 → 走過去登船 */
    for (int d=0;d<4;d++){
        int x=g->player.x+NX[d], y=g->player.y+NY[d];
        if (x<0||y<0||x>=U2_MAP_W||y>=U2_MAP_H) continue;
        if (u2_map_tile(&g->map,x,y)==SHIP_TILE){
            g->map.tile[y][x]=0; g->player.x=x; g->player.y=y;
            g->vehicle=1; g->player.tile=SHIP_TILE;
            snprintf(g->msg,sizeof g->msg,"你登上了船,可在水上航行。"); return;
        }
    }
    snprintf(g->msg,sizeof g->msg,"附近沒有船(船在水邊,走近後按 B)。");
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

/* 依模式處理一個方向鍵(dir ∈ N/S/E/W) */
static void handle_dir(Game *g, char dir)
{
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};  /* N E S W */
    if (g->mode==MODE_WORLD){
        /* 朝向怪物移動 = 攻擊(不移動);否則正常走 + 觸發怪物回合 */
        int di = (dir=='N')?0:(dir=='E')?1:(dir=='S')?2:3;
        int tx=g->player.x+FX[di], ty=g->player.y+FY[di];
        if (!g->in_town && attack_mob(g,tx,ty)){ step_mobs(g); return; }
        /* 載具感知移動:船=水域可走,步行=陸地;城鎮一律步行 */
        U2Map *am=amap(g);
        int inb = tx>=0&&ty>=0&&tx<U2_MAP_W&&ty<U2_MAP_H;
        unsigned char tt = inb ? u2_map_tile(am,tx,ty) : 0;
        int pass;
        if (!g->in_town && g->vehicle==1) pass = inb && (tt==0||tt==1);   /* 船:水域 */
        else pass = inb && u2_passable(tt);                               /* 步行 */
        if (pass){
            g->player.x=tx; g->player.y=ty;
            snprintf(g->msg,sizeof g->msg, g->vehicle?"航行 %c。":"往 %c 移動。",dir);
            unsigned char t=u2_map_tile(am,g->player.x,g->player.y);
            if (!g->in_town && t==WORLD_DUNGEON_TILE) { g->nmob=0; enter_dungeon(g); }
            else if (!g->in_town && town_map_num(t)) { g->nmob=0; enter_town_tile(g,t); }
            else if (!g->in_town){ g->turn++; step_mobs(g); spawn_mob(g); }
        } else snprintf(g->msg,sizeof g->msg,
                        (!g->in_town&&g->vehicle&&tt!=0&&tt!=1)?"船無法駛上陸地(B 下船)。":"%c 方向被擋住。",dir);
    } else { /* DUNGEON: N前進 S後退 W左轉 E右轉 */
        if (dir=='W'){ g->ddir=(g->ddir+3)&3; snprintf(g->msg,sizeof g->msg,"左轉。"); }
        else if (dir=='E'){ g->ddir=(g->ddir+1)&3; snprintf(g->msg,sizeof g->msg,"右轉。"); }
        else {
            int s=(dir=='N')?1:-1;
            int di=g->ddir; int nx=g->dx+FX[di]*s, ny=g->dy+FY[di]*s;
            if (!u2_dungeon_is_wall(&g->dg,g->dlevel,nx,ny)){ g->dx=nx; g->dy=ny;
                int lad=u2_dungeon_ladder(&g->dg,g->dlevel,nx,ny);
                if (lad>0) snprintf(g->msg,sizeof g->msg,"腳下有向下的樓梯(J 下樓)。");
                else if (lad<0) snprintf(g->msg,sizeof g->msg,"腳下有向上的樓梯(K 上樓)。");
                else snprintf(g->msg,sizeof g->msg, s>0?"前進。":"後退。");
            }
            else snprintf(g->msg,sizeof g->msg,"前方是牆。");
        }
    }
}

static char norm_dir(char c)
{
    switch (c){ case 'w':case 'N': return 'N'; case 's':case 'S': return 'S';
                case 'a':case 'W': return 'W'; case 'd':case 'E': return 'E'; }
    return 0;
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
    for (int i=5;i<argc;i++){
        if (!strcmp(argv[i],"--script") && i+2<argc){ script=argv[i+1]; out_prefix=argv[i+2]; i+=2; }
        else if (!strcmp(argv[i],"--splash") && i+1<argc){ splash_path=argv[i+1]; i+=1; }
        else if (!strcmp(argv[i],"--title") && i+1<argc){ title_path=argv[i+1]; i+=1; }
        else if (!player_save) player_save=argv[i];
    }
    int headless=(script!=NULL);

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
    /* 對話譯文:由 ui_tsv 同目錄推 talk_dialogue.tsv */
    if (ui_tsv){
        char tt[512]; snprintf(tt,sizeof tt,"%s",ui_tsv);
        char *e=strrchr(tt,'/'); e=e?e+1:tt;
        snprintf(e,sizeof tt-(e-tt),"talk_dialogue.tsv");
        g.tr = u2_strings_load(tt,2,3);
    }
    if (player_save) g.save = u2_save_load(player_save);

    SDL_Surface *cv=SDL_CreateRGBSurfaceWithFormat(0,CANVAS_W,CANVAS_H,32,SDL_PIXELFORMAT_RGBA32);
    U2Text title=u2_text_open(font_path,20), body=u2_text_open(font_path,22), small=u2_text_open(font_path,17);
    if (!title.font||!body.font||!small.font){ fprintf(stderr,"字型失敗: %s\n",TTF_GetError()); return 1; }
    SDL_Surface *splash = splash_path ? IMG_Load(splash_path) : NULL;
    SDL_Surface *titleimg = title_path ? IMG_Load(title_path) : NULL;

    g.mode=MODE_WORLD;
    find_start(&g.map,&g.player);
    g.rng = 1;                                          /* 固定 seed → headless 可重現 */
    g.php = (g.save.ok && g.save.has_character) ? g.save.hp : 400;
    place_ship(&g);                                     /* 起點附近放一艘可登的船 */
    snprintf(g.msg,sizeof g.msg,"歡迎來到 Sosaria,冒險者。");

    if (headless){
        int step=0; char out[600];
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
                snprintf(g.msg,sizeof g.msg,"切換圖塊:%s",g.tname[g.curset]); } }
            else if (c=='X'||c=='x'){ if (g.mode==MODE_DUNGEON) exit_dungeon(&g); else if (g.in_town) exit_town(&g); }
            else if (c=='B'||c=='b') board_ship(&g);
            else if (c=='D') enter_dungeon(&g);
            else if (c=='O') enter_town_tile(&g,5);   /* 強制進城(headless 測試用) */
            else continue;
            render_all(cv,&g,&title,&body,&small);
            snprintf(out,sizeof out,"%s%02d.png",out_prefix,step++); IMG_SavePNG(cv,out);
        }
        printf("腳本完成:%d 步,輸出 %s00..%02d.png\n",step-1,out_prefix,step-1);
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
                        case SDLK_t: do_talk(&g); break;
                        case SDLK_j: dungeon_descend(&g); break;
                        case SDLK_k: dungeon_ascend(&g); break;
                        case SDLK_b: board_ship(&g); break;
                        case SDLK_g: if(g.ntset){ g.curset=(g.curset+1)%g.ntset;
                            snprintf(g.msg,sizeof g.msg,"切換圖塊:%s",g.tname[g.curset]); } break;
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

    /* 離開時把執行時狀態(HP/EXP/GOLD…)寫回 player 存檔 */
    if (player_save && g.save.ok && g.save.has_character){
        g.save.hp = g.php;
        if (u2_save_store(&g.save, player_save))
            printf("已存檔:%s\n", player_save);
        else
            fprintf(stderr, "存檔失敗:%s(%s)\n", player_save, strerror(errno));
    }

    u2_text_close(&title); u2_text_close(&body); u2_text_close(&small);
    for (int i=0;i<g.ntset;i++) SDL_FreeSurface(g.tset[i]);
    if (splash) SDL_FreeSurface(splash);
    if (titleimg) SDL_FreeSurface(titleimg);
    SDL_FreeSurface(cv);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
    return 0;
}
