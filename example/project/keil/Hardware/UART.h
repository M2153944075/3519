#ifndef __UART_H
#define __UART_H

#include "ti_msp_dl_config.h"
#include <stdint.h>

void UART_Init(void);
void UART_SendString(uint8_t n, unsigned char *str);
void UART_SendByte(uint8_t n, uint8_t byte);

/* ==================== UART1 蓝牙 TX (DMA + 软件环形FIFO) ==================== */
/* 软件FIFO容量：为2的幂以便用位掩码取模环回，加快入队速度           */
#define UART1_TX_FIFO_SIZE   256u
#define UART1_TX_FIFO_MASK   (UART1_TX_FIFO_SIZE - 1u)

/* 唤醒一次DMA传输；当DMA空闲且FIFO非空时由内部任务链调用           */
void UART1_TX_DMAPump(void);

/* 向FIFO缓冲一批数据，并触发DMA异步发送（非阻塞，返回写入字节数）   */
uint16_t UART1_TX_DMASend(const uint8_t *data, uint16_t len);


/* ==================== UART7 JY901 RX (DMA + 软件环形FIFO) =================== */
/* 复位环形 FIFO 并启动首拍 UART7 RX DMA 接收（UART_Init 内部已调用一次）。
 * 若中途需要重置接收状态可再次调用。                                       */
void     UART7_RX_DMA_Reset(void);

/* 当前环形 FIFO 中可读字节数；JY901 可据此决定是否调用 Read。              */
uint16_t UART7_RX_Available(void);

/* 从环形 FIFO 取一字节；非空返回字节并 *ok=1，空返回 0 且 *ok=0。
 * ok 可传 NULL（调用方已知 FIFO 非空时）。                                 */
uint8_t  UART7_RX_Read(uint8_t *ok);

#endif 

