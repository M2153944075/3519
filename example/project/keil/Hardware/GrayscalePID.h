#ifndef GRAYSCALE_PID_H
#define GRAYSCALE_PID_H

#include <stdint.h>
#include "PID.h"   /* PID_Controller */

/* ============================================================
 * 循迹 PD 环：灰度加权和 -> 左右轮目标速度差
 * ============================================================
 * 输入为灰度加权和(目标 0)，输出为左右轮目标速度差 turn。
 * PD 环不加积分，Ki 固定为 0；输出限幅复用 TURN_SPEED_MAX。
 * 加权和 > 0 表示黑线偏右 -> 期望 turn < 0(车右转) 把线拉回中线。
 */

extern volatile float Kp_gray;
extern volatile float Ki_gray;
extern volatile float Kd_gray;
extern volatile float GRAY_I_MAX;
extern volatile float GRAY_D_FILTER_ALPHA;

void GrayPID_Init(void);
void GrayPIDParameters(void);

void Gray_Reset(void);
float Gray_GetTarget(void);
float Gray_GetError(void);

/* 灰度加权和 -> 左右轮目标速度差，目标恒为 0。
 *  weighted_sum : Grayscale_WeightedSum 读出的加权和
 *  返回         : 左右轮目标速度差 turn，±TURN_SPEED_MAX */
float Gray_PD_Update(float weighted_sum);

#endif /* GRAYSCALE_PID_H */
