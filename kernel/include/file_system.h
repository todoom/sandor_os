#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include "stdlib.h"
#include "List.h"

#define N_DISK_PARTIONS 4
#define ACTIVE_PARTION 0x80
#define NOT_ACTIVE_PARTION 0

#define ATA_DEVICE 1
#define SATA_DEVICE 2
#define USB_DEVICE 3

#define	FAT12_16 0x6
#define	FAT32 0xb
#define FAT32_ 0xc

typedef struct FAT32_BPB
{
	unsigned char 		bootjmp[3];
	unsigned char 		oem_name[8];
	unsigned short 	    bytes_per_sector;
	unsigned char		sectors_per_cluster;
	unsigned short		reserved_sector_count;
	unsigned char		table_count;
	unsigned short		root_entry_count;
	unsigned short		total_sectors_16;
	unsigned char		media_type;
	unsigned short		table_size_16;
	unsigned short		sectors_per_track;
	unsigned short		head_side_count;
	unsigned int 		hidden_sector_count;
	unsigned int 		total_sectors_32;
 
	//this will be cast to it's specific type once the driver actually knows what type of FAT this is.
 
}__attribute__((packed)) FAT32_BPB_t;

typedef struct FAT32_EBPB
{
	//extended fat32 stuff
	unsigned int		table_size_32;
	unsigned short		extended_flags;
	unsigned short		fat_version;
	unsigned int		root_cluster;
	unsigned short		fat_info;
	unsigned short		backup_BS_sector;
	unsigned char 		reserved_0[12];
	unsigned char		drive_number;
	unsigned char 		reserved_1;
	unsigned char		boot_signature;
	unsigned int 		volume_id;
	unsigned char		volume_label[11];
	unsigned char		fat_type_label[8];
 
}__attribute__((packed)) FAT32_EBPB_t;

typedef struct DiskPartion
{
	char is_active;
	char nhead_start;
	short ncyl_sec_start;
	char partion_type;
	char nhead_end;
	short ncyl_sec_end;
	uint32_t initial_sector;
	uint32_t nsectors;
} DiskPartion;

typedef struct StorageDevice
{
	DiskPartion partions[4];
	size_t nsectors;
	char type_device;
	void* device_ptr;
} StorageDevice;

typedef struct
{
	FAT32_BPB_t bpb;
	FAT32_EBPB_t ebpb;
} FAT32_BS;

typedef struct LogicalDisk
{	
	char file_system_code;
	uint32_t lba_start;
	size_t nsectors;
	StorageDevice *storage_device;
} LogicalDisk;

List disk_partions;
List* storage_devices;
List* logical_disks;

void init_file_system();
void disk_formatting(StorageDevice *storage_device, DiskPartion disk_partions);
void FAT32_partion_formatting(StorageDevice *storage_device, DiskPartion disk_partion);
void read_storage_device(StorageDevice storage_device, size_t LBA, size_t nsectors, void *buffer);

#endif