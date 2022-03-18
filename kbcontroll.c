#include "header/bootpack.h"
#include "header/kbcontroll.h"
#include "header/fifo.h"

//fifo.c
extern FIFO32* keyfifo;

mouse_desc mDesc;
int keydata;
void init_keyboard_controll(FIFO32* fifo, int data);
void wait_keyboard_controll_sendReady(){
	while(1){
		if((io_in8(PORT_KEYSTATUS) & KEYSTATUS_SEND_NOTREADY ) == 0 ){
			break;	
		}
	}
}

//INT 0X2C 0X21, Keyboard INT
void inthandler21(int* esp){
    int data;
    io_out8(PIC0_OCW2, 0X61);
    data = io_in8(PORT_KEYDAT);
    put_buffer_fifo32(keyfifo,data + keydata);
}

//32 BITS
void init_keyboard_controll(FIFO32* fifo, int data){
	keyfifo = fifo;
	keydata = data;
	wait_keyboard_controll_sendReady();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_keyboard_controll_sendReady();
	io_out8(PORT_KEYDAT,KEYBOARD_CONTROLL_MODE);
}

