#include "header/bootpack.h"
#include "header/mmemory.h"
#include "header/fifo.h"
#include "header/multitask.h"
#include "header/msheet.h"
#define NULL  ((void*)0)



SHEETCONTROLL* init_sheetControll(MEMMAN* memoryManager,unsigned char* vram,int xsize,int ysize){
    SHEETCONTROLL* ctl = (SHEETCONTROLL*) memory_alloc_4kb(memoryManager, sizeof(SHEETCONTROLL)); 
    if(ctl == NULL){
        return ctl; 
    }

    ctl->map = (unsigned char*) memory_alloc_4kb(memoryManager, xsize * ysize );
    if(ctl->map == 0){
        memory_free_4kb(memoryManager, (int)ctl, sizeof(SHEETCONTROLL)); 
        return ctl;
    }

    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1; 
    
    int idx = 0;
    while(idx<MAX_SHEETS){
        ctl->sheets0[idx].flags = 0;
        
        ctl->sheets0[idx].ctl = ctl;
        ++idx;
    }
    return ctl;
}

SHEET* sheet_alloc(SHEETCONTROLL* ctl){
    SHEET* sheet;
    int idx = 0;
    while(idx<MAX_SHEETS){
        if(ctl->sheets0[idx].flags == 0){
            sheet = &ctl->sheets0[idx];   
            sheet->flags = SHEETS_USE;  
            sheet->height = -1;         
            sheet->task = NULL;           
            return sheet;
        }
        ++idx;
    }
    return 0;
}
void sheet_free(SHEET* sheet){
    if(sheet->height >= 0){
        sheet_updown(sheet,-1);
    }
    sheet->flags = 0;
}

void sheet_buffer_set(SHEET* sheet,unsigned char* buf,int xsize,int ysize,int colInv){
    sheet->buffer = buf;
    sheet->bxsize = xsize;
    sheet->bysize = ysize;
    sheet->colorInv = colInv;
}




