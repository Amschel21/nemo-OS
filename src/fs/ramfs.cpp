#include "ramfs.hpp"

#include "../terminal.hpp"
#include "../libk/string.hpp"

static RamNode nodes[128];

static RamNode* root;
static RamNode* cwd;

static RamNode* ramfs_find(const char* name)
{
    for(int i = 0; i < 128; i++)
    {
        if(nodes[i].used &&
           nodes[i].parent == cwd &&
           strcmp(nodes[i].name, name) == 0)
        {
            return &nodes[i];
        }
    }

    return nullptr;
}

void ramfs_init()
{
    for(int i = 0; i < 128; i++)
    {
        nodes[i].used = false;
    }

    root = &nodes[0];

    root->used = true;
    root->is_dir = true;

    strcpy(root->name, "/");

    root->parent = root;

    cwd = root;
}

bool ramfs_create_dir(const char* name)
{
    for(int i = 0; i < 128; i++)
    {
        if(!nodes[i].used)
        {
            nodes[i].used = true;
            nodes[i].is_dir = true;

            strcpy(nodes[i].name, name);

            nodes[i].parent = cwd;

            return true;
        }
    }

    return false;
}

bool ramfs_create_file(const char* name)
{
    for(int i = 0; i < 128; i++)
    {
        if(!nodes[i].used)
        {
            nodes[i].used = true;
            nodes[i].is_dir = false;

            strcpy(nodes[i].name, name);

            nodes[i].parent = cwd;

            nodes[i].size = 0;

            return true;
        }
    }

    return false;
}

void ramfs_ls()
{
    for(int i = 0; i < 128; i++)
    {
        if(nodes[i].used &&
           nodes[i].parent == cwd &&
           &nodes[i] != cwd)
        {
            if(nodes[i].is_dir)
            {
                terminal.write("[DIR] ");
            }

            terminal.write(nodes[i].name);
            terminal.write("\n");
        }
    }
}

bool ramfs_cd(const char* name)
{
    if(strcmp(name, "..") == 0)
    {
        cwd = cwd->parent;
        return true;
    }

    for(int i = 0; i < 128; i++)
    {
        if(nodes[i].used &&
           nodes[i].is_dir &&
           nodes[i].parent == cwd &&
           strcmp(nodes[i].name, name) == 0)
        {
            cwd = &nodes[i];
            return true;
        }
    }

    return false;
}

const char* ramfs_pwd()
{
    return cwd->name;
}

bool ramfs_write(
    const char* name,
    const char* text)
{
    RamNode* node =
        ramfs_find(name);

    if(!node)
        return false;

    if(node->is_dir)
        return false;

    strcpy(node->data, text);

    node->size = 0;

    while(text[node->size])
    {
        node->size++;
    }

    return true;
}

bool ramfs_read(
    const char* name,
    char* buffer)
{
    RamNode* node =
        ramfs_find(name);

    if(!node)
        return false;

    if(node->is_dir)
        return false;

    strcpy(buffer, node->data);

    return true;
}