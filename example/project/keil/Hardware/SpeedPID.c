#include "SpeedPID.h"

#include <math.h>
#include <stddef.h>


/* ==================== 速度环参数 ==================== */

volatile float Kp_spd = 2.0f;
volatile float Ki_spd = 1.0f;
volatile float Kd_spd = 0.0f;

volatile float SPD_I_MAX = 400.0f;
volatile float SPD_OUTPUT_MAX = 1000.0f;
volatile float SPD_D_FILTER_ALPHA = 0.80f;


/* 左右轮必须使用独立控制器状态(串级最内环共用) */
PID_Controller speed_left_pid;
PID_Controller speed_right_pid;


/*
 * ============================================================
 *  速度 -> PWM 前馈
 * ============================================================
 *  拟合关系: speed = 1.364 * PWM - 18
 *  扩展为带符号前馈：正目标 -> 正 PWM，负目标 -> 负 PWM。
 */
static float SpeedToPWM(float speed_set)
{
    float abs_speed = fabsf(speed_set);
    float pwm_abs;

    if (abs_speed < 0.5f) {
        return 0.0f;
    }

    pwm_abs = (abs_speed + 18.0f) / 1.364f;
    pwm_abs = PID_LimitFloat(pwm_abs, 0.0f, PID_PositiveLimit(SPD_OUTPUT_MAX));

    /* 低速段前馈线性淡出，避免电机死区附近前馈跳变 */
    if (abs_speed < 20.0f) {
        pwm_abs *= abs_speed / 20.0f;
    }

    return (speed_set > 0.0f) ? pwm_abs : -pwm_abs;
}


/* ============================================================
 *  初始化 / 参数同步 / 复位
 * ============================================================ */

void SpeedPID_Init(PID_Controller *controller)
{
    if (controller == NULL) {
        return;
    }
    PID_Init(controller,
             Kp_spd, Ki_spd, Kd_spd,
             PID_CONTROL_DT,
             SPD_I_MAX, SPD_OUTPUT_MAX, SPD_D_FILTER_ALPHA);
}

void SpeedPIDParameters(PID_Controller *controller)
{
    if (controller == NULL) {
        return;
    }
    PID_SetParameters(controller,
                     Kp_spd, Ki_spd, Kd_spd,
                     PID_CONTROL_DT,
                     SPD_I_MAX, SPD_OUTPUT_MAX, SPD_D_FILTER_ALPHA);
}

void SpeedPID_Reset(PID_Controller *controller)
{
    PID_Reset(controller);
}


/*
 * ============================================================
 *  速度 -> PWM
 * ============================================================
 */
int32_t SpeedPID_Update(PID_Controller *controller,
                        float speed_set,
                        int32_t encoder_speed,
                        int32_t encoder_dir)
{
    float speed_actual;
    float pwm_feedforward;
    float output;

    if (controller == NULL) {
        return 0;
    }

    PID_ModuleInit();

    /* 保证车辆向前运动时，修正后的编码器速度为正 */
    speed_actual = (float)encoder_speed * (float)encoder_dir;

    /* 目标和实际速度都接近 0 时清状态，防止停车后残留积分/微分 */
    if (fabsf(speed_set) < 0.5f && fabsf(speed_actual) < 1.0f) {
        SpeedPID_Reset(controller);
        return 0;
    }

    SpeedPIDParameters(controller);

    pwm_feedforward = SpeedToPWM(speed_set);

    output = PID_Update(controller, speed_set, speed_actual, pwm_feedforward);

    /* 四舍五入成整数 PWM */
    return (int32_t)(output >= 0.0f ? output + 0.5f : output - 0.5f);
}