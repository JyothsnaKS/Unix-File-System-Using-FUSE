#include "syscalls.h"

int syscalls_mknod(const char* path, mode_t mode, dev_t dev){

	(void) dev;

	if(ags_namei(path)!=-1){
		LOG("mknod: file exists");
		return -1;
	}

	char* dup_path = strdup(path);

//dirname() returns the string  up  to,  but not including, the final '/', and basename() returns the component following the final '/
	char* name  = basename(dup_path);
	size_t max_name_length = FNAME_SIZE;

	if(strlen(name) > max_name_length){
		LOG("mknod: file name too long");
		free(dup_path);
		return -1;
	}

	inode parent_node;
	if(get_inode(get_parent_inode_id(path),&parent_node)==-1){
		LOG("mknod: get parent inode failed");
		free(dup_path);
		return -1;
	}

	inode node;
	if(ialloc(&node)==-1){
		LOG("mknod: no free inode found");
		free(dup_path);
		return -1;
	}

	node.type = TYPE_ORDINARY;
	node.mode = mode | S_IFREG; //File type is regular
	node.last_modified_file = time(NULL);
	node.last_accessed_file = time(NULL);
	node.last_modified_inode = time(NULL);

	// Persist the new inode
	put_inode(&node);

	// Add new entry in parent inode
	if(add_entry_to_parent(&parent_node, node.inode_id, name) == -1) {
		LOG("mknod: failed to add entry to parent inode");
		free(dup_path);
		return -1;
	}

	free(dup_path);
	return put_inode(&parent_node);
}



static int min(int x, int y) {
  return (x < y) ? x : y;
}

file_descriptor_table file_descriptors_tables[MAX_NUMBER_OF_FDT];
int next_free_loc = 0;
//TODO: See where FDT ALLOATE_FILE is used, change to index return 
int allocate_file_descriptor_table(int pid) {

	LOG("allocate_file_descriptor_table: entering");

	//file_descriptor_table * result_table;
	int ret =next_free_loc++;
	//HASH_FIND_INT(file_descriptor_tables, &pid, result_table);
    if(ret >= MAX_NUMBER_OF_FDT)
	{
		LOG("Too many process entires.");
		return -2;
	}
    if (file_descriptors_tables[ret].initialized != 1) {
	
	LOG("allocate_file_descriptor_table: init");
    	//int i;
		file_descriptors_tables[ret].initialized = 1;
    	//file_descriptor_table * table = (file_descriptor_table *) malloc(sizeof(file_descriptor_table));
    	file_descriptors_tables[ret].pid = pid;

    	file_descriptors_tables[ret].entries = (file_descriptor_entry *) malloc(MAX_NUMBER_OF_FDE * sizeof(file_descriptor_entry));
    	file_descriptors_tables[ret].total_descriptors = MAX_NUMBER_OF_FDE;
    	file_descriptors_tables[ret].used_descriptors = 3;

    	file_descriptors_tables[ret].entries[0].fd = 0;
    	file_descriptors_tables[ret].entries[1].fd = 1;
    	file_descriptors_tables[ret].entries[2].fd = 2;

    	for (int i = 3; i < file_descriptors_tables[ret].total_descriptors; i++) {
    		file_descriptors_tables[ret].entries[i].fd = FD_NOT_USED;
    	}
    	
    	//HASH_ADD_INT(file_descriptor_tables, pid, table);

	LOG("allocate_file_descriptor_table: exit");
    	return ret;
    }
    return -1;
}

int get_file_descriptor_table(int pid) {
	LOG("allocate_file_descriptor_table: enter");
	file_descriptor_table * table;
	for(int i = 0; i < next_free_loc; i++)
	{
		if(file_descriptors_tables[i].pid == pid){
			fprintf(stderr,"%s :%d\n","allocate_file_descriptor_table: entering, ",i);
			return i;
		}
	}
	//HASH_FIND_INT(file_descriptor_tables, &pid, table);
	return -1;
}

