#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>

int main(){
	int fd = open("/rootdir/log2.txt", O_CREAT);
	printf("Create %d\n",fd);
	close(fd);
}
