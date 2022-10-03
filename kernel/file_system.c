#include "include/file_system.h"
#include "include/ATA.h"
#include "include/memory_manager.h"
#include "include/tty.h" 
#include "include/List.h"


void read_storage_device(StorageDevice storage_device, size_t LBA, size_t nsectors, void *buffer)
{
	switch (storage_device.type_device)
	{
		case 0:
			break;
		case 1:
			read_ATA(*((AtaDevice*)(storage_device.device_ptr)), LBA, nsectors, buffer);
			break;
	}
}

void write_storage_device(StorageDevice storage_device, size_t LBA, size_t nsectors, void *buffer)
{
	switch (storage_device.type_device)
	{
		case 0:
			break;
		case 1:
			write_ATA(*((AtaDevice*)(storage_device.device_ptr)), LBA, nsectors, buffer);
	}
}

void read_logical_disk(LogicalDisk logical_disk, size_t lba_offset, size_t nsectors, void* buffer)
{
	read_storage_device(*(logical_disk.storage_device), logical_disk.lba_start + lba_offset, nsectors, buffer);
}


List* init_logical_disks()
{
	List *out = list_create(sizeof(LogicalDisk));

	char* buffer = kmalloc(0x200);

	foreach(item, storage_devices)
	{
		read_storage_device(*((StorageDevice*)(item->value_ptr)), 0, 1, buffer);
		LogicalDisk logical_disk;
		for (int i = 446, n = 0; n < 4; n++, i += 16)
		{
			if (buffer[i + 4] == 0) continue;

			logical_disk.file_system_code = buffer[i + 4];
			logical_disk.lba_start = *((uint32_t*)(&buffer[i + 8]));
			logical_disk.nsectors = *((uint32_t*)(&buffer[i + 12]));
			logical_disk.storage_device = item->value_ptr;
			
			list_insert(out, &logical_disk);
		}
	}
	kfree(buffer);
	return out;
}


List* init_storage_devices()
{
	List *out = list_create(sizeof(StorageDevice));

	//Append Ata devices
	StorageDevice storage_device;
	for (int i = 0; i < 4; i++)
	{
		if (!(ata_devices[i].available)) continue;
		   
		storage_device.nsectors = ata_devices[i].sector_nubmer;
		storage_device.type_device = ATA_DEVICE;
		storage_device.device_ptr = &(ata_devices[i]);

		list_insert(out, &storage_device);
	}
	return out;
}