void delete_file_descriptor_table(int pid) {

	int i = get_file_descriptor_table(pid);

	for(i; i< next_free_loc - 1; i++)
	{
		file_descriptors_tables[i] = file_descriptors_tables[i+1]; 
	}
	next_free_loc--;
	file_descriptors_tables[next_free_loc].pid = -1;
	file_descriptors_tables[next_free_loc].initialized = -1;
}

int find_available_fd(int pid) {
	int index;
	int i;

	LOG("find_available_fd: enter");

	index = get_file_descriptor_table(pid);

	if (index == -1) {
		LOG("find_available_fd: get_f_d_t failed.");
		return -1;
	}

	if (file_descriptors_tables[index].total_descriptors == file_descriptors_tables[index].used_descriptors) {
		LOG("find_available_fd: no fde available.");	
		return -1;
	}
	for (i = 3; i < file_descriptors_tables[index].total_descriptors; i++) {
		if (file_descriptors_tables[index].entries[i].fd == FD_NOT_USED) {
			fprintf(stderr,"%s :%d\n","find_avail_fd: entering, ",i);
			return i;
		}
	}
	return -1;
}

file_descriptor_entry * allocate_file_descriptor_entry(int pid) {
	//file_descriptor_table * table;
	file_descriptor_entry * entry;
	int available_fd;

LOG("allocate_file_descriptor_entry: entering");

	// Allocate file descriptor table for the process if it doesn't exist
	int index = get_file_descriptor_table(pid);
	if (index == -1) {
		index = allocate_file_descriptor_table(pid);
	}

	available_fd = find_available_fd(pid);

	if (available_fd == -1)
	{
		LOG("allocate_fde: FD Unavailable");
	}
 /*{
		// Need to resize array of entries to twice the size
		available_fd = table->total_descriptors;
		table->entries = realloc(table->entries, ((table->total_descriptors) * 2 * sizeof(file_descriptor_entry)));
		table->total_descriptors = table->total_descriptors * 2;

		int i;
		for (i = available_fd; i < table->total_descriptors; i++) {
    		table->entries[i].fd = FD_NOT_USED;
    	}
	}*/
	LOG("Entry has been found. ");
	entry = &(file_descriptors_tables[index].entries[available_fd]);
	entry->fd = available_fd;
	fprintf(stderr,"Entry fd: %d\n",entry->fd);
	file_descriptors_tables[index].used_descriptors++;

	return entry;
}

file_descriptor_entry * get_file_descriptor_entry(int pid, int fd) {
	int table;
	file_descriptor_entry * entry;

	table = get_file_descriptor_table(pid);
	if (table == -1 || fd >= file_descriptors_tables[table].total_descriptors) {
		return NULL;
	}

	entry = &(file_descriptors_tables[table].entries[fd]);

	// If file descriptor not yet assigned (properly allocated for an open file), we return NULL
	if (entry->fd == FD_NOT_USED) {
		return NULL;
	}

	return entry;
}
// FNAME_SIZE

