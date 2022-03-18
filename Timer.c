#include "header/bootpack.h"
#include "header/fifo.h"
#include "header/Timer.h"
#include "header/multitask.h"
#define  NULL    ((void*)0x00)

#define PIT_CTRL    0x0043
#define PIT_CNT0    0X0040

#define TIMER_FLAGS_ALLOC           1   
#define TIMER_FLAGS_USING           2   
#define TIMER_FLAGS_NOT_USING       0   

TIMERCTL timerctl;          

//multitask.c
extern TIMER* mtask_timer; 

void init_pit(){
    TIMER* sentryTimer;          

    io_out8(PIT_CTRL,0x34);
    io_out8(PIT_CNT0,0x9c);
    io_out8(PIT_CNT0,0x2e);
    timerctl.count = 0;   
    timerctl.next = 0xffffffff;
    timerctl.usingNum = 0;
    int idx;
    for(idx = 0;idx<MAX_TIMER;++idx){
        timerctl.timer[idx].flags = TIMER_FLAGS_NOT_USING; 
    }
    
    sentryTimer = timer_alloc();    
    sentryTimer->timeout = 0xffffffff;
    sentryTimer->flags = TIMER_FLAGS_USING;             
    sentryTimer->next = 0;

    timerctl.firstTimer = sentryTimer;    
    timerctl.next = 0xffffffff;           
}

TIMER* timer_alloc(){
    int idx;
    for(idx = 0;idx < MAX_TIMER;++idx){
        if(timerctl.timer[idx].flags == TIMER_FLAGS_NOT_USING){
            timerctl.timer[idx].flags = TIMER_FLAGS_ALLOC; 
            timerctl.timer[idx].isCancel = 0;              
            return &timerctl.timer[idx];
        }
    }
    return NULL;
}

void timer_free(TIMER* timer){
    if(timer == NULL)
        return;
    timer->flags = TIMER_FLAGS_NOT_USING;
}


void init_timer(TIMER* timer,FIFO32* fifo,int data){
    timer->timefifo = fifo;
    timer->data = data;
}

void settime_timer(TIMER* timer,unsigned int timeout){       
    int eflags;
    TIMER* nowTimer,*prevTimer;

    timer->timeout = timeout + timerctl.count;
    timer->flags = TIMER_FLAGS_USING;

    eflags = io_load_eflags();
    io_cli();

    nowTimer = timerctl.firstTimer;       
    //queue head
    if(timer->timeout <= nowTimer->timeout){
        timerctl.firstTimer = timer;
        timer->next = nowTimer;
        timerctl.next = timer->timeout;  
        io_store_eflags(eflags);
        return;
    }

    while(1){
        prevTimer = nowTimer;
        nowTimer = nowTimer->next;
        
        if(timer->timeout <= nowTimer->timeout){
            prevTimer->next = timer;   
            timer->next = nowTimer;

            io_store_eflags(eflags);
            return;
        }
    }
}

void inthandler20(int *esp){
    char ts = 0;         

    TIMER* nowTimer = NULL; 
    io_out8(PIC0_OCW2,0x60);   
    ++timerctl.count;          
    if(timerctl.next > timerctl.count){
        return;
    }
    nowTimer = timerctl.firstTimer; 
    while(1){
        if(nowTimer->timeout > timerctl.count){
            break;
        }
        nowTimer->flags = TIMER_FLAGS_ALLOC;                        
        if(nowTimer != mtask_timer  ){                                
            put_buffer_fifo32(nowTimer->timefifo, nowTimer->data);      
        }else{
            ts = 1; //times up                                              
        }
        nowTimer = nowTimer->next;                          
        
    }

    timerctl.firstTimer = nowTimer; 
    timerctl.next = timerctl.firstTimer->timeout;

    if(ts != 0){
        switch_task();
    }
}

void cancel_timerAll(FIFO32* fifo){
    int eflags,i;
    TIMER* t;
    eflags = io_load_eflags();
    io_cli();

    for(i = 0;i < MAX_TIMER; ++i){
        t = &timerctl.timer[i];
        if(t->flags != 0 && t->isCancel != 0 && t->timefifo == fifo){
            cancel_timer(t);
            timer_free(t);
        }
    }
    io_store_eflags(eflags);
}

int cancel_timer(TIMER* timer){
    int eflags;
    TIMER* t;
    eflags = io_load_eflags();
    io_cli();

    if(timer->flags == TIMER_FLAGS_USING){
        if(timer == timerctl.firstTimer){
            t = timer->next;
            timerctl.firstTimer = t;
            timerctl.next = t->timeout;
        }else{
            t = timerctl.firstTimer;
            while(t != NULL){
                if(t->next == timer){
                    break;
                }
                t = t->next;
            }
            t->next = timer->next;
        }
        timer->flags = TIMER_FLAGS_ALLOC;
        io_store_eflags(eflags);
        return 1;//success
    }
    io_store_eflags(eflags);
    return 0;//failed
}
