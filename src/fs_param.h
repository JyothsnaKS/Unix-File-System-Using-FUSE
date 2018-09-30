#ifndef FSPARAMS
#define FSPARAMS
//#define FUSE_USE_VERSION 26

#define __STDC_FORMAT_MACROS 1
 
#include <stdio.h>
#include <string.h>
//#include <fuse.h>
#include <stdlib.h>
#include <time.h>
#include<stdint.h>
#include<libgen.h>
#include <sys/types.h>
#include <fcntl.h>
#include<sys/stat.h>
#include<unistd.h>
#include "log.h"


#define FNAME_SIZE 32 // This is 32 Bytes because we have 64 files possible. 64 * 32 = 2048 for array of inodes ids. And 64 *32 for char array.
#define FS_SIZE (300 * 4096) // 1200KB 
#define BLOCK_SIZE 4096
#define NUM_OF_BLOCKS (FS_SIZE / BLOCK_SIZE)
#define NUM_FILES_DIR (BLOCK_SIZE/32)
#define INODE_SIZE (sizeof(inode))
#define NUM_OF_INODES (0.1 * NUM_OF_BLOCKS) // Need limits on number of files. Arbit put as 10%, so 30 Inode. 
#define NUM_OF_INODE_BLOCKS ((NUM_OF_INODES * INODE_SIZE)/BLOCK_SIZE + 1)
// What this means is, on file system startup, we need to pull the first 2 blocks and keep it in memory. 
// First block (block 0) is superblock and block 1 is inode table.
#define NUM_DIRECT_BLOCKS 3
#define ILIST_BEGIN 1
// FIRST INODE BLOCK NUMBER
#define BLOCK_ID_LIST_LENGTH (BLOCK_SIZE / 32)
#define ROOT_INODE_NUMBER 1
// INODE VALUE FOR ROOT.
// BLOCK_ID_LIST_LENGTH is the number of words in any block. 

#define PATH_DELIMITER "/"


typedef enum file_type{
	TYPE_ORDINARY, // its a file
	TYPE_DIRECTORY,// its a dir
	TYPE_FREE // its free.
}file_type;

typedef struct super_block {
	uint32_t fs_size;

	uint32_t num_free_blocks;
	uint32_t next_free_block_list;

	uint32_t num_free_inodes;
	uint32_t next_free_inode;

}super_block;
super_block superblock;
typedef struct inode {
	int inode_id;
	uid_t uid;
	gid_t gid;
	mode_t mode;
	file_type type;
	time_t last_modified_file;
	time_t last_accessed_file;
	time_t last_modified_inode;
	int links_nb;//TODO: Remove this and update 'extra_memory_for_future_use'
	uint32_t direct_blocks[NUM_DIRECT_BLOCKS];
      
	//uint32_t single_indirect_block;
	uint32_t num_blocks; // Number of blocks that are used
	int num_used_bytes_in_last_block; // Number of bytes used in last datablock for the file
	uint32_t num_allocated_blocks;//TODO: How is this different from num_blocks?
	char extra_memory_for_future_use[48]; // up to future programmers to change
}inode;
typedef struct block_id_list {
	uint32_t list[BLOCK_ID_LIST_LENGTH];
}block_id_list;
typedef struct data_block {
	uint32_t data_block_id;
	char block[BLOCK_SIZE];
}data_block;
typedef struct dir_block{
	int inode_ids[BLOCK_SIZE/32]; //TODO: Is this change correct?
	char names[BLOCK_SIZE/32][FNAME_SIZE]; //TODO: Is this change correct?
}dir_block;

int init_superblock(void);
int start_emulator(void);
int read_block(uint32_t block_id, void* dest);
int write_block(uint32_t block_id, void* buffer, size_t buffer_size);
int write_block_offset(uint32_t block_id, void *buffer, size_t buffer_size, int offset);

int commit_superblock(void);
int if_list_contains_free_block(char* datablock);
void do_set_block_number(char *datablock,int i, uint32_t block_no);
uint32_t get_free_block_number(char* datablock);
int data_block_alloc(data_block *block);
int bread(uint32_t data_block_no, data_block *block);
int bwrite(data_block* block);

int add_entry_to_dir_block(dir_block* dblock, uint32_t id, char* name);
uint32_t get_block_id(inode *node,uint32_t index);
int set_block_id(inode *node, uint32_t index, uint32_t block_id);
int add_entry_to_parent(inode *parent_inode, int inode_id, char *name);
int init_dir_block(dir_block *dblock, uint32_t id, uint32_t parent);
static int check_data_block_for_next_entry(dir_block *dblock, char *next_file);
int remove_entry_from_parent(inode *parent_inode, int inode_id);
int remove_entry_from_dir_block(dir_block *dblock, int inode_id);
int data_block_free(data_block *block);
int get_position_of_free_spot_in_free_list_for_datablock(char * datablock);
int is_datablock_full_of_free_datablock_numbers(char * datablock);
#endif 
