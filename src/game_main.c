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
    U2Map map; U2Mon mon;         /* 地面(overworld) */
    U2Player player;
    /* 城鎮 */
    int in_town;
    U2Map town; U2Mon tmon; U2Talk talk; int town_ok;
    int tret_x, tret_y;           /* 進城前的地面座標 */
    char town_path[512];
    /* 地牢 */
    U2Dungeon dg; int dg_ok;
    int dx, dy, ddir, dlevel;     /* 地牢內位置 / 朝向 / 樓層 */
    int ret_x, ret_y;             /* 進地牢前的座標 */
    char dungeon_path[512];
    /* 翻譯 / 存檔 */
    U2Strings ui; U2Strings tr;   /* ui=狀態標籤;tr=對話譯文 */
    U2Save save;
    char msg[200];
} Game;

/* 目前作用中的地圖 / 實體層(城鎮 or 地面) */
static U2Map *amap(Game *g) { return g->in_town ? &g->town : &g->map; }
static U2Mon *amon(Game *g) { return g->in_town ? &g->tmon : &g->mon; }

static void clampi(int *v, int lo, int hi) { if (*v<lo)*v=lo; if (*v>hi)*v=hi; }

static void find_start(const U2Map *m, U2Player *p)
{
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

static void render_world(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body, SDL_Surface *tiles)
{
    U2Map *m=amap(g); U2Mon *mon=amon(g);
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

    int px=MAP_OX+(g->player.x-cam_x)*TILE_PX, py=MAP_OY+(g->player.y-cam_y)*TILE_PX;
    if (tiles) u2_tileset_blit(cv,tiles,g->player.tile,px,py,TILE_PX);
    Uint32 col=SDL_MapRGB(cv->format,250,240,90);
    SDL_Rect t={px,py,TILE_PX,2},b={px,py+TILE_PX-2,TILE_PX,2},
             l={px,py,2,TILE_PX},rr={px+TILE_PX-2,py,2,TILE_PX};
    SDL_FillRect(cv,&t,col);SDL_FillRect(cv,&b,col);SDL_FillRect(cv,&l,col);SDL_FillRect(cv,&rr,col);

    int by=MAP_OY+VIEW_ROWS*TILE_PX+10;
    u2_text_draw(cv,body, g->in_town ? "方向鍵/WASD 移動 · T 交談 · C 角色表 · X 離開城鎮"
                                     : "方向鍵/WASD 移動 · C 角色表 · Q 離開",
                 MAP_OX,by,150,175,205);
    u2_text_draw(cv,body,g->msg,MAP_OX,by+30,210,225,205);
    char pos[64]; snprintf(pos,sizeof pos,"座標 (%d, %d)  地形 id=%d",
        g->player.x,g->player.y,u2_map_tile(m,g->player.x,g->player.y));
    u2_text_draw(cv,body,pos,MAP_OX,by+60,150,165,150);
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

static void render_all(SDL_Surface *cv, Game *g, U2Text *title, U2Text *body,
                       U2Text *small, SDL_Surface *tiles)
{
    if (g->mode==MODE_WORLD) render_world(cv,g,title,body,tiles);
    else                     render_dungeon(cv,g,title,body,small);
    if (g->show_sheet)       render_sheet_overlay(cv,g,body,small);
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

/* 進城:載入城鎮地圖 + 實體 + 對話 */
static void enter_town(Game *g)
{
    if (!g->town_ok){
        g->town = u2_map_load(g->town_path);
        char mp[512]; snprintf(mp,sizeof mp,"%s",g->town_path);
        char *b=strrchr(mp,'/'); b=b?b+1:mp; char *p=strstr(b,"map"); if(p)memcpy(p,"mon",3);
        g->tmon = u2_mon_load(mp);
        char tp[512]; snprintf(tp,sizeof tp,"%s",g->town_path);
        b=strrchr(tp,'/'); b=b?b+1:tp; p=strstr(b,"map"); if(p)memcpy(p,"tlk",3);
        g->talk = u2_talk_load(tp);
        g->town_ok = g->town.ok;
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

/* 依模式處理一個方向鍵(dir ∈ N/S/E/W) */
static void handle_dir(Game *g, char dir)
{
    int FX[4]={0,1,0,-1}, FY[4]={-1,0,1,0};  /* N E S W */
    if (g->mode==MODE_WORLD){
        int moved=u2_player_move(&g->player,amap(g),dir);
        snprintf(g->msg,sizeof g->msg, moved?"往 %c 移動。":"%c 方向被擋住。",dir);
        if (moved){
            unsigned char t=u2_map_tile(amap(g),g->player.x,g->player.y);
            if (!g->in_town && t==WORLD_DUNGEON_TILE) enter_dungeon(g);
            else if (!g->in_town && t==WORLD_TOWN_TILE) enter_town(g);
        }
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
    const char *player_save=NULL, *script=NULL, *out_prefix=NULL;
    for (int i=5;i<argc;i++){
        if (!strcmp(argv[i],"--script") && i+2<argc){ script=argv[i+1]; out_prefix=argv[i+2]; i+=2; }
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
    /* 城鎮檔路徑:同目錄的 mapx21 */
    snprintf(g.town_path,sizeof g.town_path,"%s",map_path);
    char *tb=strrchr(g.town_path,'/'); tb=tb?tb+1:g.town_path;
    snprintf(tb,sizeof g.town_path-(tb-g.town_path),"mapx21");

    g.ui = ui_tsv ? u2_strings_load(ui_tsv,2,3) : (U2Strings){0};
    /* 對話譯文:由 ui_tsv 同目錄推 talk_dialogue.tsv */
    if (ui_tsv){
        char tt[512]; snprintf(tt,sizeof tt,"%s",ui_tsv);
        char *e=strrchr(tt,'/'); e=e?e+1:tt;
        snprintf(e,sizeof tt-(e-tt),"talk_dialogue.tsv");
        g.tr = u2_strings_load(tt,2,3);
    }
    if (player_save) g.save = u2_save_load(player_save);
    SDL_Surface *tiles=u2_tileset_load(tiles_path);

    SDL_Surface *cv=SDL_CreateRGBSurfaceWithFormat(0,CANVAS_W,CANVAS_H,32,SDL_PIXELFORMAT_RGBA32);
    U2Text title=u2_text_open(font_path,20), body=u2_text_open(font_path,22), small=u2_text_open(font_path,17);
    if (!title.font||!body.font||!small.font){ fprintf(stderr,"字型失敗: %s\n",TTF_GetError()); return 1; }

    g.mode=MODE_WORLD;
    find_start(&g.map,&g.player);
    snprintf(g.msg,sizeof g.msg,"歡迎來到 Sosaria,冒險者。");

    if (headless){
        int step=0; char out[600];
        render_all(cv,&g,&title,&body,&small,tiles);
        snprintf(out,sizeof out,"%s%02d.png",out_prefix,step++); IMG_SavePNG(cv,out);
        for (const char *s=script;*s;s++){
            char c=*s, d=norm_dir(c);
            if (d) handle_dir(&g,d);
            else if (c=='C'||c=='c') g.show_sheet=!g.show_sheet;
            else if (c=='T'||c=='t') do_talk(&g);
            else if (c=='J'||c=='j') dungeon_descend(&g);
            else if (c=='K'||c=='k') dungeon_ascend(&g);
            else if (c=='X'||c=='x'){ if (g.mode==MODE_DUNGEON) exit_dungeon(&g); else if (g.in_town) exit_town(&g); }
            else if (c=='D') enter_dungeon(&g);
            else if (c=='O') enter_town(&g);   /* 強制進城(headless 測試用) */
            else continue;
            render_all(cv,&g,&title,&body,&small,tiles);
            snprintf(out,sizeof out,"%s%02d.png",out_prefix,step++); IMG_SavePNG(cv,out);
        }
        printf("腳本完成:%d 步,輸出 %s00..%02d.png\n",step-1,out_prefix,step-1);
    } else {
        SDL_Window *win=SDL_CreateWindow("Ultima II 繁中",
            SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,CANVAS_W,CANVAS_H,0);
        SDL_Renderer *ren=SDL_CreateRenderer(win,-1,SDL_RENDERER_ACCELERATED);
        int running=1;
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
                        case SDLK_t: do_talk(&g); break;
                        case SDLK_j: dungeon_descend(&g); break;
                        case SDLK_k: dungeon_ascend(&g); break;
                        case SDLK_x:
                            if (g.mode==MODE_DUNGEON) exit_dungeon(&g);
                            else if (g.in_town) exit_town(&g);
                            break;
                        case SDLK_q:case SDLK_ESCAPE: running=0; break;
                    }
                    if (d) handle_dir(&g,d);
                }
            }
            render_all(cv,&g,&title,&body,&small,tiles);
            SDL_Texture *tex=SDL_CreateTextureFromSurface(ren,cv);
            SDL_RenderClear(ren); SDL_RenderCopy(ren,tex,NULL,NULL); SDL_RenderPresent(ren);
            SDL_DestroyTexture(tex);
            SDL_Delay(16);
        }
        SDL_DestroyRenderer(ren); SDL_DestroyWindow(win);
    }

    u2_text_close(&title); u2_text_close(&body); u2_text_close(&small);
    if (tiles) SDL_FreeSurface(tiles);
    SDL_FreeSurface(cv);
    TTF_Quit(); IMG_Quit(); SDL_Quit();
    return 0;
}
