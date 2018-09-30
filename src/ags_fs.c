#include<stdio.h>
#include<stdlib.h>
#include<fuse.h>
#include <fcntl.h> 
#include <sys/stat.h>

//#include"fuse.h"

#include"fs_param.h"
#include"syscalls.h"
#include"errno.h"

static int ags_getattr(const char *path, struct stat *stbuf, struct fuse_file_info *fi){
	LOG("ags_getattr: executing");
	if(syscalls_lstat(path, stbuf) == -1){
		LOG("lstat: error");

        	return -ENOENT;
    	}
	return 0;
}
static int ags_read(const char* path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	LOG("AGS_READ : called");
	ssize_t bytes = syscalls_pread(fi->fh, buf,size,offset);
	if(bytes == -1)
		return -1;
	return bytes;
}
static int ags_mkdir(const char *path, mode_t mode)
{
	LOG("AGS_MKDIR: ENTERED");
	int res = syscalls_mkdir(path,mode);
	if(res == -1)
		return -1;
	return 0;
}
static int ags_write(const char* path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	LOG("AGS_WRITE : called");
	int bytes = syscalls_pwrite(fi->fh, buf,size,offset);
	if(bytes == -1)
		return -1;
	fprintf(stderr,"AGS_WRITE : called Bytes:%d\n",bytes);
	return bytes;
}

static int ags_readdir(const char* path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi, enum fuse_readdir_flags flags)
{
	LOG("ags_readdir: Entering");
	if(syscalls_readdir(path, buf,filler,offset) == -1)
	{
		return -1;
	}
	return 0;
}
static int ags_open(const char *path,struct fuse_file_info *fi)
{
	fprintf(stderr,"%s","HELLO.\n");	
	LOG("ags_open: executing");
	int ret_val = syscalls_openfd(path, fi->flags);
	if(ret_val == -1){
    		return -1;
    	}
	fi->fh = ret_val;
	return 0;
}
static int ags_release(const char* path, struct fuse_file_info *fi)
{
	LOG("ags_release: called.");
	if(syscalls_close(fi->fh)== -1)
	{
		return -1;
	}
	return 0;
}
static int ags_create(const char *path, mode_t mode, struct fuse_file_info *fi){
	fprintf(stderr,"%s","HELLO.\n");	
	LOG("ags_create: executing");
	int ret_val = syscalls_open(path, fi->flags);
	if(ret_val == -1){
    		return -1;
    	}
	fi->fh = ret_val;
	return 0;

}

static int ags_mknod(const char *path, mode_t mode, dev_t dev) {
	LOG("ags_mknod: executing");
	if(syscalls_mknod(path, mode, dev) == -1) {
		return -1;
	}
	return 0;
}
static int ags_rmdir(const char *path)
{
	LOG("ags_rmdir: executing");
	if(syscalls_rmdir(path) == 1)
	{
		LOG("ags_rmdir: failed");
		return -1;
	}
	return 0;
}

const struct fuse_operations ags_syscalls = {
	.getattr = ags_getattr,
	.create = ags_create,
	.mknod = ags_mknod,
	.open = ags_open,
	.release = ags_release,
	.readdir = ags_readdir,
	.read = ags_read,
	.write = ags_write,
	.mkdir = ags_mkdir,
	.rmdir = ags_rmdir,
};

int main(int argc, char*argv[]) { 

	if(start_emulator()==-1){
		LOG("main(): Failed to start emulator");
		return -1;
	}
        if(init_superblock() == -1) {
        	LOG("main(): Failed to init superblock");
        	return -1;
        }
	fuse_main(argc, argv, &ags_syscalls, NULL);
	return 0;
}

//gcc inode_code.c ags_emulator.c log.c mkfs.c namei.c ags_fs.c `pkg-config fuse3 --cflags --libs`
