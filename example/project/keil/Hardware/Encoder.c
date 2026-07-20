#include <stdint.h>
#include "ti_msp_dl_config.h"
#include "Encoder.h"


#define ENCODER_NUM 2

//正转+1 反转-1

#define ENCODER_DIR_LOGIC(b) ((b) ? 1 : -1)

/* 注意：所有会被中断修改或异步访问的成员都加上了 volatile */
typedef struct {
    volatile int32_t location;       // 累计位置
    volatile int32_t speed_cnt;      // 上次周期内的脉冲增量
    volatile int32_t last_location;  // 上次采样位置
    volatile uint8_t  dir;           // 当前方向: 1=正转, 0=反转
} Encoder_TypeDef;

static Encoder_TypeDef Encoder[ENCODER_NUM];

/* ========== 诊断探针: 编码器 GPIO 边沿中断进次数 ===========
 * 放这里方便主循环 extern 读取. 命中一次 +1. */
volatile uint32_t g_enc_isr = 0;

#define E1B_READ()   (DL_GPIO_readPins(GPIOB, TB_E1B_PIN) ? 1 : 0)
#define E2B_READ()   (DL_GPIO_readPins(GPIOB, TB_E2B_PIN) ? 0 : 1)

/*----------------------------------------------------------------------------
 * 初始化
 *----------------------------------------------------------------------------*/
void Encoder_Init(void)
{
    uint8_t i;
    for (i = 0; i < ENCODER_NUM; i++) {
        Encoder[i].location      = 0;
        Encoder[i].speed_cnt     = 0;
        Encoder[i].last_location = 0;
        Encoder[i].dir           = 0;
    }

    DL_GPIO_clearInterruptStatus(GPIOB, TB_E1A_PIN | TB_E2A_PIN);
    NVIC_ClearPendingIRQ(TB_INT_IRQN);
    NVIC_EnableIRQ(TB_INT_IRQN);
}

/*----------------------------------------------------------------------------
 * 设置当前位置（临界区保护）
 *----------------------------------------------------------------------------*/
void Encoder_SetLocation(uint8_t n, int32_t Location)
{
    if (n == 0 || n > ENCODER_NUM) return;

    Encoder[n - 1].location      = Location;
    Encoder[n - 1].last_location = Location;

}

/*----------------------------------------------------------------------------
 * 读取当前位置（volatile 保证从内存直接读）
 *----------------------------------------------------------------------------*/
int32_t Encoder_GetLocation(uint8_t n)
{
    if (n == 0 || n > ENCODER_NUM) return 0;
    return Encoder[n - 1].location;
}

/*----------------------------------------------------------------------------
 * 读取当前方向
 *----------------------------------------------------------------------------*/
uint8_t Encoder_GetDirection(uint8_t n)
{
    if (n == 0 || n > ENCODER_NUM) return 0;
    return Encoder[n - 1].dir;
}

/*----------------------------------------------------------------------------
 * 更新所有编码器的速度值（在定时中断中调用）
 * 已通过关中断保证数据一致性
 *----------------------------------------------------------------------------*/
void Encoder_Update(void)
{
    uint8_t i;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    for (i = 0; i < ENCODER_NUM; i++) {
        int32_t cur = Encoder[i].location;
        Encoder[i].speed_cnt = cur - Encoder[i].last_location;
        Encoder[i].last_location = cur;
    }

    if (!primask) __enable_irq();
}

/*----------------------------------------------------------------------------
 * 获取最近一次 Update 后的速度脉冲数
 *----------------------------------------------------------------------------*/
int32_t Encoder_GetSpeed(uint8_t n)
{
    if (n == 0 || n > ENCODER_NUM) return 0;
    return Encoder[n - 1].speed_cnt;
}

/*----------------------------------------------------------------------------
 * MSPM0G3507 的 GPIO 中断只有 GROUP0(GPIOA低16位) 和 GROUP1 两个向量,
 * 且 GPIOA_INT_IRQn == GPIOB_INT_IRQn == 1, 即 GPIOA高16位 与 GPIOB 全部
 * 共用同一个 GROUP1_IRQHandler 向量.
 *
 * 本设计中 E1A(PB10) 与 E2A(PB15) 都位于 GPIOB 低16位, 两者共用 GPIOB
 * 中断(TB_INT_IRQN), 一并落在 GROUP1_IRQHandler. 每次中断进来用
 * DL_GPIO_getPendingInterrupt(GPIOB) 查出是哪个 idx(E1A 还是 E2A),
 * 再读对应方向引脚 E1B(PB11)/E2B(PB16) 决定正反转, 累计到 location.
 * 每次处理完必须手动清除对应引脚的中断标志！
 *----------------------------------------------------------------------------*/
void GROUP1_IRQHandler(void)
{
    g_enc_isr++;   /* 诊断探针: 每次进入 GPIOB 中断计数 */
    switch (DL_GPIO_getPendingInterrupt(GPIOB)) {
        case TB_E1A_IIDX: {
            int8_t d = (int8_t)ENCODER_DIR_LOGIC(E1B_READ());
            Encoder[0].dir = (d > 0) ? 1 : 0;
            Encoder[0].location += d;
            DL_GPIO_clearInterruptStatus(GPIOB, TB_E1A_PIN);
            break;
        }
        case TB_E2A_IIDX: {
            int8_t d = (int8_t)ENCODER_DIR_LOGIC(E2B_READ());
            Encoder[1].dir = (d > 0) ? 1 : 0;
            Encoder[1].location += d;
            DL_GPIO_clearInterruptStatus(GPIOB, TB_E2A_PIN);
            break;
        }
        default:
            break;
    }
}