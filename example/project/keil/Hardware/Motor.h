#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

#define MOTOR_SPEED_MAX 1000

/* 方向脚位于当前SysConfig定义的TB_PORT(PB6～PB9)。 */
#define AIN1_OUT(x) ((x) ? DL_GPIO_setPins(TB_PORT, TB_AIN1_PIN) : DL_GPIO_clearPins(TB_PORT, TB_AIN1_PIN))
#define AIN2_OUT(x) ((x) ? DL_GPIO_setPins(TB_PORT, TB_AIN2_PIN) : DL_GPIO_clearPins(TB_PORT, TB_AIN2_PIN))
#define BIN1_OUT(x) ((x) ? DL_GPIO_setPins(TB_PORT, TB_BIN1_PIN) : DL_GPIO_clearPins(TB_PORT, TB_BIN1_PIN))
#define BIN2_OUT(x) ((x) ? DL_GPIO_setPins(TB_PORT, TB_BIN2_PIN) : DL_GPIO_clearPins(TB_PORT, TB_BIN2_PIN))

/* 单轮控制：dir=1正转，dir=0反转，speed范围0～1000。 */
void MotorA_Control(uint8_t dir, uint32_t speed);
void MotorB_Control(uint8_t dir, uint32_t speed);

/* 双轮差速：左轮/右轮输入范围-1000～+1000。 */
void Motor_Drive(int32_t v_left, int32_t v_right);
void Motor_Stop(void);
int32_t Motor_GetLastLeftOutput(void);
int32_t Motor_GetLastRightOutput(void);

/* ============================================================
 *  电机转动状态 —— 供循迹逻辑直接调用
 * ============================================================
 *  speed 范围 0~1000, 各函数内部已限幅.
 *  所有状态基于 Motor_Drive() 差速驱动.
 */
void Motor_Forward     (uint32_t speed);   /* 直走: 两轮同速正转 */
void Motor_TurnLeft    (uint32_t speed);   /* 左转: 左轮反转, 右轮正转 (原地差速) */
void Motor_TurnRight   (uint32_t speed);   /* 右转: 左轮正转, 右轮反转 (原地差速) */
void Motor_SlightLeft  (uint32_t speed);   /* 稍左转: 左轮降速, 右轮全速 (微调修正) */
void Motor_SlightRight (uint32_t speed);   /* 稍右转: 左轮全速, 右轮降速 (微调修正) */
void Motor_SharpLeft   (uint32_t speed);   /* 急左转: 左轮全速反转, 右轮全速正转 */
void Motor_SharpRight  (uint32_t speed);   /* 急右转: 左轮全速正转, 右轮全速反转 */

#endif
