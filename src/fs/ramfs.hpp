#pragma once

typedef unsigned int uint32_t;

struct RamNode
{
    char name[32];

    bool used;
    bool is_dir;

    RamNode* parent;

    char data[256];
    uint32_t size;
};

void ramfs_init();

bool ramfs_create_file(const char* name);
bool ramfs_create_dir(const char* name);

void ramfs_ls();

bool ramfs_write(
    const char* name,
    const char* text);

bool ramfs_read(
    const char* name,
    char* buffer);

bool ramfs_cd(const char* name);

const char* ramfs_pwd();