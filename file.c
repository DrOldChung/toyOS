#include "header/bootpack.h"
#include "header/file.h"
#include "header/mmemory.h"

/*tek.c*/
extern int tek_getsize(unsigned char *p);
extern int tek_decomp(unsigned char *p, char *q, int size);

void file_readFAT(int* fat, unsigned char* img){
	int i = 0,j = 0;
	for(;i<2880;i+=2){
		fat[i] =   (img[j] | img[j+1] << 8) & 0xfff;
		fat[i+1] = (img[j+1] >> 4 | img[j+2] << 4) & 0xfff;
		j += 3;
	}
}


void file_loadfile(int clustno,int size,char* buf,int* fat,char* img){
	int i;
	while(1)
	{
		if(size <= 512){
			for(i = 0; i < size; ++i){
				buf[i] = img[clustno * 512 + i];
			}
			break;
		}
		for(i = 0; i < 512; ++i){
			buf[i] = img[clustno * 512 +i];
		}

		size -= 512;
		buf +=512;
		clustno = fat[clustno];
	}
}

char* file_loadfile_overload(int clustno,int* psize,int* fat){
	int size = *psize;
	int size2 = 0;
	MEMMAN * memManager = (MEMMAN*)(MEM_MANAGER_ADDR);

	char* buf,buf2;
	
	buf = (char*)memory_alloc_4kb(memManager,size);
	file_loadfile(clustno,size,buf,fat,(char*)(ADR_DISKIMG + 0x003e00));

	if(size >= 17){
		size2 = tek_getsize(buf);
		if(size2 > 0){
			buf = (char*)memory_alloc_4kb(memManager,size2);
			tek_decomp(buf,buf2,size2);
			memory_free_4kb(memManager,(int)(buf),size);
			buf = buf2;
			*psize = size2;
		}
	}
	return buf;
}

FILEINFO* file_search(char* filename,FILEINFO* finfo, int max){
	int i,j;
	char str[12];
	for(j = 0; j < 11; ++j){
		str[j] = ' ';
	}
	j = 0;

	for(i = 0; filename[i] != '\0'; ++i){
		if(j >= 11){
			return NULL;
		}
		if(filename[i] == '.' && j <= 8){
			j = 8;
		}else{
			str[j] = filename[i];
			//Up to low
			if(str[j] >= 'a' && str[j] <= 'z'){
				str[j] -= 0x20; 
			}
			++j;
		}
	}

	for (i = 0; i < 224;)
	{
		if (finfo[i].name[0] == 0x00){
			break;
		}
		if ((finfo[i].type & 0x18) == 0){
			for (j = 0; j < 11; ++j){
				if (finfo[i].name[j] != str[j]){
					goto next;
				}
			}
			return finfo + i; 		
		}
next:
		++i;
	}
	return NULL;					
}
