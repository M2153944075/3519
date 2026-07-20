#include "ti_msp_dl_config.h"
#include "UART.h"

void UART_Init(void)
{
    /* 清除并使能相关中断标志。
     *   UART1：蓝牙 RX 仍走 RX 中断；
     *   UART7：已改用 DMA 接收，传统 RX 中断路径不再使用——外设 RX 中断使能
     *           已在 ti_msp_dl_config.c 里关掉，这里再在 NVIC 层清一次
     *           UART7 pending 并保持其 NVIC 关闭，双保险避免 UART7 handler
     *           (已从 JY901.c 删除) 被触发而落到 Default_Handler 卡板。
     *   DMA 完成中断走统一的 DMA_INT_IRQn 向量 => 由本文件的
     *           DMA_IRQHandler 把收到的一批字节冲进软件环形 FIFO。            */
    NVIC_ClearPendingIRQ(UART1_INST_INT_IRQN);
    NVIC_EnableIRQ (UART1_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(UART7_INST_INT_IRQN);   /* 清残留，但不 enable */
    NVIC_DisableIRQ(UART7_INST_INT_IRQN);

    NVIC_ClearPendingIRQ(DMA_INT_IRQn);
    NVIC_EnableIRQ (DMA_INT_IRQn);

    /* 相信 SysConfig 已就位（DMA 通道已 initChannel、CH1 中断使能、统一在
     * ti_msp_dl_config.c 里配过优先级）；这里启动 UART7 RX DMA 接收。 */
    UART7_RX_DMA_Reset();
}


void UART_SendByte(uint8_t n, uint8_t byte)
{
    UART_Regs *uart = NULL;

    switch(n)
    {
        case 1: uart = UART1_INST; break;
        case 7: uart = UART7_INST; break;
        default: return;
    }

    uint32_t timeout = 10000;

    while (DL_UART_isBusy(uart) && --timeout);

    if(timeout == 0)
    {
        DL_UART_reset(uart);
        return;
    }

    DL_UART_Main_transmitData(uart, byte);
}

void UART_SendString(uint8_t n, unsigned char *str)
{
	 while( str && *str )
      {
            UART_SendByte(n, *str++);
      }
}


/* ============================================================================
 *  UART1 蓝牙TX —— DMA 发送 + 软件 FIFO
 *
 *  设计思路：
 *    - 用户调用 UART1_TX_DMASend() 只是把数据塞进环形 FIFO，立即返回（非阻塞）。
 *    - 内部链路 UART1_TX_DMAPump() 在以下三个时机把 FIFO 里的数据搬到真正发出去：
 *        1) DMASend 刚入队后，如果 DMA 当前空闲就立刻起一拍；
 *        2) main 主循环里周期性调用 Pump（DMA 一拍发完可能 FIFO 还有货）；
 *        3) 因此工程主循环里需要周期调用 UART1_TX_DMAPump()（约 1ms 一次即可）。
 *    - DMA 通道采用 SysConfig 生成的 DMA_TxBlue (DMA_CH0, UART1 TX 触发)；
 *      模式为 SINGLE_TRANSFER_MODE，一次 setTransferSize 后由 UART TX 事件逐字节搬运，
 *      传完后channel自动停止。是否传完用 chanSWD/intr raw 位查询，不占用中断。
 *
 *  可重入性：Uart1_TxDMASend 期望被任务上下文调用；Pump 是幂等的，主循环与
 *           起 DMA 的内部逻辑都安全。
 * ========================================================================= */

#define UART1_TX_DMA_CHAN   DMA_TxBlue_CHAN_ID      /* SysConfig: 蓝牙TX DMA 通道号 */

static uint8_t  gUart1TxFIFObuf[UART1_TX_FIFO_SIZE]; /* 环形缓冲 */
static volatile uint16_t gUart1TxHead = 0;           /* FIFO 读指针（Pump取数据发） */
static volatile uint16_t gUart1TxTail = 0;           /* FIFO 写指针（DMASend入队）  */
static volatile uint8_t  gUart1TxBusy  = 0;          /* DMA正在搬：1=忙 0=空闲       */

/* 小批量直发用的静态影子缓冲。
 *   DMA 要求源地址在传输期间稳定；环形 FIFO 在传输过程中可能被新写入覆盖，
 *   故把一次要发的数据拷贝到这块静态 buffer，再交给 DMA 搬运。            */
static uint8_t  gUart1TxShadow[UART1_TX_FIFO_SIZE];
static volatile uint16_t gUart1TxShadowLen = 0;      /* 当前DMA正在搬的有效字节数 */


/* -------- 内部：DMA是否空闲 ----------------------------------------------- */
static inline uint8_t UART1_TX_DMA_IsIdle(void)
{
    /* 通过 RAW 中断状态判断上次DMA传输是否完成。
     *   DMA_TxBlue 在 SysConfig 里已 enableInterrupt，传输完成会置 CH0 raw 位。
     *   该位需要软件清零，没有清之前我们视为"还未确认完成"。                */
    if (DL_DMA_getRawInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0) != 0)
    {
        /* 上一拍已传完，收尾：清标志、释放busy、影子buffer清零 */
        DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL0);
        gUart1TxBusy    = 0;
        gUart1TxShadowLen = 0;
        return 1;
    }
    /* chanSWD / BUSY 位查询：传输正在进行中则非空闲 */
    return (gUart1TxBusy == 0);
}


