#include "ags_emulator.h"

char** disk_blocks;

int init_superblock(void) {
	data_block dblock;
	if(read_block(0, &dblock.block)) {
		LOG("init_superblock: Failed to read superblock");
		return -1;
	}

	memcpy(&superblock, &dblock.block, sizeof(superblock));
	return 0;
}

int commit_superblock(void)
{
	return write_block(0,&superblock,sizeof(super_block));
}

int if_list_contains_free_block(char* datablock)
{
	uint32_t block_number;
	int j;
	for(j=BLOCK_ID_LIST_LENGTH;j>=2;j--)
	{
		memcpy(&block_number, &datablock[(j-1) * 4],4);
		fprintf(stderr,"\n\n pwrite: --- block_id:%d\n",block_number);
		if(block_number != 0)
		{
			return 1;
		}
	}
	return 0; 
}
void do_set_block_number(char *datablock,int i, uint32_t block_no)
{
	memcpy(&datablock[(i-1) * 4],&block_no,4);
	fprintf(stderr,"\n\n pwrite: set block_id:%d\n",datablock[(i-1) * 4]);
	return;
}
uint32_t get_free_block_number(char* datablock)
{
	uint32_t block_number;
	for(int i = BLOCK_ID_LIST_LENGTH; i >= 2; i--)
	{
		memcpy(&block_number,&datablock[(i-1) * 4],4);

		if(block_number !=0)
		{
			do_set_block_number(datablock,i,0);
			return block_number;
		}
	}
	return 0;
}
int data_block_alloc(data_block *block)
{
	int first_free_list = NUM_OF_INODE_BLOCKS + 1;
	char read_buffer[BLOCK_SIZE];
	uint32_t ret_value = -1;	
	if(read_block(first_free_list,read_buffer) == -1)	
	{
		LOG("data_block_alloc:: read_block failed.");
		return -1;
	} 
//fprintf(stderr,"\n\n pwrite: 1.1\n");
//ags_namei("/f.txt");

	if(if_list_contains_free_block(read_buffer) == 1)
	{
//fprintf(stderr,"\n\n pwrite: 1.1.1\n");
//ags_namei("/f.txt");


//fprintf(stderr,"\n\n pwrite: set block_id:%d\n",read_buffer[127 * 4]);
		ret_value = get_free_block_number(read_buffer);
//fprintf(stderr,"\n\n pwrite: 1.1.2 Ret_val:%d\n",ret_value);
//fprintf(stderr,"\n\n pwrite: set block_id:%d\n",read_buffer[127 * 4]);
//ags_namei("/f.txt");
		if(write_block(first_free_list,read_buffer,BLOCK_SIZE)==-1)
		{
			LOG("data_block_alloc:: write_block failed.");
			return -1;
		}
		superblock.num_free_blocks--;
//fprintf(stderr,"\n\n pwrite: 1.2\n");
//ags_namei("/f.txt");

	}
	else
	{
		uint32_t next_free_list;
		memcpy(&next_free_list, &block[0],4);
		if(next_free_list == 0)
		{
			LOG("data_block_alloc:: no space.");
			return -1;
		}
		if(read_block(next_free_list,read_buffer) == -1)
		{
			LOG("data_block_alloc:: read_block failed of next list.");
			return -1;
		}
		if(write_block(first_free_list,read_buffer,BLOCK_SIZE) == -1)
		{
			LOG("data_block_alloc:: write_block failed of next list.");
			return -1;
		}
		ret_value = next_free_list;
	}
	block->data_block_id = ret_value;
	memset(block->block,0,BLOCK_SIZE);

	if(write_block(ret_value,block->block,BLOCK_SIZE) == -1)
	{
		LOG("data_block_alloc:: write_block failed of next list.");
		return -1;
	}
//fprintf(stderr,"\n\n pwrite: 1.3\n");
//ags_namei("/f.txt");

	commit_superblock();	
	return 0;
}

