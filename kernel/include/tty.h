#ifndef TTY_H
#define TTY_H

#include "stdlib.h"

void init_tty() asm("init_tty");
void out_char(char chr);
void out_string(char *str);
extern void clear_screen() asm("clear_screen");
void set_text_attr(char attr);
void move_cursor(unsigned int pos);
extern void printf(char *fmt, ...) asm("printf");
uint8_t in_scancode();
char in_char(bool wait);
void* in_string();

#endif 