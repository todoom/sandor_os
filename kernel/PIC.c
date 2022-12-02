#include "include/PIC.h"
#include "include/stdlib.h"

void init_PIC()
{
	init_master_PIC();
	init_slave_PIC();
}

void init_master_PIC()
{
	outportb(PIC_1_CTRL, ICW_1);	/* ICW1 */
	outportb(PIC_1_DATA, IRQ_0);	/* ICW2 */
	outportb(PIC_1_DATA, 0x4);	/* ICW3 */
	outportb(PIC_1_DATA, 1); 	/* ICW4 */
}
void init_slave_PIC()
{
	outportb(PIC_2_CTRL, ICW_1);	/* ICW1 */
	outportb(PIC_2_DATA, IRQ_8);	/* ICW2 */
	outportb(PIC_2_DATA, 0x2);	/* ICW3 */
	outportb(PIC_2_DATA, 1);	/* ICW4 */
}