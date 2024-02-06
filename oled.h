#ifndef _OLED_H
#define _OLED_H

#define u8 unsigned char
#define u32 unsigned int

#define SlaveAddress 0x3C  //OLED地址
#define OLED_CMD  0x00  //OLED写命令
#define OLED_DATA 0x40  //OLED写数据

//更新显存到LCD
void OLED_Refresh_Gram(void);
//开启OLED显示
void OLED_Display_On(void);
//关闭OLED显示
void OLED_Display_Off(void);
//清屏函数,清完屏,整个屏幕是黑色的!和没点亮一样!!!
void OLED_Clear(void);
//画点
//x:0~127
//y:0~63
//t:1 填充 0,清空
void OLED_DrawPoint(u8 x,u8 y,u8 t);
//x1,y1,x2,y2 填充区域的对角坐标
//确保x1<=x2;y1<=y2 0<=x1<=127 0<=y1<=63
//dot:0,清空;1,填充
void OLED_Fill(u8 x1,u8 y1,u8 x2,u8 y2,u8 dot);
//屏幕初始化函数
int oled_init(void);
//设置光标
void OLED_SetPos(u8 x, u8 y);
//在指定位置显示一个字符,包括部分字符
//x:0~127
//y:0~63
//mode:0,反白显示;1,正常显示
//size:选择字体 16/12
void OLED_ShowChar(u8 x,u8 y,u8 chr,u8 size,u8 mode);
//显示2个数字
//x,y :起点坐标
//len :数字的位数
//size:字体大小
//mode:模式	0,填充模式;1,叠加模式
//num:数值(0~4294967295);
void OLED_ShowNum(u8 x,u8 y,u32 num,u8 len,u8 size);
//显示字符串
//x,y:起点坐标
//size:字体大小
//*p:字符串起始地址
void OLED_ShowString(u8 x,u8 y,const u8 *p,u8 size);


#endif