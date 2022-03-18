#ifndef _MMEMORY_H_
#define _MMEMORY_H_

#define	EFLAGS_AC_BIT		0x00040000 //检查cpu是不是486以上
#define	CR0_CACHE_DISABLE	0x60000000 //保护模式的“cpu 缓存开关”

#define MEM_MANAGER_FREES   4096        //分配大概32kb,0x1000
#define MEM_MANAGER_ADDR    0x003c0000  //默认地址


typedef struct {
    unsigned int address;
    unsigned int size;    
}FREEINFO;

typedef struct{
    int frees,maxfrees,lostsize,losts;
    FREEINFO freeinfo[MEM_MANAGER_FREES]; //分配4096个信息表
}MEMMAN;//内存管理表 

#endif

//naskfunc.nas
int load_cr0();
void store_cr0(int cr0);

unsigned int memcheck(unsigned int begin, unsigned int end);


void init_memory_manager(MEMMAN* memoryManager);
unsigned int memory_manager_total(MEMMAN* memoryManger);
unsigned int memory_manager_alloc(MEMMAN* memoryManger,unsigned int needsize);
int memory_manager_free(MEMMAN* memoryManger,unsigned int addr,unsigned int freesize);

//以4字节为单位分配
unsigned int memory_alloc_4kb(MEMMAN* memoryManger,unsigned int needsize);
int memory_free_4kb(MEMMAN* memoryManger,unsigned int addr,unsigned int freesize);