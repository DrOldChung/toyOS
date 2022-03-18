#include <stdio.h>
#include "header\bootpack.h"
#include "header\color.h"
#include "header\kbcontroll.h" 
#include "header\mmemory.h"
#include "header\msheet.h"
#include "header\Timer.h"		
#include "header\file.h"		
#include "header\multitask.h"  
#include "header\fifo.h"		
#include "header\console.h"
/*sequence :  bootpack -> file -> fifo -> timer -> multitask */

#define	TIMERNUM	490		
#define BGCOLOR		color_008484	
#define STRMAXLEN   256				
			
//fifo.c
extern FIFO32* keyfifo;		
extern FIFO32* mousefifo;
extern void init_fifo32_overload(FIFO32* fifo, int size, int* buf,TASK* newtask);

//kbcontrool.c
extern mouse_desc mDesc;

//Timer.c 
extern TIMERCTL timerctl;	 

//kbcontroll.c
extern void init_keyboard_controll(FIFO32* fifo, int data);

//mouse.c
extern void enable_mouse(FIFO32* fifo, int data, mouse_desc* mdesc);

//mulitask.c
extern TASKCTL* taskctl;//控制器结构体
extern TASK* init_task(MEMMAN* memman);
extern TASK* alloc_task();
extern void  run_task(TASK* task);

//console.c 
extern void task_cmd_main(SHEET* sheet);
extern void console_task(struct SHEET *sheet);
extern void cmd_putstr0(CONSOLE* cons,const char* s);
extern void cmd_putstr1(CONSOLE* cons,const char* s,int len); 

//window.c 
extern void putstr_sht(SHEET* sht, int x, int y, int color, int bgcolor, char* str, int len);
extern void putstr_sht_another(SHEET* sht, int x, int y, int color, int bgcolor, char c, int len);
extern void make_textbox(SHEET* sht,int x0,int y0,int sx,int sy,int customColor);
extern void window_mouseMoveMode(SHEET* sht,SHEETCONTROLL* shtctl,int* mousemodeX,int* mouseModeY,int* mx,int* my);
extern void keywin_on(SHEET* keyWindow);
extern void keywin_off(SHEET* keyWindow);
extern SHEET* open_console(SHEETCONTROLL* shtctl,unsigned int memtotal);
extern void close_console(SHEET* sht);
extern void close_constask(TASK* task);


//KEY
void editKeytableData();
static char keytable[0x54] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '@', '[', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', ':', 0,   0,   ']', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.'
};

static char combination_keytable[0x80] = {
		0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '`', '{', 0,   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', '+', '*', 0,   0,   '}', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.', 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
		0,   0,   0,   '_', 0,   0,   0,   0,   0,   0,   0,   0,   0,   '|', 0,   0
};


