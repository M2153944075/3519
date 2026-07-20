#include "ti_msp_dl_config.h"

void LED1_ON(void)
{
    DL_GPIO_clearPins(LED1_PORT, LED1_PIN_22_PIN);
}

void LED1_OFF(void)
{
    DL_GPIO_setPins(LED1_PORT, LED1_PIN_22_PIN);
}

void LED1_Turn(void)
{
    DL_GPIO_togglePins(LED1_PORT, LED1_PIN_22_PIN);
}
