#ifndef __LCD_H
#define __LCD_H

// 解决 u8 u16 报错
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#define USE_HORIZONTAL 0

#include "stm32f1xx_hal.h"

//=========================LCD 引脚定义 HAL 库版=========================
#define LCD_SCL_GPIO_PORT    GPIOA
#define LCD_SCL_GPIO_PIN     GPIO_PIN_5   // 这里改成 HAL 格式

#define LCD_SDA_GPIO_PORT    GPIOA
#define LCD_SDA_GPIO_PIN     GPIO_PIN_7   // 这里改成 HAL 格式

#define LCD_RST_GPIO_PORT    GPIOA
#define LCD_RST_GPIO_PIN     GPIO_PIN_0   // 这里改成 HAL 格式

#define LCD_DC_GPIO_PORT     GPIOA
#define LCD_DC_GPIO_PIN      GPIO_PIN_1   // 这里改成 HAL 格式

#define LCD_CS_GPIO_PORT     GPIOB
#define LCD_CS_GPIO_PIN      GPIO_PIN_8   // 这里改成 HAL 格式

#define LCD_BLK_GPIO_PORT    GPIOB
#define LCD_BLK_GPIO_PIN     GPIO_PIN_9   // 这里改成 HAL 格式

//=========================LCD 控制引脚 HAL 库宏=========================
#define LCD_SCLK_Set()   HAL_GPIO_WritePin(LCD_SCL_GPIO_PORT, LCD_SCL_GPIO_PIN, GPIO_PIN_SET)
#define LCD_SCLK_Clr()   HAL_GPIO_WritePin(LCD_SCL_GPIO_PORT, LCD_SCL_GPIO_PIN, GPIO_PIN_RESET)

#define LCD_MOSI_Set()   HAL_GPIO_WritePin(LCD_SDA_GPIO_PORT, LCD_SDA_GPIO_PIN, GPIO_PIN_SET)
#define LCD_MOSI_Clr()   HAL_GPIO_WritePin(LCD_SDA_GPIO_PORT, LCD_SDA_GPIO_PIN, GPIO_PIN_RESET)

#define LCD_DC_Set()     HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_SET)
#define LCD_DC_Clr()     HAL_GPIO_WritePin(LCD_DC_GPIO_PORT, LCD_DC_GPIO_PIN, GPIO_PIN_RESET)

#define LCD_CS_Set()     HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_SET)
#define LCD_CS_Clr()     HAL_GPIO_WritePin(LCD_CS_GPIO_PORT, LCD_CS_GPIO_PIN, GPIO_PIN_RESET)

#define LCD_RST_Set()    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_SET)
#define LCD_RST_Clr()    HAL_GPIO_WritePin(LCD_RST_GPIO_PORT, LCD_RST_GPIO_PIN, GPIO_PIN_RESET)

#define LCD_BLK_Set()    HAL_GPIO_WritePin(LCD_BLK_GPIO_PORT, LCD_BLK_GPIO_PIN, GPIO_PIN_SET)
#define LCD_BLK_Clr()    HAL_GPIO_WritePin(LCD_BLK_GPIO_PORT, LCD_BLK_GPIO_PIN, GPIO_PIN_RESET)

//=============================函数声明=============================
void LCD_GPIO_Init(void);
void LCD_Writ_Bus(u8 dat);
void LCD_WR_DATA8(u8 dat);
void LCD_WR_DATA(u16 dat);
void LCD_WR_REG(u8 dat);
void LCD_Address_Set(u16 x1,u16 y1,u16 x2,u16 y2);
void LCD_Init(void);
void LCD_Clear(u16 color);
void LCD_Fill(u16 xsta,u16 ysta,u16 xend,u16 yend,u16 color);
void LCD_DrawPoint(u16 x,u16 y,u16 color);
void LCD_DrawLine(u16 x1,u16 y1,u16 x2,u16 y2,u16 color);

void LCD_ShowChinese12x12(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowChinese16x16(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowChinese24x24(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowChinese32x32(u16 x,u16 y,u8 *s,u16 fc,u16 bc,u8 sizey,u8 mode);
void LCD_ShowString(u16 x,u16 y,const u8 *str,u16 fc,u16 bc,u8 size,u8 mode);

//画笔颜色
#define WHITE         	 0xFFFF
#define BLACK         	 0x0000	  
#define BLUE           	 0x001F  
#define BRED             0XF81F
#define GRED 			 0XFFE0
#define GBLUE			 0X07FF
#define RED           	 0xF800
#define MAGENTA       	 0xF81F
#define GREEN         	 0x07E0
#define CYAN          	 0x7FFF
#define YELLOW        	 0xFFE0
#define BROWN 			 0XBC40 //棕色
#define BRRED 			 0XFC07 //棕红色
#define GRAY  			 0X8430 //灰色
#define DARKBLUE      	 0X01CF	//深蓝色
#define LIGHTBLUE      	 0X7D7C	//浅蓝色  
#define GRAYBLUE       	 0X5458 //灰蓝色
#define LIGHTGREEN     	 0X841F //浅绿色
#define LGRAY 			 0XC618 //浅灰色(PANNEL),窗体背景色
#define LGRAYBLUE        0XA651 //浅灰蓝色(中间层颜色)
#define LBBLUE           0X2B12 //浅淡蓝色(选择条目的反色)
#define ORANGE           0xFD20 //橙色

#endif