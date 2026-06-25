#include "fat32.hpp"
#include "../drivers/ata.hpp"
#include "../memory/kmalloc.hpp"
#include "../libk/memory.hpp"
#include "../libk/string.hpp"

#pragma pack(push, 1)

struct mbr_entry
{
    uint8_t  status;
    uint8_t  chs_first[3];
    uint8_t  type;
    uint8_t  chs_last[3];
    uint32_t lba_start;
    uint32_t sector_count;
};

struct mbr
{
    uint8_t  code[446];
    mbr_entry partitions[4];
    uint16_t signature;
};

struct fat32_bpb
{
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_16;
    uint8_t  media_descriptor;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t sectors_per_fat_32;
    uint16_t extended_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fsinfo_sector;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved2;
    uint8_t  ext_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
};

struct fat32_dirent
{
    char     name[11];
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  creation_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t modified_time;
    uint16_t modified_date;
    uint16_t cluster_low;
    uint32_t file_size;
};

#pragma pack(pop)

#define ATTR_READONLY  0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME_ID 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20
#define ATTR_LFN       0x0F

#define FAT32_EOC 0x0FFFFFF8

struct fat32_fs
{
    int      ata_dev;
    uint32_t partition_start;
    uint32_t bytes_per_sector;
    uint32_t sectors_per_cluster;
    uint32_t reserved_sectors;
    uint32_t fat_count;
    uint32_t sectors_per_fat;
    uint32_t root_cluster;
    uint32_t data_start;
};

struct fat32_node
{
    vfs_node  vnode;
    fat32_fs* fs;
    uint32_t  first_cluster;
    bool      is_dir;
};

static int fat32_read_sector(fat32_fs* fs, uint32_t lba, void* buf)
{
    return ata_read_sector(fs->ata_dev, fs->partition_start + lba, buf);
}

static uint32_t fat32_next_cluster(fat32_fs* fs, uint32_t cluster)
{
    uint32_t fat_offset = cluster * 4;
    uint32_t fat_sector = fs->reserved_sectors + (fat_offset / fs->bytes_per_sector);
    uint32_t fat_byte   = fat_offset % fs->bytes_per_sector;

    uint8_t buf[ATA_SECTOR_SIZE];

    if(fat32_read_sector(fs, fat_sector, buf) < 0)
        return FAT32_EOC;

    uint32_t next = *(uint32_t*)(buf + fat_byte);
    next &= 0x0FFFFFFF;

    return next;
}

static void fat32_cluster_to_lba(fat32_fs* fs, uint32_t cluster, uint32_t* sector, uint32_t* count)
{
    uint32_t rel = (cluster - 2) * fs->sectors_per_cluster;
    *sector = fs->data_start + rel;
    *count = fs->sectors_per_cluster;
}

static int fat32_read_cluster(fat32_fs* fs, uint32_t cluster, uint8_t* buf)
{
    uint32_t sector, count;

    fat32_cluster_to_lba(fs, cluster, &sector, &count);

    for(uint32_t i = 0; i < count; i++)
    {
        if(fat32_read_sector(fs, sector + i, buf + i * ATA_SECTOR_SIZE) < 0)
            return -1;
    }

    return 0;
}

static int fat32_readdir(vfs_node* node, uint32_t index, vfs_node* child);

static void fat32_node_destroy(vfs_node* node)
{
    fat32_node* fn = (fat32_node*)node;

    if(fn)
        kfree(fn);
}

static int fat32_read(vfs_node* node, uint32_t offset, uint32_t size, void* buf)
{
    fat32_node* fn = (fat32_node*)node;

    if(fn->is_dir)
        return -1;

    if(offset >= node->size)
        return 0;

    if(offset + size > node->size)
        size = node->size - offset;

    uint32_t bytes_per_cluster = fn->fs->sectors_per_cluster * ATA_SECTOR_SIZE;
    uint32_t cluster = fn->first_cluster;
    uint32_t read = 0;
    uint8_t* out = (uint8_t*)buf;

    while(cluster < FAT32_EOC && read < size)
    {
        uint32_t cluster_start = (cluster - 2) * bytes_per_cluster;

        if(offset < cluster_start + bytes_per_cluster)
        {
            uint32_t cluster_off = offset - cluster_start;
            uint32_t to_read = bytes_per_cluster - cluster_off;

            if(to_read > size - read)
                to_read = size - read;

            uint8_t cluster_buf[ATA_SECTOR_SIZE * 8];

            if(fat32_read_cluster(fn->fs, cluster, cluster_buf) < 0)
                return read;

            for(uint32_t i = 0; i < to_read; i++)
                out[read++] = cluster_buf[cluster_off + i];

            offset += to_read;
        }

        cluster = fat32_next_cluster(fn->fs, cluster);
    }

    return read;
}

