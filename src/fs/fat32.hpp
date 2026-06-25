#pragma once
#include "../fs/vfs.hpp"

// Mount a FAT32 partition from an ATA device
// ata_dev: index into ata_device array
// partition: partition number (0 = first MBR entry)
vfs_node* fat32_mount(int ata_dev, int partition);
