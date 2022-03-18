#include <stdio.h>
#include <string.h>
#include "header\bootpack.h"
#include "header\color.h"
#include "header\fifo.h"		
#include "header\kbcontroll.h"
#include "header\mmemory.h"
#include "header\file.h"
#include "header\msheet.h"
#include "header\Timer.h"	
#include "header\console.h"   
#include "header\multitask.h" 
/*sequence :  bootpack -> file -> fifo -> timer -> multitask */

//mulitask.c
extern TASKCTL* taskctl;

//window.c
extern void putstr_sht(SHEET* sht, int x, int y, int color, int bgcolor, char* str, int len);
extern void putstr_sht_another(SHEET* sht, int x, int y, int color, int bgcolor, char c, int len);
extern void make_textbox(SHEET* sht,int x0,int y0,int sx,int sy,int customColor);
extern SHEET* open_console(SHEETCONTROLL* shtctl,unsigned int memtotal);
extern TASK* open_constask(SHEET* sht,unsigned int memtotal);

//fifo.c
extern void init_fifo32_overload(FIFO32* fifo, int size, int* buf,TASK* newtask);				

//file.c
extern void file_readFAT(int* fat, unsigned char* img);
extern void file_loadfile(int clustno,int size,char* buf,int* fat,char* img);
extern int record_1;

//helper function
int compare_str(unsigned char* first,unsigned char* second);
int compare_str_n(const unsigned char* first,const unsigned char* second,unsigned int n);
int str_len(const char* s);
void hrb_api_drawline(SHEET* sht,int x0,int y0,int x1,int y1,int color);
void hrb_api_keyInput(CONSOLE* cons,TASK* task,FIFO32* sys_fifo,SHEETCONTROLL* shtctl, int eax,int* reg);
void savePrevCommand(char* nowcmd,char* prevcmd);
void str_cpy(char* first,char* second);

void cmd_newline(CONSOLE* cons);
void cmd_putchar(CONSOLE* cons,char ch,char ismove);
void cmd_run(char* command,CONSOLE* cons,int* fat,unsigned int memtotal);

void cmd_putstr0(CONSOLE* cons,const char* s);
void cmd_putstr1(CONSOLE* cons,const char* s,int len);

/*
Command：
	dir : directory
	cls : clean screen
	mem : memory
	type: view files
	hlt : halt state of the cpu
	exit: exit program
	start: open console and run 
	ncst: no console start
	langmode:language mode
*/
void cmd_mem(CONSOLE* cons,unsigned int memtotal);
void cmd_cls(CONSOLE* cons);
void cmd_dir(CONSOLE* cons);
void cmd_hlt(CONSOLE* cons,int* fat);
void cmd_type(CONSOLE* cons,int* fat,char* command);
void cmd_exit(CONSOLE* cons,int* fat);
void cmd_start(CONSOLE* cons,char* command,int memtotal);
void cmd_ncst(CONSOLE* cons,char* command,int memtotal);
int	 cmd_app(CONSOLE* cons,int* fat,char* command);
void cmd_langmode(CONSOLE* cons,char* command);

//protected
int inthandler0d(int* esp);
int inthandler0c(int* esp);

int* hrb_api(int edi,int esi,int ebp,int esp,int ebx,int edx,int ecx,int eax);