static int fat32_write(vfs_node* node, uint32_t offset, uint32_t size, const void* buf)
{
    (void)node;
    (void)offset;
    (void)size;
    (void)buf;
    return -1;
}

static vfs_node* fat32_finddir(vfs_node* node, const char* name)
{
    fat32_node* fn = (fat32_node*)node;

    if(!fn->is_dir)
        return nullptr;

    uint32_t cluster = fn->first_cluster;
    uint8_t cluster_buf[ATA_SECTOR_SIZE * 8];
    uint32_t bytes_per_cluster = fn->fs->sectors_per_cluster * ATA_SECTOR_SIZE;

    while(cluster < FAT32_EOC)
    {
        if(fat32_read_cluster(fn->fs, cluster, cluster_buf) < 0)
            return nullptr;

        for(uint32_t off = 0; off + sizeof(fat32_dirent) <= bytes_per_cluster;
            off += sizeof(fat32_dirent))
        {
            fat32_dirent* entry = (fat32_dirent*)(cluster_buf + off);

            if(entry->name[0] == 0)
                break;

            if((unsigned char)entry->name[0] == 0xE5)
                continue;

            if(entry->attr == ATTR_LFN)
                continue;

            char entry_name[13];
            int ei = 0;

            for(int j = 0; j < 8; j++)
            {
                if(entry->name[j] == ' ')
                    break;
                entry_name[ei++] = entry->name[j];
            }

            if((entry->attr & ATTR_DIRECTORY) == 0)
            {
                if(entry->name[8] != ' ')
                {
                    entry_name[ei++] = '.';

                    for(int j = 8; j < 11; j++)
                    {
                        if(entry->name[j] == ' ')
                            break;
                        entry_name[ei++] = entry->name[j];
                    }
                }
            }

            entry_name[ei] = 0;

            if(strcmp(entry_name, name) != 0)
                continue;

            fat32_node* child = (fat32_node*)kmalloc(sizeof(fat32_node));

            if(!child)
                return nullptr;

            memset(child, 0, sizeof(fat32_node));

            strcpy(child->vnode.name, entry_name);
            child->vnode.flags = (entry->attr & ATTR_DIRECTORY) ? FS_DIR : FS_FILE;
            child->vnode.parent = &fn->vnode;
            child->vnode.destroy = fat32_node_destroy;
            child->vnode.read = (entry->attr & ATTR_DIRECTORY) ? nullptr : fat32_read;
            child->vnode.write = nullptr;
            child->vnode.readdir = (entry->attr & ATTR_DIRECTORY) ? fat32_readdir : nullptr;
            child->vnode.finddir = (entry->attr & ATTR_DIRECTORY) ? fat32_finddir : nullptr;
            child->vnode.size = entry->file_size;
            child->vnode.inode = off;
            child->vnode.impl = 0;
            child->fs = fn->fs;
            child->first_cluster =
                ((uint32_t)entry->cluster_high << 16) | entry->cluster_low;
            child->is_dir = (entry->attr & ATTR_DIRECTORY) != 0;

            return &child->vnode;
        }

        cluster = fat32_next_cluster(fn->fs, cluster);
    }

    return nullptr;
}

