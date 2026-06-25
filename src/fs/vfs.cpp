#include "vfs.hpp"
#include "../libk/string.hpp"
#include "../memory/kmalloc.hpp"

#define MAX_MOUNTS 16

static struct vfs_mount
{
    char      path[256];
    vfs_node* root;
    bool      used;
} mounts[MAX_MOUNTS];

static int mount_count = 0;

void vfs_init()
{
    for(int i = 0; i < MAX_MOUNTS; i++)
        mounts[i].used = false;

    mount_count = 0;
}

int vfs_mount(const char* path, vfs_node* root)
{
    if(!path || !root)
        return -1;

    for(int i = 0; i < MAX_MOUNTS; i++)
    {
        if(!mounts[i].used)
        {
            strcpy(mounts[i].path, path);
            mounts[i].root = root;
            mounts[i].used = true;
            mount_count++;
            return 0;
        }
    }

    return -1;
}

vfs_node* vfs_get_root()
{
    for(int i = 0; i < MAX_MOUNTS; i++)
    {
        if(mounts[i].used && strcmp(mounts[i].path, "/") == 0)
            return mounts[i].root;
    }

    return nullptr;
}

static vfs_node* find_mount_root(const char* path, const char** rel_out)
{
    int best_len = -1;
    vfs_node* best_root = nullptr;

    for(int i = 0; i < MAX_MOUNTS; i++)
    {
        if(!mounts[i].used)
            continue;

        int len = 0;

        while(mounts[i].path[len] && path[len] &&
              mounts[i].path[len] == path[len])
        {
            len++;
        }

        if(mounts[i].path[len] == 0 &&
           (path[len] == '/' || path[len] == 0) &&
           len > best_len)
        {
            best_len = len;
            best_root = mounts[i].root;
        }
    }

    if(best_root && rel_out)
        *rel_out = path + best_len;

    return best_root;
}

vfs_node* vfs_resolve(const char* path)
{
    if(!path || !path[0])
        return nullptr;

    const char* rel = path;
    vfs_node* current = find_mount_root(path, &rel);

    if(!current)
        return nullptr;

    while(*rel)
    {
        while(*rel == '/')
            rel++;

        if(!*rel)
            break;

        char component[128];
        int ci = 0;

        while(*rel && *rel != '/' && ci < 127)
            component[ci++] = *rel++;

        component[ci] = 0;

        if(strcmp(component, ".") == 0)
            continue;

        if(strcmp(component, "..") == 0)
        {
            if(current->parent)
                current = current->parent;

            continue;
        }

        if(!current->finddir)
            return nullptr;

        current = current->finddir(current, component);

        if(!current)
            return nullptr;
    }

    return current;
}

vfs_node* vfs_resolve_relative(vfs_node* base, const char* path)
{
    if(!base || !path)
        return nullptr;

    if(path[0] == '/')
        return vfs_resolve(path);

    vfs_node* current = base;

    while(*path)
    {
        while(*path == '/')
            path++;

        if(!*path)
            break;

        char component[128];
        int ci = 0;

        while(*path && *path != '/' && ci < 127)
            component[ci++] = *path++;

        component[ci] = 0;

        if(strcmp(component, ".") == 0)
            continue;

        if(strcmp(component, "..") == 0)
        {
            if(current->parent)
                current = current->parent;

            continue;
        }

        if(!current->finddir)
            return nullptr;

        current = current->finddir(current, component);

        if(!current)
            return nullptr;
    }

    return current;
}

void vfs_destroy(vfs_node* node)
{
    if(!node)
        return;

    if(node->destroy)
        node->destroy(node);
}