void task_cmd_main(SHEET* sheet,unsigned int memtotal){
	TIMER* timer;
	CONSOLE cons;	
	TASK* task = query_now_task();
	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	FILEINFO* finfo = (FILEINFO*)(ADR_DISKIMG + 0x002600);
	FILEHANDLE fhandle[8];
	struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)(ADR_GDT);

	int* fat = (int*)(memory_alloc_4kb(memManager,4*2880));

	int i;
	int input_curX = 16;
	int input_curY = 28;
	int cursor_color = -1; 	
	char str[30];
	char* p;					

	char cmdline[30];			
	char prevCMD[30];

	unsigned char* nihongo = (char*) *((int*) 0x0fe8);

	//init console
	cons.sht = sheet;
	cons.cur_x = 8;
	cons.cur_y = 28;
	cons.cur_color = -1;
	task->cons = &cons;
	task->command = cmdline;

	//load font
	if(nihongo[4096] != 0xff){
		task->language_mode = 1;
	}else{
		task->language_mode = 0;//Eng
	}
	task->langbyte = 0;
	for(i = 0;i < 8;++i){
		fhandle[i].buf = NULL;
	}
	task->fhandle = fhandle;
	task->fat = fat;

	if(cons.sht != NULL){
		cons.timer = timer_alloc();
		init_timer(cons.timer,&task->fifo,1);
		settime_timer(cons.timer,50);
	}
	
	file_readFAT(fat,(unsigned char*)(ADR_DISKIMG + 0x000200));
	cmd_putchar(&cons,'>',1);

	int x,y;
	while(1){
		io_cli();
		int bufstatus = buffer_status_fifo32(&task->fifo);
		if(bufstatus == 0){
			sleep_task(task);
			io_sti();
		}else{
			i = get_buffer_fifo32(&task->fifo);
			io_sti();
			if(i <= 1 && cons.sht != NULL)
			{/*光标定时器*/
				if(i != 0)
				{
					init_timer(cons.timer,&task->fifo,0); 	
					if(cons.cur_color >= 0){				
						cons.cur_color = color_ffffff;	
					}
				}
				else
				{
					init_timer(cons.timer,&task->fifo,1);
					if(cons.cur_color >= 0){
						cons.cur_color = color_000000;
					}
				}//End if-else
				settime_timer(cons.timer,50);
			}//End if
			if(i == 2){
				cons.cur_color = color_ffffff;
			}
			if(i == 3){
				if(cons.sht != NULL){
					box(cons.sht->buffer,cons.sht->bxsize,color_000000,cons.cur_x,cons.cur_y,cons.cur_x + 7,cons.cur_y + 15);
				}
				cons.cur_color = -1;
			}
			if(i == 4){
				cmd_exit(&cons,fat);
			}

			if(i>=256 && i<=511){
				
				if(i == 4 + 256){//Up
					if(cmdline[0] != '\0' && prevCMD[0] != '\0'){
						cmd_putstr0(&cons,prevCMD);
						str_cpy(cmdline,prevCMD);
					}
				}
				else if(i == 8 + 256){
					if(cons.cur_x > 16){
						cmd_putchar(&cons,' ',0);
						cons.cur_x -= 8;
					}
				}else if(i == 10 + 256){
					cmd_putchar(&cons,' ', 0);			
					cmdline[cons.cur_x / 8 - 2] = '\0';	
					cmd_newline(&cons);	
					savePrevCommand(cmdline,prevCMD);
					cmd_run(cmdline,&cons,fat,memtotal);
					if(cons.sht == NULL){
						cmd_exit(&cons,fat);
					}
					cmd_putchar(&cons,'>',1);
				}else{
					if(cons.cur_x < 240){
						cmdline[cons.cur_x / 8 -2] = i -256;
						cmd_putchar(&cons,i-256,1);
					}
				}

			}
			if(cons.sht != NULL){
				if(cons.cur_color >= 0){
					box(cons.sht->buffer,cons.sht->bxsize,cons.cur_color, cons.cur_x, cons.cur_y, cons.cur_x + 7, cons.cur_y + 15);
				}
				sheet_refresh(cons.sht,cons.cur_x,cons.cur_y, cons.cur_x + 8, cons.cur_y + 16);			
			}
		}
	}
}

void cmd_newline(CONSOLE* cons){
	int x,y;
	SHEET* sheet = cons->sht;
	TASK* task = query_now_task();
	if(cons->cur_y < 28 + 112){
		cons->cur_y += 16;
	}else{
		if(sheet != NULL){
			for (y = 28 ; y < 28 + 112; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buffer[x + (y) * sheet->bxsize] = sheet->buffer[x + (y+16) * sheet->bxsize];
				}
			}
			for (y = 28 + 112; y < 28 + 128; y++) {
				for (x = 8; x < 8 + 240; x++) {
					sheet->buffer[x + y * sheet->bxsize] = color_000000;
				}
			}
			sheet_refresh(sheet,8,28,8+240, 28 + 128);	
		}

	}
	cons->cur_x = 8;
	if(task->language_mode == 1 && task->langbyte != 0){
		cons->cur_x += 8;
	}
}

