#include<stdio.h>
void LOG(const char* string){
	fprintf(stderr,"%s\n",string);
	/*
	FILE* log_file = fopen("log.txt","a");
	fprintf(log_file,"%s\n",string);
	fclose(log_file);
	*/
}