//Read data block and set block id
int bread(uint32_t data_block_no, data_block *block){
	
	if(read_block(data_block_no,block->block)==-1){
		LOG("bread failed");
		return -1;
	}

	block->data_block_id = data_block_no;
	return 0;
}

int bwrite(data_block* block){
	return write_block(block->data_block_id,block->block,BLOCK_SIZE);
}


int add_entry_to_dir_block(dir_block* dblock, uint32_t id, char* name){
	int len = NUM_FILES_DIR;
	int i;	
	for(i=0;i<len;i++){
		if(dblock->inode_ids[i]==0){
			dblock->inode_ids[i]=id;
			strcpy(dblock->names[i],name);
			break;
		}
	}
	if(i==len)
		return -1;
	return 0;
}

uint32_t get_block_id(inode *node,uint32_t index) {
	// Check for direct block
	if(index < NUM_DIRECT_BLOCKS) {
		return node->direct_blocks[index];
	}

	//If indirect blocks implemented, change this.
	LOG("get_block_id: index is out of range for get_block_id");
	
	return 0;
}

int set_block_id(inode *node, uint32_t index, uint32_t block_id) {
	// Check for direct block
	if(index < NUM_DIRECT_BLOCKS) {
		node->direct_blocks[index] = block_id;
		return 0;
	}
	LOG("set_block_id: index is out of range for set_block_id");
	return -1;	
}
int add_entry_to_parent(inode *parent_inode, int inode_id, char *name) { //removed const here
// Try adding entry to existing block if possible
	if(parent_inode->num_blocks > 0) {
		uint32_t block_id = get_block_id(parent_inode, parent_inode->num_blocks-1); 
		if(block_id == 0) {
			LOG("add_entry_to_parent: could not fetch block id from inode");
			return -1;
		}

		dir_block dblock;
		read_block(block_id, &dblock);

		if(add_entry_to_dir_block(&dblock, inode_id, name) == 0) {
			return write_block(block_id, &dblock, sizeof(dir_block));
		}		
	}

	// Alloc a new block and add entry there
	data_block block;
	if(data_block_alloc(&block) == -1) {
		LOG("add_entry_to_parent: could not alloc data block");
		return -1;
	}

	// Add this block to inode
	if(set_block_id(parent_inode, parent_inode->num_blocks, block.data_block_id) == -1) {
		LOG("add_entry_to_parent: could not add newly allocated block to inode");
		return -1;
	}
	parent_inode->num_blocks++;

	dir_block *dblock = (dir_block*) block.block;
	add_entry_to_dir_block(dblock, inode_id, name);
	return write_block(block.data_block_id, dblock, sizeof(dir_block));
}


int init_dir_block(dir_block *dblock, uint32_t id, uint32_t parent)
{
	int len = NUM_FILES_DIR;

	for(int i=0;i<len;i++){
		dblock->inode_ids[i] = 0;
		strcpy(dblock->names[i],"");
	}	

	add_entry_to_dir_block(dblock,id,".");
	add_entry_to_dir_block(dblock,parent,"..");

	return 0;
}

//Used to write into a block at a particular offset
int write_block_offset(uint32_t block_id, void *buffer, size_t buffer_size, int offset) {
	char string[500] = "write_block_offset: Data too big for a block. offset,buffer_size ";
	char off[100];
	char buff[100];
	sprintf(off,"%d",offset);
	sprintf(buff,"%ld",buffer_size);
	strcat(string,off);
	strcat(string," ");
	strcat(string,buff);
	if(offset + buffer_size > BLOCK_SIZE) {
		LOG(string);
		return -1;
	}
	//Read existing block, append new data and write back to block
	char *dest = malloc(sizeof(char) * BLOCK_SIZE);
	read_block(block_id, dest);
	memcpy(dest + offset, buffer, buffer_size);
	write_block(block_id, dest, BLOCK_SIZE);
	free(dest);
	return 0;
}