void cmd_putchar(CONSOLE* cons,char ch,char ismove){ //ismove:光标是否后移
	char str[2];
	str[0] = ch;
	str[1] = '\0';

	if(str[0] == 0x09){
		while(1){
			putstr_sht(cons->sht,cons->cur_x,cons->cur_y,color_ffffff,color_000000," ",1);
			cons->cur_x += 8;
			if(cons->cur_x >= 8 + 240){
				cmd_newline(cons);
			}
			if(((cons->cur_x - 8) % 32) == 0){ 
				break;
			}
		}

	}else if(str[0] == 0x0a){
		cmd_newline(cons);
	}else if(str[0] == 0x0d){
		//do nothing
	}else{
		if(cons->sht != NULL){
			putstr_sht(cons->sht,cons->cur_x,cons->cur_y,color_ffffff,color_000000,str,1);								
		}
		if(ismove != 0){
			cons->cur_x += 8;
			if(cons->cur_x >= 8 + 240){
				cmd_newline(cons);
			}
		}
	}
	return;
}

void cmd_putstr0(CONSOLE* cons,const char* s){
	if(cons == NULL){
		return;
	}
	while(*s != '\0'){
		cmd_putchar(cons,*s,1);
		++s;
	}
}
void cmd_putstr1(CONSOLE* cons,const char* s,int len){
	if(cons == NULL){
		return;
	}
	int i;
	for(i = 0; i < len; ++i){
		cmd_putchar(cons,s[i],1);
	}
}

void cmd_run(char* command,CONSOLE* cons,int* fat,unsigned int memtotal){
	if(compare_str(command,"mem") == 0 && cons->sht != NULL)
	{
		cmd_mem(cons,memtotal);
	}
	else if(compare_str(command,"cls") == 0 && cons->sht != NULL)
	{
		cmd_cls(cons);
	}
	else if(compare_str(command,"dir") == 0 && cons->sht != NULL)
	{
		cmd_dir(cons);
	}
	else if(compare_str(command,"exit") == 0)
	{
		cmd_exit(cons,fat);
	}
	else if(compare_str_n(command,"start ",6) == 0){
		cmd_start(cons,command,memtotal);
	}
	else if(compare_str_n(command,"ncst ",5) == 0){
		cmd_ncst(cons,command,memtotal);
	}
	else if(compare_str_n(command,"langmode ",9) == 0){
		cmd_langmode(cons,command);
	}
	else if(command[0] != NULL)
	{
		if(cmd_app(cons,fat,command) == 0){
			cmd_putstr0(cons,"Command wrong!\n\n");
		}
	}
}

