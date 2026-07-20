#ifndef PID_H
#define PID_H

#include <stdint.h>

/* ============================================================
 * 通用 PID 控制器
 * ============================================================
 * 位置式 PID + 微分一阶低通滤波 + 条件积分抗饱和。
 * ki 在内部乘以 dt，外部给定的 ki 为每秒积分增益。
 */
typedef struct {
    float kp;
    float ki;
    float kd;
    float dt;

    float integral_limit;
    float output_limit;
    float derivative_filter_alpha;

    float integral;
    float previous_error;
    float derivative;
    uint8_t initialized;
} PID_Controller;

float PID_LimitFloat(float value, float min_value, float max_value);
float PID_PositiveLimit(float value);
float PID_Wrap180(float angle);
float PID_ValidDt(float dt);
float PID_ValidFilterAlpha(float alpha);

void PID_Init(PID_Controller *controller,
              float kp,
              float ki,
              float kd,
              float dt,
              float integral_limit,
              float output_limit,
              float derivative_filter_alpha);
void PID_SetParameters(PID_Controller *controller,
                       float kp,
                       float ki,
                       float kd,
                       float dt,
                       float integral_limit,
                       float output_limit,
                       float derivative_filter_alpha);
void PID_Reset(PID_Controller *controller);
float PID_UpdateError(PID_Controller *controller,
                      float error,
                      float feedforward);
float PID_Update(PID_Controller *controller,
                 float target,
                 float actual,
                 float feedforward);

/* 模块一次性初始化（幂等），装配所有控制环。 */
void PID_ModuleInit(void);

/* ============================================================
 * 共享参数
 * ============================================================ */
extern volatile float PID_CONTROL_DT;

/* 左右轮速度差 / 角速度内环输出 / 循迹输出的统一上限。 */
extern volatile float TURN_SPEED_MAX;

/* ============================================================
 * 远程调参
 * ============================================================
 * id=1..9：速度环、偏航角环、偏航角速度环的 KP/KI/KD；
 * id=10：控制周期；id=11..13：循迹环 KP/KI/KD。
 */
typedef struct {
    float speed_kp;
    float speed_ki;
    float speed_kd;
    float yaw_angle_kp;
    float yaw_angle_ki;
    float yaw_angle_kd;
    float yaw_rate_kp;
    float yaw_rate_ki;
    float yaw_rate_kd;
    float track_kp;
    float track_ki;
    float track_kd;
    float control_dt;
} PID_RemoteSnapshot;

uint8_t PID_RemoteSetParameter(uint8_t id, float value);
void PID_RemoteGetSnapshot(PID_RemoteSnapshot *snapshot);

/* ============================================================
 * 各专用环接口（拆分自原 PID.c，保持旧调用方零改动）
 * ============================================================ */
#include "SpeedPID.h"
#include "YawPID.h"
#include "GrayscalePID.h"

#endif /* PID_H */
