/*!
    \file  main.c
    \brief USART echo interrupt demo

    \version 2019-06-05, V1.0.0, demo for GD32VF103
*/

/*
    Copyright (c) 2019, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its
contributors may be used to endorse or promote products derived from this
software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "gd32vf103.h"
#include "gd32vf103v_eval.h"
#include "lcd.h"
#include "model.h"
#include "pch.h"
#include "systick.h"
#include <stdio.h>

// #define BG(x, y) GRAY
#define BG(x, y) blocks[xy2block[(x) / 10][(y) / 10]][(x) % 10][(y) % 10]

const int border_x1 = 0;
const int border_y1 = 0;
const int border_x2 = 159 - 20;
const int border_y2 = 79 - 20;

const int denom = 50;

const int human_0_camp = 0;

Control input_data;

extern int num_ir;
extern int num_ird;

int rhor2xy[24][2] = {
    {10, 0},  {10, -3},  {9, -5},  {7, -7},  {5, -9},  {3, -10},
    {0, -10}, {-3, -10}, {-5, -9}, {-7, -7}, {-9, -5}, {-10, -3},
    {-10, 0}, {-10, 3},  {-9, 5},  {-7, 7},  {-5, 9},  {-3, 10},
    {0, 10},  {3, 10},   {5, 9},   {7, 7},   {9, 5},   {10, 3},
};

int pose2xy[10][2] = {{10, 0},   {-10, 0}, {8, -6},  {-8, -6}, {3, -10},
                      {-3, -10}, {3, 10},  {-3, 10}, {8, 6},   {-8, 6}};

unsigned char need_clear[160 * 80 / 8];
unsigned char is_painted[160 * 80 / 8];

typedef struct {
    int xpos;
    int ypos;
    int last_x_character;
    int last_y_character;
    int last_x_character_backup;
    int last_y_character_backup;
    int right_face;
    int vx;
    int vy;
    int state;
    int sheild_cd;
    int weapon;
    int fire_cd;
} GameObject;

typedef struct {
    int damage;
    int speed;
    int diverge;
    int cd;
    int mana_cost;
} Weapon;

int weapon_num = 2;
Weapon weapon_list[] = {{5, 10, 20, 20, 0}, {2, 20, 40, 5, 1}};

void draw_init() {
    for (int i = 0; i < 400; i++) ((int *)need_clear)[i] = 0xffffffff;
    for (int i = 0; i < 400; i++) ((int *)is_painted)[i] = 0;
    for (int i = 0; i < 15; i++) {
        ((int *)need_clear)[i * 160 / 8 / 4] = 0xc0000000;
        ((int *)is_painted)[i * 160 / 8 / 4] = 0x3fffffff;
        ((int *)need_clear)[(i * 160 + 159) / 8 / 4] = 0x00000003;
        ((int *)is_painted)[(i * 160 + 159) / 8 / 4] = 0xfffffffc;
    }
}

int xy2block[16][8];
const int weapon_offset = 4;

void clean_last_character(GameObject pos) {
    for (int i = 0; i < 20 + weapon_offset; i++) {
        if (i + pos.last_y_character_backup < 80)
            for (int j = 0; j < 20; j++) {
                int subs = j + pos.last_x_character_backup +
                           (i + pos.last_y_character_backup) * 160;
                if (need_clear[subs / 8] & (1 << (subs % 8)))
                    LCD_DrawPoint(j + pos.last_x_character_backup,
                                  i + pos.last_y_character_backup,
                                  BG(j + pos.last_x_character_backup,
                                     i + pos.last_y_character_backup));
            }
    }
}

void draw_weapon(GameObject pos, int pose) {
    int weapon_id = pos.weapon;
    int x = pos.xpos / denom;
    int y = pos.ypos / denom + weapon_offset;
    if (pose % 2) {
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                int pos = j + x + (i + y) * 160;
                if (weapons[weapon_id][pose / 2][i][19 - j] == 65535 ||
                    (is_painted[pos / 8] & (1 << (pos % 8)))) {
                } else {
                    need_clear[pos / 8] &= ~(1 << (pos % 8));
                    is_painted[pos / 8] |= (1 << (pos % 8));
                    LCD_DrawPoint(j + x, i + y,
                                  weapons[weapon_id][pose / 2][i][19 - j]);
                }
            }
        }
    } else {
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                int pos = j + x + (i + y) * 160;
                if (weapons[weapon_id][pose / 2][i][j] == 65535 ||
                    (is_painted[pos / 8] & (1 << (pos % 8)))) {
                } else {
                    need_clear[pos / 8] &= ~(1 << (pos % 8));
                    is_painted[pos / 8] |= (1 << (pos % 8));
                    LCD_DrawPoint(j + x, i + y,
                                  weapons[weapon_id][pose / 2][i][j]);
                }
            }
        }
    }
}

GameObject player[2];
int camp2hero[2] = {0, 1};

int get_10pose_from_target(int my_camp_id) {
    int tan1000 = (player[my_camp_id].ypos - player[my_camp_id ^ 1].ypos) *
                  1000 /
                  (player[my_camp_id ^ 1].xpos - player[my_camp_id].xpos);
    int right = player[my_camp_id ^ 1].xpos > player[my_camp_id].xpos;
    if (right) {
        if (tan1000 > 1376) return 2 * 2;
        if (tan1000 > 325) return 1 * 2;
        if (tan1000 > -325) return 0 * 2;
        if (tan1000 > -1376) return 4 * 2;
        return 3 * 2;
    } else {
        if (tan1000 > 1376) return 3 * 2 + 1;
        if (tan1000 > 325) return 4 * 2 + 1;
        if (tan1000 > -325) return 0 * 2 + 1;
        if (tan1000 > -1376) return 1 * 2 + 1;
        return 2 * 2 + 1;
    }
}

int get_24pose_from_target(int my_camp_id) {
    int tan1000 = (player[my_camp_id].ypos - player[my_camp_id ^ 1].ypos) *
                  1000 /
                  (player[my_camp_id ^ 1].xpos - player[my_camp_id].xpos);
    int right = player[my_camp_id ^ 1].xpos > player[my_camp_id].xpos;
    if (right) {
        if (tan1000 > 7595) return 6;
        if (tan1000 > 2414) return 5;
        if (tan1000 > 1303) return 4;
        if (tan1000 > 767) return 3;
        if (tan1000 > 414) return 2;
        if (tan1000 > 131) return 1;
        if (tan1000 > -131) return 0;
        if (tan1000 > -414) return 23;
        if (tan1000 > -767) return 22;
        if (tan1000 > -1303) return 21;
        if (tan1000 > -2414) return 20;
        if (tan1000 > -7595) return 19;
        return 18;
    } else {
        if (tan1000 > 7595) return 18;
        if (tan1000 > 2414) return 17;
        if (tan1000 > 1303) return 16;
        if (tan1000 > 767) return 15;
        if (tan1000 > 414) return 14;
        if (tan1000 > 131) return 13;
        if (tan1000 > -131) return 12;
        if (tan1000 > -414) return 11;
        if (tan1000 > -767) return 10;
        if (tan1000 > -1303) return 9;
        if (tan1000 > -2414) return 8;
        if (tan1000 > -7595) return 7;
        return 6;
    }
}

void draw_character(int my_camp_id) {
    GameObject *pos = &player[my_camp_id];
    const u16(*player_model)[20] = character[camp2hero[my_camp_id]];
    if (pos->state >> 16)
        draw_weapon(player[my_camp_id], get_10pose_from_target(my_camp_id));
    else
        player_model = character_dead[camp2hero[my_camp_id]];
    int x = pos->xpos / denom;
    int y = pos->ypos / denom;
    pos->last_x_character_backup = pos->last_x_character;
    pos->last_y_character_backup = pos->last_y_character;
    if (x > pos->last_x_character) pos->right_face = 1;
    if (x < pos->last_x_character) pos->right_face = 0;
    if (pos->right_face) {
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                int pos = j + x + (i + y) * 160;
                if (player_model[i][j] == 65535 ||
                    (is_painted[pos / 8] & (1 << (pos % 8)))) {
                } else {
                    need_clear[pos / 8] &= ~(1 << (pos % 8));
                    is_painted[pos / 8] |= (1 << (pos % 8));
                    LCD_DrawPoint(j + x, i + y, player_model[i][j]);
                }
            }
        }
    } else {
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                int pos = j + x + (i + y) * 160;
                if (player_model[i][19 - j] == 65535 ||
                    (is_painted[pos / 8] & (1 << (pos % 8)))) {
                } else {
                    need_clear[pos / 8] &= ~(1 << (pos % 8));
                    is_painted[pos / 8] |= (1 << (pos % 8));
                    LCD_DrawPoint(j + x, i + y, player_model[i][19 - j]);
                }
            }
        }
    }
    pos->last_x_character = x;
    pos->last_y_character = y;
}
int get_ms() { return get_timer_value() * 4000 / (SystemCoreClock); }
int start_frame = 0, mid_frame = 0, frame = 0;
int start_ms = 0, mid_ms = 0;
int get_fps() {
    int current_ms = get_ms();
    if (current_ms - start_ms < 2000) {
        mid_ms = current_ms;
        mid_frame = frame;
    } else if (current_ms - mid_ms > 1000) {
        start_ms = mid_ms;
        start_frame = mid_frame;
    }
    return (frame - start_frame) * 1000 / (current_ms - start_ms);
}

void init_player(GameObject *p, int x, int y) {
    p->xpos = x * denom;
    p->ypos = y * denom;
    p->last_x_character = x;
    p->last_x_character = y;
    p->vx = 0;
    p->vy = 0;
    p->state = (20 << 16) + (20 << 8) + 20;
    p->weapon = 0;
    p->fire_cd = 0;
}

void update_pos(GameObject *p, int next_x_offset, int next_y_offset,
                int speed) {

    p->xpos += next_x_offset * speed;
    p->ypos += next_y_offset * speed;
    if (p->xpos < border_x1 * denom) p->xpos = border_x1 * denom;
    if (p->xpos > border_x2 * denom) p->xpos = border_x2 * denom;
    if (p->ypos < border_y1 * denom) p->ypos = border_y1 * denom;
    if (p->ypos > border_y2 * denom) p->ypos = border_y2 * denom;
}

void init_map() {
    for (int x = 0; x < 160; x += 10)
        for (int y = 0; y < 80; y += 10) {
            xy2block[x / 10][y / 10] =
                rand() % (sizeof(blocks) / (10 * 10 * sizeof(u16)));
            LCD_Address_Set(x, y, x + 9, y + 9);
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 10; j++)
                    LCD_WR_DATA(blocks[xy2block[x / 10][y / 10]][j][i]);
        }
}

void random_walk(int camp_id) {
    player[camp_id].vx += rand() % 19 - 9;
    player[camp_id].vy += rand() % 19 - 9;
    update_pos(&player[camp_id], player[camp_id].vx, player[camp_id].vy, 1);
    if (player[camp_id].xpos <= border_x1 * denom) player[camp_id].vx = 0;
    if (player[camp_id].xpos >= border_x2 * denom) player[camp_id].vx = 0;
    if (player[camp_id].ypos <= border_y1 * denom) player[camp_id].vy = 0;
    if (player[camp_id].ypos >= border_y2 * denom) player[camp_id].vy = 0;
}

#define queue_size 20
GameObject bullet_queue[queue_size + 1];
int begin = 0, end = 0;

void shield_recharge(int target_player) {
    int cd = player[target_player].sheild_cd;
    int hp = (player[target_player].state >> 16) & 0xff;
    if (hp == 0) return;
    int shield = (player[target_player].state >> 8) & 0xff;
    int mana = player[target_player].state & 0xff;
    cd--;
    if (cd <= 0) {
        cd = 30;
        shield += 5;
        if (shield > 20) shield = 20;
    }
    player[target_player].state = (hp << 16) + (shield << 8) + mana;
    player[target_player].sheild_cd = cd;
}

void mana_recharge(int target_player) {
    if (((player[target_player].state >> 16) & 0xff) == 0) return;
    if (frame % 30 == 0) {
        int mana = player[target_player].state & 0xff;
        if (mana >= 20) return;
        player[target_player].state =
            (player[target_player].state & 0xffffff00) | (mana + 1);
    }
}

void deal_damage(int target_player, int bullet_type) {
    int hp = (player[target_player].state >> 16) & 0xff;
    int shield = (player[target_player].state >> 8) & 0xff;
    int mana = player[target_player].state & 0xff;
    shield -= weapon_list[bullet_type / 2].damage;
    if (shield < 0) {
        hp += shield;
        shield = 0;
    }
    if (hp < 0) hp = 0;
    player[target_player].state = (hp << 16) + (shield << 8) + mana;
    player[target_player].sheild_cd = 60;
}

void render_bullet() {
    while (begin != end && bullet_queue[begin % queue_size].right_face == 0) {
        begin++;
    }
    for (int k = begin % queue_size; k != end % queue_size;
         k = (k + 1) % queue_size) {
        if (bullet_queue[k].last_x_character_backup <= -10 ||
            bullet_queue[k].last_y_character_backup <= -10 ||
            bullet_queue[k].last_x_character_backup >= 160 ||
            bullet_queue[k].last_y_character_backup >= 80)
            bullet_queue[k].right_face = 0;
        if (bullet_queue[k].right_face == 0) { continue; }
        int x = bullet_queue[k].xpos / denom;
        int y = bullet_queue[k].ypos / denom;

        int bullet_center_x = x + 5;
        int bullet_center_y = y + 5;

        for (int i = 0; i < 2; i++) {
            int character_center_x = player[i].xpos / denom + 10;
            int character_center_y = player[i].ypos / denom + 10;
            if (bullet_queue[k].right_face == -1 &&
                abs(bullet_center_x - character_center_x) <= 5 &&
                abs(bullet_center_y - character_center_y) <= 5) {
                bullet_queue[k].vx = bullet_queue[k].vy = 0;
                bullet_queue[k].right_face = 30;
                deal_damage(i, bullet_queue[k].state);
            }
        }

        if (bullet_queue[k].vx || bullet_queue[k].vy)
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    int pos = j + x + (i + y) * 160;
                    if (pos >= 0 && pos < 160 * 80) {
                        if (bullets[bullet_queue[k].state][i][j] == 65535 ||
                            (is_painted[pos / 8] & (1 << (pos % 8)))) {
                        } else {
                            need_clear[pos / 8] &= ~(1 << (pos % 8));
                            is_painted[pos / 8] |= (1 << (pos % 8));
                            LCD_DrawPoint(j + x, i + y,
                                          bullets[bullet_queue[k].state][i][j]);
                        }
                    }
                }
            }
        else {
            if (bullet_queue[k].right_face > 1)
                for (int i = 0; i < 20; i++) {
                    for (int j = 0; j < 20; j++) {
                        int pos = j + bullet_center_x - 10 +
                                  (i + bullet_center_y - 10) * 160;
                        if (pos >= 0 && pos < 160 * 80) {
                            if (blood[i][j] == 65535 ||
                                (is_painted[pos / 8] & (1 << (pos % 8)))) {
                            } else {
                                need_clear[pos / 8] &= ~(1 << (pos % 8));
                                is_painted[pos / 8] |= (1 << (pos % 8));
                                LCD_DrawPoint(j + bullet_center_x - 10,
                                              i + bullet_center_y - 10,
                                              blood[i][j]);
                            }
                        }
                    }
                }
            bullet_queue[k].right_face--;
        }
        bullet_queue[k].last_x_character_backup =
            bullet_queue[k].last_x_character;
        bullet_queue[k].last_y_character_backup =
            bullet_queue[k].last_y_character;
        bullet_queue[k].last_x_character = x;
        bullet_queue[k].last_y_character = y;
        bullet_queue[k].xpos += bullet_queue[k].vx;
        bullet_queue[k].ypos += bullet_queue[k].vy;
    }
}

void clean_last_bullet() {
    for (int k = begin % queue_size; k != end % queue_size;
         k = (k + 1) % queue_size) {
        GameObject pos = bullet_queue[k];
        if (bullet_queue[k].vx || bullet_queue[k].vy)
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    int subs = j + pos.last_x_character_backup +
                               (i + pos.last_y_character_backup) * 160;
                    if (need_clear[subs / 8] & (1 << (subs % 8)))
                        LCD_DrawPoint(j + pos.last_x_character_backup,
                                      i + pos.last_y_character_backup,
                                      BG(j + pos.last_x_character_backup,
                                         i + pos.last_y_character_backup));
                }
            }
        else if (bullet_queue[k].right_face == 0) {

            // LCD_ShowNum(40, 0, k, 4, WHITE);
            for (int i = 0; i < 20; i++) {
                for (int j = 0; j < 20; j++) {
                    int subs = j + pos.last_x_character_backup + 5 - 10 +
                               (i + pos.last_y_character_backup + 5 - 10) * 160;
                    // if (subs >= 0 && subs < 160 * 80)
                    if (need_clear[subs / 8] & (1 << (subs % 8))) {
                        LCD_DrawPoint(
                            j + pos.last_x_character_backup + 5 - 10,
                            i + pos.last_y_character_backup + 5 - 10,
                            BG(j + pos.last_x_character_backup + 5 - 10,
                               i + pos.last_y_character_backup + 5 - 10));
                    }
                }
            }
        }
    }
}

void bullet_push(int x, int y, int vx, int vy, int type) {
    if (end - begin >= queue_size - 1) return;
    bullet_queue[end % queue_size].xpos = x;
    bullet_queue[end % queue_size].last_x_character =
        bullet_queue[end % queue_size].last_x_character_backup = x / denom;
    bullet_queue[end % queue_size].ypos = y;
    bullet_queue[end % queue_size].last_y_character =
        bullet_queue[end % queue_size].last_y_character_backup = y / denom;
    bullet_queue[end % queue_size].vx = vx;
    bullet_queue[end % queue_size].vy = vy;
    bullet_queue[end % queue_size].right_face = -1;
    bullet_queue[end % queue_size].state = type;
    end++;
}

void init_panel() {
    LCD_Address_Set(0, 0, 29, 14);
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 30; j++) LCD_WR_DATA(panel[i][j]);
    LCD_Address_Set(130, 0, 159, 14);
    for (int i = 0; i < 15; i++)
        for (int j = 0; j < 30; j++) LCD_WR_DATA(panel[i][j]);
}

void update_panel(int camp) {
    int hp = player[camp].state >> 16;
    int shield = (player[camp].state >> 8) & 0xff;
    int mana = player[camp].state & 0xff;
    int start;
    if (camp == human_0_camp)
        start = 7;
    else
        start = 137;
    LCD_Fill(start, 1, start + hp - 1, 1, 60140);
    LCD_Fill(start, 2, start + hp - 1, 2, 55719);
    LCD_Fill(start, 3, start + hp - 1, 3, 41155);
    LCD_Fill(start + hp, 1, start + 19, 3, 27271);

    LCD_Fill(start, 5, start + shield - 1, 5, 52857);
    LCD_Fill(start, 6, start + shield - 1, 6, 38001);
    LCD_Fill(start, 7, start + shield - 1, 7, 27371);
    LCD_Fill(start + shield, 5, start + 19, 7, 27271);

    LCD_Fill(start, 9, start + mana - 1, 9, 25852);
    LCD_Fill(start, 10, start + mana - 1, 10, 9079);
    LCD_Fill(start, 11, start + mana - 1, 11, 4688);
    LCD_Fill(start + mana, 9, start + 19, 11, 27271);
}

#define START (button & (1 << 0))
#define SELECT (button & (1 << 1))
#define TRIANGLE (button & (1 << 2))
#define CIRCLE (button & (1 << 3))
#define CROSS (button & (1 << 4))
#define SQUARE (button & (1 << 5))

void try_fire(int camp_id) {

    if (player[camp_id].fire_cd-- > 0) return;
    int mana = player[camp_id].state & 0xff;
    if (mana < weapon_list[player[camp_id].weapon].mana_cost) return;
    mana -= weapon_list[player[camp_id].weapon].mana_cost;
    player[camp_id].state = (player[camp_id].state & 0xffffff00) | mana;
    player[camp_id].fire_cd = weapon_list[player[camp_id].weapon].cd;

    int pose = get_24pose_from_target(camp_id);
    Weapon *target_weapon = &weapon_list[player[camp_id].weapon];
    bullet_push(player[camp_id].xpos + (10 / 2 + rhor2xy[pose][0]) * denom,
                player[camp_id].ypos +
                    (weapon_offset + 10 / 2 - 1 + rhor2xy[pose][1]) * denom,
                rhor2xy[pose][0] * target_weapon->speed +
                    rand() % (target_weapon->diverge * 2 - 1) -
                    (target_weapon->diverge - 1),
                rhor2xy[pose][1] * target_weapon->speed +
                    rand() % (target_weapon->diverge * 2 - 1) -
                    (target_weapon->diverge - 1),
                player[camp_id].weapon * 2 + camp_id);
}

void DrawCover() {
    int id = rand() % 2;
    LCD_Address_Set(0, 0, 159, 79);
    for (int i = 0; i < 80; i++)
        for (int j = 0; j < 160; j++) LCD_WR_DATA(cover_image[id][i][j]);
}

int Get_BOOT0(void) { return (int)(gpio_input_bit_get(GPIOA, GPIO_PIN_8)); }

int main(void) {
    srand(get_timer_value());
    /* initialize the LEDs */
    Lcd_Init();
    // LCD_Clear(BG);
    // LCD_ShowNum(0, 32, sizeof(blocks), 10, WHITE);
    DrawCover();
    /* enable the global interrupt */
    eclic_global_interrupt_enable();
    eclic_priority_group_set(ECLIC_PRIGROUP_LEVEL3_PRIO1);
    eclic_irq_enable(USART0_IRQn, 1, 0);
    /* configure EVAL_COM0 */
    gd_eval_com_init(EVAL_COM0);
    /* enable USART0 receive interrupt */
    usart_interrupt_enable(USART0, USART_INT_RBNE);

    // eclic_irq_enable(TIMER1_IRQn, 2, 0);
    // hw_time_set(0);
    // LCD_Address_Set(40, 40, 59, 59);

    for (int i = 0; i < 3000; i++) {
        u8 button = input_data.byte[5];
        if (button) break;
        delay_1ms(1);
    }

    init_map();
    init_panel();
    init_player(&player[0], 40, 40);
    init_player(&player[1], 120, 40);

    // int count = 0;
    int last_x_offset = 0;
    int last_y_offset = 0;
    while (1) {
        frame++;

        if (player[human_0_camp].state >> 16) {
            static int human = 1;
            if (Get_BOOT0()) {
                human ^= 1;
                delay_1ms(500);
            }
            if (!human) {
                random_walk(human_0_camp);
                try_fire(human_0_camp);
            } else {
                u8 stick = input_data.byte[6];
                u8 button = input_data.byte[5];

                u8 angle = stick >> 3;

                // LCD_ShowNum(40, 0, begin, 2, WHITE);
                // LCD_ShowNum(80, 0, end, 2, WHITE);
                // LCD_ShowNum(0, 32, count++, 5, WHITE);
                if (angle >= 0 && angle <= 23) {

                    int speed = stick & 0b111;
                    int next_x_offset = rhor2xy[angle][0];
                    int next_y_offset = rhor2xy[angle][1];
                    if (speed == 0) next_x_offset = next_y_offset = 0;
                    if (next_x_offset == last_x_offset &&
                        next_y_offset == last_y_offset) {
                    } else {
                        last_x_offset = next_x_offset;
                        last_y_offset = next_y_offset;
                    }
                    update_pos(&player[human_0_camp], next_x_offset,
                               next_y_offset, speed);
                }

                static int last_circle = 0;
                if (CIRCLE && !last_circle) {
                    player[human_0_camp].weapon =
                        (player[human_0_camp].weapon + 1) % weapon_num;
                }
                last_circle = CIRCLE;
                if (CROSS) { try_fire(human_0_camp); }
            }
        }
        if (player[human_0_camp ^ 1].state >> 16) {
            random_walk(human_0_camp ^ 1);
            if ((player[human_0_camp ^ 1].state & 0xff) > 15)
                player[human_0_camp ^ 1].weapon = 1;
            if ((player[human_0_camp ^ 1].state & 0xff) < 5)
                player[human_0_camp ^ 1].weapon = 0;
            try_fire(human_0_camp ^ 1);
        }

        draw_init();

        shield_recharge(human_0_camp);
        shield_recharge(human_0_camp ^ 1);
        mana_recharge(human_0_camp);
        mana_recharge(human_0_camp ^ 1);

        render_bullet();

        int up_layer_camp;
        if (player[human_0_camp].ypos > player[human_0_camp ^ 1].ypos) {
            up_layer_camp = human_0_camp;
        } else {
            up_layer_camp = human_0_camp ^ 1;
        }
        clean_last_bullet();
        draw_character(up_layer_camp);
        draw_character(up_layer_camp ^ 1);

        clean_last_character(player[human_0_camp]);
        clean_last_character(player[human_0_camp ^ 1]);

        update_panel(human_0_camp);
        update_panel(human_0_camp ^ 1);

        // LCD_ShowNum(0, 0, get_fps(), 2, WHITE);
        // LCD_ShowNum(0, 16, hp, 2, WHITE);
        // LCD_ShowNum(0, 32, shield, 2, WHITE);
        // LCD_ShowNum(0, 48, mana, 2, WHITE);
    }
}