void sheet_updown(SHEET* sht, int height){
    SHEETCONTROLL* ctl = sht->ctl;
    int h, old = sht->height;
    if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
    sht->height = height;
    //Sort
    if(old > height){
        if(height >= 0){
            for(h = old; h > height; --h){
                ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
            sheet_refresh_map(ctl, sht->vramX0, sht->vramY0, sht->vramX0 + sht->bxsize, sht->vramY0 + sht->bysize, height+1);
            sheet_refresh_sub(ctl, sht->vramX0, sht->vramY0, sht->vramX0 + sht->bxsize, sht->vramY0 + sht->bysize, height+1, old);
        }else{
            if(ctl->top > old){
                for (h = old; h < ctl->top; ++h) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
            }
            --ctl->top;
            sheet_refresh_map(ctl,sht->vramX0,sht->vramY0,sht->vramX0 + sht->bxsize, sht->vramY0 + sht->bysize, 0);
            sheet_refresh_sub(ctl,sht->vramX0,sht->vramY0,sht->vramX0 + sht->bxsize, sht->vramY0 + sht->bysize, 0, old -1);
        }
        
    }
    else if(old < height){
        if(old >= 0){
            for (h = old; h < height; ++h) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
            ctl->sheets[height] = sht;
        }else{
            for (h = ctl->top; h >= height; --h) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
            ctl->sheets[height] = sht;
            ++ctl->top;
        }
        sheet_refresh_map(ctl,sht->vramX0,sht->vramY0,sht->vramX0 + sht->bxsize, sht->vramY0 + sht->bysize, height);
        sheet_refresh_sub(ctl,sht->vramX0,sht->vramY0,sht->vramX0 + sht->bxsize, sht->vramY0 + sht->bysize, height, height);
    }
    
}


void sheet_refresh(SHEET* sheet,int bx0,int by0,int bx1,int by1){
    if(sheet->height >= 0){
        sheet_refresh_sub(sheet->ctl,sheet->vramX0 + bx0, sheet->vramY0 + by0, sheet->vramX0 + bx1, sheet->vramY0 + by1, sheet->height, sheet->height);//最后2个参数：图层层级
    } 
}


void sheet_refresh_sub(SHEETCONTROLL* ctl,int vx0,int vy0,int vx1,int vy1,int layer, int anotherLayer){
    if(ctl == NULL){
        return;
    }
    SHEET* sheet = NULL;
    int h,vx,vy;
    int bx0,bx1,by0,by1;//buffer size —— bx0:start bx1:end
    int bx2;
    int i,k;

    int* map_addr;
    int* vram_addr;
    int* buf_addr;

    unsigned char sid;
    int sid_4byte;     

    unsigned char* buf = NULL;
    unsigned char* vram = ctl->vram;
    unsigned char* map = ctl->map;

    if(vx0 < 0){vx0 = 0;}
    if(vy0 < 0){vy0 = 0;}
    if(vx1 > ctl->xsize){vx1 = ctl->xsize;}
    if(vy1 > ctl->ysize){vy1 = ctl->ysize;}

    int newbx,newby;
    for(h = layer;h <= anotherLayer;++h){
        sheet = ctl->sheets[h];
        buf = sheet->buffer;

        sid = sheet - ctl->sheets0;

        bx0 = vx0 - sheet->vramX0;
        by0 = vy0 - sheet->vramY0;

        bx1 = vx1 - sheet->vramX0;
        by1 = vy1 - sheet->vramY0;

        if(bx0 < 0){bx0 = 0;}
        if(by0 < 0){by0 = 0;}
        if(bx1 > sheet->bxsize){bx1 = sheet->bxsize;}
        if(by1 > sheet->bysize){by1 = sheet->bysize;}

        if((sheet->vramX0 & 3) == 0)
        {
            /*4byte*/
            i = (bx0 + 3) / 4; 
            k =  bx1 / 4;    
            k = k - i;
            sid_4byte = sid | sid << 8 | sid << 16 | sid << 24; 
            for(newby = by0; newby < by1; ++newby){
                vy = sheet->vramY0 + newby;
                for(newbx = bx0; newbx < bx1 && (newbx & 3) != 0; ++newbx){
                    vx = sheet->vramX0 + newbx;
                    if(map[vy * ctl->xsize + vx] == sid){
                        vram[vy * ctl->xsize + vx] = buf[newby * sheet->bxsize + newbx]; 
                    } 
                }//end for-loop
                vx = sheet->vramX0 + newbx;
                map_addr  = (int*) &map[vy * ctl->xsize + vx];
                vram_addr = (int*) &vram[vy * ctl->xsize + vx];
                buf_addr  = (int*) &buf[newby * sheet->bxsize + newbx]; 
                for(i = 0; i < k; ++i){
                    if(map_addr[i] == sid_4byte)
                    {
                        vram_addr[i] = buf_addr[i]; 
                    }
                    else
                    {
                        bx2 = newbx + i * 4;
                        vx = sheet->vramX0 + bx2;
                        if(map[vy * ctl->xsize + vx + 0] == sid){
                            vram[vy * ctl->xsize + vx + 0] = buf[newby * sheet->bxsize + bx2 + 0];
                        }
                        if(map[vy * ctl->xsize + vx + 1] == sid){
                            vram[vy * ctl->xsize + vx + 1] = buf[newby * sheet->bxsize + bx2 + 1];
                        }
                        if(map[vy * ctl->xsize + vx + 2] == sid){
                            vram[vy * ctl->xsize + vx + 2] = buf[newby * sheet->bxsize + bx2 + 2];
                        }
                        if(map[vy * ctl->xsize + vx + 3] == sid){
                            vram[vy * ctl->xsize + vx + 3] = buf[newby * sheet->bxsize + bx2 + 3];
                        }
                    }
                }
                newbx += k * 4;
                for(;newbx < bx1; ++newbx){
                    vx = sheet->vramX0 + newbx;
                    if(map[vy * ctl->xsize + vx] == sid){
                        vram[vy * ctl->xsize + vx] = buf[newby * sheet->bxsize + newbx];
                    }
                }//end for-loop
            }//end for-loop

        }
        else
        {/*1byte*/
            for(newby = by0;newby < by1;++newby){
                vy = sheet->vramY0 + newby;
                for(newbx = bx0;newbx < bx1;++newbx){
                    vx = sheet->vramX0 + newbx;
                    if(map[vy * ctl->xsize + vx]  == sid){
                        vram[vy * ctl->xsize + vx] = buf[newby * sheet->bxsize + newbx];
                    }

                }
            }//end-for loop

        }//end if-else
    }

}

void sheet_refresh_map(SHEETCONTROLL* ctl,int vx0,int vy0,int vx1,int vy1,int layer){//只针对当前层级的map进行刷新
    SHEET* sheet = NULL;
    int h,vx,vy;
    int bx0,bx1,by0,by1;//buffer size —— bx0:start bx1:end
    
    unsigned char sid; 
    int sid_4byte;   
    int *p;             
    unsigned char* buf = NULL, *map = ctl->map;

    if(vx0 < 0){vx0 = 0;}
    if(vy0 < 0){vy0 = 0;}
    if(vx1 > ctl->xsize){vx1 = ctl->xsize;}
    if(vy1 > ctl->ysize){vy1 = ctl->ysize;}

    for(h = layer;h <= ctl->top;++h){
        sheet = ctl->sheets[h];
        buf = sheet->buffer;
        sid = sheet - ctl->sheets0;

        bx0 = vx0 - sheet->vramX0;
        by0 = vy0 - sheet->vramY0;

        bx1 = vx1 - sheet->vramX0;
        by1 = vy1 - sheet->vramY0;

        if(bx0 < 0){bx0 = 0;}
        if(by0 < 0){by0 = 0;}
        if(bx1 > sheet->bxsize){bx1 = sheet->bxsize;}
        if(by1 > sheet->bysize){by1 = sheet->bysize;}

        int newbx,newby;
        if(sheet->colorInv <= -1){
            if((sheet->vramX0 & 3) == 0 && (bx0 & 3) == 0 && (bx1 & 3) == 0 ){
                //4byte
                bx1 = (bx1 - bx0) /4;  
                sid_4byte = sid | sid <<8 | sid << 16  | sid << 24;
                for(newby = by0; newby < by1; ++newby){
                    vy = sheet->vramY0 + newby;
                    vx = sheet->vramX0 + bx0;
                    p = (int*) &map[vy * ctl->xsize + vx];
                    for(newbx = 0; newbx < bx1; ++newbx){
                        p[newbx] = sid_4byte; 
                    }
                }

            }else{
                for(newby = by0; newby < by1; ++newby){
                    vy = sheet->vramY0 + newby;
                    for(newbx = 0; newbx < bx1; ++newbx){
                        vx = sheet->vramX0 + newbx;
                        map[vy * ctl->xsize + vx] = sid;
                    }
                }
            }
        }else{
            //AERO layer
            for(newby = by0;newby < by1;++newby){
                vy = sheet->vramY0 + newby;
                for(newbx = bx0;newbx < bx1;++newbx){
                    vx = sheet->vramX0 + newbx;
                    if(buf[newby * sheet->bxsize + newbx] != sheet->colorInv){
                        map[vy * ctl->xsize + vx] = sid;
                    }

                }
            }
        }        
    }

}

void sheet_slided(SHEET* sheet,int newvx0,int newvy0){
    if(sheet == NULL){
       return;
    }
    SHEETCONTROLL* ctl = sheet->ctl;
    int oldvx0 = sheet->vramX0;
    int oldvy0 = sheet->vramY0;
    
    sheet->vramX0 = newvx0;
    sheet->vramY0 = newvy0;
    if(sheet->height >= 0){
        sheet_refresh_map(ctl,oldvx0, oldvy0,oldvx0 + sheet->bxsize, oldvy0 + sheet->bysize,0);
        sheet_refresh_map(ctl, newvx0, newvy0, newvx0 + sheet->bxsize, newvy0 + sheet->bysize,sheet->height);
        sheet_refresh_sub(ctl,oldvx0, oldvy0,oldvx0 + sheet->bxsize, oldvy0 + sheet->bysize,0,sheet->height - 1);
        sheet_refresh_sub(ctl,newvx0, newvy0,newvx0 + sheet->bxsize, newvy0 + sheet->bysize,sheet->height,sheet->height);
    }
}

