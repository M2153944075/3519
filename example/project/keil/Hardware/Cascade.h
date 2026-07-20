#ifndef CASCADE_H
#define CASCADE_H

#include <stdint.h>

#include "PID.h"

/* ============================================================
 * 串级控制
 * ============================================================
 * 使用流程：
 * 1. 通过 YawAngle_SetTarget() 设置偏航目标；
 * 2. 每个控制周期调用一种 Cascade_*_PWM()；
 * 3. 新任务开始或切换控制链时调用 Cascade_Reset()。
 *
 * 速度差与基速合成：
 * left_speed  = base_speed + turn
 * right_speed = base_speed - turn
 */

/* 角度 -> 角速度 -> 速度 -> PWM。
 * 返回末级左右轮速度差 turn，供调试使用。 */
float Cascade_AngleRateSpeed_PWM(float base_speed,
                                 float angle_now,
                                 float gyro_now);

/* 角度 -> 速度 -> PWM，省去角速度内环。 */
float Cascade_AngleSpeed_PWM(float base_speed, float angle_now);

/* 直接设置左右轮目标速度。 */
void Cascade_WheelSpeed_PWM(float left_speed_set,
                            float right_speed_set);

/* 两轮共同目标速度 -> PWM；返回值保留，当前固定为 0。 */
int32_t Cascade_Speed_PWM(float speed_set);

/* 灰度加权和 -> 循迹 PD -> 速度 -> PWM；返回值当前固定为 0。 */
int32_t Cascade_Track_PWM(float base_speed, float weighted_sum);

/* 自读灰度后执行循迹。
 * 返回 1 表示未检测到黑线，此时保持上拍输出，由调用方决定停车；
 * 返回 0 表示已执行本次循迹控制。 */
uint8_t Cascade_TrackStep(float base_speed);

/* ============================================================
 * 状态管理与调试取值
 * ============================================================ */
void Cascade_Reset(void);

/* 角度误差与角速度同时满足阈值时返回 1。 */
uint8_t Cascade_IsStable(float gyro_z_dps,
                         float angle_tolerance_deg,
                         float rate_tolerance_dps);

float Cascade_GetRateTarget(void);
float Cascade_GetTurnOutput(void);

#endif /* CASCADE_H */