/* -------- 内部：从FIFO取出数据起一拍DMA ----------------------------------- */
static void UART1_TX_DMA_Kick(void)
{
    uint16_t head, tail, cnt, i;

    /* 临界区：关中断保护 head/tail 快照一致（ ')tail 可能被DMASend改写） */
    __disable_irq();
    head = gUart1TxHead;
    tail = gUart1TxTail;
    cnt  = (uint16_t)(tail - head) & UART1_TX_FIFO_MASK;
    if (cnt == 0)
    {
        __enable_irq();
        return;                       /* FIFO空，没东西可发 */
    }
    if (gUart1TxBusy)
    {
        __enable_irq();
        return;                       /* 上一拍还在发，等它结束 */
    }

    /* 把环形 FIFO 里这一段数据拷到静态影子 buffer，保证 DMA 传输期间源稳定 */
    for (i = 0; i < cnt; i++)
        gUart1TxShadow[i] = gUart1TxFIFObuf[(head + i) & UART1_TX_FIFO_MASK];

    gUart1TxHead       = (uint16_t)((head + cnt) & UART1_TX_FIFO_MASK);
    gUart1TxShadowLen  = cnt;
    gUart1TxBusy       = 1;
    __enable_irq();

    /* 配置并启动DMA：源=影子buffer首字节，大小=要发字节数 */
    DL_DMA_setSrcAddr      (DMA, UART1_TX_DMA_CHAN, (uint32_t)gUart1TxShadow);
    DL_DMA_setDestAddr     (DMA, UART1_TX_DMA_CHAN, (uint32_t)&(UART1_INST->TXDATA));
    DL_DMA_setTransferSize (DMA, UART1_TX_DMA_CHAN, cnt);
    DL_DMA_enableChannel   (DMA, UART1_TX_DMA_CHAN);
}


/* ============================================================================
 *  对外：把数据塞进 FIFO 并触发发送（非阻塞）
 *    data/len：要发的字节，返回实际入队的字节数（FIFO满则丢弃溢出部分）
 * ========================================================================= */
uint16_t UART1_TX_DMASend(const uint8_t *data, uint16_t len)
{
    uint16_t i, head, tail;

    if (data == 0 || len == 0)
        return 0;

    __disable_irq();
    head = gUart1TxHead;
    tail = gUart1TxTail;
    for (i = 0; i < len; i++)
    {
        /* 满判定：(tail+1) == head 即再写会追上读指针 */
        if ((uint16_t)(((tail + 1) & UART1_TX_FIFO_MASK) ^ head) == 0)
            break;                                                    /* FIFO满，停 */
        gUart1TxFIFObuf[tail] = data[i];
        tail = (uint16_t)((tail + 1) & UART1_TX_FIFO_MASK);
    }
    gUart1TxTail = tail;
    __enable_irq();

    /* 入队后尝试立刻起一拍（若DMA正空闲则立刻发出，否则等下次Pump） */
    UART1_TX_DMAPump();
    return i;
}


/* ============================================================================
 *  对外：推进DMA发送。建议在主循环或1ms调度里周期调用。
 *    - 若DMA已完成上一拍 → 收尾并检查FIFO是否还有货，有则起下一拍。
 *    - 幂等：可被多处调用而不会乱。
 * ========================================================================= */