int cmd_app(CONSOLE* cons,int* fat,char* command){
	FILEINFO* finfo;
	SHEET* sht;
	SHEETCONTROLL* shtctl;
	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)(ADR_GDT);
	TASK* task = query_now_task();	

	int i;
	int cmdlen = str_len(command);
	int segsize;//segement size 
	int appsize;//application size
	int datasize;
	int datahrb;
	int esp;

	char filename[18];
	char* p,*q;
	char info[128];
	
	for(i = 0;i < 13; ++i){
		if(command[i] <= ' '){
			break;
		}
		filename[i] = command[i];
	}
	filename[i] = '\0';

	finfo = file_search(filename,(FILEINFO*)(ADR_DISKIMG + 0x002600),224);
	if(finfo == NULL && filename[i - 1] != '.'){
		//利用扩展名寻找
		filename[i] = '.';
		filename[i + 1] = 'H';
		filename[i + 2] = 'R';
		filename[i + 3] = 'B';
		filename[i + 4] = '\0';
		//重新寻找
		finfo = file_search(filename,(FILEINFO*)(ADR_DISKIMG + 0x002600),224);

	}

	if(finfo != NULL){
		appsize = finfo->size;
		p = file_loadfile_overload(finfo->clustno,&appsize,fat);
		if(appsize >= 36 && strncmp(p+4,"Hari",4) == 0 && *p == 0x00 ){	
			//init segsize、esp、datasize、datahrb....
			segsize  = *((int*)(p + 0x0000));	//0x00
			esp		 = *((int*)(p + 0x000c));	//0x0c
			datasize = *((int*)(p + 0x0010));	//0x10
			datahrb  = *((int*)(p + 0x0014));	//0x14

			q =(char*) memory_alloc_4kb(memManager,segsize);
			task->ds_base = (int)(q);
	
			set_segmdesc(task->ldt , finfo->size - 1, (int)(p), AR_CODE32_ER + 0x60);  //set cs
			set_segmdesc(task->ldt + 1, segsize - 1,(int)(q),AR_DATA32_RW + 0x60);     //set ds
			
			int idx = 0;
			for(;idx < datasize; ++idx){
				q[esp+idx] = p[datahrb+idx];
			}
			start_app(0x1b, 0 * 8 + 4, esp, 1 * 8 + 4, &(task->tss.esp0));
			shtctl = (SHEETCONTROLL*) *((int *) 0x0fe4);
			for(i = 0;i < MAX_SHEETS;++i){
				sht = &(shtctl->sheets0[i]);
				//ESC
				if((sht->flags & 0x11) == 0x11 && sht->task == task){
					sheet_free(sht);
				}
			}
			for(i = 0;i < 8;++i){
				if(task->fhandle[i].buf != NULL){
					memory_free_4kb(memManager,(int)task->fhandle[i].buf,task->fhandle[i].size);
					task->fhandle[i].buf = NULL;
				}
			}
			cancel_timerAll(&task->fifo);
			memory_free_4kb(memManager,(int)(q),segsize);
			task->langbyte = 0;
		}else{
			cmd_putstr0(cons,".hrb file format error!\n");
		}
		memory_free_4kb(memManager,(int)(p),appsize);
		cmd_newline(cons); 
		return 1;
	}
	return 0;
}

void cmd_mem(CONSOLE* cons,unsigned int memtotal){

	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	char str[128];

	sprintf(str,"total: %dMB\nfree:  %dMB\n\n",memtotal,memory_manager_total(memManager)/1024);
	cmd_putstr0(cons,str);
	
}

void cmd_cls(CONSOLE* cons){
	SHEET* sheet = cons->sht;
	int x,y;
	for (y = 28; y < 28 + 128; ++y)
	{
		for (x = 8; x < 8 + 240; ++x)
		{
			sheet->buffer[x + y * sheet->bxsize] = color_000000; //填充黑色
		}
	}
	sheet_refresh(sheet, 8, 28, 8 + 240, 28 + 128);
	cons->cur_y = 28; //回到开头
}

void cmd_dir(CONSOLE* cons){
	FILEINFO* finfo = (FILEINFO*)(ADR_DISKIMG + 0x002600);
	SHEET* sheet = cons->sht;
	char str[30];
	int i, j;

	for (i = 0; i < 224; ++i)
	{
		if (finfo[i].name[0] == 0x00)
		{
			break;
		}
		if (finfo[i].name[0] != 0xe5)
		{
			if ((finfo[i].type & 0x18) == 0)
			{
				sprintf(str, "filename.ext %2d\n", finfo[i].size);
				for (j = 0; j < 8; ++j)
				{
					str[j] = finfo[i].name[j];
				}
				str[9] = finfo[i].ext[0];
				str[10] = finfo[i].ext[1];
				str[11] = finfo[i].ext[2];
				cmd_putstr0(cons,str);
			}
		}
	}
	cmd_newline(cons);
}

