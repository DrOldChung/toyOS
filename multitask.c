#include "header/bootpack.h" 
#include "header/mmemory.h"
#include "header/Timer.h"
#include "header/msheet.h"   
#include "header/fifo.h"
#include "header/multitask.h"

#define   NULL      ((void*)0x00)


TASKCTL* taskctl;
TIMER* mtask_timer;

int mtask_tr = 0; 


void switch_multitask(){
    if(mtask_timer ==  NULL){
        return;
    }

    if(mtask_tr == 3*8){
        mtask_tr = 4*8;
    }else if(mtask_tr == 4*8){
        mtask_tr = 3*8;
    }
    settime_timer(mtask_timer,2);
    farjmp(0,mtask_tr);        
}

TASK* alloc_task()                       
{
    int i;
    TASK* task = NULL;
    for(i = 0;i < MAX_TASKS; ++i){
        if(taskctl->tasks0[i].flags == 0){
            task = &taskctl->tasks0[i];
            task->flags = 1;       
            task->tss.eflags = 0x00000202;  

            task->tss.eax = 0;
            task->tss.ebx = 0;
            task->tss.ecx = 0;
            task->tss.edx = 0;

            task->tss.ebp = 0;
            task->tss.edi = 0;
            task->tss.esi = 0;

            task->tss.es = 0;
            task->tss.ds = 0;
            task->tss.fs = 0;
            task->tss.gs = 0;

            task->tss.iomap = 0x40000000;
            task->tss.ss0 = 0;
            return task;       
        }
    }
    return NULL;    
}



void run_task_overload(TASK* task, int newlevel, int newPriority){
    
    if(newlevel < 0){
        newlevel = task->level;    
    }
    if(newPriority > 0){
        task->priority = newPriority;
    }

    if(task->flags == 2 && task->level != newlevel){
        
        remove_task(task);        
    }

    if(task->flags != 2){
        task->level = newlevel;
        add_task(task);         
    }

    taskctl->lv_change_flags = 1;   

}

void switch_task(){
    TASKLEVEL* tasklv = &taskctl->level[taskctl->now_level];
    TASK* now_task = tasklv->tasks[tasklv->nowtask];
    TASK* new_task = NULL;

    ++tasklv->nowtask;

    if(tasklv->nowtask == tasklv->running_amount){
        tasklv->nowtask = 0;   
    }
    if(taskctl->lv_change_flags !=0){
        switchsub_task();
        tasklv = &taskctl->level[taskctl->now_level];  
    }

    new_task = tasklv->tasks[tasklv->nowtask];

    settime_timer(mtask_timer,new_task->priority);

    if(new_task != now_task){
        farjmp(0,new_task->gdtno);
    }    

}

void sleep_task(TASK* task){
    TASK* now_task = NULL;

    if(task->flags == 2){
        now_task = query_now_task();
        remove_task(task);        
        if(task == now_task){
            switchsub_task();
            now_task = query_now_task();    
            farjmp(0, now_task->gdtno);     
        }
    }
}

void idle_task(){
    while(1){
        io_hlt();
    }
}

TASK* query_now_task(){
    TASKLEVEL* tasklevels = &taskctl->level[taskctl->now_level];
    return tasklevels->tasks[tasklevels->nowtask];              
}

void add_task(TASK* task){
    TASKLEVEL* tasklv = &taskctl->level[task->level];
    
    tasklv->tasks[tasklv->running_amount] = task;                  
    ++tasklv->running_amount;
    task->flags = 2;                                                
}

void remove_task(TASK* task){
    TASKLEVEL* tasklv = &taskctl->level[task->level];
    int i;
    int runningNum = tasklv->running_amount;

    for(i = 0; i < tasklv->running_amount; ++i){
        if(tasklv->tasks[i] == task){
            break;
        }
    }

    --tasklv->running_amount;
    runningNum = tasklv->running_amount;

    if(i < tasklv->nowtask){
        --tasklv->nowtask;
    }

    if(tasklv->nowtask >= tasklv->running_amount){
        tasklv->nowtask = 0;
    }

    task->flags = 1;  

    for(; i < tasklv->running_amount; ++i){
        tasklv->tasks[i] = tasklv->tasks[i + 1];
    }

}


void switchsub_task(){
    int i = 0;
    for(; i < MAX_TASKLEVELS; ++i){
        if(taskctl->level[i].running_amount > 0){
            break; 
        }
    }
    taskctl->now_level = i;       
    taskctl->lv_change_flags = 0; 
}


TASK* init_task(MEMMAN* memman)       
{
    int idx;
    TASK* task = NULL;
    TASK* idle = NULL;
    
    struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)(ADR_GDT);//init gdt

    taskctl  = (TASKCTL*) memory_alloc_4kb(memman, sizeof(TASKCTL));     //alloc
    
    for(idx = 0;idx < MAX_TASKS; ++idx){//MAX_TASK:500
        taskctl->tasks0[idx].flags = 0;     
        taskctl->tasks0[idx].gdtno = (TASK_GDT0 + idx) * 8 ;     
        taskctl->tasks0[idx].tss.ldtr = (TASK_GDT0 + MAX_TASKS + idx) * 8;  
        set_segmdesc(gdt + (TASK_GDT0+idx), 103, (int)(&taskctl->tasks0[idx].tss), AR_TSS32);           
        set_segmdesc(gdt + (TASK_GDT0+MAX_TASKS+idx), 15, (int)(taskctl->tasks0[idx].ldt), AR_LDT);    
    }
    
   
    for(idx = 0; idx < MAX_TASKLEVELS; ++idx){
        taskctl->level[idx].running_amount = 0;
        taskctl->level[idx].nowtask = 0;
    }

    task = alloc_task();
    task->flags = 2;       
    task->priority = 2;     //0.02 seconds
    task->level = 0;        

    add_task(task);         
    switchsub_task();    

    load_tr(task->gdtno);
    mtask_timer = timer_alloc();
    settime_timer(mtask_timer,task->priority);

    //create idle task
    idle = alloc_task();
    idle->tss.esp = memory_alloc_4kb(memman, 64*1024) + 64 * 1024;
    idle->tss.eip = (int)(&idle_task);       
    idle->tss.cs = 2 * 8;
    idle->tss.ds = 1 * 8;
    idle->tss.es = 1 * 8;
    idle->tss.fs = 1 * 8;
    idle->tss.gs = 1 * 8;
    idle->tss.ss = 1 * 8;
    run_task_overload(idle, MAX_TASKLEVELS - 1, 1);         
    return task;  
}