void UART1_TX_DMAPump(void)
{
    /* 先让"上一拍是否完成"的状态刷新一下 */
    UART1_TX_DMA_IsIdle();

    if (gUart1TxBusy == 0)
        UART1_TX_DMA_Kick();
}


/* ============================================================================
 *  UART7 JY901 (陀螺仪) RX —— DMA REPEAT_SINGLE 循环接收 + 软件环形 FIFO
 *
 *  模式：FULL_CH_REPEAT_SINGLE_TRANSFER_MODE（SysConfig 里配）。
 *    - 启动时装填一次：源=UART7->RXDATA(固定)，目的=gUart7RxLineBuf(线性递增)，
 *      DMASZ=BURST。每个 UART7 RX 事件搬一字节；DMASZ 数到 0 置 IRQ，DMA 硬件
 *      自动把 DMASA/DMADA/DMASZ 回卷到初值并保持 enable —— IRQ 里无需重装。
 *    - IRQ 进来瞬间：DMASZ 刚到 0，DMA 已回卷但还在等下一个 RX 事件，没在写；
 *      下一字节要等 ~87us(115200) 才到。IRQ 把 gUart7RxLineBuf[0..15] 这 16 字节
 *      拷进环形 FIFO 只要几 us，远小于 87us，不覆盖。即便偶尔被高优先级抢占
 *      延迟丢一字节，JY901 协议遇 0x55 重开一帧，能自愈。
 *    - 因此 DMA_IRQChannel1Handler 不碰任何 DMA 寄存器，只：① 压 FIFO；② 清 raw。
 *  应用层(JY901) 主动调 UART7_RX_Available()/UART7_RX_Read() 从环形 FIFO 取。
 * ========================================================================= */

#define UART7_RX_DMA_CHAN      DMA_RxJY_COPY_CHAN_ID        /* SysConfig: UART7 RX DMA 通道 (CH1) */

/* 一次 DMA 搬 BURST 字节再中断；取 16 (=UART RX FIFO 深度)，搬运期间不丢字节 */
#define UART7_RX_DMA_BURST     16u

/* 软件环形 FIFO：2 的幂，位掩码取模 */
#define UART7_RX_FIFO_SIZE     256u
#define UART7_RX_FIFO_MASK     (UART7_RX_FIFO_SIZE - 1u)

/* DMA 单块线性接收缓冲：REPEAT_SINGLE 模式下，DMASZ 归零后 DMA 硬件自动
 * 回卷到这块的初值，原地继续写。IRQ 在下一字节到达前(~87us@115200)把这块
 * 16 字节拷进环形 FIFO 即可，时不覆盖（详见上文注释）。                */
static uint8_t  gUart7RxLineBuf[UART7_RX_DMA_BURST];

static uint8_t          gUart7RxFIFO   [UART7_RX_FIFO_SIZE];   /* 软件环形 FIFO */
static volatile uint16_t gUart7RxHead = 0;                    /* FIFO 读指针 (应用读) */
static volatile uint16_t gUart7RxTail = 0;                    /* FIFO 写指针 (DMA IRQ 写) */


/* -------- 内部：装填 UART7 RX DMA（启动时调用一次，REPEAT 模式自动循环）--- */
static void UART7_RX_DMA_Arm(void)
{
    /* 关中断保护装填过程，避免和其它中断相互打断寄存器序列。            */
    __disable_irq();

    DL_DMA_disableChannel  (DMA, UART7_RX_DMA_CHAN);
    DL_DMA_setSrcAddr      (DMA, UART7_RX_DMA_CHAN, (uint32_t)&(UART7_INST->RXDATA));
    DL_DMA_setDestAddr     (DMA, UART7_RX_DMA_CHAN, (uint32_t)gUart7RxLineBuf);
    DL_DMA_setTransferSize (DMA, UART7_RX_DMA_CHAN, UART7_RX_DMA_BURST);
    /* REPEAT_SINGLE：使能后 DMA 保持 enable，收满 BURST 字节自动回卷续收。 */
    DL_DMA_enableChannel   (DMA, UART7_RX_DMA_CHAN);

    __enable_irq();
}


