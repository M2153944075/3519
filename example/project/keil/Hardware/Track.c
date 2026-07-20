#include "Track.h"

#include "Encoder.h"
#include "Grayscale.h"
#include "GrayscalePID.h"
#include "JY901.h"
#include "Motor.h"
#include "SpeedPID.h"
#include "YawPID.h"

/* Track_Control() 由 board_update() 每 10 ms 调用一次。 */
#define TRACK_CONTROL_PERIOD_MS       10u
#define TRACK_STRAIGHT_TIME_MS        300u
#define TRACK_STRAIGHT_CYCLES         (TRACK_STRAIGHT_TIME_MS / TRACK_CONTROL_PERIOD_MS)
#define TRACK_MARKER_CONFIRM_CYCLES   1u

/* 使两轮前进方向均为正的编码器方向系数。 */
#define TRACK_LEFT_ENCODER_DIR        1
#define TRACK_RIGHT_ENCODER_DIR       1

#define GRAY_LEFT_MARKER_MASK         0x06u /* L2 + L1（bit1、bit2） */
#define GRAY_RIGHT_MARKER_MASK        0x60u /* R1 + R2（bit5、bit6） */
#define GRAY_CENTER_MASK              0x18u /* ML 或 MR（bit3、bit4） */

/* JY901 偏航角符号因安装方式而异。按当前电机约定，负的轮速差为左转。
 * 若 IMU 上报的偏航方向相反，则同时修改下面两个符号。 */
#define TRACK_LEFT_TURN_YAW_SIGN      (1.0f)
#define TRACK_RIGHT_TURN_YAW_SIGN     (-1.0f)


#define TRACK_SIDE_NONE               0u
#define TRACK_SIDE_LEFT               1u
#define TRACK_SIDE_RIGHT              2u

typedef enum {
    TRACK_STATE_NORMAL = 0,
    TRACK_STATE_MARKER_STRAIGHT,
    TRACK_STATE_TURN_LEFT,
    TRACK_STATE_TURN_RIGHT
} Track_State;

static Track_State g_state;
static uint8_t g_marker_side;
static uint8_t g_marker_armed;
static uint8_t g_marker_candidate_side;
static uint8_t g_marker_confirm_cycles;
static uint16_t g_straight_cycles;
static uint16_t g_turn_cycles;
static int16_t g_last_weighted_sum;
static uint32_t g_active_base_speed;

static int32_t Track_LimitTarget(float target)
{
    if (target > (float)MOTOR_SPEED_MAX) {
        target = (float)MOTOR_SPEED_MAX;
    } else if (target < -(float)MOTOR_SPEED_MAX) {
        target = -(float)MOTOR_SPEED_MAX;
    }
    return (int32_t)(target >= 0.0f ? target + 0.5f : target - 0.5f);
}

/* 外环只生成左右轮目标速度。下面所有路径（包括标记直行和陀螺仪转弯），
 * 在写入 PWM 之前都要经过这两个内环速度控制器。 */
static void Track_DriveWheelTargets(float left_target, float right_target)
{
    int32_t left_pwm;
    int32_t right_pwm;

    left_target = (float)Track_LimitTarget(left_target);
    right_target = (float)Track_LimitTarget(right_target);

    left_pwm = SpeedPID_Update(&speed_left_pid,
                               left_target,
                               Encoder_GetSpeed(1u),
                               TRACK_LEFT_ENCODER_DIR);
    right_pwm = SpeedPID_Update(&speed_right_pid,
                                right_target,
                                Encoder_GetSpeed(2u),
                                TRACK_RIGHT_ENCODER_DIR);
    Motor_Drive(left_pwm, right_pwm);
}

static void Track_ResetControlLoops(void)
{
    Gray_Reset();
    SpeedPID_Reset(&speed_left_pid);
    SpeedPID_Reset(&speed_right_pid);
}

static uint8_t Track_IsCenter(uint8_t gray)
{
    return (uint8_t)((gray & GRAY_CENTER_MASK) != 0u);
}

static uint8_t Track_IsLeftMarker(uint8_t gray)
{
    return (uint8_t)(gray == GRAY_LEFT_MARKER_MASK);
}

static uint8_t Track_IsRightMarker(uint8_t gray)
{
    return (uint8_t)(gray == GRAY_RIGHT_MARKER_MASK);
}

static uint8_t Track_ConfirmMarker(uint8_t gray)
{
    uint8_t side = TRACK_SIDE_NONE;

    if (g_marker_armed == 0u) {
        g_marker_candidate_side = TRACK_SIDE_NONE;
        g_marker_confirm_cycles = 0u;
        return TRACK_SIDE_NONE;
    }

    if (Track_IsLeftMarker(gray)) {
        side = TRACK_SIDE_LEFT;
    } else if (Track_IsRightMarker(gray)) {
        side = TRACK_SIDE_RIGHT;
    }

    if (side == TRACK_SIDE_NONE) {
        g_marker_candidate_side = TRACK_SIDE_NONE;
        g_marker_confirm_cycles = 0u;
        return TRACK_SIDE_NONE;
    }

    if (side != g_marker_candidate_side) {
        g_marker_candidate_side = side;
        g_marker_confirm_cycles = 1u;
        return TRACK_SIDE_NONE;
    }

    if (g_marker_confirm_cycles < TRACK_MARKER_CONFIRM_CYCLES) {
        ++g_marker_confirm_cycles;
    }

    if (g_marker_confirm_cycles >= TRACK_MARKER_CONFIRM_CYCLES) {
        g_marker_candidate_side = TRACK_SIDE_NONE;
        g_marker_confirm_cycles = 0u;
        return side;
    }
    return TRACK_SIDE_NONE;
}

