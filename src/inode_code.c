#include "inode_code.h"
//TODO: get_inode and put_inode redundant.
int iput(inode* node){

	if(node->links_nb == 0)
	{
		LOG("iput: freeing node.");
		data_block dblock;
		uint32_t total_blocks = node->num_blocks;
		uint32_t i;
		for(i = 0; i<total_blocks;i++)
		{
			uint32_t block_id = get_block_id(node,i);
			if(block_id > 0)
			{
				dblock.data_block_id = block_id;
				data_block_free(&dblock);
				node->num_allocated_blocks--;
			}
		}
		return ifree(node);
	}

	data_block block;
	if(bread((ILIST_BEGIN + (node->inode_id - 1)/(BLOCK_SIZE/INODE_SIZE)), &block)==-1){
		LOG("iput: bread failed");
		return -1;
	}

	uint32_t inode_index = ((node->inode_id - 1)%(BLOCK_SIZE/INODE_SIZE))*INODE_SIZE;
	memcpy(&(block.block[inode_index]),node, sizeof(inode));
	return bwrite(&block);
}

int put_inode(inode *node)
{
	if(iput(node)==0)
		return 0;
	return -1;
}

int get_inode(int inode_id, inode *node) {
        if(iget(inode_id, node) == 0) {
            return 0;
        }
	return -1;
}

int get_parent_inode_id(const char *path) {
	if(strcmp(PATH_DELIMITER, path) == 0) {
		return ROOT_INODE_NUMBER;
	}
	char *dup_path = strdup(path);
	char *dir = dirname(dup_path);
	int inode_id = ags_namei(dir); //Root inode -> it's data block -> dir block's entries to find this path an returning corresponding inode id
	free(dup_path);
	fprintf(stderr,"gpii: %d\n",inode_id);
	return inode_id;
}
int next_free_inode_number(void)
{
	data_block block;
	inode* node;
	int i,j;
	
	node=(inode*)malloc(sizeof(inode));
	
	for(i=1;i<=NUM_OF_INODE_BLOCKS;i++){
		bread(i,&block);
		for(j=0;j<BLOCK_SIZE/INODE_SIZE;j++){
			memcpy(node,&(block.block[j*INODE_SIZE]),sizeof(inode));
			if(node->type==TYPE_FREE)
				return node->inode_id;		
		}
	}	
	return -1;
}

int iget(int inode_no, inode* target)
{
	int block_no;
	int inode_index;

	if(inode_no<1||inode_no>NUM_OF_INODES){
		LOG("iget: Invalid inode");
		return -1;
	}

	block_no = ILIST_BEGIN + ((inode_no - 1)/(BLOCK_SIZE/INODE_SIZE));
	inode_index  = ((inode_no - 1)%(BLOCK_SIZE/INODE_SIZE))*INODE_SIZE;

	data_block inode_block;

	if(bread(block_no,&inode_block)==-1){
		LOG("iget: bread failed");
		return -1;
	}

	memcpy(target,&(inode_block.block[inode_index]),sizeof(inode));
	return 0;
}

int ialloc(inode *node)
{
	data_block block;
	int free_inode_number;
	int inode_index;

	free_inode_number = next_free_inode_number();
	if(free_inode_number==-1){
		LOG("No free inode number");
		return -1;
	}

	iget(free_inode_number, node);

	superblock.num_free_inodes--;
	commit_superblock();
	node->links_nb = 1;

	//Update on disk
	bread(ILIST_BEGIN + ((node->inode_id - 1)/(BLOCK_SIZE/INODE_SIZE)), &block);

	inode_index = ((node->inode_id - 1)%(BLOCK_SIZE/INODE_SIZE)) * INODE_SIZE;
	memcpy(&(block.block[inode_index]), node, sizeof(inode));	

	if(bwrite(&block)==0){
		if(commit_superblock()==-1){
			LOG("ialloc: commit super block failed");
			return -1;
		}
 	}
	return 0;
}

int ifree(inode *node)
{
	data_block block;
	int inode_offset;
	superblock.num_free_inodes++;
	commit_superblock();
	
	inode new_inode;
	new_inode.inode_id = node->inode_id;
	new_inode.type = TYPE_FREE;
	bread(ILIST_BEGIN + ((new_inode.inode_id - 1) / (BLOCK_SIZE/INODE_SIZE)), &block);
	inode_offset = ((new_inode.inode_id -1) % (BLOCK_SIZE / INODE_SIZE)) *INODE_SIZE;
	memcpy(&(block.block[inode_offset]),&new_inode,sizeof(inode));
	if(bwrite(&block)==0)
	{
		if(commit_superblock() == 0)
			return 0;
		else
		{
			LOG("ifree: commit_superblock failed.");
		}
	}
	LOG("ifree: bwrite failed.");
	return -1;
}