int syscalls_mkdir(const char *path, mode_t mode)
{
	//1. Check if the name already exists

	if (ags_namei(path) != -1)	// Returns -1 if file doesn't exists
	{
		LOG("\nFile already exists");
		return -1;
	}

	//1.1 Check for name length
	char *dup_path = strdup(path);
	//fprintf(stderr,"path: %s, basename: %s\n",path,basename(dup_path));
	//char *dup_path = strdup(path);
	char *name = basename(dup_path);	// Resolve the filename from the pathname
	size_t max_name_length = FNAME_SIZE;
	//fprintf(stderr,"name: %s\n",name);
	if(strlen(name) > max_name_length)
	{
		LOG("\nName too long");
		free(dup_path);
		return -1;
	}
	//fprintf(stderr,"name: %s\n",name);
//	free(dup_path);

	//2. Get parent node
	
	inode parent_inode;
	if (get_inode(get_parent_inode_id(path), &parent_inode) == -1 )
	{
		LOG("\nCannot get parent node");
		return -1;
	}
	
	//3. Get a new Data Block for creating empty dir block
	
	data_block block;

	if (data_block_alloc(&block) == -1)
	{
		LOG("\nCannot find a free data block");
		return -1;
	}
	
	//4. Create an inode 
	
	inode node;
	
	if (ialloc(&node) == -1)
	{
		LOG("\nCannot allocate a free inode ");
		return -1;
	}
	
	node.type=TYPE_DIRECTORY;
	node.mode=mode | S_IFDIR;
	node.last_modified_file = time(NULL);
	node.last_accessed_file = time(NULL);
	node.last_modified_inode = time(NULL);
	
	// 5. Initialize the new directory block
	
	uint32_t block_id=block.data_block_id;
	dir_block dblock;
	if(init_dir_block(&dblock, node.inode_id, parent_inode.inode_id) == -1)
	{
		LOG("\nUnable to format the directory block");
		return -1;
	}
	
	// 6. Write the new dir block
	
	write_block(block_id, &dblock, sizeof(dblock));
	
	
	// 7. Update the new dir block
	if(set_block_id(&node, 0, block_id) == -1)
	{
		LOG("\nFailed to set data block in inode\n");
		return -1;
	}
	
	node.num_blocks++;
	put_inode(&node);
	
	//fprintf(stderr,"Name before aetp: %s",name);
	//8. Add entry to the parent node
	if(add_entry_to_parent(&parent_inode, node.inode_id, name) == -1)
	{
		LOG("\nFailed to update the parent node");
		return -1;
	}
	
	return put_inode(&parent_inode);
	
}
void delete_file_descriptor_entry(int pid, int fd) {
	int table;
	file_descriptor_entry * entry;

	table = get_file_descriptor_table(pid);
	if (table == -1 || fd >= file_descriptors_tables[table].total_descriptors || fd <= 2) {
		return;
	}

	entry = &(file_descriptors_tables[table].entries[fd]);

	// We just set the fd to be unused (-1). It would be cool to realloc the entries if we notice that 
	// all the allocated fd entries are together and below the max allocated entry to save memory.
	entry->fd = FD_NOT_USED;
	file_descriptors_tables[table].used_descriptors--;
}

int syscalls_get_pid(void) {
	fprintf(stderr,"The pid is: %d\n",(int)fuse_get_context()->pid);
	return (int)fuse_get_context()->pid;
}


int syscalls_openfd(const char *path, int oflag, ...) {
	file_descriptor_entry * fde;
	int inode_number, pid;
 	inode node;

	inode_number = ags_namei(path);
	if (inode_number == -1) {
		return -1;
	}

	get_inode(inode_number, &node);

	pid = syscalls_get_pid();
	fde = allocate_file_descriptor_entry(pid);

	fde->byte_offset = 0;

	fde->inode_number = inode_number;

	if (oflag & O_WRONLY) {
		// Check for write permissions here
		fde->mode = WRITE;
	}
	else if (oflag & O_RDWR) {
		// Check for read/write permissions here
		fde->mode = READ_WRITE;
	}
	else {
		fde->mode = READ;	
	}
	return fde->fd;
}


int syscalls_open(const char *path, int oflag, ...) {
	file_descriptor_entry * fde;
	int inode_number, pid;
 	inode node;
	mode_t mode;

	if (oflag & O_CREAT) { //(1){//
		va_list ap;

		va_start(ap, oflag);
		mode = va_arg(ap, int);
		va_end(ap);

		//TODO: Remove this
		mode = S_IRWXU|S_IRWXG|S_IRWXO;

		// TODO We should call mknod here (not supported now cause not a priority as mknod exists and fuse doesn't use flag O_CREAT)
		if (syscalls_mknod(path, mode, 0) == -1) {
			return -1;
		}
	}


	// Get inode number related to path (we suppose creation flag doesn't work in open for now as FUSE doesn't support it in open)
	inode_number = ags_namei(path);
	if (inode_number == -1) {
		//errno = ENOENT;
		return -1;
	}

	get_inode(inode_number, &node);

	pid = syscalls_get_pid();
	fde = allocate_file_descriptor_entry(pid);

	// O_CREAT, O_TRUNC (not implemented for now cause fuse doesn't use those flags)

	// TODO Here we should check for permissions when opening a file. We need to add the field integer file_permission in inode which stores R,W...
	if (oflag & O_WRONLY) {
		// Check for write permissions here
		fde->mode = WRITE;
	}
	else if (oflag & O_RDWR) {
		// Check for read/write permissions here
		fde->mode = READ_WRITE;
	}
	else {
		fde->mode = READ;	
	}

	fde->byte_offset = 0;

	fde->inode_number = inode_number;

	node.last_accessed_file = time(NULL);
	put_inode(&node);
	return fde->fd;
}

