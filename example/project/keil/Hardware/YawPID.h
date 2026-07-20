#ifndef YAW_PID_H
#define YAW_PID_H

#include <stdint.h>
#include "PID.h"   /* PID_Controller */

/* ============================================================
 * 偏航控制：角度外环 + 角速度内环 + 角度->速度差单环
 * ============================================================ */

/* 偏航角外环参数：角度 -> 角速度。 */
extern volatile float Kp_yaw_angle;
extern volatile float Ki_yaw_angle;
extern volatile float Kd_yaw_angle;
extern volatile float YAW_ANGLE_I_MAX;
extern volatile float YAW_RATE_TARGET_MAX;
extern volatile float YAW_ANGLE_D_FILTER_ALPHA;

/* 偏航角速度内环参数：角速度 -> 左右轮速度差。 */
extern volatile float Kp_yaw_rate;
extern volatile float Ki_yaw_rate;
extern volatile float Kd_yaw_rate;
extern volatile float YAW_RATE_I_MAX;
extern volatile float YAW_RATE_D_FILTER_ALPHA;

/* 初始化 / 在线参数同步 */
void YawAnglePID_Init(void);
void YawRatePID_Init(void);
void YawAngleSpeedPID_Init(void);   /* 内部：角度->速度差，复用角度环增益 */

void YawAnglePIDParameters(void);
void YawRatePIDParameters(void);

/* 状态管理 */
void YawAngle_SetTarget(float target_deg);
void YawAngle_Reset(void);
void YawRate_Reset(void);
float YawAngle_GetTarget(void);
float YawAngle_GetError(void);

/* 独立环调用 */
float YawAngle_Update(float yaw_now_deg);                              /* 角度 -> 目标角速度(deg/s) */
float YawRate_Update(float gyro_target_dps, float gyro_now_dps);       /* 角速度 -> 左右轮速度差 */
float Angle_To_Speed(float angle_now_deg);                             /* 角度 -> 左右轮速度差 */

#endif /* YAW_PID_H */