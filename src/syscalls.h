#ifndef SYSCALLS
#define SYSCALLS

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdarg.h>
#include "fs_param.h"
#include "syscalls.h"
#include "namei.h"
#include "fuse.h"

#define FD_NOT_USED -1
#define MAX_NUMBER_OF_FDT 20
#define MAX_NUMBER_OF_FDE 50
typedef enum access_mode{
	READ,
	WRITE, 
	READ_WRITE
} access_mode;

typedef struct file_descriptor_entry{
	int fd;
	access_mode mode;
	int inode_number;
	int byte_offset;
}file_descriptor_entry;

typedef struct file_descriptor_table {
	int initialized;	
	int pid;
	int total_descriptors;
	int used_descriptors;
	struct file_descriptor_entry *entries;
}file_descriptor_table; // We need to use dynamic allocation with that hash table otherwise it doesn't work correctly


int syscalls_mknod(const char* path, mode_t mode, dev_t dev);
int syscalls_mkdir(const char *path, mode_t mode);
int syscalls_open(const char *path, int oflag, ... );
int syscalls_close(int fildes);


int allocate_file_descriptor_table(int pid);
int get_file_descriptor_table(int pid);
void delete_file_descriptor_table(int pid);

int find_available_fd(int pid);

file_descriptor_entry * allocate_file_descriptor_entry(int pid);
file_descriptor_entry * get_file_descriptor_entry(int pid, int fd);
void delete_file_descriptor_entry(int pid, int fd);
int syscalls_readdir(const char* path, void* buffer,fuse_fill_dir_t filler, off_t offset);
int syscalls_get_pid(void);
int syscalls_lstat(const char *path, struct stat *buf);
ssize_t syscalls_pread(int fd,void *buf, size_t num_bytes, off_t offset);
int syscalls_pwrite(int fd, const void *buf, size_t num_bytes, off_t offset);

int syscalls_openfd(const char *path, int oflag, ...);
int syscalls_rmdir(const char *path);
#endif
