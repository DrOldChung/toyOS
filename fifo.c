#include "header/bootpack.h"       
#include "header/file.h"
#include "header/fifo.h"     
#include "header/multitask.h" 

#define  FLAGS_OVERFLOW 0x0001
#define  NULL    ((void*)0x00)

 
FIFO timerfifo; 
FIFO32* keyfifo;
FIFO32* mousefifo;

void init_fifo32_overload(FIFO32* fifo, int size, int* buf,TASK* newtask);

int IsEmpty(FIFO* fifo){
    if(fifo == NULL){
        return -1;
    }
    return 1;
}

void init_fifo32(FIFO32* fifo, int size, int* buf){
    fifo->size = size;
    fifo->buf  = buf;
    fifo->flags= 0;
    fifo->free = size;
    fifo->next_read = 0;
    fifo->next_write = 0;
}

void init_fifo32_overload(FIFO32* fifo, int size, int* buf,TASK* newtask){
    fifo->size = size;
    fifo->buf  = buf;
    fifo->flags= 0;
    fifo->free = size;
    fifo->next_read = 0; 
    fifo->next_write = 0;
    fifo->task = newtask;
}

int put_buffer_fifo32(FIFO32* fifo, int data){
    if(fifo->free ==0){
        fifo->flags |= FLAGS_OVERFLOW; 
        return -1;
    }
    fifo->buf[fifo->next_write] = data;
    ++fifo->next_write;
    if(fifo->next_write >=fifo->size){
        fifo->next_write = 0;
    }
    --fifo->free;

    if(fifo->task != NULL){
        if(fifo->task->flags != 2){                
            run_task_overload(fifo->task,-1,0);
        }
    }
    return 0;
}
int get_buffer_fifo32(FIFO32* fifo){
    int data;
    if(fifo->free == fifo->size){//NULL
        return -1;
    }
    data = fifo->buf[fifo->next_read];
    ++fifo->next_read;
    if(fifo->next_read >=fifo->size){
        fifo->next_read = 0;
    }
    ++fifo->free;
    return data;
}

int buffer_status_fifo32(FIFO32* fifo){
    return fifo->size - fifo->free;
}

