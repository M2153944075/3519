#ifndef __GRAYSCALE_H
#define __GRAYSCALE_H

#include <stdint.h>
#include "ti_msp_dl_config.h"

/* bit0=L3，bit7=R3；黑线检测为高电平。 */
void Grayscale_Init(void);
uint8_t Grayscale_Read(void);

/* 八路权重依次为 -5、-4、-3、-2、+2、+3、+4、+5。 */
int16_t Grayscale_WeightedSum(uint8_t gray_value);

#endif
