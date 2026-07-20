#include "PID.h"

#include <math.h>
#include <stddef.h>

/*
 * ============================================================
 *  共享参数
 * ============================================================
 */

volatile float PID_CONTROL_DT = 0.01f;

/* 速度差 / 角速度内环输出 / 循迹输出的统一上限 */
volatile float TURN_SPEED_MAX = 180.0f;


/*
 * ============================================================
 *  通用辅助函数
 * ============================================================
 */

float PID_LimitFloat(float value, float min_value, float max_value)
{
    if (value > max_value) {
        return max_value;
    }
    if (value < min_value) {
        return min_value;
    }
    return value;
}

float PID_PositiveLimit(float value)
{
    return (value >= 0.0f) ? value : -value;
}

float PID_Wrap180(float angle)
{
    while (angle > 180.0f) {
        angle -= 360.0f;
    }
    while (angle < -180.0f) {
        angle += 360.0f;
    }
    return angle;
}

float PID_ValidDt(float dt)
{
    /* 防止 dt 为 0 或负数导致微分项除零 */
    if (dt <= 0.0f) {
        return 0.01f;
    }
    return dt;
}

float PID_ValidFilterAlpha(float alpha)
{
    return PID_LimitFloat(alpha, 0.0f, 1.0f);
}


/*
 * ============================================================
 *  通用 PID 实现
 * ============================================================
 */

void PID_Init(PID_Controller *controller,
              float kp, float ki, float kd, float dt,
              float integral_limit, float output_limit,
              float derivative_filter_alpha)
{
    if (controller == NULL) {
        return;
    }

    controller->kp = kp;
    controller->ki = ki;
    controller->kd = kd;
    controller->dt = PID_ValidDt(dt);

    controller->integral_limit = PID_PositiveLimit(integral_limit);
    controller->output_limit = PID_PositiveLimit(output_limit);
    controller->derivative_filter_alpha = PID_ValidFilterAlpha(derivative_filter_alpha);

    PID_Reset(controller);
}


void PID_SetParameters(PID_Controller *controller,
                       float kp, float ki, float kd, float dt,
                       float integral_limit, float output_limit,
                       float derivative_filter_alpha)
{
    if (controller == NULL) {
        return;
    }

    controller->kp = kp;
    controller->ki = ki;
    controller->kd = kd;
    controller->dt = PID_ValidDt(dt);

    controller->integral_limit = PID_PositiveLimit(integral_limit);
    controller->output_limit = PID_PositiveLimit(output_limit);
    controller->derivative_filter_alpha = PID_ValidFilterAlpha(derivative_filter_alpha);

    /* 参数在线修改时保留已有状态，但重新限制积分，
     * 防止新的积分上限小于旧积分值。 */
    controller->integral = PID_LimitFloat(
        controller->integral,
        -controller->integral_limit,
         controller->integral_limit);
}


void PID_Reset(PID_Controller *controller)
{
    if (controller == NULL) {
        return;
    }
    controller->integral       = 0.0f;
    controller->previous_error = 0.0f;
    controller->derivative     = 0.0f;
    controller->initialized    = 0U;
}


