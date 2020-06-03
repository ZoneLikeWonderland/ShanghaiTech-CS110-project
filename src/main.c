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

#include "character.h"
#include "gd32vf103.h"
#include "gd32vf103v_eval.h"
#include "lcd.h"
#include "pch.h"
#include "systick.h"
#include <stdio.h>

#define BG GRAY
const int border_x1 = 0;
const int border_y1 = 0;
const int border_x2 = 159 - 20;
const int border_y2 = 79 - 20;

const int denom = 50;

Control input_data;

extern int num_ir;
extern int num_ird;

int rhor2xy[24][2] = {
    {10, 0},  {10, -3},  {9, -5},  {7, -7},  {5, -9},  {3, -10},
    {0, -10}, {-3, -10}, {-5, -9}, {-7, -7}, {-9, -5}, {-10, -3},
    {-10, 0}, {-10, 3},  {-9, 5},  {-7, 7},  {-5, 9},  {-3, 10},
    {0, 10},  {3, 10},   {5, 9},   {7, 7},   {9, 5},   {10, 3},
};

unsigned char need_clear[160 * 80 / 8];

typedef struct {
    int xpos;
    int ypos;
    int last_x_character;
    int last_y_character;
    int last_x_character_backup;
    int last_y_character_backup;
    int right_face;
} Pos;

void draw_init() {
    for (int i = 0; i < 400; i++) ((int *)need_clear)[i] = 0xffffffff;
}

void clean_last_character(Pos pos) {
    for (int i = 0; i < 20; i++) {
        for (int j = 0; j < 20; j++) {
            int subs = j + pos.last_x_character_backup +
                       (i + pos.last_y_character_backup) * 160;
            if (need_clear[subs / 8] & (1 << (subs % 8)))
                LCD_DrawPoint(j + pos.last_x_character_backup,
                              i + pos.last_y_character_backup, BG);
        }
    }
}

void draw_character(int id, Pos *pos) {
    int x = pos->xpos / denom;
    int y = pos->ypos / denom;
    u16(*player_model)[20] = character[id];
    pos->last_x_character_backup = pos->last_x_character;
    pos->last_y_character_backup = pos->last_y_character;
    // if (x == pos->last_x_character && y == pos->last_y_character) {
    //     for (int i = 0; i < 20; i++) {
    //         for (int j = 0; j < 20; j++) {
    //             if (player_model[i][j] == 65535) {
    //             } else {
    //                 int pos = j + x + (i + y) * 160;
    //                 need_clear[pos / 8] &= ~(1 << (pos % 8));
    //                 LCD_DrawPoint(j + x, i + y, player_model[i][j]);
    //             }
    //         }
    //     }
    //     return;
    // }
    if (x > pos->last_x_character) pos->right_face = 1;
    if (x < pos->last_x_character) pos->right_face = 0;
    if (pos->right_face) {
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                if (player_model[i][j] == 65535) {
                } else {
                    int pos = j + x + (i + y) * 160;
                    need_clear[pos / 8] &= ~(1 << (pos % 8));
                    LCD_DrawPoint(j + x, i + y, player_model[i][j]);
                }
            }
        }
    } else {
        for (int i = 0; i < 20; i++) {
            for (int j = 0; j < 20; j++) {
                if (player_model[i][19 - j] == 65535) {
                } else {
                    int pos = j + x + (i + y) * 160;
                    need_clear[pos / 8] &= ~(1 << (pos % 8));
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

Pos pos_info[2];

void init_pos(Pos *p) {
    p->xpos = 80 * denom;
    p->ypos = 40 * denom;
    p->last_x_character = 80;
    p->last_x_character = 40;
}

void update_pos(Pos *p, int next_x_offset, int next_y_offset, int speed) {

    p->xpos += next_x_offset * speed;
    p->ypos += next_y_offset * speed;
    if (p->xpos < border_x1 * denom) p->xpos = border_x1 * denom;
    if (p->xpos > border_x2 * denom) p->xpos = border_x2 * denom;
    if (p->ypos < border_y1 * denom) p->ypos = border_y1 * denom;
    if (p->ypos > border_y2 * denom) p->ypos = border_y2 * denom;
}

int main(void) {
    /* initialize the LEDs */
    Lcd_Init();
    LCD_Clear(BG);
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
        u8 stick = input_data.byte[6];
        u8 button = input_data.byte[5];

        u8 angle = stick >> 3;

        // LCD_ShowNum(0, 0, angle, 5, WHITE);
        // LCD_ShowNum(80, 0, button, 5, WHITE);
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
            update_pos(&pos_info[0], next_x_offset, next_y_offset, speed);
        }
        draw_init();
        draw_character(0, &pos_info[0]);

        static int xs = 0;
        static int ys = 0;
        xs += rand() % 19 - 9;
        ys += rand() % 19 - 9;

        update_pos(&pos_info[1], xs, ys, 1);
        if (pos_info[1].xpos <= border_x1 * denom) xs = 0;
        if (pos_info[1].xpos >= border_x2 * denom) xs = 0;
        if (pos_info[1].ypos <= border_y1 * denom) ys = 0;
        if (pos_info[1].ypos >= border_y2 * denom) ys = 0;
        draw_character(1, &pos_info[1]);

        clean_last_character(pos_info[0]);
        clean_last_character(pos_info[1]);

        LCD_ShowNum(0, 0, get_fps(), 6, WHITE);
    }
}
