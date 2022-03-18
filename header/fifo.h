
#ifndef _FIFO_H_
#define _FIFO_H_
#define BUFFERMAX   256

//8bits
typedef struct buffer_FIFO8{
    unsigned char*  buf;
    int next_read,next_write;
    int size;
    int free;
    int flags;    
}FIFO;

extern struct _TASK_;

//32bits
struct buffer_FIFO32{
    int*  buf;
    int next_read,next_write;
    int size;
    int free;
    int flags;    
    struct _TASK_ * task;       
};
typedef struct buffer_FIFO32 FIFO32;

#endif


// 8bits
void init_fifo(FIFO* fifo, int size, unsigned char* buf);
int put_buffer_fifo(FIFO* fifo, unsigned char data);
int get_buffer_fifo(FIFO* fifo);
int buffer_status_fifo(FIFO* fifo);

//32bits
void init_fifo32(FIFO32* fifo, int size, int* buf);
int put_buffer_fifo32(FIFO32* fifo, int data);
int get_buffer_fifo32(FIFO32* fifo);
int buffer_status_fifo32(FIFO32* fifo);

int IsEmpty(FIFO* fifo);