int remove_entry_from_parent(inode *parent_inode, int inode_id)
{
	dir_block dblock;
	uint32_t total_dir_blocks = parent_inode -> num_blocks;
	uint32_t i;
	for(i = 0; i< total_dir_blocks; i++)
	{
		uint32_t block_id = get_block_id(parent_inode,i);
		if(block_id > 0 && read_block(block_id, &dblock)== 0)
		{
			if(remove_entry_from_dir_block(&dblock, inode_id) == 0)
			{
				return write_block(block_id, &dblock,sizeof(dir_block));
			}
			else LOG("remove_entry_from_parent: remove_entry_from_dir_block failed"); 
		}
	}
	LOG("remove_entry_from_parent: unsucessful");
	return -1;
}

int remove_entry_from_dir_block(dir_block *dblock, int inode_id)
{
	int len = BLOCK_SIZE / 32;
	int i;
	for(i = 0; i < len; i++)
	{
		if(dblock->inode_ids[i] == inode_id)
		{
			dblock-> inode_ids[i] = 0;
			strcpy(dblock->names[i],"");
			LOG("remove_entry_from_dir_block: success.");
			return 0;
		}
	}
	LOG("remove_entry_from_dir_block: failed.");
	return -1;
}

int data_block_free(data_block *block) 
{
	int position_of_first_datablock, ith_position;
	char read_buffer[BLOCK_SIZE], buffer[BLOCK_SIZE];

	position_of_first_datablock = 1 + NUM_OF_INODE_BLOCKS;

	if (read_block(position_of_first_datablock, read_buffer) == -1) 
	{
		LOG("data_block_free: read_block failed");
		return -1;
	}

	if (is_datablock_full_of_free_datablock_numbers(read_buffer) == 1) 
	{
	
		if (write_block(block->data_block_id, read_buffer, BLOCK_SIZE) == -1) 
		{
			LOG("data_block_free: write_block failed");
			return -1;
		}

		memset(buffer, 0, BLOCK_SIZE); 
		do_set_block_number(buffer, 1, block->data_block_id);
		if (write_block(position_of_first_datablock, buffer, BLOCK_SIZE) == -1) 
		{
			LOG("data_block_free: read_block failed");
			return -1;
		}
	}
	else 
	{

		memset(block->block, 0, BLOCK_SIZE);
		if (write_block(block->data_block_id, block->block, BLOCK_SIZE) == -1) 
		{
			LOG("data_block_free: write_block failed");
			return -1;
		}

		ith_position = get_position_of_free_spot_in_free_list_for_datablock(read_buffer);
		do_set_block_number(read_buffer, ith_position, block->data_block_id);
		if (write_block(position_of_first_datablock, read_buffer, BLOCK_SIZE) == -1) 
		{
			LOG("data_block_free: write_block failed");
			return -1;
		}
	}

	superblock.num_free_blocks++;
	commit_superblock();

	return 0;
}

int get_position_of_free_spot_in_free_list_for_datablock(char * datablock) 
{
	uint32_t  block_number;
	uint32_t i;

	for (i = 2; i <= BLOCK_ID_LIST_LENGTH; i++) 
	{
		memcpy(&block_number, &datablock[(i-1) * 4], 4);
		if (block_number == 0) 
		{
			return i;
		}
	}
	return -1;
}

int is_datablock_full_of_free_datablock_numbers(char * datablock) 
{
	uint32_t block_number;
	uint32_t i;

	for (i = 2; i <= BLOCK_ID_LIST_LENGTH; i++) 
	{
		memcpy(&block_number, &datablock[(i-1) * 4], 4);

		if (block_number == 0) 
		{
			return 0;
		}
	}
	return 1;
}



static int disk;
int disk_exists = -1;

int start_emulator(){
	if(disk_exists==-1){
		disk = open("/home/jks/usp_fs/root", O_CREAT|O_EXCL|O_RDWR);
		if(disk!=-1){				
			if(create_fs()==-1){
				LOG("start_emulator(): fs create failed");
				return -1;
			}
		}
		else
			disk = open("/home/jks/usp_fs/root", O_RDWR);
		disk_exists=0;
		return 0;
	}
	return -1;
}


