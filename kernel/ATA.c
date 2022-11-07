#include "include/ATA.h"
#include "include/stdlib.h"
#include "include/memory_manager.h"

bool hd_busy(int port)
{
    unsigned char status;
    int i = 6000;
    do 
    {
	   inportb(port + HD_STATUS_OFFSET, status);
       i--;
    } while ((status & 0x80) && i);
    if (i == 0) return true;
    return false;
}

bool hd_ready(int port)
{
    unsigned char status;

    int i = 6000;
    do 
    {
       inportb(port + HD_STATUS_OFFSET, status);
       i--;
    } while ((!(status & 0x40)) && i);
    if (i == 0) return false;
    return true;
}

bool hd_drq(int port)
{
	unsigned char status;

    int i = 6000;
    do 
    {
       inportb(port + HD_STATUS_OFFSET, status);
       i--;
    } while ((!(status & 0x8)) && i);
    if (i == 0) return false;
    return true;
}
bool send_command(int port, int slave_bit, int hd_nsector, size_t lba, int command)
{
    if (hd_busy(port)) return false;
    outportb(port + HD_CURRENT_OFFSET, 0xe0 | (slave_bit << 4) | ((lba >> 24) & 0xf));
    if (!hd_ready(port)) return false;
    if (hd_busy(port)) return false;
    outportb(port + HD_NSECTOR_OFFSET, hd_nsector);
    outportb(port + LBA_0_7_OFFSET, lba);
    outportb(port + LBA_8_15_OFFSET, lba >> 8);
    outportb(port + LBA_16_23_OFFSET, lba >> 16);

    //send command
    outportb(port + HD_STATUS_OFFSET, command);
    hd_busy(port);
    hd_drq(port);
    return true;
}
void write_ATA(AtaDevice ata_device, size_t lba, int count, void* buffer)
{
    if (count >= 0x100)
    for (int j = lba; j < lba + count; j += 0x100)
    {
        send_command(ata_device.port, ata_device.slave_bit, count, lba, WRITE_COMMAND);

        asm("cld \n \
             rep outsw "
             ::"S"(buffer),"d"(ata_device.port), "c"(count * SECTOR_SIZE / 2));
    }
}

void read_ATA(AtaDevice ata_device, size_t lba, int count, void* buffer)
{
    for (int j = lba; j < lba + count; j += 0x100)
    {       
        send_command(ata_device.port, ata_device.slave_bit, count, lba, READ_COMMAND);

        asm("cld \n \
             rep insw "
             ::"D"(buffer),"d"(ata_device.port), "c"(count * SECTOR_SIZE / 2));
    }
}

void init_ATA_device(int port, char slave_bit, AtaDevice* ata_device)
{
    ata_device->available = send_command(port, slave_bit, 0, 0, IDENTIFY_COMMAND);

    if (!ata_device->available) return;

    short* buffer = kmalloc(0x200);
    asm("cld \n \
         rep insw"
         ::"D"(buffer),"d"(port + HD_DATA_OFFSET), "c"(256));

    ata_device->port = port;
    ata_device->slave_bit = slave_bit;
    ata_device->sector_nubmer = buffer[1] * buffer[3] * buffer[6];
    if (buffer[83] & 0x400)
    {
        ata_device->is_lba48_supported = 1;
    }
    else
    {
        ata_device->is_lba48_supported = 0;
    }
    kfree(buffer);
}
void init_ATA_devices()
{
    init_ATA_device(0x1f0, 0, &(ata_devices[0]));
    init_ATA_device(0x1f0, 1, &(ata_devices[1]));
    init_ATA_device(0x170, 0, &(ata_devices[2]));
    init_ATA_device(0x170, 1, &(ata_devices[3]));
}
