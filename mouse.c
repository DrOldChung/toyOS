
#include "header/bootpack.h"
#include "header/kbcontroll.h"
#include "header/fifo.h"

extern FIFO32* mousefifo;//from fifo.c

int mousedata;

void enable_mouse(FIFO32* fifo, int data, mouse_desc* mdesc){
    mousefifo = fifo;
    mousedata = data;

    wait_keyboard_controll_sendReady();
	io_out8(PORT_KEYCMD,KEYCMD_SENDTO_MOUSE);//send to mouse
	wait_keyboard_controll_sendReady();
	io_out8(PORT_KEYDAT,MOUSECMD_ENABLE);
    mdesc->state = MOUSE_WAIT_STATE;
}


int mouse_decode(mouse_desc* mdesc, unsigned char data){
    if( mdesc ==((void*)0) )
        return -1;
    //state
    unsigned int mouseState = mdesc->state;
    switch(mouseState){
        case MOUSE_WAIT_STATE:
            mdesc->state = (data == 0xfa) ? 1 : mdesc->state;
            return 0;
            break;
        case MOUSE_CLICK_STATE:
            if((data & 0xc8) == 0x08){
			    mdesc->mouseBuf[0] = data; //click
			    mdesc->state = MOUSE_MOVEX_STATE;
            }
            return 0;
			break;
        
        case MOUSE_MOVEX_STATE:
			mdesc->mouseBuf[1] = data;//move lr
			mdesc->state = MOUSE_MOVEY_STATE;
            return 0;
			break;
		
        case MOUSE_MOVEY_STATE:
			mdesc->mouseBuf[2] = data;//move up low
			mdesc->state = MOUSE_CLICK_STATE;
            mdesc->button = (mdesc->mouseBuf[0]) & 0x07;
            mdesc->x = mdesc->mouseBuf[1];
            mdesc->y = mdesc->mouseBuf[2];

            if((mdesc->mouseBuf[0] & 0x10) != 0){
                mdesc->x |=0xffffff00;
            }
            if((mdesc->mouseBuf[0] & 0x20) != 0){
                mdesc->y |=0xffffff00;
            }
            mdesc->y = -mdesc->y;
            return 1;
			break;
        default:
			break;
    }
    return -1;
}


void inthandler2c(int* esp){
    int data;
    io_out8(PIC1_OCW2,0x64);
    io_out8(PIC0_OCW2,0x62);
    data = io_in8(PORT_KEYDAT);
    put_buffer_fifo32(mousefifo,data + mousedata);
}