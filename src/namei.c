#include "namei.h"

static int check_data_block_for_next_entry(dir_block *dblock, char *next_file) { 
	// returns -1 if entry not found
	int len = NUM_FILES_DIR;
	int i;
	for(i = 0; i < len; i++){
		if(i<5)
		fprintf(stderr,"Dir entries %s\n", dblock->names[i]);

		if(strcmp(next_file, dblock->names[i]) == 0){
			int inumber = dblock->inode_ids[i];
			return inumber;
		}
	}
	return -1;
} 

//Returns inode id corresponding file path mentioned
int ags_namei(const char *path) {// --> ./ or something/
	
LOG(path);
	
	if(path == NULL || path[0] != PATH_DELIMITER[0]){
		LOG("namei: Invalid path name");
		return -1;
	}

	//returns a pointer to a new string which is a duplicate of the string path
	char *dup_path = strdup(path);
LOG(path);
	//breaks a string into a sequence of zero or more nonempty tokens
	char *next_file = strtok(dup_path, PATH_DELIMITER);
LOG(next_file);
	inode next_inode;
	int next_inode_success = get_inode(ROOT_INODE_NUMBER, &next_inode);//Reading root directory's inode block
	dir_block dblock;
	uint32_t block_id;

	while(next_file != NULL && next_inode_success == 0) {

		int next_inode_number = -1;
		uint32_t i;
		uint32_t len = next_inode.num_blocks;//no. of blocks already used
fprintf(stdout,"len:%d\n",len);
		for(i = 0; i < len; i++) {
			block_id = get_block_id(&next_inode, i); 
fprintf(stdout,"block id:%d\n",block_id);
			read_block(block_id, &dblock);//Reading root directory's data block
fprintf(stdout,"block:%s\n",dblock.names[0]);
			
			next_inode_number = check_data_block_for_next_entry(&dblock, next_file);//Checking if entry exists for path
			if(next_inode_number != -1) {
				break;
			}
fprintf(stdout,"inode:%d\n",next_inode_number);
		}
		if(next_inode_number == -1) {
			free(dup_path);			
			LOG("namei: unable to find next inode number");
			return -1;
		}

		next_file = strtok(NULL, PATH_DELIMITER);
		next_inode_success = get_inode(next_inode_number, &next_inode); 
	}

	free(dup_path);
	if(next_inode_success == -1){
		LOG("namei: iget returned -1");
		return -1;
	}

	return next_inode.inode_id;
}
