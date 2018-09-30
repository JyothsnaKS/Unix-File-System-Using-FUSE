all: agsmount
agsmount: inode_code.c ags_emulator.c log.c mkfs.c namei.c ags_fs.c syscalls.c inode_code.h ags_emulator.h log.h mkfs.h namei.h syscalls.h fs_param.h
	gcc -o agsmount inode_code.c ags_emulator.c log.c mkfs.c namei.c ags_fs.c syscalls.c `pkg-config fuse3 --cflags --libs`
clean:
	rm -rf *.o
