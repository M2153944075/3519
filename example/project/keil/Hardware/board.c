#include "board.h"


void HardFault_Handler(void) {
    volatile uint32_t *stack;
    __asm volatile ("mrs %0, msp" : "=r"(stack));
    volatile uint32_t pc = stack[6], lr = stack[5];
    volatile uint32_t r0 = stack[0], r3 = stack[3];
    (void)pc; (void)lr; (void)r0; (void)r3;   /* 断点停这里 */
    while (1);
}


volatile uint32_t g_tick_ms;

uint32_t millis(void)
{
    return g_tick_ms;
}

void board_init(void)
{
    SYSCFG_DL_init();

    g_tick_ms = 0u;
    /* PWM 定时器由应用层在安全占空比写入后再启动，避免复位期间默认比较值导致满占空比。 */
    Motor_Stop();
    DL_TimerA_startCounter(PWMA_INST);
    DL_TimerG_startCounter(PWMB_INST);

    UART_Init();
    Bluetooth_Init();
    Encoder_Init();
    Grayscale_Init();
    PID_ModuleInit();
    OLED_Init();
    OLED_Clear();
    Track_Init();

    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_TimerA_startCounter(TIMER_0_INST);
}

/* 10ms控制周期：普通GPIO编码器、QEI、Track依次更新。 */
void board_update(void)
{
	Encoder_Update();

//    Encoder_UpdateQEI(10u);
    Track_Update();
    
}

void TIMER_0_INST_IRQHandler(void)
{
    static uint8_t div10;

    if (DL_TimerA_getPendingInterrupt(TIMER_0_INST) == DL_TIMER_IIDX_ZERO) {
        g_tick_ms++;


        if (++div10 >= 10u) {
            div10 = 0u;
            board_update();
        }
    }
}

void delay_us(unsigned long us)
{
    uint32_t ticks = (uint32_t)(us * (CPUCLK_FREQ / 1000000u));
    uint32_t previous = SysTick->VAL;
    uint32_t elapsed = 0u;

    while (elapsed < ticks) {
        uint32_t current = SysTick->VAL;
        if (current <= previous) {
            elapsed += previous - current;
        } else {
            elapsed += previous + (SysTick->LOAD - current);
        }
        previous = current;
    }
}

void delay_ms(unsigned long ms) { delay_us(ms * 1000u); }
void delay_1us(unsigned long us) { delay_us(us); }
void delay_1ms(unsigned long ms) { delay_ms(ms); }