/* -------- 对外：复位并启动首拍 (UART_Init 调用) ------------------------------ */
void UART7_RX_DMA_Reset(void)
{
    gUart7RxHead = 0u;
    gUart7RxTail = 0u;
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL1);
    UART7_RX_DMA_Arm();
}


/* ----------------------------------------------------------------------------
 *  DMA 通道 1 (UART7 RX) 完成中断处理
 *
 *  DMA 共享单条 NVIC 向量 DMA_INT_IRQn => DMA_IRQHandler。进中断后用
 *  DL_DMA_getPendingInterrupt() 读 IIDX 判断通道；这里只处理 CH1 (UART7 RX)。
 *
 *  REPEAT_SINGLE 模式下 DMA 硬件已自动回卷并保持 enable，本函数不碰任何 DMA
 *  寄存器，只做两件事：
 *    1) 把 gUart7RxLineBuf[0..BURST-1] 这一拍收到的字节压入环形 FIFO；
 *    2) 清 CH1 raw 标志。下一拍由 DMA 硬件自动回卷续收。
 *  压 FIFO 时若已满，丢新字节（保住 FIFO 里老字节，符合 JY901 遇 0x55 重开
 *  一帧的快速重同步策略——丢新字节比丢半截帧影响小）。
 * -------------------------------------------------------------------------- */
static void DMA_IRQChannel1Handler(void)
{
    uint16_t head, tail;
    uint16_t i;

    head = gUart7RxHead;            /* 读指针快照 */
    tail = gUart7RxTail;            /* 写指针快照 */

    /* 把这一拍 DMA 收的 BURST 个字节逐个压入环形 FIFO */
    for (i = 0u; i < UART7_RX_DMA_BURST; i++)
    {
        /* 满判定：再写一个会追上读指针则丢剩余字节 */
        if (((uint16_t)((tail + 1u) & UART7_RX_FIFO_MASK) ^ head) == 0u) {
            break;                  /* FIFO 满 */
        }
        gUart7RxFIFO[tail] = gUart7RxLineBuf[i];
        tail = (uint16_t)((tail + 1u) & UART7_RX_FIFO_MASK);
    }
    gUart7RxTail = tail;

    /* 清 CH1 raw 标志；无需重装 DMA（REPEAT_SINGLE 硬件已自动回卷并保持 enable）。 */
    DL_DMA_clearInterruptStatus(DMA, DL_DMA_INTERRUPT_CHANNEL1);
}


/* 统一 DMA 中断向量分发。MSPM0 全部 DMA 通道共用 DMA_INT_IRQn 一个入口。 */
void DMA_IRQHandler(void)
{
    DL_DMA_EVENT_IIDX ch = DL_DMA_getPendingInterrupt(DMA);

    /* IIDX 优先编码返回当前最高优先级待处理通道；这里只关心 CH1 (UART7 RX)。 */
    if (ch == DL_DMA_EVENT_IIDX_DMACH1) {
        DMA_IRQChannel1Handler();
        return;
    }
    /* 其余通道(蓝牙TX CH0 当前不开中断)走默认：我们不处理也不清标志，
     * 避免 SysConfig 未挂的通道被误清。 */
}


/* -------- 对外：环形 FIFO 读取接口 (JY901 调用) ---------------------------- */

/* 返回当前可读字节数 (可被 JY901 用来判断是否有数据) */
uint16_t UART7_RX_Available(void)
{
    uint16_t head, tail;
    __disable_irq();
    head = gUart7RxHead;
    tail = gUart7RxTail;
    __enable_irq();
    return (uint16_t)((tail - head) & UART7_RX_FIFO_MASK);
}

/* 从环形 FIFO 取一个字节；无数据返回 0 并把 *ok 置 0，有数据返回字节且 *ok=1 */
uint8_t UART7_RX_Read(uint8_t *ok)
{
    uint16_t head, tail;
    uint8_t  byte;

    __disable_irq();
    head = gUart7RxHead;
    tail = gUart7RxTail;
    if (head == tail) {
        __enable_irq();
        if (ok) *ok = 0u;
        return 0u;                  /* FIFO 空 */
    }
    byte = gUart7RxFIFO[head];
    gUart7RxHead = (uint16_t)((head + 1u) & UART7_RX_FIFO_MASK);
    __enable_irq();

    if (ok) *ok = 1u;
    return byte;
}


