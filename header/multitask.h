#ifndef _MULTITASK_H
#define _MULTITASK_H
//flags: 0——未使用 1——休眠状态 2——唤醒状态
#define MAX_TASKS   1000
#define TASK_GDT0   3       

#define MAX_TASKS_LEVEL     100         
#define MAX_TASKLEVELS      10 


extern struct buffer_FIFO32;
extern struct _TASK_STATUS_SEGMENT;
extern struct _MY_CONSOLE;
extern struct _FILE_HANDLE;

struct _TASK_ 
{
    int gdtno;          //gdt编号
    int flags;
    int priority;
    int level;          //当前任务所处的等级，等级越小越优先执行

    struct SEGMENT_DESCRIPTOR ldt[2];       /*LDT*/
    struct _TASK_STATUS_SEGMENT tss;
    struct buffer_FIFO32 fifo;        //新增fifo 用于字符输入等操作
    struct _MY_CONSOLE_ * cons;         //地址 /*每个任务自带一个窗口结构*/
    struct _FILE_HANDLE* fhandle;     //文件句柄
    
    int* fat;
    int ds_base;                        //ds偏移量
    int cons_stack;                     //保存栈地址

    char* command;                      //命令行
    unsigned char language_mode;                  //语言模式
    unsigned char langbyte;                      //当前语言所占用字节
};

typedef struct _TASK_ TASK;

typedef struct _TASKLEVEL_
{
    int running_amount;     //当前队列正在运行的进程数量
    int nowtask;            //记录当前运行什么任务
    TASK* tasks[MAX_TASKS_LEVEL]; 
}TASKLEVEL;

typedef struct TASK_CONTROLLER
{
    int now_level;                                  //目前任务等级
    char lv_change_flags;                           //检查下次任务切换时是否切换
    TASKLEVEL level[MAX_TASKLEVELS];                //表示有多少个等级队列(0-10级)
    TASK tasks0[MAX_TASKS];

}TASKCTL;

#endif
void init_multitask();
void switch_multitask();
void switch_task();
void run_task(TASK* task);
void run_task_overload(TASK* task,int newlevel,int newPriority);
void sleep_task(TASK* task);
void idle_task();
TASK* query_now_task();
void add_task(TASK* task);
void remove_task(TASK* task);
void switchsub_task();