int syscalls_close(int fildes) {
	file_descriptor_entry * fde;
	int fdt;
	int pid;

	pid = syscalls_get_pid();
	fde = get_file_descriptor_entry(pid, fildes);

	if (fde == NULL) {
		//errno = EBADF;
		return -1;
	}

	delete_file_descriptor_entry(pid, fildes);
	fdt = get_file_descriptor_table(pid);

	// Only the standard input, output and error are opened so we close the file descriptor table for that process
	if (file_descriptors_tables[fdt].used_descriptors == 3) {
		delete_file_descriptor_table(pid);
	}
	return 0;
}
int syscalls_readdir(const char* path, void* buffer,fuse_fill_dir_t filler, off_t offset)
{
	(void) offset;
	inode node;
	if(get_inode(ags_namei(path),&node) == -1)
	{
		LOG("syscalls_readdir: get_inode failed.");
		return -1;
	}
	uint32_t total_dir_blocks = node.num_blocks;
	dir_block dblock;
	int len = NUM_FILES_DIR;
	unsigned int i;
	int j;
	for(i = 0; i< total_dir_blocks; i++)
	{
		uint32_t block_id = get_block_id(&node,i);
		if(block_id > 0 && read_block(block_id,&dblock) == 0)
		{
			for(j = 0; j < len; j++)
			{
				if(dblock.inode_ids[j] != 0)
				{
					struct stat stat = {
						.st_dev = 0
					};
					stat.st_ino = dblock.inode_ids[j];
					filler(buffer,dblock.names[j],&stat,0,0);
				}
			}
		}
	}
	return 0;
}
int get_size_of_file(uint32_t num_used_blocks, int num_used_bytes_in_last_block)
{
	if(num_used_blocks == 0)
		return 0;
	return (num_used_blocks -1) * BLOCK_SIZE + num_used_bytes_in_last_block;
}
ssize_t syscalls_pread(int fd,void *buf, size_t num_bytes, off_t offset)
{
	file_descriptor_entry *fde;
	inode node;
	data_block block;
	int pid;
	size_t rem_bytes, bytes_to_be_copied, read_bytes;
	off_t current_offset;
	uint32_t block_num_pos, current_block;
	pid = syscalls_get_pid();
	fde = get_file_descriptor_entry(pid,fd);

	fprintf(stderr,"pread: %d fd:%d\n",pid,fd);

	if(fde == NULL)
	{
		LOG("pread: fde failed.");
		return -1;
	}
	if((fde->mode != READ) && (fde->mode != READ_WRITE))
	{
		LOG("pread: no read rights.");
		return -1;
	}
	if(get_inode(fde->inode_number,&node)== -1)
	{
		LOG("pread: get indoe failed.");
		return -1;
	}
 	if(offset > get_size_of_file(node.num_blocks,node.num_used_bytes_in_last_block))
	{
		return 0;
	}
	
	block_num_pos = (offset/BLOCK_SIZE) + 1;
	current_block = get_block_id(&node,block_num_pos - 1);
	read_bytes = 0;
	rem_bytes = num_bytes;
	current_offset = offset % BLOCK_SIZE;	
	while(rem_bytes > 0)
	{
		if(rem_bytes == num_bytes)
			bytes_to_be_copied = min(BLOCK_SIZE - current_offset,rem_bytes);
		else
			bytes_to_be_copied = min(BLOCK_SIZE,rem_bytes);
		if(block_num_pos == node.num_blocks)
		{
			if((node.num_used_bytes_in_last_block == current_offset) && rem_bytes > 0)
			{
				fde->byte_offset = get_size_of_file(node.num_blocks,node.num_used_bytes_in_last_block);
				return read_bytes;
			}
			bytes_to_be_copied = min(bytes_to_be_copied,node.num_used_bytes_in_last_block - current_offset);
		}
		if(current_block == 0)
		{
			memset(&((char*)buf)[read_bytes], 0 , bytes_to_be_copied);
		}
		else 
		{
			if(bread(current_block, &block) == -1)
				return -1;
			memcpy(&((char*)buf)[read_bytes],&block.block[current_offset],bytes_to_be_copied);
		}
		read_bytes += bytes_to_be_copied;
		current_offset = (current_offset + bytes_to_be_copied) % BLOCK_SIZE;
		rem_bytes -= bytes_to_be_copied;
		if(current_offset == 0)
		{
			block_num_pos++;
			current_block = get_block_id(&node,block_num_pos -1);
		}
	}
	fde->byte_offset = offset + read_bytes;
	node.last_accessed_file = time(NULL);
	if(put_inode(&node) == -1)
		return -1;
	return read_bytes;
}