void HariMain()
{
	BOOTINFO* binfo = (BOOTINFO*)(ADR_BOOTINFO);
	MEMMAN* memManger = (MEMMAN*)(MEM_MANAGER_ADDR); //ADDR: 0x003c0000
	struct SEGMENT_DESCRIPTOR* gdt = (struct SEGMENT_DESCRIPTOR*) ADR_GDT;

	SHEETCONTROLL* shtctl;
	SHEET* shtBack;
	SHEET* shtMouse;
	SHEET* shtWindow;
	SHEET* shtCmd;
	SHEET* key_win;	
	SHEET* sht;		//temp

	TSS32 tss_a,tss_b;
	tss_a.ldtr = 0;
	tss_a.iomap = 0x40000000;
	tss_b.ldtr = 0;
	tss_b.iomap = 0x40000000;

	TASK* task_main;		//主要进程
	TASK* task_cmd;			//控制台进程

	//Timer
	TIMER* timer_ts;//task switch	

	int count = 0;										
	
	//font
	int* fat;
	unsigned char* nihongo;
	FILEINFO* finfo;
	extern char hankaku[4096];
	
	//fifo setting
	FIFO32 uniquefifo;											
	FIFO32 keycmdfifo;											
	int fifobuf[128];		
	int keycmdbuf[32];		
	int* cons_fifo[MAXCONSOLES];
	
	

	//window mouse 
	int xsize,ysize;
	int mouseX,mouseY;
	int new_mx = -1,new_my = 0;
	int new_wx = 0x7fffffff;
	int new_wy = 0;

	char bgAndmouseColor = color_008484;
	char str[STRMAXLEN];
   	char mouseCursor[256];

	//buffer setting
	unsigned char mousebuf[128],keybuf[32]; 
	unsigned char* bufBack,bufMouseLayer[256];
	unsigned char* bufCmdLayer;

	unsigned int memCapacity;
	int freeCapacity;

	//Init
	init_gdtidt();						
	init_pic();							
	io_sti();						

	init_fifo32_overload(&uniquefifo, 128, fifobuf, NULL);	
	
	init_pit();							
	init_keyboard_controll(&uniquefifo, 256);
	enable_mouse(&uniquefifo, 512, &mDesc);


	io_out8(PIC0_IMR,0xf8);				//allow pit pitc keyboard
	io_out8(PIC1_IMR,0xef);				//allow mouse 	
	init_fifo32_overload(&keycmdfifo, 32, keycmdbuf,0);

	
	//Memory
	memCapacity = memcheck(0x00400000, 0xbfffffff); 
	init_memory_manager(memManger);
	memory_manager_free(memManger,0x00001000, 0x0009e000);
	memory_manager_free(memManger,0x00400000,memCapacity - 0x00400000);

	freeCapacity = memory_manager_total(memManger) / 1024;
	memCapacity /= (1024*1024);

	//Color
	init_color();

	
	//Sheet
	shtctl = init_sheetControll(memManger,binfo->vram,binfo->screenX,binfo->screenY);
	
	task_main = init_task(memManger);
	uniquefifo.task = task_main;
	run_task_overload(task_main,1,2);

	*((int*) 0x0fe4) = (int)(shtctl);						
	task_main->language_mode = 0;							//default eng

	/*Background*/
	shtBack = sheet_alloc(shtctl);
	bufBack =  (unsigned char*) memory_alloc_4kb(memManger, binfo->screenX * binfo->screenY);
	sheet_buffer_set(shtBack,bufBack,binfo->screenX,binfo->screenY,-1);
	background(bufBack,binfo->screenX,binfo->screenY,BGCOLOR);
	taskbar(bufBack,binfo->screenX,binfo->screenY);

	/*Console*/
	key_win = open_console(shtctl,memCapacity);

	/*Mouse*/
	shtMouse = sheet_alloc(shtctl);
	sheet_buffer_set(shtMouse,bufMouseLayer,16,16,AERO);
	init_mouse_cursor(bufMouseLayer,AERO);				
	mouseX = (binfo->screenX - 16) / 2;					
	mouseY = (binfo->screenY - 28 - 16) / 2;

	/*Position*/
	sheet_slided(shtBack,0,0);	
	sheet_slided(key_win, DEFAULT_CONSOLE_X, DEFAULT_CONSOLE_Y);//32,4
	sheet_slided(shtMouse,mouseX,mouseY);
	
	
	/*Display priority*/
	sheet_updown(shtBack,0);
	sheet_updown(key_win,1);
	sheet_updown(shtMouse,2);
	keywin_on(key_win);

	//Refresh window
	sheet_refresh(shtBack,0,0,binfo->screenX,binfo->screenY); 


	int keybufData = 0;
	int timerbufData = 0;
	int allbufData = 0;
	int stridx = 0;

	int key_to = 0;
	int key_shift = 0;	
	int key_ctrl = 0;	
	int key_leds = (binfo->leds >> 4) & 7;	//Special key status
	int keycmd_wait = -1;	

	editKeytableData();
	
	//Key status Setting
	put_buffer_fifo32(&keycmdfifo, KEYCMD_LED);
	put_buffer_fifo32(&keycmdfifo,key_leds);
	

	int mmx = -1,mmy = -1;
	int mmx_tmp = 0;
	
	*((int*)ADR_CONSOLE) = (int)&uniquefifo;

	/*Load font*/
	nihongo = (unsigned char*) memory_alloc_4kb(memManger,16 * 256 + 32 * 94 * 47);
	fat = (int*) memory_alloc_4kb(memManger,4*2880);
	file_readFAT(fat,(unsigned char*)(ADR_DISKIMG + 0x000200));
	finfo = file_search("nihongo.fnt",(FILEINFO*)(ADR_DISKIMG + 0x002600),224);
	if(finfo != NULL){
		int i = finfo->size;
		nihongo = file_loadfile_overload(finfo->clustno,&i,fat);
	}else{
		int i;
		nihongo = (unsigned char*)memory_alloc_4kb(memManger, 16 * 256 + 32 * 94 * 47);
		for(i = 0; i < 16 * 256;++i){
			nihongo[i] = hankaku[i];
		}
		for(i = 16*256; i < 16 * 256 + 32 * 94 * 47;++i){
			nihongo[i] = 0xff;
		}
	}
	*((int*)0x0fe8) = (int)(nihongo);
	memory_free_4kb(memManger,(int)fat,4*2880);


	while(1)
	{		
		if(buffer_status_fifo32(&keycmdfifo) > 0 && keycmd_wait < 0){
			keycmd_wait = get_buffer_fifo32(&keycmdfifo);
			wait_keyboard_controll_sendReady();
			io_out8(PORT_KEYDAT,keycmd_wait);
		}
		io_cli();
		int allStatus = buffer_status_fifo32(&uniquefifo);
		if(allStatus == 0){	
			if(new_mx >= 0){
				io_sti();
				sheet_slided(shtMouse,new_mx,new_my);
				new_mx = -1;
			}else if(new_wx != 0x7fffffff){
				io_sti();
				sheet_slided(sht,new_wx,new_wy);
				new_wx = 0x7fffffff;
			}else{
				sleep_task(task_main);
				io_sti();
			}
		}
		else
		{
			allbufData = get_buffer_fifo32(&uniquefifo);
			io_sti();
			if(key_win != NULL && key_win->flags == 0){
				if(shtctl->top == 1 ){
					key_win = NULL;
				}else{
					key_win = shtctl->sheets[shtctl->top - 1];	
					keywin_on(key_win);
				}
			}		
			if(allbufData >= 256 && allbufData<= 511){ 
				if(allbufData < 0x80 + 256){	
					if(key_shift == 0){//The shift key is not pressed
					    str[0] = keytable[allbufData - 256];
						if(allbufData == 0x48 + 256 && allbufData != 0x0b + 256){//Up
							str[0] = '\0';
							put_buffer_fifo32(&key_win->task->fifo,4 + 256);
						}
					}else{
						str[0] = combination_keytable[allbufData - 256];
					}
				}else{
					str[0] = '\0';
				}
				
				if(str[0] >= 'A' && str[0] <= 'Z'){
					if((key_leds & 4) == 0 && key_shift == 0 || (key_leds & 4) != 0 && key_shift !=0 ){
						str[0] += 0x20;			//Uppercase to lowercase
					}
				}

				if(str[0] != '\0' && key_win != NULL){
					put_buffer_fifo32(&key_win->task->fifo,str[0] + 256);
				}

				/*Backspace*/
				if(allbufData == 0x0e + 256 && key_win !=  NULL){
					put_buffer_fifo32(&key_win->task->fifo, 8 + 256);
				}
				

				/*Enter*/
				if(allbufData == 0x1c + 256 && key_win != NULL){
					if(key_win != shtWindow){
						put_buffer_fifo32(&key_win->task->fifo,10 + 256);		
					}
				}

				/*Tab*/
				if(allbufData == 0x0f + 256 && key_win != NULL){	
					keywin_off(key_win);
					int h = key_win->height - 1;
					if(h == 0){
						h = shtctl->top - 1;
					}			
					key_win = shtctl->sheets[h];
					keywin_on(key_win);
				}

				
				switch(allbufData - 256){
					case 0x2a:
						key_shift |= 1;		//press and hold the left shift
						break;
					case 0x36:
						key_shift |= 2;		//press and hold the right shift
						break;
					case 0xaa:
						key_shift &= (~1);	//left shift release
						break;
					case 0xb6:
						key_shift &= (~2);	//right shift release
						break;
					default:
						break;
				}

				
				switch(allbufData - 256){
					case 0x1d:
						key_ctrl |= 1;		//press and hold the ctrl
						break;
					case 0x9d:
						key_ctrl &= (~1);	//release
					default:
						break;
				}

				//Switch window
				if(allbufData == 0x57 + 256 && shtctl->top > 2 ){
					sheet_updown(shtctl->sheets[1],shtctl->top - 1);
				}

				/*CapsLock*/
				if(allbufData == 0x3a + 256){
					key_leds ^= 4;	
					put_buffer_fifo32(&keycmdfifo,KEYCMD_LED);
					put_buffer_fifo32(&keycmdfifo,key_leds);
				}

				/*NumLock*/
				if(allbufData == 0x45 + 256){
					key_leds ^= 2;
					put_buffer_fifo32(&keycmdfifo,KEYCMD_LED);
					put_buffer_fifo32(&keycmdfifo,key_leds);
				}

				/*ScrollLock*/
				if(allbufData == 0x46 + 256){
					key_leds ^= 1;
					put_buffer_fifo32(&keycmdfifo,KEYCMD_LED);
					put_buffer_fifo32(&keycmdfifo,key_leds);
				}

				/*Exit*/
				if(allbufData == 0x01 + 256 && key_shift !=0){
					TASK* nowtask = key_win->task;
					if(nowtask != NULL && nowtask->tss.ss0 != 0 ){
						cmd_putstr0(nowtask->cons,"\nTerminate(key):\n");
						io_cli();		
						nowtask->tss.eax = (int)&(nowtask->tss.esp0);
						nowtask->tss.eip = (int)asm_end_app;
						io_sti();
						run_task_overload(nowtask,-1,0);
					}
				}

				/*Open Console*/
				if(allbufData == 0x3c + 256 && key_shift != 0){
					if(key_win != NULL){
						keywin_off(key_win);
					}
					key_win = open_console(shtctl,memCapacity);
					
					sheet_slided(key_win, DEFAULT_CONSOLE_X, DEFAULT_CONSOLE_Y);
					sheet_updown(key_win,shtctl->top);

					keywin_on(key_win);
				}

				if(allbufData == 0xfa + 256){
					keycmd_wait = -1;
				}
				if(allbufData == 0xfe + 256){
					wait_keyboard_controll_sendReady();
					io_out8(PORT_KEYDAT,keycmd_wait);
				}
			}			
			else if(allbufData >= 512 && allbufData <= 767){//Mouse
				int signal = mouse_decode(&mDesc,allbufData - 512); 
				if(signal != 0){
					mouseX+=mDesc.x;
					mouseY+=mDesc.y;
					if(mouseX<0){
						mouseX = 0;
					}
					if(mouseY<0){
						mouseY = 0;
					}
					if(mouseX > binfo->screenX-1){
						mouseX = binfo->screenX - 1;
					}
					if(mouseY > binfo->screenY - 1){
						mouseY = binfo->screenY - 1;
					}
					sheet_slided(shtMouse,mouseX,mouseY);
	
					new_mx = mouseX;
					new_my = mouseY;

					//click mouse button
					if((mDesc.button & 0x01) != 0){
						int x,y,j;
						if(mmx < 0){
							for(j = shtctl->top - 1; j > 0; j--){
								sht = shtctl->sheets[j];
								x = mouseX - sht->vramX0;
								y = mouseY - sht->vramY0;
								if(x >= 0 && x < sht->bxsize && y >= 0  && y < sht->bysize){
									if(sht->buffer[y * sht->bxsize + x] != sht->colorInv){
										sheet_updown(sht, shtctl->top - 1);
										
										/*switch window*/
										if(sht != key_win){
											keywin_off(key_win);
											key_win = sht; 
											keywin_on(key_win);
										}

										/*move mode*/
										if(x >= 3 && x < sht->bxsize - 3 && y >= 3 && y < 21){
											mmx = mouseX;
											mmy = mouseY;
											mmx_tmp = sht->vramX0;
											new_wy = sht->vramY0;
										}

										//press button 'X'
										if(x >= sht->bxsize -21 && x < sht->bxsize -5 && y >= 5 && y < 19){
											if((sht->flags & 0x10) != 0){
												TASK* nowtask = sht->task;
												cmd_putstr0(nowtask->cons,"\nBreak Program(mouse)\n");
												io_cli();
												nowtask->tss.eax = (int)&(nowtask->tss.esp0);
												nowtask->tss.eip = (int)(asm_end_app);
												io_sti();
												run_task_overload(nowtask,-1,0);
											}
											else{
												TASK* nowtask = sht->task;
												sheet_updown(sht,-1);//hide window
												keywin_off(key_win);
												key_win = shtctl->sheets[shtctl->top - 1];
												keywin_on(key_win);
												io_cli();
												put_buffer_fifo32(&nowtask->fifo,4);
												io_sti();
											}
										}
										break;
									}
								}
							}
						}
						else{
							x = mouseX - mmx;
							y = mouseY - mmy;
							new_wx = (mmx_tmp + x + 2) & ~3;
							new_wy = new_wy + y;
							mmy = mouseY;
						}
					}else{
						//not click
						mmx = -1;
						if(new_wx != 0x7fffffff){
							sheet_slided(sht,new_wx,new_wy);
							new_wx = 0x7fffffff;
						}
					}

				}

			}
			else if(allbufData >= 768 && allbufData <= 1023){
				close_console(shtctl->sheets0 + (allbufData - 768));
			}
			else if(allbufData >= 1024 && allbufData <= 2023){
				close_constask(taskctl->tasks0 + (allbufData - 1024));
			}else if(allbufData >= 2024 && allbufData <= 2279){
				//only console
				SHEET* nowsht = shtctl->sheets0 + (allbufData - 2024);
				memory_free_4kb(memManger,(int)nowsht->buffer,256*165);
				sheet_free(nowsht);
			}
		}
	}
}

void editKeytableData(){
	keytable[0x29] = '`';
	keytable[0x1a] = '[';
	keytable[0x1b] = ']';
	keytable[0x28] = '\'';	// ' : 0x39
	combination_keytable[0x27] = ':';
	combination_keytable[0x28] = '\"';
	combination_keytable[0x29] = '~';
}









