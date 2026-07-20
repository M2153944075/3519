#include "YawPID.h"

#include <stddef.h>


/* ==================== 偏航角外环参数 ====================
 *  第一版建议 Ki_yaw_angle = 0、Kd_yaw_angle = 0，
 *  先把角度外环当 P 控制器调通，再按需要加 I / D。
 */
volatile float Kp_yaw_angle = 2.0f;
volatile float Ki_yaw_angle = 0.0f;
volatile float Kd_yaw_angle = 0.0f;

volatile float YAW_ANGLE_I_MAX = 30.0f;
volatile float YAW_RATE_TARGET_MAX = 90.0f;
volatile float YAW_ANGLE_D_FILTER_ALPHA = 0.85f;


/* ==================== 偏航角速度内环参数 ==================== */

volatile float Kp_yaw_rate = 1.0f;
volatile float Ki_yaw_rate = 0.1f;
volatile float Kd_yaw_rate = 0.0f;

volatile float YAW_RATE_I_MAX = 80.0f;
/* TURN_SPEED_MAX 定义在 PID.c（共享） */
volatile float YAW_RATE_D_FILTER_ALPHA = 0.80f;


/*
 * ============================================================
 *  内部状态
 * ============================================================
 */

/* 偏航角外环、角速度内环：模块内单例 */
static PID_Controller yaw_angle_pid;
static PID_Controller yaw_rate_pid;

/* 角度 -> 速度差 专用控制器：复用角度环增益，输出限幅用 TURN_SPEED_MAX */
static PID_Controller angle_speed_pid;

/* 偏航目标与误差 */
static float yaw_target = 0.0f;
static float yaw_angle_error = 0.0f;


/*
 * ============================================================
 *  初始化 / 参数同步
 * ============================================================
 */

void YawAnglePID_Init(void)
{
    PID_Init(&yaw_angle_pid,
             Kp_yaw_angle, Ki_yaw_angle, Kd_yaw_angle,
             PID_CONTROL_DT,
             YAW_ANGLE_I_MAX, YAW_RATE_TARGET_MAX, YAW_ANGLE_D_FILTER_ALPHA);
}

void YawRatePID_Init(void)
{
    PID_Init(&yaw_rate_pid,
             Kp_yaw_rate, Ki_yaw_rate, Kd_yaw_rate,
             PID_CONTROL_DT,
             YAW_RATE_I_MAX, TURN_SPEED_MAX, YAW_RATE_D_FILTER_ALPHA);
}

/* 角度 -> 速度差 控制器(内部，复用角度环增益，输出限幅用 TURN_SPEED_MAX) */
void YawAngleSpeedPID_Init(void)
{
    PID_Init(&angle_speed_pid,
             Kp_yaw_angle, Ki_yaw_angle, Kd_yaw_angle,
             PID_CONTROL_DT,
             YAW_ANGLE_I_MAX, TURN_SPEED_MAX, YAW_ANGLE_D_FILTER_ALPHA);
}

void YawAnglePIDParameters(void)
{
    PID_SetParameters(&yaw_angle_pid,
                     Kp_yaw_angle, Ki_yaw_angle, Kd_yaw_angle,
                     PID_CONTROL_DT,
                     YAW_ANGLE_I_MAX, YAW_RATE_TARGET_MAX, YAW_ANGLE_D_FILTER_ALPHA);
}

void YawRatePIDParameters(void)
{
    PID_SetParameters(&yaw_rate_pid,
                     Kp_yaw_rate, Ki_yaw_rate, Kd_yaw_rate,
                     PID_CONTROL_DT,
                     YAW_RATE_I_MAX, TURN_SPEED_MAX, YAW_RATE_D_FILTER_ALPHA);
}

/* 角度 -> 速度差 控制器同步(内部) */
static void AngleSpeedPIDParameters(void)
{
    PID_SetParameters(&angle_speed_pid,
                     Kp_yaw_angle, Ki_yaw_angle, Kd_yaw_angle,
                     PID_CONTROL_DT,
                     YAW_ANGLE_I_MAX, TURN_SPEED_MAX, YAW_ANGLE_D_FILTER_ALPHA);
}


/*
 * ============================================================
 *  偏航角目标 / 状态管理
 * ============================================================
 */

void YawAngle_SetTarget(float target_deg)
{
    PID_ModuleInit();
    yaw_target = PID_Wrap180(target_deg);
    YawAngle_Reset();
}

void YawAngle_Reset(void)
{
    PID_ModuleInit();
    PID_Reset(&yaw_angle_pid);
    PID_Reset(&angle_speed_pid);
    yaw_angle_error = 0.0f;
}

void YawRate_Reset(void)
{
    PID_ModuleInit();
    PID_Reset(&yaw_rate_pid);
}

float YawAngle_GetTarget(void)
{
    return yaw_target;
}

float YawAngle_GetError(void)
{
    return yaw_angle_error;
}


/*
 * ============================================================
 *  独立环调用
 * ============================================================
 */

/* 1. 角度 -> 角速度 (deg/s) */
float YawAngle_Update(float yaw_now_deg)
{
    PID_ModuleInit();
    YawAnglePIDParameters();

    /* 角度误差先限制在 [-180,180]，跨越 ±180° 时才不会选错旋转方向 */
    yaw_angle_error = PID_Wrap180(yaw_target - yaw_now_deg);

    return PID_UpdateError(&yaw_angle_pid, yaw_angle_error, 0.0f);
}

/* 2. 角速度 -> 左右轮目标速度差 */
float YawRate_Update(float gyro_target_dps, float gyro_now_dps)
{
    PID_ModuleInit();
    YawRatePIDParameters();

    return PID_Update(&yaw_rate_pid, gyro_target_dps, gyro_now_dps, 0.0f);
}

/* 3. 角度 -> 左右轮目标速度差 */
float Angle_To_Speed(float angle_now_deg)
{
    PID_ModuleInit();
    AngleSpeedPIDParameters();

    /* 复用偏航角误差(与 YawAngle_Update 同一目标) */
    yaw_angle_error = PID_Wrap180(yaw_target - angle_now_deg);

    return PID_UpdateError(&angle_speed_pid, yaw_angle_error, 0.0f);
}