void cmd_hlt(CONSOLE* cons,int* fat){
	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	FILEINFO* finfo = file_search("HLT.HRB",(FILEINFO*)(ADR_DISKIMG + 0x002600),224);
	struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*)(ADR_GDT);
	char* p;

	if(finfo != NULL)
	{
		p = (char *)memory_alloc_4kb(memManager, finfo->size); 

		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00)); 
		set_segmdesc(gdt + 1003, finfo->size - 1, (int)(p), AR_CODE32_ER);
		farcall(0, 1003*8);

		memory_free_4kb(memManager,(int)(p),finfo->size);
	}
	else
	{
		cmd_newline(cons);
		putstr_sht(cons->sht, 8, cons->cur_y, color_ffffff, color_000000, "File not found!", 17);
		cmd_newline(cons);
	}
	cmd_newline(cons);
}

void cmd_type(CONSOLE* cons,int* fat,char* command){
	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	FILEINFO* finfo = file_search(command + 5,(FILEINFO*)(ADR_DISKIMG + 0x002600),224);
	char* p;
	int i;
	if(finfo != NULL)
	{
		p = (char *)memory_alloc_4kb(memManager, finfo->size);
		file_loadfile(finfo->clustno, finfo->size, p, fat, (char *)(ADR_DISKIMG + 0x003e00));
		cmd_putstr1(cons,p,finfo->size);
		memory_free_4kb(memManager,(int)(p),finfo->size);
	}
	else
	{
		cmd_putstr0(cons,"File not found!\n");
	}
	cmd_newline(cons);
}

void cmd_exit(CONSOLE* cons,int* fat){
	MEMMAN* memManager = (MEMMAN*) MEM_MANAGER_ADDR;
	TASK* task = query_now_task();
	SHEETCONTROLL* shtctl = (SHEETCONTROLL*) *((int*) 0x0fe4);
	FIFO32* fifo = (FIFO32*) *((int*) ADR_CONSOLE);

	if(cons->sht != NULL){
		cancel_timer(cons->timer);
	}

	memory_free_4kb(memManager,(int)(fat),4 * 2880);
	io_cli();

	if(cons->sht != NULL){
		put_buffer_fifo32(fifo,cons->sht - shtctl->sheets0 + 768);	
	}else{
		put_buffer_fifo32(fifo,task - taskctl->tasks0 + 1024);	
	}

	io_sti();
	while(1){
		sleep_task(task);
	}
}

void cmd_start(CONSOLE* cons,char* command,int memtotal){
	SHEETCONTROLL* shtctl = (SHEETCONTROLL*) *((int*) 0xfe4);
	SHEET* sht = open_console(shtctl,memtotal);
	FIFO32* fifo = &sht->task->fifo;

	sheet_slided(sht,DEFAULT_CONSOLE_X,DEFAULT_CONSOLE_Y);
	sheet_updown(sht,shtctl->top);

	int i;
	for(i = 6;command[i] != '\0'; ++i){
		put_buffer_fifo32(fifo,command[i] + 256);
	}
	put_buffer_fifo32(fifo,10 + 256);
	cmd_newline(cons);
}

void cmd_ncst(CONSOLE* cons,char* command,int memtotal){
	TASK* task = open_constask(NULL,memtotal);
	FIFO32* fifo = &task->fifo;
	int i;
	for(i = 5;command[i] != '\0'; ++i){
		put_buffer_fifo32(fifo,command[i] + 256);
	}
	put_buffer_fifo32(fifo,10 + 256);
	cmd_newline(cons);
}

void cmd_langmode(CONSOLE* cons,char* command){
	TASK* task = query_now_task();
	unsigned char mode = command[9] - '0';
	if(mode <= 2){
		task->language_mode = mode;
	}else{
		cmd_putstr0(cons,"language mode number error!\n");
	}
	cmd_newline(cons);
}

int compare_str(unsigned char* first,unsigned char* second){
	if(first == NULL || second == NULL){
		return -1;
	}
	char* p =first;
	char* q =second;
	while(*p != '\0' && *q !='\0'){
		if(*p != *q){
			return -1;
		}
		++p;
		++q;
	}
	if( (*p != '\0' && *q == '\0') || (*p == '\0' && *q !='\0') ){
		return -1;
	}
	return 0;
}

