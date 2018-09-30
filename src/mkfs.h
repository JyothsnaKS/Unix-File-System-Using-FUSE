#ifndef MKFS
#define MKFS
#include "fs_param.h"
#include "inode_code.h"
int create_fs(void);
int create_super_block(void);
int create_inodes(void);
int write_inode(inode* node);
int create_free_blocks(void);
int create_root_dir(void);
#endif
