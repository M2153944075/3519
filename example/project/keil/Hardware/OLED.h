#ifndef __OLED_H
#define __OLED_H


#include "ti_msp_dl_config.h"
#include <stdint.h>
#include "OLED_Data.h"



#ifndef u8
#define u8 uint8_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif


//-----------------OLED端口定义----------------

static inline void OLED_I2C_Delay(void)
{
	delay_cycles(160);
}

/* 软件 I2C 采用开漏风格输出：
 * 输出低电平表示 0，释放引脚表示 1。
 */
#define OLED_SCL_Clr() do { DL_GPIO_clearPins(OLED_PORT, OLED_SCL_PIN); DL_GPIO_enableOutput(OLED_PORT, OLED_SCL_PIN); OLED_I2C_Delay(); } while (0)
#define OLED_SCL_Set() do { DL_GPIO_disableOutput(OLED_PORT, OLED_SCL_PIN); OLED_I2C_Delay(); } while (0)

#define OLED_SDA_Clr() do { DL_GPIO_clearPins(OLED_PORT, OLED_SDA_PIN); DL_GPIO_enableOutput(OLED_PORT, OLED_SDA_PIN); OLED_I2C_Delay(); } while (0)
#define OLED_SDA_Set() do { DL_GPIO_disableOutput(OLED_PORT, OLED_SDA_PIN); OLED_I2C_Delay(); } while (0)

#define OLED_W_SCL(bit_val)  do { if (bit_val) OLED_SCL_Set(); else OLED_SCL_Clr(); } while(0)
#define OLED_W_SDA(bit_val)  do { if (bit_val) OLED_SDA_Set(); else OLED_SDA_Clr(); } while(0)

#define OLED_I2C_ADDRESS			0x78



/*参数宏定义*********************/

/*FontSize参数取值*/
/*此参数值不仅用于判断，而且用于计算横向字符偏移，默认值为字体像素宽度*/
#define OLED_8X16				8
#define OLED_6X8				6

/*IsFilled参数数值*/
#define OLED_UNFILLED			0
#define OLED_FILLED				1

/*********************参数宏定义*/


/*函数声明*********************/

/*初始化函数*/
void OLED_Init(void);
void OLED_WriteCommand(uint8_t Command);

/*更新函数*/
void OLED_Update(void);
void OLED_UpdateArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显存控制函数*/
void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, uint8_t Width, uint8_t Height);

/*显示函数*/
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowImage(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, const uint8_t *Image);
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);

/*绘图函数*/
void OLED_DrawPoint(int16_t X, int16_t Y);
uint8_t OLED_GetPoint(int16_t X, int16_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, uint8_t Width, uint8_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, uint8_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, uint8_t A, uint8_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, uint8_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);

/*********************函数声明*/

#endif


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/
