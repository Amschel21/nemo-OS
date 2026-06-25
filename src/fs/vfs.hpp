#pragma once
#include <stdint.h>

#define FS_FILE        1
#define FS_DIR         2
#define FS_MOUNTPOINT  4

struct vfs_node;

typedef void (*vfs_destroy_t)(vfs_node* node);
typedef int  (*vfs_read_t)(vfs_node* node, uint32_t offset, uint32_t size, void* buf);
typedef int  (*vfs_write_t)(vfs_node* node, uint32_t offset, uint32_t size, const void* buf);
typedef int  (*vfs_readdir_t)(vfs_node* node, uint32_t index, vfs_node* child);
typedef vfs_node* (*vfs_finddir_t)(vfs_node* node, const char* name);

struct vfs_node
{
    char          name[128];
    uint32_t      flags;
    uint32_t      size;
    uint32_t      inode;
    uint32_t      impl;
    vfs_node*     parent;

    vfs_destroy_t  destroy;
    vfs_read_t     read;
    vfs_write_t    write;
    vfs_readdir_t  readdir;
    vfs_finddir_t  finddir;
};

void     vfs_init();
int      vfs_mount(const char* path, vfs_node* root);
vfs_node* vfs_get_root();
vfs_node* vfs_resolve(const char* path);
vfs_node* vfs_resolve_relative(vfs_node* base, const char* path);
void     vfs_destroy(vfs_node* node);
