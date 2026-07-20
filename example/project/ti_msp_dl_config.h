/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G351X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G351X
#define CONFIG_MSPM0G3519

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for PWMA */
#define PWMA_INST                                                          TIMA0
#define PWMA_INST_IRQHandler                                    TIMA0_IRQHandler
#define PWMA_INST_INT_IRQN                                      (TIMA0_INT_IRQn)
#define PWMA_INST_CLK_FREQ                                               4000000
/* GPIO defines for channel 3 */
#define GPIO_PWMA_C3_PORT                                                  GPIOA
#define GPIO_PWMA_C3_PIN                                          DL_GPIO_PIN_12
#define GPIO_PWMA_C3_IOMUX                                       (IOMUX_PINCM34)
#define GPIO_PWMA_C3_IOMUX_FUNC                      IOMUX_PINCM34_PF_TIMA0_CCP3
#define GPIO_PWMA_C3_IDX                                     DL_TIMER_CC_3_INDEX

/* Defines for PWMB */
#define PWMB_INST                                                          TIMG0
#define PWMB_INST_IRQHandler                                    TIMG0_IRQHandler
#define PWMB_INST_INT_IRQN                                      (TIMG0_INT_IRQn)
#define PWMB_INST_CLK_FREQ                                               4000000
/* GPIO defines for channel 1 */
#define GPIO_PWMB_C1_PORT                                                  GPIOA
#define GPIO_PWMB_C1_PIN                                          DL_GPIO_PIN_13
#define GPIO_PWMB_C1_IOMUX                                       (IOMUX_PINCM35)
#define GPIO_PWMB_C1_IOMUX_FUNC                      IOMUX_PINCM35_PF_TIMG0_CCP1
#define GPIO_PWMB_C1_IDX                                     DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA1)
#define TIMER_0_INST_IRQHandler                                 TIMA1_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA1_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                            (39U)



/* Defines for UART1 */
#define UART1_INST                                                         UART1
#define UART1_INST_FREQUENCY                                            32000000
#define UART1_INST_IRQHandler                                   UART1_IRQHandler
#define UART1_INST_INT_IRQN                                       UART1_INT_IRQn
#define GPIO_UART1_RX_PORT                                                 GPIOB
#define GPIO_UART1_TX_PORT                                                 GPIOB
#define GPIO_UART1_RX_PIN                                          DL_GPIO_PIN_5
#define GPIO_UART1_TX_PIN                                          DL_GPIO_PIN_4
#define GPIO_UART1_IOMUX_RX                                      (IOMUX_PINCM18)
#define GPIO_UART1_IOMUX_TX                                      (IOMUX_PINCM17)
#define GPIO_UART1_IOMUX_RX_FUNC                       IOMUX_PINCM18_PF_UART1_RX
#define GPIO_UART1_IOMUX_TX_FUNC                       IOMUX_PINCM17_PF_UART1_TX
#define UART1_BAUD_RATE                                                   (9600)
#define UART1_IBRD_32_MHZ_9600_BAUD                                        (208)
#define UART1_FBRD_32_MHZ_9600_BAUD                                         (21)
/* Defines for UART7 */
#define UART7_INST                                                         UART7
#define UART7_INST_FREQUENCY                                            32000000
#define UART7_INST_IRQHandler                                   UART7_IRQHandler
#define UART7_INST_INT_IRQN                                       UART7_INT_IRQn
#define GPIO_UART7_RX_PORT                                                 GPIOA
#define GPIO_UART7_RX_PIN                                         DL_GPIO_PIN_24
#define GPIO_UART7_IOMUX_RX                                      (IOMUX_PINCM54)
#define GPIO_UART7_IOMUX_RX_FUNC                       IOMUX_PINCM54_PF_UART7_RX
#define UART7_BAUD_RATE                                                 (115200)
#define UART7_IBRD_32_MHZ_115200_BAUD                                       (17)
#define UART7_FBRD_32_MHZ_115200_BAUD                                       (23)





/* Defines for DMA_TxBlue */
#define DMA_TxBlue_CHAN_ID                                                   (0)
#define UART1_INST_DMA_TRIGGER                               (DMA_UART1_TX_TRIG)
/* Defines for DMA_RxJY_COPY */
#define DMA_RxJY_COPY_CHAN_ID                                                (1)
#define UART7_INST_DMA_TRIGGER                               (DMA_UART7_RX_TRIG)


/* Port definition for Pin Group LED1 */
#define LED1_PORT                                                        (GPIOB)

/* Defines for PIN_22: GPIOB.22 with pinCMx 50 on package pin 21 */
#define LED1_PIN_22_PIN                                         (DL_GPIO_PIN_22)
#define LED1_PIN_22_IOMUX                                        (IOMUX_PINCM50)
/* Port definition for Pin Group OLED */
#define OLED_PORT                                                        (GPIOA)

/* Defines for SCL: GPIOA.1 with pinCMx 2 on package pin 34 */
#define OLED_SCL_PIN                                             (DL_GPIO_PIN_1)
#define OLED_SCL_IOMUX                                            (IOMUX_PINCM2)
/* Defines for SDA: GPIOA.0 with pinCMx 1 on package pin 33 */
#define OLED_SDA_PIN                                             (DL_GPIO_PIN_0)
#define OLED_SDA_IOMUX                                            (IOMUX_PINCM1)
/* Port definition for Pin Group TB */
#define TB_PORT                                                          (GPIOB)

