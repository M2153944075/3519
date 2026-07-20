#include "Motor.h"

static volatile int32_t g_last_left_output;
static volatile int32_t g_last_right_output;

/* 边沿对齐向下计数、LOAD = MOTOR_SPEED_MAX - 1 = 999。
   比较值 = LOAD 时占空比 ~0%（真停止），= 0 时满占空比。
   speed=0 必须映射到 999：若写成 1000，硬件永不匹配会输出恒高（满速）。 */
static uint32_t Motor_LimitSpeed(uint32_t speed)
{
    if (speed > MOTOR_SPEED_MAX) {
        speed = MOTOR_SPEED_MAX;
    }
    if (speed == 0u) {
        return MOTOR_SPEED_MAX - 1u;
    }
    return MOTOR_SPEED_MAX - speed;
}

void MotorA_Control(uint8_t dir, uint32_t speed)
{
    speed = Motor_LimitSpeed(speed);

    if (dir != 0u) {
        AIN1_OUT(0);
        AIN2_OUT(1);
    } else {
        AIN1_OUT(1);
        AIN2_OUT(0);
    }

    /* 当前PWM为边沿对齐、周期1000、比较值999时占空比约0%（停止）。 */
    DL_TimerA_setCaptureCompareValue(
        PWMA_INST, speed, GPIO_PWMA_C3_IDX);
}

void MotorB_Control(uint8_t dir, uint32_t speed)
{
    speed = Motor_LimitSpeed(speed);

    if (dir != 0u) {
        BIN1_OUT(1);
        BIN2_OUT(0);
    } else {
        BIN1_OUT(0);
        BIN2_OUT(1);
    }

    DL_TimerG_setCaptureCompareValue(
        PWMB_INST, speed, GPIO_PWMB_C1_IDX);
}

void Motor_Drive(int32_t v_left, int32_t v_right)
{
    uint32_t left_abs;
    uint32_t right_abs;
    uint32_t max_abs;

    left_abs = (v_left < 0) ? (uint32_t)(-(int64_t)v_left) : (uint32_t)v_left;
    right_abs = (v_right < 0) ? (uint32_t)(-(int64_t)v_right) : (uint32_t)v_right;
    max_abs = (left_abs > right_abs) ? left_abs : right_abs;

    /* 等比例限幅，避免弯道时一侧饱和破坏差速比例。 */
    if (max_abs > MOTOR_SPEED_MAX) {
        left_abs = (left_abs * MOTOR_SPEED_MAX) / max_abs;
        right_abs = (right_abs * MOTOR_SPEED_MAX) / max_abs;
    }

    g_last_left_output = (v_left < 0) ? -(int32_t)left_abs : (int32_t)left_abs;
    g_last_right_output = (v_right < 0) ? -(int32_t)right_abs : (int32_t)right_abs;

    MotorA_Control((v_left >= 0) ? 1u : 0u, left_abs);
    MotorB_Control((v_right >= 0) ? 1u : 0u, right_abs);
}

void Motor_Stop(void)
{
    MotorA_Control(1u, 0u);
    MotorB_Control(1u, 0u);
    g_last_left_output = 0;
    g_last_right_output = 0;
}

int32_t Motor_GetLastLeftOutput(void) { return g_last_left_output; }
int32_t Motor_GetLastRightOutput(void) { return g_last_right_output; }

/* ============================================================
 *  电机转动状态实现
 * ============================================================
 *  利用 Motor_Drive() 差速驱动, 该函数自带等比例限幅.
 */

/* 直走: 两轮同速正转 */
void Motor_Forward(uint32_t speed)
{
    Motor_Drive((int32_t)speed, (int32_t)speed);
}

/* 左转: 左轮反转 + 右轮正转 (原地差速左转) */
void Motor_TurnLeft(uint32_t speed)
{
    /* 左轮反转 60% 降冲击, 右轮全速 → 车体向左旋转 */
    Motor_Drive((int32_t)(speed * 3 / 5), -(int32_t)speed);
}

/* 右转: 左轮正转 + 右轮反转 (原地差速右转) */
void Motor_TurnRight(uint32_t speed)
{
    /* 左轮全速, 右轮反转 60% → 车体向右旋转 */
    Motor_Drive(-(int32_t)speed, (int32_t)(speed * 3 / 5));
}

/* 稍左转: 左轮降速 ~40%, 右轮全速 (小角度左修正, 巡线微调) */
void Motor_SlightLeft(uint32_t speed)
{
    /* 差速比 2:5 → 车体轻微偏左前进 */
    Motor_Drive((int32_t)(speed * 2 / 5), (int32_t)speed);
}

/* 稍右转: 左轮全速, 右轮降速 ~40% (小角度右修正, 巡线微调) */
void Motor_SlightRight(uint32_t speed)
{
    /* 差速比 5:2 → 车体轻微偏右前进 */
    Motor_Drive((int32_t)speed, (int32_t)(speed * 2 / 5));
}

/* 急左转: 左轮全速反转, 右轮全速正转 (快速旋转左转) */
void Motor_SharpLeft(uint32_t speed)
{
    Motor_Drive(-(int32_t)speed, (int32_t)speed);
}

/* 急右转: 左轮全速正转, 右轮全速反转 (快速旋转右转) */
void Motor_SharpRight(uint32_t speed)
{
    Motor_Drive((int32_t)speed, -(int32_t)speed);
}