int check_block_range(uint32_t ith_block){
	if(ith_block<1 || ith_block>3) // 3 direct blocks
		return 0;
	return 1;
}

int set_ith_datablock_number(inode* node, uint32_t ith_block, uint32_t block_number){
fprintf(stderr,"\n\n pwrite: 3.1\n");
ags_namei("/f.txt");
	if(set_block_id(node,ith_block-1,block_number)==-1)
		return -1;
fprintf(stderr,"\n\n pwrite: 3.2\n");
ags_namei("/f.txt");
	if(node->num_blocks<ith_block){
		node->num_blocks = ith_block;
		node->num_used_bytes_in_last_block = 0;
	}
	return 0;

}
int syscalls_pwrite(int fd, const void *buf, size_t num_bytes, off_t offset)
{
	file_descriptor_entry * fde;
	inode node;
	data_block db;

	int pid;
	size_t rem_bytes,bytes_to_be_copied,written_bytes;
	off_t current_offset;
	uint32_t block_num_pos, current_block;


	pid = syscalls_get_pid();
	fde = get_file_descriptor_entry(pid, fd);

	fprintf(stderr,"pwrite: %d fd:%d\n",pid,fd);
	if(fde == NULL)
	{
		LOG("pwrite: fde failed.");
		return -1;
	}
	if((fde->mode != WRITE) && (fde->mode != READ_WRITE))
	{
		LOG("pwrite: no write rights.");
		return -1;
	}

	get_inode(fde->inode_number, &node);
	block_num_pos = (offset/BLOCK_SIZE) + 1;
	

	if(offset >= get_size_of_file(node.num_blocks, node.num_used_bytes_in_last_block) && !check_block_range(block_num_pos)){
		return -1;
	}	

	current_block = get_block_id(&node, block_num_pos-1);
	rem_bytes = num_bytes;
	current_offset= offset%BLOCK_SIZE;
	written_bytes = 0;


	while(rem_bytes>0){
		bytes_to_be_copied = min(BLOCK_SIZE-current_offset,rem_bytes);		
		
		if(current_block==0){

fprintf(stderr,"\n\n pwrite: 1\n");
ags_namei("/f.txt");
			if(data_block_alloc(&db)==-1)
				return -1;

fprintf(stderr,"\n\n pwrite: 2\n");
ags_namei("/f.txt");
			if(set_ith_datablock_number(&node,block_num_pos,db.data_block_id)==-1)
				return -1;
fprintf(stderr,"\n\n pwrite: 3\n");
ags_namei("/f.txt");
			node.num_allocated_blocks++;
fprintf(stderr,"\n\n pwrite: 4\n");
ags_namei("/f.txt");
		}
		else{
			if(bread(current_block,&db)==-1)
				return -1;
		}
		
		memcpy(&db.block[current_offset], &((char*)buf)[written_bytes], bytes_to_be_copied);
		if(bwrite(&db)==-1)
			return -1;
		written_bytes += bytes_to_be_copied;
		current_offset = (current_offset + bytes_to_be_copied) % BLOCK_SIZE;
		rem_bytes -= bytes_to_be_copied;

		if(current_offset == 0){
			block_num_pos++;
			current_block=get_block_id(&node, block_num_pos-1);
		}
	}

	if(block_num_pos> node.num_blocks || (block_num_pos==node.num_blocks && current_offset>node.num_used_bytes_in_last_block)){
		if(current_offset==0){
			node.num_blocks=block_num_pos-1;
			node.num_used_bytes_in_last_block = BLOCK_SIZE;
		}
		else{
			node.num_blocks=block_num_pos;
			node.num_used_bytes_in_last_block = current_offset;
		}
		node.last_modified_inode = time(NULL);
	}	
	node.last_modified_file = time(NULL);
	if(put_inode(&node)==-1)
		return -1;
	fde->byte_offset = offset + written_bytes;
	return 	written_bytes;
}
int syscalls_lstat(const char *path, struct stat *buf) { //ags_namei(path); return 0;
	inode node;
	if(get_inode(ags_namei(path), &node) == -1) {
		LOG("syscalls_lstat: failed to get inode");
		//errno = ENOENT;
		return -1;
	}
	buf->st_ino = node.inode_id;
	buf->st_mode = node.mode;
	buf->st_nlink = node.links_nb;
	buf->st_uid = node.uid;
	buf->st_gid = node.gid;

	if (node.type == TYPE_ORDINARY) {
		buf->st_size = node.num_blocks == 0 ? 0 : (node.num_blocks - 1) * BLOCK_SIZE + node.num_used_bytes_in_last_block;
	}
	else {
		buf->st_size = node.num_blocks * BLOCK_SIZE;
	}

	buf->st_blocks = node.num_allocated_blocks;
	buf->st_blksize = BLOCK_SIZE;
	buf->st_atime = node.last_accessed_file;
	buf->st_mtime = node.last_modified_file;
	buf->st_ctime = node.last_modified_inode;
	return 0;
}

