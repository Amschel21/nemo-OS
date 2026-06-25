#pragma once
#include "vfs.hpp"

// RAM root creation
vfs_node* ramfs_create_root();

// File creation helpers (relative to root)
vfs_node* ramfs_create_file(vfs_node* parent, const char* name);
vfs_node* ramfs_create_dir(vfs_node* parent, const char* name);

// Convenience (resolve then create)
vfs_node* ramfs_touch(const char* name);
vfs_node* ramfs_mkdir(const char* name);
