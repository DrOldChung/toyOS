#include "header/bootpack.h"
#include "header/file.h"
#include "header/fifo.h"
#include "header/Timer.h"
#include "header/multitask.h"
#include "header/color.h"

void init_color(){
	//RGB 256
	static unsigned char color_tab[16*3]={
		0x00,0x00,0x00,
		0x00,0xff,0xff,	
		0x00,0x00,0x84,
		0xff,0x00,0x00,
		0xff,0xff,0xff,
		0x84,0x00,0x84,
		0x00,0xff,0x00,
		0xc6,0xc6,0xc6,
		0x00,0x84,0x84,
		0xff,0xff,0x00,
		0x84,0x00,0x00,
		0x84,0x84,0x84,
		0x00,0x00,0xff,
		0x00,0x84,0x00,
		0xff,0x00,0xff,
		0x84,0x84,0x00,
	};
	static unsigned char color_tab256[216*3];
	int r,g,b;
	set_color(0,15,color_tab);
	for(b=0;b<6;++b){
		for(g=0;g<6;++g){
			for(r=0;r<6;++r){
				int RGBformula = (r + g * 6 + b * 36) * 3;
				color_tab256[RGBformula + 0] = r * 51;
				color_tab256[RGBformula + 1] = g * 51;
				color_tab256[RGBformula + 2] = b * 51;
			}
		}
	}
	set_color(16,231,color_tab256);
}

void set_color(int start,int end,unsigned char* rgb){
	int i=start;
	int eflags = io_load_eflags();
	io_cli();
	io_out8(0x03c8,start);
	for(i=start;i<=end;++i){
		io_out8(0x03c9,rgb[0]/4 );
		io_out8(0x03c9,rgb[1]/4 );
		io_out8(0x03c9,rgb[2]/4 );
		rgb += 3;
	}
	io_store_eflags(eflags);
}
void init_mouse_cursor(char* mouse, char bgcolor){
	const static char cursor[16][16]={
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};

	int x,y;
	for(y=0;y<16;++y){
		for(x=0;x<16;++x){
			if(cursor[y][x] == '*')
				mouse[y*16+x]= color_000000;
			else if(cursor[y][x]=='O')
				mouse[y*16+x]=color_ffffff; 
			else if(cursor[y][x]=='.'){
				mouse[y*16+x]= bgcolor;
			}
		}
	}

}



//Menu
void background(unsigned char* vram,int x,int y,char color){
	box(vram, x, color,  0, 0, x ,y);
}





//Shape 
void box(unsigned char* vram, int xsize,unsigned char color,int x0,int y0,int x1,int y1){
	int x,y;//坐标设置 ,x代表横向，y代表纵向
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = color;
	}
}

void white_square(unsigned char* vram,char color,int offset){
	box(vram,320,color,50+offset,10+offset,250+offset,10+offset);

	box(vram,320,color,50+offset,10+offset,50+offset,150+offset);

	box(vram,320,color,250+offset,10+offset,250+offset,150+offset);

	box(vram,320,color,50+offset,150+offset,250+offset,150+offset);
}


void taskbar(unsigned char* vram,int xsize,int ysize){//任务进度条
	
	//背景颜色
	 //box(vram,xsize,color_008484, 0 , 0 , xsize -1 , ysize-29);
	 //box(vram,xsize,color_c6c6c6, 0 , ysize - 28 , xsize -1 , ysize-28);

	 //进度条绘制
	 box(vram,xsize,color_ffffff, 0 , ysize - 27 , xsize -1 , ysize-29);
	 box(vram,xsize,color_c6c6c6, 0 , ysize -26 , xsize -1 , ysize-1);
	 //
	 box(vram,xsize,color_ffffff, 3 , ysize -  24 , 59 , ysize - 24);
	 box(vram,xsize,color_ffffff, 2 , ysize -  24 , 2 , ysize - 4);
	 box(vram,xsize,color_848484, 3 , ysize -  4 , 59 , ysize - 4);
	 box(vram,xsize,color_848484, 59 , ysize -23 , 59 , ysize - 5);
	 box(vram,xsize,color_000000, 2 , ysize - 3 , 59 , ysize - 3);
	 box(vram,xsize,color_000000, 60 , ysize - 24 , 60 , ysize - 3);
	 //
	 box(vram,xsize,color_848484, xsize - 47 , ysize - 24 , xsize - 4 , ysize - 24);
	 box(vram,xsize,color_848484, xsize - 47 , ysize - 24 , xsize - 47 , ysize- 4);
	 box(vram,xsize,color_ffffff, xsize - 47 , ysize - 3 ,  xsize - 4 , ysize - 3);
	 box(vram,xsize,color_ffffff, xsize - 3, ysize - 24 , xsize - 3 , ysize - 3);
}