int syscalls_rmdir(const char *path) {

	inode node;
	if(get_inode(ags_namei(path), &node) == -1) {
		LOG("rmdir: get_inode failed.");
		return -1;
	}

	if(node.type != TYPE_DIRECTORY) {
		LOG("rmdir: Not a dir.");
		return -1;
	}

	// Check if it's empty
	int is_empty = 1;
	dir_block dblock;
	uint32_t len = BLOCK_SIZE / 32;
	uint32_t i, j;
	uint32_t total_dir_blocks = node.num_blocks;
	for(i = 0; i < total_dir_blocks; ++i) {
		uint32_t block_id = get_block_id(&node, i);
		if(block_id > 0 && read_block(block_id, &dblock) == 0) {
			for(j = 0; j < len; j++) {
				if(i == 0 && j < 2) {
					continue;
				}
				if(dblock.inode_ids[j] != 0){ //|| strcmp("", dblock.names[j]) != 0) {
					fprintf(stderr,"no: %d\n",dblock.inode_ids[j]);
					fprintf(stderr,"name: %s\n",dblock.names[j]);
					is_empty = 0;
					break;
				}
			}
		}
	}
	fprintf(stderr,"%d empty\n",is_empty);

	if(!is_empty) {
		LOG("rmdir: directory not empty");
		return -1;
	}

	node.links_nb = 0;
	if(put_inode(&node) == -1) {
		LOG("rmdir: put_inode failed");
		return -1;
	}

	inode parent_inode;
	if(get_inode(get_parent_inode_id(path), &parent_inode) == -1) {
		LOG("rmdir: get_parent_inode_id failed");
		return -1;
	}

	if(remove_entry_from_parent(&parent_inode, node.inode_id) == -1) {
		LOG("rmdir: remove_entry_from_parent failed.");
		return -1;
	}
	return put_inode(&parent_inode);
}
