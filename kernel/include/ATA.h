#ifndef ATA_H
#define ATA_H

#include "stdlib.h"

#define HD_DATA_OFFSET     0x0       /* регистр данных */
#define HD_ERROR_OFFSET    0x1       /* регистр ошибок */
#define HD_NSECTOR_OFFSET  0x2       /* регистр счетчика секторов */
#define LBA_0_7_OFFSET     0x3       /* регистр стартового сектора */
#define LBA_8_15_OFFSET    0x4       /* регистр младшего байта номера цилиндра */
#define LBA_16_23_OFFSET   0x5       /* регистр старшего байта номера цилиндра */
#define HD_CURRENT_OFFSET  0x6       /* 101dhhhh , d=устройство, hhhh=головка */
#define HD_STATUS_OFFSET   0x7       /* регистр состояния/команд */	

#define IDENTIFY_COMMAND 0xec
#define READ_COMMAND 	 0x20
#define WRITE_COMMAND 	 0x30

#define SECTOR_SIZE 0x200

typedef struct AtaDevice
{
	char available;
	size_t port;
	size_t slave_bit;
	size_t sector_nubmer;
	size_t is_lba48_supported;
} AtaDevice;

AtaDevice ata_devices[4];
void init_ATA_devices();
void read_ATA(AtaDevice ata_device, size_t lba, int count, void* buffer);
void write_ATA(AtaDevice ata_device, size_t lba, int count, void* buffer);
#endif