#ifndef __BLUETOOTH_H
#define __BLUETOOTH_H

#include <stdint.h>

void Bluetooth_Init(void);
void Bluetooth_Process(void);
void Bluetooth_SendTelemetry(void);

/* UART1 RX中断每收到一个字节调用一次；也可用于单元测试。 */
void Bluetooth_ReceiveByte(uint8_t byte);

#endif
