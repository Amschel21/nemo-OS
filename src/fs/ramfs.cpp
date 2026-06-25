#include "ramfs.hpp"
#include "../terminal.hpp"
#include "../libk/string.hpp"
#include "../libk/itoa.hpp"
#include "../memory/kmalloc.hpp"
#include "../memory/pmm_bitmap.hpp"

#define MAX_DATA 256

struct ramfs_node
{
    vfs_node vnode;
    ramfs_node* next_sibling;
    ramfs_node* first_child;
    ramfs_node* parent;
    char data[MAX_DATA];
};

static ramfs_node* node_pool[128];
static int pool_count = 0;
static ramfs_node* root_node;

static ramfs_node* alloc_node()
{
    if(pool_count >= 128)
        return nullptr;

    ramfs_node* n = (ramfs_node*)kmalloc(sizeof(ramfs_node));

    if(!n)
        return nullptr;

    node_pool[pool_count++] = n;
    return n;
}

static void ramfs_destroy(vfs_node* node)
{
    (void)node;
}

static int ramfs_read(vfs_node* node, uint32_t offset, uint32_t size, void* buf)
{
    if(!node || !(node->flags & FS_FILE))
        return -1;

    ramfs_node* rn = (ramfs_node*)node;

    if(offset >= node->size)
        return 0;

    uint32_t remain = node->size - offset;

    if(size > remain)
        size = remain;

    for(uint32_t i = 0; i < size; i++)
        ((uint8_t*)buf)[i] = (uint8_t)rn->data[offset + i];

    return size;
}

static int ramfs_write(vfs_node* node, uint32_t offset, uint32_t size, const void* buf)
{
    if(!node || !(node->flags & FS_FILE))
        return -1;

    ramfs_node* rn = (ramfs_node*)node;

    if(offset + size > MAX_DATA)
        size = MAX_DATA - offset;

    for(uint32_t i = 0; i < size; i++)
        rn->data[offset + i] = ((const uint8_t*)buf)[i];

    if(offset + size > node->size)
        node->size = offset + size;

    return size;
}

static int ramfs_readdir(vfs_node* node, uint32_t index, vfs_node* child)
{
    if(!node || !(node->flags & FS_DIR))
        return -1;

    ramfs_node* rn = (ramfs_node*)node;
    ramfs_node* cur = rn->first_child;
    uint32_t i = 0;

    while(cur)
    {
        if(i == index)
        {
            strcpy(child->name, cur->vnode.name);
            child->flags = cur->vnode.flags;
            child->size = cur->vnode.size;
            child->inode = (uint32_t)cur;
            child->impl = cur->vnode.impl;
            child->read = nullptr;
            child->write = nullptr;
            child->readdir = nullptr;
            child->finddir = nullptr;
            child->destroy = nullptr;
            return 0;
        }

        cur = cur->next_sibling;
        i++;
    }

    return -1;
}

static vfs_node* ramfs_finddir(vfs_node* node, const char* name)
{
    if(!node || !(node->flags & FS_DIR))
        return nullptr;

    ramfs_node* rn = (ramfs_node*)node;
    ramfs_node* cur = rn->first_child;

    while(cur)
    {
        if(strcmp(cur->vnode.name, name) == 0)
            return &cur->vnode;

        cur = cur->next_sibling;
    }

    return nullptr;
}

vfs_node* ramfs_create_root()
{
    root_node = alloc_node();

    if(!root_node)
        return nullptr;

    strcpy(root_node->vnode.name, "/");
    root_node->vnode.flags = FS_DIR;
    root_node->vnode.size = 0;
    root_node->vnode.inode = 0;
    root_node->vnode.impl = 0;
    root_node->vnode.parent = &root_node->vnode;
    root_node->vnode.destroy = ramfs_destroy;
    root_node->vnode.read = nullptr;
    root_node->vnode.write = nullptr;
    root_node->vnode.readdir = ramfs_readdir;
    root_node->vnode.finddir = ramfs_finddir;
    root_node->next_sibling = nullptr;
    root_node->first_child = nullptr;
    root_node->parent = root_node;

    return &root_node->vnode;
}

vfs_node* ramfs_create_file(vfs_node* parent, const char* name)
{
    if(!parent || !(parent->flags & FS_DIR))
        return nullptr;

    ramfs_node* pn = (ramfs_node*)parent;
    ramfs_node* n = alloc_node();

    if(!n)
        return nullptr;

    strcpy(n->vnode.name, name);
    n->vnode.flags = FS_FILE;
    n->vnode.size = 0;
    n->vnode.inode = (uint32_t)n;
    n->vnode.impl = 0;
    n->vnode.parent = &pn->vnode;
    n->vnode.destroy = ramfs_destroy;
    n->vnode.read = ramfs_read;
    n->vnode.write = ramfs_write;
    n->vnode.readdir = nullptr;
    n->vnode.finddir = nullptr;
    n->next_sibling = nullptr;
    n->first_child = nullptr;
    n->parent = pn;

    n->next_sibling = pn->first_child;
    pn->first_child = n;

    return &n->vnode;
}

vfs_node* ramfs_create_dir(vfs_node* parent, const char* name)
{
    if(!parent || !(parent->flags & FS_DIR))
        return nullptr;

    ramfs_node* pn = (ramfs_node*)parent;
    ramfs_node* n = alloc_node();

    if(!n)
        return nullptr;

    strcpy(n->vnode.name, name);
    n->vnode.flags = FS_DIR;
    n->vnode.size = 0;
    n->vnode.inode = (uint32_t)n;
    n->vnode.impl = 0;
    n->vnode.parent = &pn->vnode;
    n->vnode.destroy = ramfs_destroy;
    n->vnode.read = nullptr;
    n->vnode.write = nullptr;
    n->vnode.readdir = ramfs_readdir;
    n->vnode.finddir = ramfs_finddir;
    n->next_sibling = nullptr;
    n->first_child = nullptr;
    n->parent = pn;

    n->next_sibling = pn->first_child;
    pn->first_child = n;

    return &n->vnode;
}

vfs_node* ramfs_touch(const char* name)
{
    vfs_node* cwd = vfs_resolve(".");

    if(!cwd)
        return nullptr;

    return ramfs_create_file(cwd, name);
}

vfs_node* ramfs_mkdir(const char* name)
{
    vfs_node* cwd = vfs_resolve(".");

    if(!cwd)
        return nullptr;

    return ramfs_create_dir(cwd, name);
}