/* Defines for AIN1: GPIOB.9 with pinCMx 26 on package pin 61 */
#define TB_AIN1_PIN                                              (DL_GPIO_PIN_9)
#define TB_AIN1_IOMUX                                            (IOMUX_PINCM26)
/* Defines for AIN2: GPIOB.8 with pinCMx 25 on package pin 60 */
#define TB_AIN2_PIN                                              (DL_GPIO_PIN_8)
#define TB_AIN2_IOMUX                                            (IOMUX_PINCM25)
/* Defines for BIN1: GPIOB.7 with pinCMx 24 on package pin 59 */
#define TB_BIN1_PIN                                              (DL_GPIO_PIN_7)
#define TB_BIN1_IOMUX                                            (IOMUX_PINCM24)
/* Defines for BIN2: GPIOB.6 with pinCMx 23 on package pin 58 */
#define TB_BIN2_PIN                                              (DL_GPIO_PIN_6)
#define TB_BIN2_IOMUX                                            (IOMUX_PINCM23)
/* Defines for E2A: GPIOB.15 with pinCMx 32 on package pin 3 */
// pins affected by this interrupt request:["E2A","E1A"]
#define TB_INT_IRQN                                             (GPIOB_INT_IRQn)
#define TB_INT_IIDX                             (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define TB_E2A_IIDX                                         (DL_GPIO_IIDX_DIO15)
#define TB_E2A_PIN                                              (DL_GPIO_PIN_15)
#define TB_E2A_IOMUX                                             (IOMUX_PINCM32)
/* Defines for E2B: GPIOB.16 with pinCMx 33 on package pin 4 */
#define TB_E2B_PIN                                              (DL_GPIO_PIN_16)
#define TB_E2B_IOMUX                                             (IOMUX_PINCM33)
/* Defines for E1A: GPIOB.10 with pinCMx 27 on package pin 62 */
#define TB_E1A_IIDX                                         (DL_GPIO_IIDX_DIO10)
#define TB_E1A_PIN                                              (DL_GPIO_PIN_10)
#define TB_E1A_IOMUX                                             (IOMUX_PINCM27)
/* Defines for L3: GPIOB.13 with pinCMx 30 on package pin 1 */
#define GRAY_L3_PORT                                                     (GPIOB)
#define GRAY_L3_PIN                                             (DL_GPIO_PIN_13)
#define GRAY_L3_IOMUX                                            (IOMUX_PINCM30)
/* Defines for L2: GPIOA.27 with pinCMx 60 on package pin 31 */
#define GRAY_L2_PORT                                                     (GPIOA)
#define GRAY_L2_PIN                                             (DL_GPIO_PIN_27)
#define GRAY_L2_IOMUX                                            (IOMUX_PINCM60)
/* Defines for L1: GPIOA.25 with pinCMx 55 on package pin 26 */
#define GRAY_L1_PORT                                                     (GPIOA)
#define GRAY_L1_PIN                                             (DL_GPIO_PIN_25)
#define GRAY_L1_IOMUX                                            (IOMUX_PINCM55)
/* Defines for ML: GPIOB.25 with pinCMx 56 on package pin 27 */
#define GRAY_ML_PORT                                                     (GPIOB)
#define GRAY_ML_PIN                                             (DL_GPIO_PIN_25)
#define GRAY_ML_IOMUX                                            (IOMUX_PINCM56)
/* Defines for MR: GPIOB.20 with pinCMx 48 on package pin 19 */
#define GRAY_MR_PORT                                                     (GPIOB)
#define GRAY_MR_PIN                                             (DL_GPIO_PIN_20)
#define GRAY_MR_IOMUX                                            (IOMUX_PINCM48)
/* Defines for R1: GPIOA.14 with pinCMx 36 on package pin 7 */
#define GRAY_R1_PORT                                                     (GPIOA)
#define GRAY_R1_PIN                                             (DL_GPIO_PIN_14)
#define GRAY_R1_IOMUX                                            (IOMUX_PINCM36)
/* Defines for R2: GPIOA.16 with pinCMx 38 on package pin 9 */
#define GRAY_R2_PORT                                                     (GPIOA)
#define GRAY_R2_PIN                                             (DL_GPIO_PIN_16)
#define GRAY_R2_IOMUX                                            (IOMUX_PINCM38)
/* Defines for R3: GPIOA.17 with pinCMx 39 on package pin 10 */
#define GRAY_R3_PORT                                                     (GPIOA)
#define GRAY_R3_PIN                                             (DL_GPIO_PIN_17)
#define GRAY_R3_IOMUX                                            (IOMUX_PINCM39)
/* Defines for E1B: GPIOB.11 with pinCMx 28 on package pin 63 */
#define TB_E1B_PIN                                              (DL_GPIO_PIN_11)
#define TB_E1B_IOMUX                                             (IOMUX_PINCM28)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWMA_init(void);
void SYSCFG_DL_PWMB_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_UART1_init(void);
void SYSCFG_DL_UART7_init(void);
void SYSCFG_DL_DMA_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
