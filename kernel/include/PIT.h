#ifndef PIT_H
#define PIT_H

void init_PIT();
void timer_handler();
void enable_PIT() asm("enable_PIT");

#endif