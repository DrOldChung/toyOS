#ifndef _TIMER_H_
#define _TIMER_H_
#include "fifo.h"  
#endif
#define MAX_TIMER    500

typedef struct TIMER {
    struct TIMER* next;         //next指针：存放下一个即将超时的计时器的地址
    FIFO32* timefifo;         //用作记录超时信息
    unsigned int timeout;   
    unsigned int flags;     //记录当前计时器状态
    unsigned int isCancel;  //是否在应用程序结束时自动取消计时器 1:取消 0：不取消
    int data;
}TIMER;



typedef struct {
    unsigned int count;              //计时
    unsigned int next;               //下一个该更新的时间 
    unsigned int usingNum;           //正在使用的计时器有多少个
    TIMER* firstTimer;
    TIMER  timer[MAX_TIMER];         //设置500个计时器
}TIMERCTL;

void init_pit();
void init_timer(TIMER* timer,FIFO32* fifo,int data);
void settime_timer(TIMER* timer,unsigned int timeout);
int cancel_timer(TIMER* timer);
void cancel_timerAll(FIFO32* fifo);
TIMER* timer_alloc();
void timer_free(TIMER* timer);





