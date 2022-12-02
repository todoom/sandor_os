#ifndef PIC_H
#define PIC_H

#define ICW_1 0x11			
 
#define PIC_1_CTRL 0x20		
#define PIC_2_CTRL 0xA0		
 
#define PIC_1_DATA 0x21		
#define PIC_2_DATA 0xA1		
 
#define IRQ_0 0x20		
#define IRQ_8 0x28	

void init_PIC();

#endif