#ifndef __ENCODER_H
#define __ENCODER_H

#include <stdint.h>

/* 软件编码器: 用 GPIO 边沿 + 方向引脚实现
 *   E1A(PB10) 上升沿中断 -> 读 E1B(PB11) 决定正/反转
 *   E2A(PB15) 上升沿中断 -> 读 E2B(PB16) 决定正/反转
 * 两者均在 GPIOB, 共用 TB_INT_IRQN(GROUP1 向量). 引脚与中断由
 * SysConfig(ti_msp_dl_config)已完成配置 */

void Encoder_Init(void);
void Encoder_SetLocation(uint8_t n, int32_t Location);  /* n=1,2 */
int32_t Encoder_GetLocation(uint8_t n);                  /* 累计脉冲数(带符号) */
int32_t Encoder_GetSpeed(uint8_t n);                     /* 周期内脉冲增量, 需主循环周期调用 */
uint8_t Encoder_GetDirection(uint8_t n);                 /* 1=正转 0=反转 */
void Encoder_Update(void);  /* 需主循环周期调用, 用于测速 */


#endif


