#ifndef _MSHEET_H_
#define _MSHEET_H_
#define MAX_SHEETS      256     //最大图层数量
#define SHEETS_USE      1       //标记正在使用状态
extern struct _TASK_;
#endif

typedef struct SHTCTL  SHEETCONTROLL;
typedef struct SHEET   SHEET;
struct SHEET{
    unsigned char* buffer;        //记录地址
    int bxsize,bysize;            //大小
    int vramX0,vramY0;            //画面显示坐标
    int colorInv;                 //颜色与透明度
    int height;                   //高度
    int flags;                    //记录设定信息
    SHEETCONTROLL* ctl;
    struct _TASK_ * task;              
    
};

struct SHTCTL{
    unsigned char* vram;            //显存地址
    unsigned char* map;             //地图（记录图层层级像素数据) 
    int xsize,ysize,top;            //画面大小
    SHEET* sheets[MAX_SHEETS];      //记录地址
    SHEET  sheets0[MAX_SHEETS];     //用于排列图层，后放入sheets表
};//Controller

SHEETCONTROLL* init_sheetControll(MEMMAN* memoryManager,unsigned char* vram,int xsize,int ysize);
SHEET* sheet_alloc(SHEETCONTROLL* ctl);//分配sheet内存
void sheet_free(SHEET* sheet);//释放sheet内存
void sheet_buffer_set(SHEET* sheet,unsigned char* buf,int xsize,int ysize,int colInv);
void sheet_updown(SHEET* sheet, int newheight);//调成图层高度
void sheet_refresh(SHEET* sheet,int bx0,int by0,int bx1,int by1);//画面刷新
void sheet_refresh_sub(SHEETCONTROLL* ctl,int vx0,int vy0,int vx1,int vy1,int layer, int anotherLayer);//图层按需刷新  layer:图层优先级(0 - n)
void sheet_refresh_map(SHEETCONTROLL* ctl,int vx0,int vy0,int vx1,int vy1,int layer);//只针对当前层级的map进行刷新，避免了出现两个图层间重叠问题(闪烁)
void sheet_slided(SHEET* sheet,int newvx0,int newvy0);//画面拖动



