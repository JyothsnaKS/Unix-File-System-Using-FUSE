#ifndef _COMMON_H_
#define _COMMON_H_

#include "fs_param.h"
#include "inode_code.h"

static int check_data_block_for_next_entry(dir_block *dblock, char *next_file);
int ags_namei(const char *path);

#endif