int compare_str_n(const unsigned char* first,const unsigned char* second,unsigned int n){
	if(first == NULL || second == NULL){
		return -1;
	}
	char* p =first;
	char* q =second;
	int i;
	for(i = 0;i<n;++i){
		if(p[i] != q[i]){
			return -1;
		}
		if( (p[i] != '\0' && q[i] == '\0') || (p[i] == '\0' && q[i] !='\0') ){
			return -1;
		}
	}
	return 0;
}

int str_len(const char* s){
	if(s == NULL)
		return 0;
	
	int count = 0;
	while(*s != '\0'){
		++s;
		++count;
	}
	return count;
}


int* hrb_api(int edi,int esi,int ebp,int esp,int ebx,int edx,int ecx,int eax){
	SHEET* sht;
	TASK* task = query_now_task();
	CONSOLE* cons = task->cons;
	int ds_base = task->ds_base;
	char str[128];

	FILEINFO* finfo;
	FILEHANDLE* fhandle;
	SHEETCONTROLL* shtctl =  (SHEETCONTROLL*) *((int*) 0x0fe4);
	MEMMAN* memManager = (MEMMAN*)(MEM_MANAGER_ADDR);
	FIFO32* sys_fifo = (FIFO*) *((int*) ADR_CONSOLE);
	
	int* reg = &eax + 1;					
	int i = 0;
	int data = 0;		//sound data
	int idx = 0;

	switch(edx){
		case 1:
			cmd_putchar(cons,eax & 0xff,1);
			break;
		case 2:
			cmd_putstr0(cons,(char*)(ebx + ds_base));
			break;
		case 3:
			cmd_putstr1(cons,(char*)(ebx + ds_base),ecx);
			break;
		case 4:
			return &(task->tss.esp0);
			break;
		case 5:
			sht = sheet_alloc(shtctl);
			sht->task = task;	
			sht->flags |= 0x10; 
			sheet_buffer_set(sht,(char*)(ebx + ds_base),esi,edi,eax);
			create_window_other((char*)(ebx + ds_base), esi, edi, (char*)(ecx + ds_base), 0);
			sheet_slided(sht,(shtctl->xsize - esi) / 2 & (~3), (shtctl->ysize - edi) /2); 
			sheet_updown(sht,shtctl->top);		
			reg[7] = (int)(sht);
			break;
		case 6:
			sht = (SHEET*)(ebx & 0xfffffffe);
			putstr(sht->buffer,sht->bxsize,esi,edi,eax,(char*)(ebp + ds_base));
			if((ebx & 1) == 0){
				sheet_refresh(sht, esi, edi, esi + ecx * 8, edi + 16);
			}
			break;
		case 7:
			sht = (SHEET*)(ebx & 0xfffffffe);
			box(sht->buffer, sht->bxsize, ebp, eax, ecx, esi, edi);
			if((ebx & 1) == 0){
				sheet_refresh(sht, eax, ecx, esi + 1, edi + 1);
			}
			break;
		case 8:
			init_memory_manager((MEMMAN*)(ebx + ds_base));
			ecx &= 0xfffffff0;//Align
			memory_manager_free((MEMMAN*)(ebx + ds_base), eax, ecx);
			break;
		case 9:
			ecx	= (ecx + 0x0f) & 0xfffffff0;
			reg[7] = memory_manager_alloc((MEMMAN*)(ebx + ds_base),ecx);
			break;
		case 10:
			ecx = (ecx + 0x0f) & 0xfffffff0;
			memory_manager_free((MEMMAN*)(ebx + ds_base),eax,ecx);
			break;
		case 11:
			sht = (SHEET*)(ebx & 0xfffffffe);
			sht->buffer[sht->bxsize * edi + esi] = eax;
			if((ebx & 1) == 0){
				sheet_refresh(sht,esi,edi,esi+1,edi+1);		
			}
			break;
		case 12:
			sht = (SHEET*)(ebx);
			sheet_refresh(sht,eax,ecx,esi,edi);
			break;

		case 13:
			sht = (SHEET*)(ebx & 0xfffffffe);
			hrb_api_drawline(sht,eax,ecx,esi,edi,ebp);
			if((ebx & 1) == 0){
				if(eax > esi){
					i = eax;
					eax = esi;
					esi = i;
				}
				if(ecx > edi){
					i = ecx;
					ecx = edi;
					edi = i;
				}
				sheet_refresh(sht,eax,ecx,esi+1,edi+1);
			}
			break;
		case 14:
			//关闭窗口
			sheet_free((SHEET*)(ebx));
			break;
		case 15:
			hrb_api_keyInput(cons,task,sys_fifo,shtctl,eax,reg);
			break;
		case 16:
			reg[7] = (int)timer_alloc();		
			((TIMER*)(reg[7]))->isCancel = 1;//allow cancel
			break;
		case 17:
			init_timer((TIMER *) ebx, &task->fifo, eax+256);	
			break;
		case 18:
			settime_timer((TIMER*)ebx,eax);
			break;
		case 19:
			timer_free((TIMER*)(ebx));
			break;
		case 20:
			if(eax == 0){
				data = io_in8(0x61);
				io_out8(0x61,data & 0x0d);
			}else{
				data = 1193180000 / eax;//1.19318mhz
				io_out8(0x43,0xb6);
				io_out8(0x42,data & 0xff);
				io_out8(0x42,data >> 8);
				data = io_in8(0x61);
				io_out8(0x61,(data | 0x03) & 0x0f);
			}
			break;
		case 21:
			//file open
			for(idx = 0;idx < 8;++idx){
				if(task->fhandle[idx].buf == NULL){
					break;
				}
			}
			fhandle = &task->fhandle[idx];
			reg[7] = 0;
			if(idx < 8){
				finfo = file_search((char*)(ebx + ds_base), (FILEINFO*)(ADR_DISKIMG + 0x002600),224);
				if(finfo != NULL){
					reg[7] = (int)(fhandle);
					fhandle->buf = (char*)memory_alloc_4kb(memManager,finfo->size);
					fhandle->size = finfo->size;
					fhandle->pos = 0;
					fhandle->buf = file_loadfile_overload(finfo->clustno,&fhandle->size,task->fat);
				}
			}
			break;

		case 22:
			//file close
			fhandle = (FILEHANDLE*)(eax);
			memory_free_4kb(memManager,(int)(fhandle->buf),fhandle->size);
			fhandle->buf = NULL;
			break;
		
		case 23:
			//file seek
			fhandle = (FILEHANDLE*)(eax);
			if(ecx == 0){
				fhandle->pos = ebx;
			}else if(ecx == 1){
				fhandle->pos += ebx;
			}else if(ecx == 2){
				fhandle->pos = fhandle->size + ebx;
			}

			if(fhandle->pos < 0){
				fhandle->pos = 0;
			}
			if(fhandle->pos > fhandle->size){
				fhandle->pos = fhandle->size;
			}
			break;

		case 24:
			//file size
			fhandle = (FILEHANDLE*)(eax);
			if(ecx == 0){
				reg[7] = fhandle->size;	
			}else if(ecx == 1){
				reg[7] = fhandle->pos;
			}else if(ecx == 2){
				reg[7] = fhandle->pos - fhandle->size;
			}
			break;

		case 25:
			//file load
			fhandle = (FILEHANDLE*)(eax);
			for(;idx < ecx;++idx){
				if(fhandle->pos == fhandle->size){
					break;
				}
				*((char*) ebx + ds_base + idx) = fhandle->buf[fhandle->pos];
				++fhandle->pos;
			}
			reg[7] = idx;
			break;

		case 26:
			//command
			idx = 0;
			while(1){
				*((char*) ebx + ds_base + idx) = task->command[idx];
				if(task->command[idx] == '\0' || task->command[idx] == NULL){
					break;
				}
				if(idx >= ecx){
					break;
				}
				++idx;
			}
			reg[7] = idx;
			break;
		case 27:
			reg[7] = task->language_mode;
			break;
		default:
			break;
	}
	return NULL;
}