//print ASCII编码
void putfont(char* vram,int xsize,int x,int y,char color,unsigned char* font){
	int i;
	char* p,data;
	for(i=0;i<16;++i){
		p = vram + (y + i) * xsize + x;//地址偏移
		data=font[i];
		if((data & 0x80)!=0) 
			p[0] = color;

		if((data & 0x40)!=0) 
			p[1] = color;
		
		if((data & 0x20)!=0) 
			p[2] = color;

		if((data & 0x10)!=0) 
			p[3] = color;

		if((data & 0x08)!=0) 
			p[4] = color;
	
		if((data & 0x04)!=0) 
			p[5] = color;

		if((data & 0x02)!=0) 
			p[6] = color;
		
		if((data & 0x01)!=0) 
			p[7] = color;
		
	}
}
void putstr(char* vram,int xsize,int x,int y,char color,unsigned char* str){
	//字体样式
	extern char hankaku[4096];
	TASK* task = query_now_task();
	char* nihongo = (char*) *((int*) 0x0fe8);
	char* font;
	//k:区号 t:点号
	int k,t;

	//语言模式切换
	if(task->language_mode == 0){//英文
		for(;*str!='\0' || *str != 0x00;++str){
			putfont(vram,xsize,x,y,color, hankaku + *str * 16);
			x += 8;
		}
	}
	else if(task->language_mode == 1){//日文
		for(;*str !='\0' || *str != 0x00;++str)
		{
			if(task->langbyte == 0)
			{
				if((*str >= 0x81 && *str <= 0x9f) || (*str >= 0xe0 && *str <= 0xfc)){
					task->langbyte = *str;
				}else{
					putfont(vram,xsize,x,y,color,nihongo + *str * 16);
				}
			}
			else
			{
				if(task->langbyte >= 0x81 && task->langbyte <= 0x9f){
					k = (task->langbyte - 0x81) * 2;
				}else{
					k = (task->langbyte - 0xe0) * 2 + 62;
				}

				if(*str >= 0x40 && *str <= 0x7e){
					t = *str - 0x40;
				}else if(*str >= 0x80 && *str <= 0x9e){
					t = *str - 0x80 + 63;
				}else{
					t = *str - 0x9f;
					++k;
				}
				task->langbyte = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont(vram,xsize,x - 8,y,color,font);
				putfont(vram,xsize,x,y,color,font + 16);
			}
			x += 8;
		}
	}
	else if(task->language_mode == 2){//EUC支持，中日均可用
		for(;*str != '\0' || *str != NULL; ++str)
		{
			if(task->langbyte == 0)
			{
				if(*str >= 0x81 && *str <= 0xfe){
					task->langbyte = *str;
				}else{
					putfont(vram,xsize,x,y,color,nihongo + *str * 16);
				}
			}
			else
			{
				k = task->langbyte - 0xa1;
				t = *str - 0xa1;
				task->langbyte = 0;
				font = nihongo + 256 * 16 + (k * 94 + t) * 32;
				putfont(vram,xsize,x - 8,y,color,font);
				putfont(vram,xsize,x,y,color,font + 16);
			}
			x += 8;
		}
	}
	
}


/*void putstr_sht(SHEET* sht, int x, int y, int color, int bgcolor, char* str, int len){//汇总打印字符串功能 from msheet.h
	box(sht->buffer, sht->bxsize, bgcolor, x, y, x + len * 8 - len, y+15);
	putstr(sht->buffer, sht->bxsize, x, y, color, str);
	sheet_refresh(sht, x, y, x + len * 8, y+16);
}*/


void putblock(char* vram,int v_xsize,int pic_xsize,int pic_ysize,int pic_xpos,int pic_ypos,char* buf,int buf_xsize){
	int x,y;
	for(y=0;y<pic_ysize;++y){
		for(x=0;x<pic_xsize;++x){
			vram[(pic_ypos + y) * v_xsize + (pic_xpos + x)] = buf[y * buf_xsize + x];
		}
	}
}





void init_screen8(char *vram, int x, int y)
{
	box(vram, x, color_008484,  0,     0,      x -  1, y - 29);
	box(vram, x, color_c6c6c6,  0,     y - 28, x -  1, y - 28);
	box(vram, x, color_ffffff,  0,     y - 27, x -  1, y - 27);
	box(vram, x, color_c6c6c6,  0,     y - 26, x -  1, y -  1);

	box(vram, x, color_ffffff,  3,     y - 24, 59,     y - 24);
	box(vram, x, color_ffffff,  2,     y - 24,  2,     y -  4);
	box(vram, x, color_848484,  3,     y -  4, 59,     y -  4);
	box(vram, x, color_848484, 59,     y - 23, 59,     y -  5);
	box(vram, x, color_000000,  2,     y -  3, 59,     y -  3);
	box(vram, x, color_000000, 60,     y - 24, 60,     y -  3);

	box(vram, x, color_848484, x - 47, y - 24, x -  4, y - 24);
	box(vram, x, color_848484, x - 47, y - 23, x - 47, y -  4);
	box(vram, x, color_ffffff, x - 47, y -  3, x -  4, y -  3);
	box(vram, x, color_ffffff, x -  3, y - 24, x -  3, y -  3);
}