#ifndef TASK_H
#define TASK_H

extern void enter_usermode() asm("enter_usermode");
extern void enter_kernelmode() asm("enter_kernelmode");

#endif