void init_file_system()
{
	init_ATA_devices();

	storage_devices = init_storage_devices();
	logical_disks = init_logical_disks();
	
	FAT32_BS *bs_buffer = kmalloc(0x200);
	LogicalDisk* logical_disk = list_get_value(logical_disks, 0);

	read_logical_disk(*logical_disk, 0, 1, bs_buffer);

	printf("%x\n", bs_buffer->bpb.sectors_per_cluster);
	printf("%x\n", bs_buffer->ebpb.root_cluster * bs_buffer->bpb.sectors_per_cluster);
}
/*
{
	void disk_formatting(StorageDevice *storage_device, DiskPartion disk_partions)
	{
		char* buffer = kmalloc(0x200);
		read_storage_device(*storage_device, 0, 1, buffer);

		memcpy((DiskPartion*)(&(buffer[446])), &disk_partions, 0x10 * 4); 

		write_storage_device(*storage_device, 0, 1, buffer);
		kfree(buffer);

		memcpy(storage_device->partions, &disk_partions, 0x10 * 4);
	}

	void FAT32_partion_formatting(StorageDevice *storage_device, DiskPartion disk_partion)
	{
		char *buffer = kmalloc(0x200);
		FAT32_BPB_t *fat_boot = ((FAT32_BPB_t*)buffer);
		FAT32_EBPB_t *fat_boot_ext = ((FAT32_EBPB_t*)(buffer + 0x20));
		for (int i = 0; i < 0x200; i++) buffer[i] = 0;

		fat_boot->bootjmp[0] = 0xeb; // 0x00
		fat_boot->bootjmp[1] = 0xfe;
		fat_boot->bootjmp[2] = 0x90;
		fat_boot->oem_name[0] = 'M'; // 0x03
		fat_boot->oem_name[1] = 'S';
		fat_boot->oem_name[2] = 'W';
		fat_boot->oem_name[3] = 'I';
		fat_boot->oem_name[4] = 'N';
		fat_boot->oem_name[5] = '4';
		fat_boot->oem_name[6] = '.';
		fat_boot->oem_name[7] = '1';
		fat_boot->bytes_per_sector = 0x020; // 0x0b
		fat_boot->sectors_per_cluster = 8; // 0x0d
		fat_boot->reserved_sector_count = 8; // 0x0e
		fat_boot->table_count = 2; // 0x10
		fat_boot->root_entry_count = 0; // 0x11
		fat_boot->total_sectors_16 = 0; // 0x13
		fat_boot->media_type = 0xf8; // 0x15
		fat_boot->table_size_16 = 0; // 0x16
		fat_boot->sectors_per_track = 0; // 0x18
		fat_boot->head_side_count = 0; // 0x1a
		fat_boot->hidden_sector_count = 0; // 0x1c
		fat_boot->total_sectors_32 = disk_partion.nsectors; // 0x20

		fat_boot_ext->table_size_32 = 0x07f8; // 0x024
		fat_boot_ext->extended_flags = 0; // 0x028
		fat_boot_ext->fat_version = 0; // 0x02a
		fat_boot_ext->root_cluster = 2; // 0x02c
		fat_boot_ext->fat_info = 1; // 0x030
		fat_boot_ext->backup_BS_sector = 6; // 0x032
		fat_boot_ext->reserved_0[12]; // 0x034
		fat_boot_ext->drive_number = 0x80; // 0x040
		fat_boot_ext->reserved_1 = 0; // 0x041
		fat_boot_ext->boot_signature = 0x29; // 0x042
		fat_boot_ext->volume_id = 0x468d758d; // 0x043
		fat_boot_ext->volume_label[11]; // 0x047
		fat_boot_ext->fat_type_label[0] = 'F'; // 0x052
		fat_boot_ext->fat_type_label[1] = 'A';
		fat_boot_ext->fat_type_label[2] = 'T';
		fat_boot_ext->fat_type_label[3] = '3';
		fat_boot_ext->fat_type_label[4] = '2';

		buffer[510] = 0x55;
		buffer[511] = 0xaa;
		
		write_storage_device(*storage_device, disk_partion.initial_sector, 1, buffer);
		//reserve copy
		write_storage_device(*storage_device, disk_partion.initial_sector + fat_boot_ext->backup_BS_sector, 1, buffer);

		//
		for (int i = 0; i < 0x200; i++) buffer[i] = 0;
		uint32_t *buffer2 = ((uint32_t*)buffer);

		buffer2[0] = 0x41615252;
		buffer2[0x1e4] = 0x61417272;
		buffer2[0x1e8] = 0xffffffff;
		buffer2[0x1ec] = 0xffffffff;
		buffer2[0x1fc] = 0xaa550000;

		write_storage_device(*storage_device, disk_partion.initial_sector + 1, 1, buffer);

		printf("%x\n", get_page_info(0x1000, fat_boot));
		printf("%x\n", ((int)fat_boot) & 0xfff);
	}


	void partion_formatting(StorageDevice *storage_device, DiskPartion disk_partion)
	{
		switch (disk_partion.partion_type)
		{
			case FAT12_16:
				break;
			case FAT32:
				FAT32_partion_formatting(storage_device, disk_partion);
		}

	}


	void FAT32_logical_disk_init(StorageDevice *storage_device, char npartion)
	{
		void* buffer = kmalloc(0x200);
		read_storage_device(*storage_device, ((DiskPartion*)(storage_device->partions))[npartion].initial_sector, 1, buffer);

		LogicalDisk logical_disk = {
			((DiskPartion*)(storage_device->partions))[npartion].partion_type, \
			((DiskPartion*)(storage_device->partions))[npartion].initial_sector, \
			storage_device
		};
		add(&logical_disks, &logical_disk);

		kfree(buffer);
	}


	void init_logical_disk(StorageDevice *storage_device, char npartion)
	{
		DiskPartion *disk_partions = storage_device->partions;

		switch (disk_partions[npartion].partion_type)
		{	
			case FAT12_16:
				break;
			case FAT32:
				FAT32_logical_disk_init(storage_device, npartion);
				break;
			case FAT32_:
				FAT32_logical_disk_init(storage_device, npartion);
				break;
		}
	}
}
*/