static void Track_BeginTurn(void)
{
    float yaw_now = JY901_GetAngle(2u);
    float sign = (g_marker_side == TRACK_SIDE_LEFT) ?
                 TRACK_LEFT_TURN_YAW_SIGN : TRACK_RIGHT_TURN_YAW_SIGN;

    /* 记录直行段末端的角度。YawAngle_SetTarget 会把目标角归一化，
     * 并为这次新转弯复位角度环状态。 */
    YawAngle_SetTarget(yaw_now + sign * TRACK_TURN_ANGLE_DEG);
    YawRate_Reset();
    g_turn_cycles = 0u;
    g_state = (g_marker_side == TRACK_SIDE_LEFT) ?
              TRACK_STATE_TURN_LEFT : TRACK_STATE_TURN_RIGHT;
    Track_ResetControlLoops();
}

static void Track_RunPreciseTurn(uint8_t gray)
{
    float rate_target;
    float turn;

    /* 精确对准阶段：此处刻意不调用循迹 PD。角度环产生偏航角速度目标，
     * 角速度环产生左右轮速度差，内环轮速 PID 产生 PWM。 */
    rate_target = YawAngle_Update(JY901_GetAngle(2u));
    turn = YawRate_Update(rate_target, JY901_GetGyro(2u));
    Track_DriveWheelTargets(turn, -turn);

    ++g_turn_cycles;
    /* 检测到中线即请求的交接条件。要求完成一拍控制采样，可避免状态切换
     * 瞬间残留的中线电平立刻取消本次转弯。 */
    if (g_turn_cycles > 1u && Track_IsCenter(gray)) {
        g_state = TRACK_STATE_NORMAL;
        g_marker_side = TRACK_SIDE_NONE;
        g_last_weighted_sum = 0;
        Track_ResetControlLoops();
        Motor_Stop();
    }
}

void Track_Init(void)
{
    g_state = TRACK_STATE_NORMAL;
    g_marker_side = TRACK_SIDE_NONE;
    g_marker_armed = 1u;
    g_marker_candidate_side = TRACK_SIDE_NONE;
    g_marker_confirm_cycles = 0u;
    g_straight_cycles = 0u;
    g_turn_cycles = 0u;
    g_last_weighted_sum = 0;
    g_active_base_speed = TRACK_BASE_SPEED;
    Track_ResetControlLoops();
    YawAngle_Reset();
    YawRate_Reset();
}

void Track_Update(void)
{
    Track_Control(g_active_base_speed);
}

void Track_OnLostLine(void)
{
    float turn = Gray_PD_Update((float)g_last_weighted_sum);
    Track_DriveWheelTargets((float)g_active_base_speed + turn,
                            (float)g_active_base_speed - turn);
}

void Track_Control(uint32_t base_speed)
{
    uint8_t gray;
    uint8_t confirmed_marker;
    float turn;

    g_active_base_speed = base_speed;
    gray = Grayscale_Read();

    if (g_state == TRACK_STATE_MARKER_STRAIGHT) {
        /* 标记已记录；在短距离直行期间关闭循迹 PD。 */
        Track_DriveWheelTargets((float)base_speed, (float)base_speed);
        if (++g_straight_cycles >= TRACK_STRAIGHT_CYCLES) {
            g_straight_cycles = 0u;
            Track_BeginTurn();
        }
        return;
    }

    if (g_state == TRACK_STATE_TURN_LEFT || g_state == TRACK_STATE_TURN_RIGHT) {
        Track_RunPreciseTurn(gray);
        return;
    }

    /* 仅在标记对失活后再次使能。 */
    if ((gray & (GRAY_LEFT_MARKER_MASK | GRAY_RIGHT_MARKER_MASK)) == 0u) {
        g_marker_armed = 1u;
    }

    /* 左右标记同时出现存在歧义（例如十字路口），不能据此意外选定转向。 */
    confirmed_marker = Track_ConfirmMarker(gray);
    if (confirmed_marker != TRACK_SIDE_NONE) {
        g_marker_side = confirmed_marker;
        g_marker_armed = 0u;
        g_straight_cycles = 0u;
        g_state = TRACK_STATE_MARKER_STRAIGHT;
        Track_ResetControlLoops();
        Track_DriveWheelTargets((float)base_speed, (float)base_speed);
        return;
    }

    if (gray == 0u) {
        Track_OnLostLine();
        return;
    }

    {
        int16_t weighted_sum = Grayscale_WeightedSum(gray);
        turn = Gray_PD_Update((float)weighted_sum);
        if (weighted_sum != 0) {
            g_last_weighted_sum = weighted_sum;
        }
    }

    Track_DriveWheelTargets((float)base_speed + turn,
                            (float)base_speed - turn);
}

uint8_t Track_GetState(void)
{
    return (uint8_t)g_state;
}

uint8_t Track_GetMarkerSide(void)
{
    return g_marker_side;
}
