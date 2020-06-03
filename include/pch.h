#include "lcd.h"

typedef union {
    u32 word[2];
    u8 byte[8];
} Control;
