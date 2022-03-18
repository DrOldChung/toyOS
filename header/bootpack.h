#ifndef	_BOOTPACK_H
#define	_BOOTPACK_H
#define ADR_BOOTINFO    0x00000ff0 	//启动区起始
#define ADR_CONSOLE		0x0fec		//控制台窗口地址入口
#define MAXCONSOLES		2			//控制台最大数量
#define CURWEIGHT	1				//光标显示宽度

#define DEFAULT_CONSOLE_X		32
#define	DEFAULT_CONSOLE_Y		4

#define KEYCMD_LED		0xed
#define	ADR_DISKIMG		0x00100000 //磁盘镜像起始地址

#endif



//GDT 全局段号记录设定
struct SEGMENT_DESCRIPTOR{
	short limit_low,base_low;
	char base_mid,access_right;
	char limit_high,base_high;
};

struct GATE_DESCRIPTOR{
	short offset_low,selector;
	char dw_count,access_right;
	short offset_high;
};



typedef struct BOOTINF{
	/* data */
	char cyls;//启动区读硬盘终点位置(0x0ff0-0x0fff)
    char leds;//键盘led状态  第4位： ScrollLock状态 0x46   第5位：NumLock状态 0x45 第6位：CapsLock（大写切换）状态 0x3a  模拟器显示的是(0xba)
    char vmode;//显卡多少bit颜色
    char reserve;//
	short screenX,screenY;//分辨率
	char* vram;//显存入口
}BOOTINFO;  

//任务状态段TSS 32位
typedef struct _TASK_STATUS_SEGMENT{
	int backlink, esp0, ss0, esp1, ss1, esp2, ss2, cr3;
	int eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
	int es, cs, ss, ds, fs, gs;
	int ldtr, iomap;
}TSS32;

//naskfunction
void write_mem8(int addr, int data);
void print();
void change(int s,int d);
void io_hlt(void);
void io_cli();
void io_sti();	
void io_stlhlt();
int io_in8(int port);
void io_out8(int port,int data);
int  io_load_eflags();
void io_store_eflags(int eflags);
void asm_inthandler21();
void asm_inthandler27();
void asm_inthandler2c();
void asm_inthandler20();
void asm_inthandler0d();
void asm_inthandler0c();
void asm_cons_putchar();
void asm_end_app();
void asm_hrb_api();

void start_app(int eip,int cs,int esp,int ds,int* tss_esp0);

void set_brightness(int start,int end,unsigned char* rgb,float val);
void setBright(float val);

//display
void init_Screen(char* vram,int* x,int* y);
void init_mouse_cursor(char* mouse, char bgcolor);//鼠标指针
void putfont(char* vram,int xsize,int x,int y,char color,unsigned char* font);//打印字符
void putstr(char* vram,int xsize,int x,int y,char color,unsigned char* str);//字符串
void putblock(char* vram,int v_xsize,int pic_xsize,int pic_ysize,int pic_xpos,int pic_ypos,char* buf,int buf_xsize);

//menu
void background(unsigned char* vram,int x,int y,char color);
//Color
void init_color();
void set_color(int start,int end,unsigned char* rgb);

//Shape 绘制各种图形
void box(unsigned char* vram, int xsize,unsigned char color,int x0,int y0,int x1,int y1);
void white_square(unsigned char* vram,char color,int offset);//空心方型
void taskbar(unsigned char* vram,int xsize,int ysize);//任务进度条
void init_screen8(char *vram, int x, int y);


void create_window(unsigned char* buf,int xsize,int ysize,char* title);//创建窗口
void create_window_other(unsigned char* buf,int xsize,int ysize,char* title, char act);//创建其他窗口样式
void make_title(unsigned char* buf, int xsize, char* title, char act);//标题栏


//任务切换 from naskfunc.nas
void task_swtich4();
void task_swtich3();
void farjmp(int eip,int cs);
void farcall(int eip,int cs);

/*gdt idt*/
void load_tr(int tr);
void load_gdtr(int gdtNO,int addr);
void load_idtr(int idtNO,int addr);
void init_gdtidt();
void set_segmdesc(struct SEGMENT_DESCRIPTOR* sd, unsigned int limit,int base,int ar);//ar:address
void set_gatedesc(struct GATE_DESCRIPTOR* gd, int offset,int selector,int ar);
//int asm_inthandler21();
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e
#define AR_TSS32		0x0089
#define AR_LDT			0x0082		//局部段描述表


/*pic setting*/
void init_pic();
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
void inthandler20(int *esp);//IRQ0 调用的中断处理程序
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
