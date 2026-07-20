#include "GrayscalePID.h"

#include <stddef.h>


/* ==================== 循迹 PD 环参数 ====================
 *  输入为灰度加权和(目标 0)，输出为左右轮目标速度差 turn。
 *  PD 环不加积分，Ki 固定为 0；输出限幅复用 TURN_SPEED_MAX。
 *  加权和 > 0 表示黑线偏右 -> 期望 turn < 0(车右转) 把线拉回中线。
 */
volatile float Kp_gray = 2.3f;
volatile float Ki_gray = 0.0f;
volatile float Kd_gray = 0.0f;

volatile float GRAY_I_MAX = 0.0f;
volatile float GRAY_D_FILTER_ALPHA = 0.80f;


/* 循迹 PD 环：模块内单例 */
static PID_Controller gray_pid;

/* 循迹 PD 环：目标恒为 0(加权和=0 表示线在中线)，保存最近误差供调试 */
static const float gray_target = 0.0f;
static float gray_error = 0.0f;


/* ============================================================
 *  初始化 / 参数同步
 * ============================================================ */

void GrayPID_Init(void)
{
    PID_Init(&gray_pid,
             Kp_gray, Ki_gray, Kd_gray,
             PID_CONTROL_DT,
             GRAY_I_MAX, TURN_SPEED_MAX, GRAY_D_FILTER_ALPHA);
}

void GrayPIDParameters(void)
{
    PID_SetParameters(&gray_pid,
                     Kp_gray, Ki_gray, Kd_gray,
                     PID_CONTROL_DT,
                     GRAY_I_MAX, TURN_SPEED_MAX, GRAY_D_FILTER_ALPHA);
}


/* ============================================================
 *  状态管理
 * ============================================================ */

void Gray_Reset(void)
{
    PID_ModuleInit();
    PID_Reset(&gray_pid);
    gray_error = 0.0f;
}

float Gray_GetTarget(void)
{
    return gray_target;
}

float Gray_GetError(void)
{
    return gray_error;
}


/* ============================================================
 *  循迹 PD：灰度加权和 -> 左右轮目标速度差
 * ============================================================ */
float Gray_PD_Update(float weighted_sum)
{
    PID_ModuleInit();
    GrayPIDParameters();

    gray_error = Gray_GetTarget() - weighted_sum;

    return PID_UpdateError(&gray_pid, gray_error, 0.0f);
}
