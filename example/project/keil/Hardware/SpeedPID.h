#ifndef SPEED_PID_H
#define SPEED_PID_H

#include <stdint.h>
#include "PID.h"   /* PID_Controller */

/* ============================================================
 * 速度环：速度 -> PWM
 * ============================================================ */

/* 速度环参数。 */
extern volatile float Kp_spd;
extern volatile float Ki_spd;
extern volatile float Kd_spd;
extern volatile float SPD_I_MAX;
extern volatile float SPD_OUTPUT_MAX;
extern volatile float SPD_D_FILTER_ALPHA;

/* 左右轮必须使用独立的速度 PID 状态。 */
extern PID_Controller speed_left_pid;
extern PID_Controller speed_right_pid;

void SpeedPID_Init(PID_Controller *controller);
void SpeedPIDParameters(PID_Controller *controller);
void SpeedPID_Reset(PID_Controller *controller);

/* 速度 -> PWM。
 * controller    : &speed_left_pid / &speed_right_pid（控制器实例）
 * speed_set     : 目标速度（PWM 量级）
 * encoder_speed : 编码器周期脉冲增量（已带方向）
 * encoder_dir   : 安装方向修正，取 +1 或 -1
 * 返回          : 带符号 PWM，范围为 ±SPD_OUTPUT_MAX
 */
int32_t SpeedPID_Update(PID_Controller *controller,
                        float speed_set,
                        int32_t encoder_speed,
                        int32_t encoder_dir);

#endif /* SPEED_PID_H */