float PID_UpdateError(PID_Controller *controller,
                      float error, float feedforward)
{
    float proportional;
    float derivative_raw;
    float derivative_output;
    float integral_candidate;
    float output_candidate;
    float output;

    if (controller == NULL) {
        return 0.0f;
    }

    controller->dt = PID_ValidDt(controller->dt);

    proportional = controller->kp * error;

    /* 首拍没有历史误差，微分置 0，避免微分冲击 */
    if (controller->initialized == 0U) {
        derivative_raw = 0.0f;
        controller->derivative = 0.0f;
        controller->initialized = 1U;
    } else {
        derivative_raw = (error - controller->previous_error) / controller->dt;

        /* 一阶低通滤波: D_f = a*D_prev + (1-a)*D_raw */
        controller->derivative =
            controller->derivative_filter_alpha * controller->derivative +
            (1.0f - controller->derivative_filter_alpha) * derivative_raw;
    }

    derivative_output = controller->kd * controller->derivative;

    integral_candidate =
        controller->integral + controller->ki * error * controller->dt;
    integral_candidate = PID_LimitFloat(
        integral_candidate,
        -controller->integral_limit,
         controller->integral_limit);

    output_candidate =
        feedforward + proportional + integral_candidate + derivative_output;

    /* 条件积分抗饱和：
     * 1. 输出未饱和，允许积分；
     * 2. 正向饱和且误差为负，允许积分(往回拉)；
     * 3. 负向饱和且误差为正，允许积分(往回拉)。 */
    if ((output_candidate <=  controller->output_limit &&
         output_candidate >= -controller->output_limit) ||
        (output_candidate >  controller->output_limit && error < 0.0f) ||
        (output_candidate < -controller->output_limit && error > 0.0f)) {

        controller->integral = integral_candidate;
    }

    output = feedforward + proportional +
             controller->integral + derivative_output;

    output = PID_LimitFloat(output, -controller->output_limit, controller->output_limit);

    controller->previous_error = error;
    return output;
}


float PID_Update(PID_Controller *controller,
                 float target, float actual, float feedforward)
{
    return PID_UpdateError(controller, target - actual, feedforward);
}


/*
 * ============================================================
 *  模块一次性初始化（幂等）
 * ============================================================
 *  装配四个专用环：左右轮速度、偏航角外环、偏航角速度内环、
 *  角度->速度差、循迹 PD。各环的具体定义在对应 .c 中。
 */
static uint8_t pid_module_initialized = 0U;

void PID_ModuleInit(void)
{
    if (pid_module_initialized != 0U) {
        return;
    }

    SpeedPID_Init(&speed_left_pid);
    SpeedPID_Init(&speed_right_pid);
    YawAnglePID_Init();
    YawRatePID_Init();
    YawAngleSpeedPID_Init();
    GrayPID_Init();

    pid_module_initialized = 1U;
}


/*
 * ============================================================
 *  远程调参：统一分发到各专用环的参数变量
 * ============================================================
 */

uint8_t PID_RemoteSetParameter(uint8_t id, float value)
{
    switch (id) {
        case 1:
            Kp_spd = value;
            break;
        case 2:
            Ki_spd = value;
            break;
        case 3:
            Kd_spd = value;
            break;
        case 4:
            Kp_yaw_angle = value;
            break;
        case 5:
            Ki_yaw_angle = value;
            break;
        case 6:
            Kd_yaw_angle = value;
            break;
        case 7:
            Kp_yaw_rate = value;
            break;
        case 8:
            Ki_yaw_rate = value;
            break;
        case 9:
            Kd_yaw_rate = value;
            break;
        case 10:
            PID_CONTROL_DT = PID_ValidDt(value);
            break;
        case 11:
            Kp_gray = value;
            break;
        case 12:
            Ki_gray = value;
            break;
        case 13:
            Kd_gray = value;
            break;
        default:
            return 0u;
    }

    return 1u;
}

void PID_RemoteGetSnapshot(PID_RemoteSnapshot *snapshot)
{
    if (snapshot == NULL) {
        return;
    }

    snapshot->speed_kp = Kp_spd;
    snapshot->speed_ki = Ki_spd;
    snapshot->speed_kd = Kd_spd;
    snapshot->yaw_angle_kp = Kp_yaw_angle;
    snapshot->yaw_angle_ki = Ki_yaw_angle;
    snapshot->yaw_angle_kd = Kd_yaw_angle;
    snapshot->yaw_rate_kp = Kp_yaw_rate;
    snapshot->yaw_rate_ki = Ki_yaw_rate;
    snapshot->yaw_rate_kd = Kd_yaw_rate;
    snapshot->track_kp = Kp_gray;
    snapshot->track_ki = Ki_gray;
    snapshot->track_kd = Kd_gray;
    snapshot->control_dt = PID_CONTROL_DT;
}