static int fat32_readdir(vfs_node* node, uint32_t index, vfs_node* child)
{
    fat32_node* fn = (fat32_node*)node;

    if(!fn->is_dir)
        return -1;

    uint32_t cluster = fn->first_cluster;
    uint8_t cluster_buf[ATA_SECTOR_SIZE * 8];
    uint32_t bytes_per_cluster = fn->fs->sectors_per_cluster * ATA_SECTOR_SIZE;
    uint32_t count = 0;

    while(cluster < FAT32_EOC)
    {
        if(fat32_read_cluster(fn->fs, cluster, cluster_buf) < 0)
            return -1;

        for(uint32_t off = 0; off + sizeof(fat32_dirent) <= bytes_per_cluster;
            off += sizeof(fat32_dirent))
        {
            fat32_dirent* entry = (fat32_dirent*)(cluster_buf + off);

            if(entry->name[0] == 0)
                break;

            if((unsigned char)entry->name[0] == 0xE5)
                continue;

            if(entry->attr == ATTR_LFN)
                continue;

            if(count == index)
            {
                char entry_name[13];
                int ei = 0;

                for(int j = 0; j < 8; j++)
                {
                    if(entry->name[j] == ' ')
                        break;
                    entry_name[ei++] = entry->name[j];
                }

                if((entry->attr & ATTR_DIRECTORY) == 0)
                {
                    if(entry->name[8] != ' ')
                    {
                        entry_name[ei++] = '.';

                        for(int j = 8; j < 11; j++)
                        {
                            if(entry->name[j] == ' ')
                                break;
                            entry_name[ei++] = entry->name[j];
                        }
                    }
                }

                entry_name[ei] = 0;

                strcpy(child->name, entry_name);
                child->flags = (entry->attr & ATTR_DIRECTORY) ? FS_DIR : FS_FILE;
                child->size = entry->file_size;
                child->inode = off;
                child->impl = 0;
                child->read = nullptr;
                child->write = nullptr;
                child->readdir = nullptr;
                child->finddir = nullptr;
                child->destroy = nullptr;

                return 0;
            }

            count++;
        }

        cluster = fat32_next_cluster(fn->fs, cluster);
    }

    return -1;
}

static vfs_node* fat32_get_root_node(fat32_fs* fs)
{
    fat32_node* root = (fat32_node*)kmalloc(sizeof(fat32_node));

    if(!root)
        return nullptr;

    memset(root, 0, sizeof(fat32_node));

    strcpy(root->vnode.name, "/");
    root->vnode.flags = FS_DIR;
    root->vnode.size = 0;
    root->vnode.inode = 0;
    root->vnode.impl = 0;
    root->vnode.destroy = fat32_node_destroy;
    root->vnode.read = nullptr;
    root->vnode.write = nullptr;
    root->vnode.readdir = fat32_readdir;
    root->vnode.finddir = fat32_finddir;
    root->fs = fs;
    root->first_cluster = fs->root_cluster;
    root->is_dir = true;

    return &root->vnode;
}

vfs_node* fat32_mount(int ata_dev, int partition)
{
    if(ata_dev < 0 || ata_dev >= ata_device_count())
        return nullptr;

    uint8_t mbr_buf[512];

    if(ata_read_sector(ata_dev, 0, mbr_buf) < 0)
        return nullptr;

    mbr* m = (mbr*)mbr_buf;

    if(m->signature != 0xAA55)
        return nullptr;

    if(partition < 0 || partition >= 4)
        return nullptr;

    mbr_entry* part = &m->partitions[partition];

    if(part->type != 0x0B && part->type != 0x0C)
        return nullptr;

    fat32_fs* fs = (fat32_fs*)kmalloc(sizeof(fat32_fs));

    if(!fs)
        return nullptr;

    memset(fs, 0, sizeof(fat32_fs));

    fs->ata_dev = ata_dev;
    fs->partition_start = part->lba_start;

    uint8_t bpb_buf[512];

    if(ata_read_sector(ata_dev, part->lba_start, bpb_buf) < 0)
    {
        kfree(fs);
        return nullptr;
    }

    fat32_bpb* bpb = (fat32_bpb*)bpb_buf;

    fs->bytes_per_sector = bpb->bytes_per_sector;
    fs->sectors_per_cluster = bpb->sectors_per_cluster;
    fs->reserved_sectors = bpb->reserved_sectors;
    fs->fat_count = bpb->fat_count;
    fs->sectors_per_fat = bpb->sectors_per_fat_32;
    fs->root_cluster = bpb->root_cluster;
    fs->data_start = fs->reserved_sectors +
                     fs->fat_count * fs->sectors_per_fat;

    return fat32_get_root_node(fs);
}
