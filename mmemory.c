#include "header/bootpack.h" 
#include "header/mmemory.h"
#define  NULL    ((void*)0x00)

unsigned int memcheck(unsigned int begin, unsigned int end){
	char cpuIs486 = 0;
	unsigned int eflag,cr0,result;

	eflag = io_load_eflags();
	eflag |=  EFLAGS_AC_BIT;
	io_store_eflags(eflag);
	eflag = io_load_eflags();

	if ((eflag & EFLAGS_AC_BIT) != 0){
		cpuIs486 = 1;
	}

	eflag =  eflag & (~EFLAGS_AC_BIT);
	io_store_eflags(eflag);

	if(cpuIs486 != 0){
		cr0 = load_cr0(); 
		cr0 != (CR0_CACHE_DISABLE);//cache disable
		store_cr0(cr0);
	}

	result = memcheck_sub(begin,end);

	//check again
	if(cpuIs486 != 0){
		cr0 = load_cr0();  
		cr0 &= (~CR0_CACHE_DISABLE);
		store_cr0(cr0);
	}
	return result;
}



void init_memory_manager(MEMMAN* memoryManager){
	memoryManager->frees = 0; 
	memoryManager->maxfrees = 0; 
	memoryManager->lostsize = 0;
	memoryManager->losts = 0
}

unsigned int memory_manager_total(MEMMAN* memoryManger){
	if(memoryManger == NULL) 
		return 0;
	unsigned int idx = 0, capacity = 0;
	while(idx<memoryManger->frees){
		capacity += memoryManger->freeinfo[idx].size;
		++idx;
	}
	return capacity;
}

unsigned int memory_manager_alloc(MEMMAN *man, unsigned int size){
	unsigned int i, a;
	for (i = 0; i < man->frees; i++){
		if (man->freeinfo[i].size >= size){
			a = man->freeinfo[i].address;
			man->freeinfo[i].address += size;
			man->freeinfo[i].size -= size;

			if (man->freeinfo[i].size == 0) {
				--man->frees;
				for (; i < man->frees; i++) {
					man->freeinfo[i] = man->freeinfo[i + 1]; 
				}
			}
			return a;
		}
	}
	return 0;
}


int memory_manager_free(MEMMAN *man, unsigned int addr, unsigned int size){
	int i, j;
	for (i = 0; i < man->frees; i++) {
		if (man->freeinfo[i].address > addr) {
			break;
		}
	}
	if (i > 0){
		if (man->freeinfo[i - 1].address + man->freeinfo[i - 1].size == addr){
			man->freeinfo[i - 1].size += size;
			if (i < man->frees){
				if (addr + size == man->freeinfo[i].address){
					man->freeinfo[i - 1].size += man->freeinfo[i].size;
					man->frees--;
					for (; i < man->frees; i++) {
						man->freeinfo[i] = man->freeinfo[i + 1]; 
					}
				}
			}
			return 0;
		}	
	}
	if (i < man->frees) {
		if (addr + size == man->freeinfo[i].address) {
			man->freeinfo[i].address = addr;
			man->freeinfo[i].size += size;
			return 0; 
		}
	}
	if (man->frees < MEM_MANAGER_FREES){
		for (j = man->frees; j > i; j--) {
			man->freeinfo[j] = man->freeinfo[j - 1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees; 
		}
		man->freeinfo[i].address = addr;
		man->freeinfo[i].size = size;
		return 0; 
	}
	man->losts++;
	man->lostsize += size;
	return -1; 
}


unsigned int memory_alloc_4kb(MEMMAN* memoryManger,unsigned int needsize){
	unsigned int result;
	needsize = (needsize+0xfff) & 0xfffff000;
	result = memory_manager_alloc(memoryManger,needsize);
	return result;
}

int memory_free_4kb(MEMMAN* memoryManger,unsigned int addr,unsigned int freesize){
	unsigned int result;
	freesize = (freesize+0xfff) & 0xfffff000;
	result = memory_manager_free(memoryManger,addr,freesize);
	return result;
}