void free_disk_emulator(){

	if(disk_exists == -1) {
		LOG("free_disk_emulator:no disk opened");
		return;
	}

	disk_exists = -1;
	if(close(disk) == 0) {
		LOG("free_disk_emulator: disk store successfully closed");
	} else {
		LOG("free_disk_emulator: failed to close disk store\n");
	}
}


int read_block(uint32_t block_id, void * target) {

	if(block_id >= NUM_OF_BLOCKS) {
		LOG("read_block: cannot read block id outside range\n");
		return -1;
	}

	off_t seek_pos = block_id * BLOCK_SIZE;
	if(lseek(disk, seek_pos, SEEK_SET) == -1) {
		LOG("read_block: failed to seek");
		return -1;
	}

	ssize_t read_bytes = read(disk, target, BLOCK_SIZE);
	if(read_bytes == -1) {
		LOG("read_block:: failed to read block");
		return -1;
	}

	if((uint32_t) read_bytes != BLOCK_SIZE) {
		LOG("read_block: read returned less than expected bytes");
	}
	return 0;
}

int write_block(uint32_t block_id, void * buffer, size_t buffer_size) {

	if(block_id >= NUM_OF_BLOCKS) {
		LOG("write_block: cannot write block id outside range");
		return -1;
	}

	off_t seek_pos = block_id * BLOCK_SIZE;
	if(lseek(disk, seek_pos, SEEK_SET) == -1) {
		LOG("write_block: failed to seek");
		return -1;
	}

	if(buffer_size > BLOCK_SIZE) {
		LOG("write_block: not writing data beyond block size");
	}

	if(buffer_size < BLOCK_SIZE) {
		char data[BLOCK_SIZE];
		memcpy(data, buffer, buffer_size);
		memset(data + buffer_size, 0, BLOCK_SIZE - buffer_size);
		buffer = data;
	}

	ssize_t write_bytes = write(disk, buffer, BLOCK_SIZE);
	if(write_bytes == -1) {
		LOG("write_block: failed to write block");
		return -1;
	}

	if((uint32_t) write_bytes != BLOCK_SIZE) {
		LOG("write_block: write returned less than expected bytes");
	}
	return 0;
}


/*
//Allocate memory to disk blocks
int start_emulator(void)
{

	disk_blocks = (char**)malloc(sizeof(char*)*NUM_OF_BLOCKS);
	
	if(!disk_blocks)
	{
		LOG("start_emulator: No memory for disk blocks.");
		return -1;
	}
	int i;
	for(i=0;i<NUM_OF_BLOCKS;i++)
	{
		disk_blocks[i] = (char*)malloc(sizeof(char)*BLOCK_SIZE);
		if(!disk_blocks[i])
		{
			LOG("start_emulator: No memory for disk blocks.");
			return -1;
		}
	}
	LOG("start_emulator: memory allocated for disk blocks.");
	return create_fs();	
}

int read_block(uint32_t block_id, void* dest){
	if(block_id >= NUM_OF_BLOCKS){
		LOG("read_block: Block ID outside range");
		return -1;
	}
	memcpy(dest, disk_blocks[block_id], BLOCK_SIZE); //Read entire block
	return 0;
}

int write_block(uint32_t block_id, void* buffer, size_t buffer_size){
	if(block_id >= NUM_OF_BLOCKS){
		LOG("write_block: Block ID outside range");
		return -1;
	}
	size_t copy_size = (buffer_size < BLOCK_SIZE) ? buffer_size : BLOCK_SIZE;
	memcpy(disk_blocks[block_id], buffer, copy_size);

	if(copy_size<BLOCK_SIZE)
		memset(disk_blocks[block_id]+copy_size, 0, BLOCK_SIZE-copy_size);
	return 0;
}
*/
