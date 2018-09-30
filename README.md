# UnixFileSystem

Implementation of Unix file system using FUSE (File System in User Space) library

## File System Overview

![image](https://user-images.githubusercontent.com/14028499/46258151-36c06980-c4e3-11e8-86df-9e4a3c7dd18d.png)

### Inode Structure

![image](https://user-images.githubusercontent.com/14028499/46258161-62dbea80-c4e3-11e8-873c-f3ebe34fa281.png)

### File Descriptor table

![image](https://user-images.githubusercontent.com/14028499/46258165-7a1ad800-c4e3-11e8-8a92-1733ba8ac895.png)

## List of system calls implemented
1. open / creat
2. close
3. pread 
4. pwrite
5. lstat
6. mkdir
7. rmdir
8. rm
9. readdir
10. mknod
