#include "Cascade.h"

#include "Encoder.h"
#include "Grayscale.h"
#include "GrayscalePID.h"
#include "Motor.h"
#include "SpeedPID.h"
#include "YawPID.h"

#include <math.h>

#define ENC_LEFT_DIR  1
#define ENC_RIGHT_DIR 1

static float s_rate_target;
static float s_turn_output;

static void DriveWheelTargets(float left_speed_set, float right_speed_set)
{
    int32_t left_pwm = SpeedPID_Update(&speed_left_pid, left_speed_set,
                                       Encoder_GetSpeed(1u), ENC_LEFT_DIR);
    int32_t right_pwm = SpeedPID_Update(&speed_right_pid, right_speed_set,
                                        Encoder_GetSpeed(2u), ENC_RIGHT_DIR);
    Motor_Drive(left_pwm, right_pwm);
}

static void DriveWheels(float base_speed, float turn)
{
    DriveWheelTargets(base_speed + turn, base_speed - turn);
}

void Cascade_WheelSpeed_PWM(float left_speed_set, float right_speed_set)
{
    s_rate_target = 0.0f;
    s_turn_output = (left_speed_set - right_speed_set) * 0.5f;
    DriveWheelTargets(left_speed_set, right_speed_set);
}

float Cascade_AngleRateSpeed_PWM(float base_speed,
                                 float angle_now,
                                 float gyro_now)
{
    float rate = YawAngle_Update(angle_now);
    float turn = YawRate_Update(rate, gyro_now);

    s_rate_target = rate;
    s_turn_output = turn;
    DriveWheels(base_speed, turn);
    return turn;
}

float Cascade_AngleSpeed_PWM(float base_speed, float angle_now)
{
    float turn = Angle_To_Speed(angle_now);

    s_rate_target = 0.0f;
    s_turn_output = turn;
    DriveWheels(base_speed, turn);
    return turn;
}

int32_t Cascade_Speed_PWM(float speed_set)
{
    s_turn_output = 0.0f;
    s_rate_target = 0.0f;
    DriveWheels(speed_set, 0.0f);
    return 0;
}

int32_t Cascade_Track_PWM(float base_speed, float weighted_sum)
{
    float turn = Gray_PD_Update(weighted_sum);

    s_turn_output = turn;
    s_rate_target = 0.0f;
    DriveWheels(base_speed, turn);
    return 0;
}

uint8_t Cascade_TrackStep(float base_speed)
{
    uint8_t gray = Grayscale_Read();

    if (gray == 0u) {
        return 1u;
    }
    Cascade_Track_PWM(base_speed, (float)Grayscale_WeightedSum(gray));
    return 0u;
}

void Cascade_Reset(void)
{
    YawAngle_Reset();
    YawRate_Reset();
    Gray_Reset();
    SpeedPID_Reset(&speed_left_pid);
    SpeedPID_Reset(&speed_right_pid);
    s_rate_target = 0.0f;
    s_turn_output = 0.0f;
}

uint8_t Cascade_IsStable(float gyro_z_dps,
                          float angle_tolerance_deg,
                          float rate_tolerance_dps)
{
    if (angle_tolerance_deg < 0.0f) {
        angle_tolerance_deg = -angle_tolerance_deg;
    }
    if (rate_tolerance_dps < 0.0f) {
        rate_tolerance_dps = -rate_tolerance_dps;
    }
    return (uint8_t)(fabsf(YawAngle_GetError()) <= angle_tolerance_deg &&
                     fabsf(gyro_z_dps) <= rate_tolerance_dps);
}

float Cascade_GetRateTarget(void)
{
    return s_rate_target;
}

float Cascade_GetTurnOutput(void)
{
    return s_turn_output;
}
