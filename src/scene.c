/* Vector artwork: all animation is rendered on the device, never streamed. */
#include "lvgl.h"
#include "stm32u5xx_hal.h"
#include "scene.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
static lv_obj_t *actor,*enemy,*arm,*forearm,*gun,*shot,*status,*ammo_label,*enemy_label,*connection;
static lv_point_precise_t arm_points[2],fore_points[2],shot_points[2];
static uint32_t action_start,last_contact;
static unsigned ammo=6,hits,enemy_hp=3;
static int drawn,walking;
static const char *action="idle",*last_error="";
static char state_json[200];
static lv_obj_t *box(lv_obj_t *parent,int x,int y,int w,int h,uint32_t color,int radius) {
    lv_obj_t *o=lv_obj_create(parent);lv_obj_remove_style_all(o);lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(color),0);lv_obj_set_style_bg_opa(o,255,0);lv_obj_set_style_radius(o,radius,0);return o;
}
static lv_obj_t *label(lv_obj_t *parent,const char *text,int x,int y,uint32_t color) {
    lv_obj_t *o=lv_label_create(parent);lv_label_set_text(o,text);lv_obj_set_pos(o,x,y);lv_obj_set_style_text_color(o,lv_color_hex(color),0);return o;
}
static lv_obj_t *segment(lv_obj_t *parent,lv_point_precise_t *points,uint32_t color,int width) {
    lv_obj_t *o=lv_line_create(parent);lv_line_set_points(o,points,2);lv_obj_set_style_line_color(o,lv_color_hex(color),0);
    lv_obj_set_style_line_width(o,width,0);lv_obj_set_style_line_rounded(o,true,0);return o;
}
static lv_obj_t *person(lv_obj_t *screen,int x,uint32_t coat,int robot) {
    lv_obj_t *p=lv_obj_create(screen);lv_obj_remove_style_all(p);lv_obj_set_size(p,100,180);lv_obj_set_pos(p,x,205);lv_obj_add_flag(p,LV_OBJ_FLAG_OVERFLOW_VISIBLE);
    box(p,34,0,32,36,robot?0xD36F73:0xE8B98E,robot?6:18);
    box(p,29,35,42,73,coat,12);box(p,32,98,17,72,0x253247,6);box(p,54,98,17,72,0x253247,6);
    box(p,27,162,25,10,0x101927,4);box(p,52,162,27,10,0x101927,4);
    box(p,30,91,40,7,0xB9915B,2);
    if(robot){box(p,39,13,8,5,0xFFE6A3,2);box(p,54,13,8,5,0xFFE6A3,2);box(p,27,42,10,50,coat,5);box(p,65,42,10,50,coat,5);}
    else {box(p,33,0,34,10,0x272333,6);box(p,60,13,4,4,0x272333,2);}
    return p;
}
static void set_pose(float amount,float recoil) {
    /* Right hand travels from the belt to an extended firing position. */
    float hx=65+amount*34-recoil,hy=96-amount*48;
    arm_points[0]=(lv_point_precise_t){63,45};arm_points[1]=(lv_point_precise_t){72+amount*8,73-amount*22};
    fore_points[0]=arm_points[1];fore_points[1]=(lv_point_precise_t){hx,hy};
    lv_line_set_points(arm,arm_points,2);lv_line_set_points(forearm,fore_points,2);
    lv_obj_set_pos(gun,(int)hx-4,(int)hy-5);lv_obj_set_style_transform_rotation(gun,(int)((1-amount)*900),0);
}
static void refresh_text(void) {
    const char *text=!strcmp(action,"draw")?"SILAH CIKARILIYOR":!strcmp(action,"holster")?"SILAH YERINE KONUYOR":
      !strcmp(action,"fire")?"ATES":!strcmp(action,"reload")?"YENIDEN DOLDURULUYOR":enemy_hp==0?"HEDEF TAMAM":drawn?"SILAH HAZIR":"KOMUT BEKLENIYOR";
    lv_label_set_text(status,text);lv_label_set_text_fmt(ammo_label,"MERMI  %u / 6",ammo);
    lv_label_set_text_fmt(enemy_label,"HEDEF  %u / 3",enemy_hp);
    lv_label_set_text(connection,HAL_GetTick()-last_contact<5000?"DEVICE STUDIO BAGLI":"BILGISAYAR BEKLENIYOR");
}
static void tick(lv_timer_t *timer) {
    (void)timer;uint32_t elapsed=HAL_GetTick()-action_start;float pose=(float)drawn,recoil=0;
    if(!strcmp(action,"draw")){pose=fminf(elapsed/600.0f,1);if(elapsed>=600){drawn=1;action="idle";}}
    else if(!strcmp(action,"holster")){pose=1-fminf(elapsed/600.0f,1);if(elapsed>=600){drawn=0;action="idle";}}
    else if(!strcmp(action,"fire")){recoil=elapsed<120?8*(1-elapsed/120.0f):0;if(elapsed>=250)action="idle";}
    else if(!strcmp(action,"reload") && elapsed>=900){ammo=6;action="idle";}
    set_pose(pose,recoil);
    lv_obj_set_style_opa(shot,!strcmp(action,"fire") && elapsed<130?255:0,0);
    lv_obj_set_style_opa(enemy,enemy_hp?255:90,0);
    if(HAL_GetTick()-last_contact>5000)walking=0;
    int bob=walking?(int)(3*sinf(HAL_GetTick()/100.0f)):0;
    lv_obj_set_y(actor,205+bob);refresh_text();
}
const char *duello_command(const char *cmd) {
    last_contact=HAL_GetTick();last_error="";
    if(!strcmp(cmd,"RESET")){drawn=walking=0;ammo=6;hits=0;enemy_hp=3;action="idle";}
    else if(!strcmp(cmd,"STATUS")){}
    else if(!strcmp(cmd,"STOP"))walking=0;
    else if(strcmp(action,"idle"))last_error="busy";
    else if(!strcmp(cmd,"DRAW")){if(drawn)last_error="already_drawn";else action="draw";}
    else if(!strcmp(cmd,"HOLSTER")){if(!drawn)last_error="already_holstered";else action="holster";}
    else if(!strcmp(cmd,"FIRE")){
        if(!drawn)last_error="draw_first";else if(!ammo)last_error="empty";else if(!enemy_hp)last_error="target_finished";
        else{--ammo;++hits;--enemy_hp;action="fire";}
    }else if(!strcmp(cmd,"RELOAD")){if(!drawn)last_error="draw_first";else action="reload";}
    else if(!strcmp(cmd,"WALK"))walking=1;
    else last_error="unsupported_command";
    if(strcmp(cmd,"STATUS") && !*last_error)action_start=HAL_GetTick();
    snprintf(state_json,sizeof state_json,"{\"gun\":\"%s\",\"action\":\"%s\",\"ammo\":%u,\"hits\":%u,\"target_hp\":%u,\"walking\":%s,\"error\":\"%s\"}",
      drawn?"drawn":"holstered",action,ammo,hits,enemy_hp,walking?"true":"false",last_error);
    return state_json;
}
void duello_scene_init(void) {
    lv_obj_t *s=lv_screen_active();lv_obj_set_style_bg_color(s,lv_color_hex(0x101B2B),0);lv_obj_remove_flag(s,LV_OBJ_FLAG_SCROLLABLE);
    box(s,0,330,480,150,0x152E39,0);box(s,55,355,370,2,0x2D6265,0);
    box(s,70,165,45,166,0x1D354B,5);box(s,285,145,65,188,0x1D354B,5);
    for(int y=180;y<310;y+=30){box(s,80,y,8,12,0x376679,2);box(s,300,y-12,10,12,0x376679,2);}
    lv_obj_t *title=label(s,"DUELLO",0,42,0xF5C879);lv_obj_set_style_text_letter_space(title,5,0);lv_obj_align(title,LV_ALIGN_TOP_MID,0,40);
    connection=label(s,"BILGISAYAR BEKLENIYOR",0,85,0x8CB7C2);lv_obj_align(connection,LV_ALIGN_TOP_MID,0,83);
    actor=person(s,85,0x4D9CA0,0);enemy=person(s,305,0x9E455B,1);
    arm=segment(actor,arm_points,0x4D9CA0,13);forearm=segment(actor,fore_points,0xE8B98E,10);
    gun=box(actor,60,90,30,9,0xC6CCD5,2);lv_obj_add_flag(gun,LV_OBJ_FLAG_OVERFLOW_VISIBLE);box(gun,1,5,8,13,0x756762,2);
    shot_points[0]=(lv_point_precise_t){190,255};shot_points[1]=(lv_point_precise_t){340,255};
    shot=segment(s,shot_points,0xFFE6A3,4);lv_obj_set_style_opa(shot,0,0);
    status=label(s,"KOMUT BEKLENIYOR",0,390,0xE6EAF0);lv_obj_align(status,LV_ALIGN_BOTTOM_MID,0,-67);
    ammo_label=label(s,"MERMI  6 / 6",105,430,0xF5C879);enemy_label=label(s,"HEDEF  3 / 3",265,430,0xF5C879);
    set_pose(0,0);lv_timer_create(tick,20,NULL);
}
