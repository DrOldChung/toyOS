#ifndef _KBCONTROLL_H
#define _KBCONTROLL_H

//键鼠控制电路设置
#define PORT_KEYDAT    	0x0060				//键位16进制数据   
#define	PORT_KEYSTATUS	0X0064				//状态
#define	PORT_KEYCMD		0X0064				//命令

#define	KEYSTATUS_SEND_NOTREADY		0X02	//设为未准备状态
#define	KEYCMD_WRITE_MODE			0x60	//写入模式
#define	KEYBOARD_CONTROLL_MODE		0x47	//键盘控制模式  0x47:鼠标模式

#define MOUSE_WAIT_STATE    0               //等待接受0xfa信号状态     
#define	MOUSE_CLICK_STATE	1               //点击状态
#define	MOUSE_MOVEX_STATE	2               //左右移动状态
#define	MOUSE_MOVEY_STATE	3               //上下移动状态

#define	KEYCMD_SENDTO_MOUSE			0xd4
#define	MOUSECMD_ENABLE				0xf4	//开启鼠标	

#define MOUSE_SENSITIVITY   15              //鼠标固定灵敏度 

typedef struct MOUSE_DESCRIPTION{
    unsigned char mouseBuf[3];
    unsigned int state;
    int x,y;
    int button;
}mouse_desc;

#endif

void wait_keyboard_controll_sendReady();
int mouse_decode(mouse_desc* mouse_desc, unsigned char data);