int inthandler0d(int* esp){
	TASK* task = query_now_task();
	CONSOLE* cons = task->cons;	
	char s[128];
	cmd_putstr0(cons,"\n INT 0D : \n General Protected Exception\n");
	sprintf(s,"EIP : %08X\n",esp[11]);
	cmd_putstr0(cons,s);
	return &(task->tss.esp0);
}

int inthandler0c(int* esp){
	TASK* task = query_now_task();
	CONSOLE* cons = task->cons; 
	char s[128];
	cmd_putstr0(cons,"\n INT 0C : \n Stack Exception\n");
	sprintf(s,"EIP : %08X\n",esp[11]);	
	cmd_putstr0(cons,s);
	return &(task->tss.esp0)
}

void hrb_api_drawline(SHEET* sht,int x0,int y0,int x1,int y1,int color){
	int i;
	int x,y,len,dx,dy;

	dx = x1 - x0;
	dy = y1 - y0;
	x = x0 << 10;
	y = y0 << 10;
	
	if(dx < 0){
		dx = -dx;
	}

	if(dy < 0){
		dy = -dy;
	}

	if(dx >= dy){
		len = dx + 1;

		if(x0 > x1){
			dx = -1024;
		}else{
			dx = 1024;
		}

		if(y0 <= y1){
			dy = ((y1 - y0 + 1) << 10) / len;
		}else{
			dy = ((y1 - y0 - 1) << 10) / len;
		}

	}else{
		len = dy + 1;

		if(y0 > y1){
			dy = -1024;
		}else{
			dy = 1024;
		}

		if(x0 <= x1){
			dx = ((x1 - x0 + 1) << 10) / len;
		}else{
			dx = ((x1 - x0 - 1) << 10) / len;
		}
	}

	//draw line
	for(i = 0;i < len;++i){
		sht->buffer[(y>>10) * sht->bxsize + (x>>10)] = color;
		x += dx;
		y += dy;
	}

}


