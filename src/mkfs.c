#include "mkfs.h"
int create_fs(void){
	if(create_super_block() == -1){	
		LOG("create_fs: Superblock failed.");
		return -1;
	}
	if(create_inodes() == -1){	
		LOG("create_fs: create inodes failed.");
		return -1;
	}
	if(create_free_blocks() == -1){	
		LOG("create_fs: freeblocks failed.");
		return -1;
	}
	if(create_root_dir() == -1){	
		LOG("create_fs: Root Dir failed.");
		return -1;
	}
	LOG("Yay! FS created.");
	return 0;
}

int create_super_block(void){
	
	superblock.fs_size = FS_SIZE;
	
	superblock.num_free_blocks = (NUM_OF_BLOCKS - NUM_OF_INODE_BLOCKS - 1) * (BLOCK_ID_LIST_LENGTH-1) / BLOCK_ID_LIST_LENGTH;
	// Extra blocks are required for managing the free lists, hence we do the above.
	//Super block is block number 0
	superblock.next_free_block_list = NUM_OF_INODE_BLOCKS + 1;
	superblock.num_free_inodes = NUM_OF_INODES;
	superblock.next_free_inode = 1;
	// root dir should take up inode 0;
	LOG("create_super_block: Superblock created.");
	return write_block(0, &superblock, sizeof(super_block));
}

int create_inodes(void){
	int i;
	for(i=0;i<NUM_OF_INODES;i++){
		inode node;
		node.inode_id = i + 1;
		node.type = TYPE_FREE;
		node.num_blocks = 0;

		if(write_inode(&node)==-1){
			LOG("create_inode: Inode could not be written");
			return -1;
		}
	}
	fprintf(stderr,"sizeof inode:%ld\n",sizeof(inode));
	LOG("create_inodes: Inodes created.");
	return 0;
}

int create_free_blocks(void)
{
	block_id_list bid_list;
	uint32_t num_free_block_lists = NUM_OF_BLOCKS - (1 + NUM_OF_INODE_BLOCKS + superblock.num_free_blocks);
	// above value is 3;
	uint32_t free_block_id = 1 + NUM_OF_INODE_BLOCKS + num_free_block_lists;
	// in our case, first free block is indexed at 5.
	uint32_t block_id;
	uint32_t next_block_id;
	for(int i = 0; i < num_free_block_lists; i++)
	{
		block_id = 1 + NUM_OF_INODE_BLOCKS + i;
		next_block_id = 0;
		if(i < num_free_block_lists - 1)
			next_block_id = block_id + 1;
		bid_list.list[0] = next_block_id;
		
		for(int j = BLOCK_ID_LIST_LENGTH - 1; j > 0 ; j--)
		{
			if(free_block_id < NUM_OF_BLOCKS)
				bid_list.list[j] = free_block_id++;
			else 
				bid_list.list[j] = 0;
		}
		if(write_block(block_id, &bid_list, sizeof(block_id_list)) == -1)
		{
			LOG("create_free_blocks: Error, write block while intializing free lists failed.");
			return -1;
		}
	} 
	LOG("create_free_blocks: Free block created.");
	return 0;
}
int create_root_dir(void)
{
	data_block block;
	if(data_block_alloc(&block) == -1)
	{
		LOG("create_root_dir: data_block alloc failed.");
		return -1;
	}
	inode node;
	if(ialloc(&node) == 1)
	{
		LOG("create_root_dir: inode alloc failed.");
		return -1;
	}
	node.type = TYPE_DIRECTORY;
	node.mode = S_IFDIR;

	dir_block dblock;
	uint32_t block_id = block.data_block_id;	
	if(init_dir_block(&dblock, node.inode_id, ROOT_INODE_NUMBER))
	{
		LOG("create_root_dir: dir_block alloc failed.");
		return -1;
	}
	write_block(block_id,&dblock,sizeof(dir_block));
	node.direct_blocks[0] = block_id;
	node.num_blocks +=1;
	put_inode(&node);

	LOG("create_root_dir: root dir created.");
	return 0;
}


int write_inode(inode* node){
	//Mapping inode_id to corresponding inode block_id
	//First inode_id is 1 (excluding root dir's inode_d). Hence, inode_id - 1
	//First inode block id is 1.Hence, the + 1
	uint32_t block_id = ((node->inode_id - 1) / (BLOCK_SIZE / INODE_SIZE)) + 1;
	//Calculating offset where this inode needs to be written in the inode block.
	// Mod(%) range from 0 -> (max no. of inodes in a block - 1)
	int offset = ((node->inode_id - 1) % (BLOCK_SIZE / INODE_SIZE)) * INODE_SIZE;	
	
	return write_block_offset(block_id, node, sizeof(struct inode), offset);
}

