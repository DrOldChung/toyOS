#include "header/bootpack.h"
#include "header/color.h"
#include "header/mmemory.h"
#include "header/msheet.h" 
#include "header/Timer.h"
#include "header/multitask.h"	
#include "header/fifo.h"

/*mcmd.c*/
extern void task_cmd_main(SHEET* sheet,unsigned int memtotal);


void putstr_sht(SHEET* sht, int x, int y, int color, int bgcolor, char* str, int len){
	TASK* task = query_now_task();
	box(sht->buffer, sht->bxsize, bgcolor, x, y, x + len * 8  - 1, y + 15);
	if(task->language_mode != 0 && task->langbyte != 0){
		putstr(sht->buffer, sht->bxsize, x, y, color, str);
		sheet_refresh(sht, x - 8, y, x + len * 8, y + 16);
	}else{
		putstr(sht->buffer, sht->bxsize, x, y, color, str);
		sheet_refresh(sht, x, y, x + len * 8, y + 16);
	}
}

void putstr_sht_another(SHEET* sht, int x, int y, int color, int bgcolor, char c, int len){
	box(sht->buffer, sht->bxsize, bgcolor, x-8, y,  x + len * 8 - 1 , y+16);
	char ch[2] ={'0','\0'};
	ch[0] = c;
	putstr(sht->buffer, sht->bxsize, x, y, color, ch);
	sheet_refresh(sht, x, y, x + len * 8, y+16);
}


