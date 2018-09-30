#ifndef INODE_CODE
#define INODE_CODE

#include "fs_param.h"
#include "namei.h"
int iget(int inode_no, inode* target);
int ialloc(inode *node);
int iput(inode* node);
int next_free_inode_number(void);
int put_inode(inode *node);
int get_inode(int inode_id, inode *node);
int get_parent_inode_id(const char *path);
int ifree(inode *node);

#endif
