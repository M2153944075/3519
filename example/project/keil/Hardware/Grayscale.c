#include "ti_msp_dl_config.h"
#include "Grayscale.h"


void Grayscale_Init(void)
{
    /* 灰度GPIO方向和上下拉由SysConfig生成代码完成。 */
}


uint8_t Grayscale_Read(void)
{
    uint8_t value = 0u;

    if (DL_GPIO_readPins(GRAY_L3_PORT, GRAY_L3_PIN) != 0u) {
        value |= (1u << 0);
    }
    if (DL_GPIO_readPins(GRAY_L2_PORT, GRAY_L2_PIN) != 0u) {
        value |= (1u << 1);
    }
    if (DL_GPIO_readPins(GRAY_L1_PORT, GRAY_L1_PIN) != 0u) {
        value |= (1u << 2);
    }
    if (DL_GPIO_readPins(GRAY_ML_PORT, GRAY_ML_PIN) != 0u) {
        value |= (1u << 3);
    }
    if (DL_GPIO_readPins(GRAY_MR_PORT, GRAY_MR_PIN) != 0u) {
        value |= (1u << 4);
    }
    if (DL_GPIO_readPins(GRAY_R1_PORT, GRAY_R1_PIN) != 0u) {
        value |= (1u << 5);
    }
    if (DL_GPIO_readPins(GRAY_R2_PORT, GRAY_R2_PIN) != 0u) {
        value |= (1u << 6);
    }
    if (DL_GPIO_readPins(GRAY_R3_PORT, GRAY_R3_PIN) != 0u) {
        value |= (1u << 7);
    }

    return value;
}


int16_t Grayscale_GetError(uint8_t gray_value)
{
    static const int8_t weights[8] = {
        -5, -4, -3, -1, 1, 3, 4, 5
    };

    int16_t sum = 0;
    uint8_t count = 0;
    uint8_t i;

    for (i = 0u; i < 8u; i++) {
        if ((gray_value & (uint8_t)(1u << i)) != 0u) {
            sum += weights[i];
            count++;
        }
    }

    if (count == 0u) {
        /* 丢线，不能做除法 */
        return 0;
    }

    /* 放大10倍，提高整数计算精度 */
    return (int16_t)(sum * 10 / count);
}

/* 循迹/串级层使用的对外名称。保留原 GetError 入口以兼容旧应用代码。 */
int16_t Grayscale_WeightedSum(uint8_t gray_value)
{
    return Grayscale_GetError(gray_value);
}
