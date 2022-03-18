#ifndef _FILE_H
#define _FILE_H
#define NULL ((void*)0x00)
typedef struct _FILE_INFORMATION{
	unsigned char name[8], ext[3], type;
	char reserve[10];
	unsigned short time, date, clustno;
	unsigned int size;
}FILEINFO;

//文件句柄
typedef struct _FILE_HANDLE{
	char* buf;
	int size;
	int pos;
}FILEHANDLE;

#endif

void file_readFAT(int* fat, unsigned char* img);
void file_loadfile(int clustno,int size,char* buf,int* fat,char* img);
char* file_loadfile_overload(int clustno,int* psize,int* fat);

FILEINFO* file_search(char* filename,FILEINFO* finfo, int max);