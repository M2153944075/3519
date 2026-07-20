#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <string.h>
#include "JY901.h"
#include "UART.h"

struct STime stcTime;           //时间（年、月、日、时、分、秒、毫秒）
struct SAcc stcAcc;				//加速度（x, y, z 轴，以及温度 T）
struct SGyro stcGyro;			//角速度（x, y, z 轴，以及温度 T）
struct SAngle stcAngle;			//角度/姿态角（横滚角 Roll，俯仰角 Pitch，偏航角 Yaw，以及温度 T）
struct SMag stcMag;				//磁场强度（x, y, z 轴，以及温度 T）
struct SDStatus stcDStatus;		//数字IO状态（4个引脚的状态）
struct SPress stcPress;			//气压和高度（气压值，海拔高度）
struct SLonLat stcLonLat;		//经度和纬度（GPS数据）
struct SGPSV stcGPSV;			//GPS速度/高度（GPS高度，GPS航向角，GPS速度）
struct SQ stcQ;					//四元数（姿态解算的4个元素 q0, q1, q2, q3）


/* 解析后的浮点结果，供外部读取 */
static float g_acc_g[3], g_gyro_dps[3], g_angle_deg[3];

/* 每类帧各自独立的就绪标志：收到该帧即置位，ProcessData 按位消费后再清。
 * 旧实现里三组数据共用一个 g_data_ready，它只在 0x53(角度)帧置位，导致
 * 加速度/角速度的浮点刷新频率被强制绑到角度帧，0x51/0x52 帧的更新被跳过。
 * 拆成按帧就绪后，每类数据各自按自身帧率独立刷新。                          */
static volatile uint8_t g_ready_acc;
static volatile uint8_t g_ready_gyro;
static volatile uint8_t g_ready_angle;

float JY901_GetAcc (uint8_t i) { return g_acc_g[i];     }
float JY901_GetGyro(uint8_t i) { return g_gyro_dps[i];   }
float JY901_GetAngle(uint8_t i){ return g_angle_deg[i];  }

/* 把环形 FIFO 里积攒的字节逐字节喂进 JY901_Data() 解析。
 * UART7 已改用 DMA 接收 + 软件环形 FIFO（见 UART.c），JY901 不再在串口
 * 中断里解析；改为在主循环/调度里主动调用本函数拉取并解析。
 * 一次性尽量排空 FIFO，但为防 IRQ 中又压进来字节导致本函数无限循环，
 * 用入口时的 available 数做上限。                                            */
void JY901_PollRx(void)
{
    uint16_t n = UART7_RX_Available();
    while (n--) {
        uint8_t ok;
        uint8_t b = UART7_RX_Read(&ok);
        if (!ok) break;
        JY901_Data(b);
    }
}

/* 只在主循环中调用，把原始数据转成浮点——不在 ISR 里做浮点运算。
 * 三类帧各自独立判断就绪位，避免把加速度/角速度刷新绑到角度帧。       */
void JY901_ProcessData(void)
{
    uint8_t ready_acc;
    uint8_t ready_gyro;
    uint8_t ready_angle;
    int i;

    __disable_irq();
    ready_acc   = g_ready_acc;
    ready_gyro  = g_ready_gyro;
    ready_angle = g_ready_angle;
    g_ready_acc   = 0u;
    g_ready_gyro  = 0u;
    g_ready_angle = 0u;
    __enable_irq();

    if (ready_acc) {
        for (i = 0; i < 3; i++) {
            g_acc_g[i] = stcAcc.a[i] / 32768.0f * 16.0f;
        }
    }
    if (ready_gyro) {
        for (i = 0; i < 3; i++) {
            g_gyro_dps[i] = stcGyro.w[i] / 32768.0f * 2000.0f;
        }
    }
    if (ready_angle) {
        for (i = 0; i < 3; i++) {
            g_angle_deg[i] = stcAngle.Angle[i] / 32768.0f * 180.0f;
        }
    }
}

/* 每帧 11 字节：0x55, type[0x50~0x59], 8B data, 1B sum。
   逐字节喂入。丢字节/OVERRUN 后靠"中途遇 0x55 即重开一帧"快速重同步，
   避免老实现里 ucRxCnt 卡在中间、要等很久才偶然对齐导致角度长时间不更新。 */
void JY901_Data(uint8_t ucData)
{
    static uint8_t ucRxBuffer[11];
    static uint8_t ucRxCnt = 0;

    /* 0x55 是唯一的帧头。任何时候收到都强制重开一帧；
       这就解决了"流中间丢字节后 ucRxCnt 卡死在半截、长时间不恢复"的问题。 */
    if (ucData == 0x55u) {
        ucRxCnt = 0;
        ucRxBuffer[ucRxCnt++] = ucData;
        return;
    }

    /* 还没拿到帧头，丢弃静默期内的非 0x55 字节 */
    if (ucRxCnt == 0u) {
        return;
    }

    /* 饱和保护：任何意外下 ucRxCnt 不越界。照理走不到这里，防御性写。 */
    if (ucRxCnt >= sizeof(ucRxBuffer)) {
        ucRxCnt = 0;
        return;
    }

    ucRxBuffer[ucRxCnt++] = ucData;

    /* 第二字节必须是合法 type，否则当前半截帧作废，等下一个 0x55 */
    if (ucRxCnt == 2u && (ucRxBuffer[1] < 0x50u || ucRxBuffer[1] > 0x59u)) {
        ucRxCnt = 0;
        return;
    }

    if (ucRxCnt < 11u) {
        return;
    }

    /* 满帧，做校验 */
    uint8_t sum = 0;
    for (uint8_t i = 0u; i < 10u; i++) {
        sum = (uint8_t)(sum + ucRxBuffer[i]);
    }
    ucRxCnt = 0;                    /* 无论校验通过与否，本帧已结束，准备收下一帧 */
    if (sum != ucRxBuffer[10]) {
        return;                     /* 校验失败，丢包 */
    }

    switch (ucRxBuffer[1]) {
        case 0x50: memcpy(&stcTime,    &ucRxBuffer[2], 8); break;
        case 0x51: memcpy(&stcAcc,     &ucRxBuffer[2], 8); g_ready_acc   = 1u; break;
        case 0x52: memcpy(&stcGyro,    &ucRxBuffer[2], 8); g_ready_gyro  = 1u; break;
        case 0x53: memcpy(&stcAngle,   &ucRxBuffer[2], 8); g_ready_angle = 1u; break;
        case 0x54: memcpy(&stcMag,     &ucRxBuffer[2], 8); break;
        case 0x55: memcpy(&stcDStatus, &ucRxBuffer[2], 8); break;
        case 0x56: memcpy(&stcPress,   &ucRxBuffer[2], 8); break;
        case 0x57: memcpy(&stcLonLat,  &ucRxBuffer[2], 8); break;
        case 0x58: memcpy(&stcGPSV,    &ucRxBuffer[2], 8); break;
        case 0x59: memcpy(&stcQ,       &ucRxBuffer[2], 8); break;
    }
}

uint8_t a = 0;

/* 注：UART7 接收已改由 DMA 完成，DMA 通道1(CH1) 收满一批后触发
 * 统一 DMA_INT_IRQn -> DMA_IRQHandler (在 UART.c) 把字节压环形 FIFO。
 * JY901 不再在串口中断里收数据，改在主循环中调用 JY901_PollRx() 拉取并解析。
 * 原先在这里的 UART7_IRQHandler 已删除。                                     */