void make_textbox(SHEET* sht,int x0,int y0,int sx,int sy,int customColor){
	int x1 = x0 + sx;
	int y1=  y0 + sy;
	box(sht->buffer, sht->bxsize, color_848484, x0 - 2, y0 - 3, x1 + 1, y0 - 3);
	
	box(sht->buffer, sht->bxsize, color_848484, x0 - 3, y0 - 3, x0 - 3, y1 + 1);

	box(sht->buffer, sht->bxsize, color_ffffff, x0 - 3, y1 + 2, x1 + 1, y1 + 2);

	box(sht->buffer, sht->bxsize, color_ffffff, x1 + 2, y0 - 3, x1 + 2, y1 + 2);

	box(sht->buffer, sht->bxsize, color_000000, x0 - 1, y0 - 2, x1 + 0, y0 - 2);

	box(sht->buffer, sht->bxsize, color_000000, x0 - 2, y0 - 2, x0 - 2, y1 + 0);

	box(sht->buffer, sht->bxsize, color_c6c6c6, x0 - 2, y1 + 1, x1 + 0, y1 + 1);

	box(sht->buffer, sht->bxsize, color_c6c6c6, x1 + 1, y0 - 2, x1 + 1, y1 + 1);

	box(sht->buffer, sht->bxsize, customColor,  x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	
}



void create_window(unsigned char* buf,int xsize,int ysize,char* title){
	static char closebtn[14][16]={
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@",
	};

	box(buf, xsize, color_c6c6c6, 0,         0,         xsize - 1, 0        );
	box(buf, xsize, color_ffffff, 1,         1,         xsize - 2, 1        );
	box(buf, xsize, color_c6c6c6, 0,         0,         0,         ysize - 1);
	box(buf, xsize, color_ffffff, 1,         1,         1,         ysize - 2);
	box(buf, xsize, color_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	box(buf, xsize, color_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	box(buf, xsize, color_c6c6c6, 2,         2,         xsize - 3, ysize - 3);
	box(buf, xsize, color_000084, 3,         3,         xsize - 4, 20       );
	box(buf, xsize, color_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	box(buf, xsize, color_000000, 0,         ysize - 1, xsize - 1, ysize - 1);

	int x,y;
	char ch;
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			ch = closebtn[y][x];
			if (ch == '@') {
				ch =color_000000;
			} else if (ch == '$') {
				ch = color_848484;
			} else if (ch == 'Q') {
				ch = color_c6c6c6;
			} else {
				ch = color_ffffff;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = ch;
		}
	}
	
}

void make_title(unsigned char* buf, int xsize, char* title, char act){
	static char closebtn[14][16]={
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@",
	};
	char titleColor,btnColor;
	if(act != 0){
		titleColor = color_ffffff;
		btnColor = color_000084;
	}else{
		titleColor = color_c6c6c6;
		btnColor = color_848484;
	}
	box(buf,xsize,btnColor,3,3,xsize-4,20);
	putstr(buf,xsize,24,4,titleColor,title);

	int x,y;
	char ch;
	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			ch = closebtn[y][x];
			if (ch == '@') {
				ch =color_000000;
			} else if (ch == '$') {
				ch = color_848484;
			} else if (ch == 'Q') {
				ch = color_c6c6c6;
			} else {
				ch = color_ffffff;
			}
			buf[(5 + y) * xsize + (xsize - 21 + x)] = ch;
		}
	}
}


void create_window_other(unsigned char* buf,int xsize,int ysize,char* title, char act){	
	box(buf, xsize, color_c6c6c6, 0,         0,         xsize - 1, 0        );
	box(buf, xsize, color_ffffff, 1,         1,         xsize - 2, 1        );
	box(buf, xsize, color_c6c6c6, 0,         0,         0,         ysize - 1);
	box(buf, xsize, color_ffffff, 1,         1,         1,         ysize - 2);
	box(buf, xsize, color_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	box(buf, xsize, color_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	box(buf, xsize, color_c6c6c6, 2,         2,         xsize - 3, ysize - 3);
	box(buf, xsize, color_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	box(buf, xsize, color_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	make_title(buf,xsize,title,act);
} 


void window_mouseMoveMode(SHEET* sheet,SHEETCONTROLL* shtctl,int* mousemodeX,int* mouseModeY,int* mx,int* my)//mmx:move mode x mmy: move mode y
{
	int i = 0,j = 0,x = 0,y = 0;
	int mmx = *mousemodeX;
	int mmy = *mouseModeY;
	int mouseX = *mx;
	int mouseY = *my;
	SHEET* sht = 0;
	if (mmx < 0)
	{
		for (j = shtctl->top - 1; j > 0; j--)
		{
			sht = shtctl->sheets[j];
			x = mouseX - sht->vramX0;
			y = mouseY - sht->vramY0;
			if (x >= 0 && x < sht->bxsize && y >= 0 && y < sht->bysize)
			{
				if (sht->buffer[y * sht->bxsize + x] != sht->colorInv)
				{
					sheet_updown(sht, shtctl->top - 1);
					if (x >= 3 && x < sht->bxsize - 3 && y >= 3 && y < 21)
					{
						mmx = mouseX;
						mmy = mouseY;
					}
					break;
				}
			}
		} // end for-loop
	}
	else
	{
		x = mouseX - mmx;
		y = mouseY - mmy;
		sheet_slided(sht, sht->vramX0 + x, sht->vramY0 + y);
		mmx = mouseX;
		mmy = mouseY;
	}

	*mousemodeX = mmx;
	*mouseModeY = mmy;
}

void change_winTitle(SHEET* sht,char act){
	int x,y;
	int xsize = sht->bxsize;
	char color,titleColor_new,btnColor_new;
	char titleColor_old,btnColor_old;
	char* buf = sht->buffer;

	if(act != 0){
		titleColor_new = color_ffffff;
		btnColor_new = color_000084;

		titleColor_old = color_c6c6c6;
		btnColor_old = color_848484;
	}else{
		titleColor_new = color_c6c6c6;
		btnColor_new = color_848484;

		titleColor_old = color_ffffff;
		btnColor_old = color_000084;
	}

	for(y = 3; y <= 20; ++y){
		for(x = 3; x <= xsize - 4; ++x){
			color = buf[y * xsize + x];
			if(color == titleColor_old && x <= xsize -22){
				color = titleColor_new;
			}else if(color == btnColor_old){
				color = btnColor_new;
			}
			buf[y * xsize + x] = color;
		}
	}
	sheet_refresh(sht,3,3,xsize,21);
}

void keywin_off(SHEET* keyWindow){
	change_winTitle(keyWindow,0);
	if((keyWindow->flags & 0x20) != 0){
		put_buffer_fifo32(&keyWindow->task->fifo,3);//cursor close
	}
}
void keywin_on(SHEET* keyWindow){
	change_winTitle(keyWindow,1);
	if((keyWindow->flags & 0x20) != 0){
		put_buffer_fifo32(&keyWindow->task->fifo,2);//cursor open
	}
}
TASK* open_constask(SHEET* sht,unsigned int memtotal){
	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	TASK* task_cons = alloc_task();
	
	int* cons_fifo = (int*) memory_alloc_4kb(memManager,128 * 4);
	task_cons->cons_stack = memory_alloc_4kb(memManager,64 * 1024);
	task_cons->tss.esp = task_cons->cons_stack + 64 * 1024 - 12;   
	task_cons->tss.eip = (int)(&task_cmd_main);					
	task_cons->tss.es = 1 * 8;
	task_cons->tss.cs = 2 * 8;
	task_cons->tss.ss = 1 * 8;
	task_cons->tss.ds = 1 * 8;
	task_cons->tss.fs = 1 * 8;
	task_cons->tss.gs = 1 * 8;

	*((int *)(task_cons->tss.esp + 4)) = (int)(sht);
	*((int *)(task_cons->tss.esp + 8)) = memtotal;		 

	run_task_overload(task_cons, 2, 2);					  
	init_fifo32_overload(&task_cons->fifo, 128, cons_fifo, task_cons);

	return task_cons;
}

SHEET* open_console(SHEETCONTROLL* shtctl,unsigned int memtotal){
	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	SHEET* sht = sheet_alloc(shtctl);
	unsigned char* buf = (unsigned char*) memory_alloc_4kb(memManager,256*156);
	sheet_buffer_set(sht, buf, 256, 165, -1);
	create_window_other(buf, 256, 165, "console", 0); 
	make_textbox(sht, 8, 28, 240, 128, color_000000);
	
	sht->task = open_constask(sht,memtotal);
	sht->flags |= 0x20;
	return sht;
}


void close_constask(TASK* task){
	MEMMAN* memManager = (MEMMAN*) MEM_MANAGER_ADDR;
	sleep_task(task);

	memory_free_4kb(memManager,task->cons_stack,64 * 1024);
	memory_free_4kb(memManager,(int)(task->fifo.buf),128 * 4);
	task->flags = 0;
}

void close_console(SHEET* sht){
	MEMMAN* memManager = (MEMMAN*) MEM_MANAGER_ADDR;
	TASK* task = sht->task;

	memory_free_4kb(memManager,(int)sht->buffer,256 * 165);
	sheet_free(sht);
	close_constask(task);
}