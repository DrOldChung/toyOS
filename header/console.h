#ifndef _CONSOLE_H
#define _CONSOLE_H

extern struct SHEET;
extern struct TIMER;

typedef	struct _MY_CONSOLE{
	struct SHEET* sht;
	int cur_x;
	int cur_y;
	int cur_color;
	struct TIMER* timer;
}CONSOLE;

#endif