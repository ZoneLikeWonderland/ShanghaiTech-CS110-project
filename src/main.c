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
    int tag;
} Pos;

void draw_init() {
    for (int i = 0; i < 400; i++) ((int *)need_clear)[i] = 0xffffffff;
    for (int i = 0; i < 400; i++) ((int *)is_painted)[i] = 0;
}

int xy2block[16][8];
const int weapon_offset = 4;

void clean_last_character(Pos pos) {
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

void draw_weapon(Pos pos, int weapon_id, int pose) {
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

Pos pos_info[2];
int camp2hero[2] = {0, 1};

int get_pose_from_target(int my_camp_id) {
    int tan1000 = (pos_info[my_camp_id].ypos - pos_info[my_camp_id ^ 1].ypos) *
                  1000 /
                  (pos_info[my_camp_id ^ 1].xpos - pos_info[my_camp_id].xpos);
    int right = pos_info[my_camp_id ^ 1].xpos > pos_info[my_camp_id].xpos;
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

void draw_character(int my_camp_id) {
    draw_weapon(pos_info[my_camp_id], 0, get_pose_from_target(my_camp_id));
    Pos *pos = &pos_info[my_camp_id];
    int x = pos->xpos / denom;
    int y = pos->ypos / denom;
    u16(*player_model)[20] = character[camp2hero[my_camp_id]];
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

void init_pos(Pos *p) {
    p->xpos = 80 * denom;
    p->ypos = 40 * denom;
    p->last_x_character = 80;
    p->last_x_character = 40;
    p->vx = 0;
    p->vy = 0;
}

void update_pos(Pos *p, int next_x_offset, int next_y_offset, int speed) {

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
    pos_info[camp_id].vx += rand() % 19 - 9;
    pos_info[camp_id].vy += rand() % 19 - 9;
    update_pos(&pos_info[camp_id], pos_info[camp_id].vx, pos_info[camp_id].vy,
               1);
    if (pos_info[camp_id].xpos <= border_x1 * denom) pos_info[camp_id].vx = 0;
    if (pos_info[camp_id].xpos >= border_x2 * denom) pos_info[camp_id].vx = 0;
    if (pos_info[camp_id].ypos <= border_y1 * denom) pos_info[camp_id].vy = 0;
    if (pos_info[camp_id].ypos >= border_y2 * denom) pos_info[camp_id].vy = 0;
}

#define queue_size 10
Pos bullet_queue[queue_size + 1];
int begin = 0, end = 0;

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
            int character_center_x = pos_info[i].xpos / denom + 10;
            int character_center_y = pos_info[i].ypos / denom + 10;
            if (abs(bullet_center_x - character_center_x) <= 5 &&
                abs(bullet_center_y - character_center_y) <= 5) {
                bullet_queue[k].vx = bullet_queue[k].vy = 0;
                bullet_queue[k].right_face = 30;
            }
        }

        if (bullet_queue[k].vx || bullet_queue[k].vy)
            for (int i = 0; i < 10; i++) {
                for (int j = 0; j < 10; j++) {
                    int pos = j + x + (i + y) * 160;
                    if (pos >= 0 && pos < 160 * 80) {
                        if (bullets[bullet_queue[k].tag][i][j] == 65535 ||
                            (is_painted[pos / 8] & (1 << (pos % 8)))) {
                        } else {
                            need_clear[pos / 8] &= ~(1 << (pos % 8));
                            is_painted[pos / 8] |= (1 << (pos % 8));
                            LCD_DrawPoint(j + x, i + y,
                                          bullets[bullet_queue[k].tag][i][j]);
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
        Pos pos = bullet_queue[k];
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
    bullet_queue[end % queue_size].right_face = 1;
    bullet_queue[end % queue_size].tag = type;
    end++;
}

#define START (button & (1 << 0))
#define SELECT (button & (1 << 1))
#define TRIANGLE (button & (1 << 2))
#define CIRCLE (button & (1 << 3))
#define CROSS (button & (1 << 4))
#define SQUARE (button & (1 << 5))

void try_fire(int camp_id) {

    int pose = get_pose_from_target(camp_id);
    bullet_push(pos_info[camp_id].xpos + (10 / 2 + pose2xy[pose][0]) * denom,
                pos_info[camp_id].ypos +
                    (weapon_offset + 10 / 2 - 1 + pose2xy[pose][1]) * denom,
                pose2xy[pose][0] * 10 + rand() % 39 - 19,
                pose2xy[pose][1] * 10 + rand() % 39 - 19, camp_id);
}

int main(void) {
    srand(get_timer_value());
    /* initialize the LEDs */
    Lcd_Init();
    // LCD_Clear(BG);
    // LCD_ShowNum(0, 32, sizeof(blocks), 10, WHITE);
    init_map();
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

    init_pos(&pos_info[0]);
    init_pos(&pos_info[1]);

    // int count = 0;
    int last_x_offset = 0;
    int last_y_offset = 0;
    while (1) {
        frame++;

        {
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
                update_pos(&pos_info[human_0_camp], next_x_offset,
                           next_y_offset, speed);
            }

            static int cooldown = 0;
            if (CROSS) {
                if (cooldown <= 0) {
                    try_fire(human_0_camp);
                    cooldown = 20;
                }
            }
            cooldown--;
        }
        {
            random_walk(human_0_camp ^ 1);

            static int cooldown2 = 0;
            if (cooldown2 <= 0) {
                try_fire(human_0_camp ^ 1);
                cooldown2 = 20;
            }
            cooldown2--;
        }

        draw_init();

        // render_hit();

        render_bullet();

        int up_layer_camp;
        if (pos_info[human_0_camp].ypos > pos_info[human_0_camp ^ 1].ypos) {
            up_layer_camp = human_0_camp;
        } else {
            up_layer_camp = human_0_camp ^ 1;
        }
        clean_last_bullet();
        draw_character(up_layer_camp);
        draw_character(up_layer_camp ^ 1);

        clean_last_character(pos_info[human_0_camp]);
        clean_last_character(pos_info[human_0_camp ^ 1]);
        LCD_ShowNum(0, 0, get_fps(), 2, WHITE);
    }
}
