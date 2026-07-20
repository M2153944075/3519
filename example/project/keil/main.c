#include "ti_msp_dl_config.h"
#include "board.h"
#include "JY901.h"

int main(void)
{
    uint32_t last_telemetry = 0u;

    board_init();

	OLED_ShowString(0, 0, "LSpeed:", OLED_8X16);
	OLED_ShowString(0, 16, "RSpeed`:", OLED_8X16);
    OLED_ShowString(0, 32, "Angle:", OLED_8X16);
    
	while (1)
	{
        JY901_PollRx();         /* 从 UART7 DMA 环形 FIFO 拉取并解析陀螺仪帧 */
        JY901_ProcessData();
		float yaw_angle = JY901_GetAngle(2);
        uint8_t gray = Grayscale_Read();
        uint8_t i;

        Bluetooth_Process();
		uint8_t track_state = Track_GetState();
        OLED_ShowNum(0, 48, (uint32_t)track_state, 1, OLED_8X16);
        // /* 调试显示八路原始电平，黑线高电平显示为1。 */
        // for (i = 0u; i < 8u; i++) {
        //     OLED_ShowNum((uint8_t)(i * 8u), 48,
        //                  (uint32_t)((gray >> i) & 1u), 1, OLED_8X16);
        // }

        int16_t Lspeed = Encoder_GetSpeed(1);
		int16_t Rspeed = Encoder_GetSpeed(2);
		OLED_ShowSignedNum(64, 0, Lspeed, 4, OLED_8X16);
		OLED_ShowSignedNum(64, 16, Rspeed, 4, OLED_8X16);
        OLED_ShowFloatNum(64, 32, yaw_angle, 3, 3, OLED_8X16);
        OLED_Update();

//        if ((uint32_t)(millis() - last_telemetry) >= 100u) {
//            last_telemetry = millis();
//            Bluetooth_SendTelemetry();
//        }
    }
}
