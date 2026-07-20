#include "ti_msp_dl_config.h"
#include "Key.h"


static uint8_t Key_Num = 0;

uint8_t Key_GetNum(void)
{
    uint8_t temp = Key_Num;
    Key_Num = 0;
    return temp;
}

uint8_t Key_GetState(void)
{
    if (DL_GPIO_readPins(KEY_K1_PORT, KEY_K1_PIN) == 0)
    {
        return 1;
    }

    if (DL_GPIO_readPins(KEY_K2_PORT, KEY_K2_PIN) == 0)
    {
        return 2;
    }

    if (DL_GPIO_readPins(KEY_K3_PORT, KEY_K3_PIN) == 0)
    {
        return 3;
    }

    return 0;
}

void Key_Tick(void)
{
    static uint8_t Count = 0;
    static uint8_t CurrState = 0;
    static uint8_t PrevState = 0;

    Count++;

    if (Count >= 20)
    {
        Count = 0;

        PrevState = CurrState;
        CurrState = Key_GetState();

        if ((CurrState == 0) && (PrevState != 0))
        {
            Key_Num = PrevState;
        }
    }
}