void hrb_api_keyInput(CONSOLE* cons,TASK* task,FIFO32* sys_fifo,SHEETCONTROLL* shtctl, int eax,int* reg){//api窗口输入功能
	int i;
	for (;;)
	{
		io_cli();
		if (buffer_status_fifo32(&task->fifo) == 0)
		{
			if (eax != 0)
			{
				sleep_task(task); 
			}
			else
			{
				io_sti();
				reg[7] = -1;
				return;
			}
		}
		i = get_buffer_fifo32(&task->fifo);
		io_sti();
		if (i <= 1)
		{ 
			init_timer(cons->timer, &task->fifo, 1); 
			settime_timer(cons->timer, 50);
		}
		if (i == 2)
		{ 
			cons->cur_color = color_ffffff;
		}
		if (i == 3)
		{
			cons->cur_color = -1;
		}
		if(i == 4)
		{//close console
			cancel_timer(cons->timer);
			io_cli();
			put_buffer_fifo32(sys_fifo,cons->sht - shtctl->sheets0 +  2024);//2024 - 2079
			cons->sht = NULL;
			io_sti();
		}
		if (i >= 256)
		{
			reg[7] = i - 256;
			return;
		}
	}
}


void savePrevCommand(char* nowcmd,char* prevcmd){
	char* q =prevcmd;
	int len = str_len(q);
	int i;
	for(i = 0;i < len;++i){
		prevcmd[i] = '\0';
	}
	char* p = nowcmd;
	for(i = 0;p[i] !='\0';++i){
		q[i] = p[i];
	}
}

void str_cpy(char* first,char* second){
	if(second == NULL && first == NULL){
		return;
	}
	char* p = first;
	char* q = second;
	int len0 = strlen(p);
	int len1 = strlen(q);

	if(len0 < len1){
		return;
	}

	int idx;
	for(idx = 0;idx < len0;++idx){
		p[idx] = '\0';
	}
	while(*q != '\0'){
		*p = *q;
		++q;
		++p;
	}
	*p = '\0';
}