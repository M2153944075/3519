#include "Bluetooth.h"
#include "ti_msp_dl_config.h"
#include "UART.h"
#include "Encoder.h"
#include "Motor.h"
#include "PID.h"

#define BLE_FRAME_HEAD       0xA5u
#define BLE_FRAME_TAIL       0x5Au
#define BLE_CMD_TELEMETRY     0x10u

static volatile uint8_t g_rx_state;
static volatile uint8_t g_rx_id;
static volatile uint8_t g_rx_index;
static volatile uint8_t g_rx_checksum;
static uint8_t g_rx_float_bytes[4];

static uint32_t FloatToBytes(float value, uint8_t *out)
{
    union {
        float value;
        uint8_t bytes[4];
    } data;

    data.value = value;
    out[0] = data.bytes[3];
    out[1] = data.bytes[2];
    out[2] = data.bytes[1];
    out[3] = data.bytes[0];
    return 4u;
}

static float BytesToFloat(const uint8_t *bytes)
{
    union {
        float value;
        uint8_t bytes[4];
    } data;

    data.bytes[3] = bytes[0];
    data.bytes[2] = bytes[1];
    data.bytes[1] = bytes[2];
    data.bytes[0] = bytes[3];
    return data.value;
}

void Bluetooth_Init(void)
{
    g_rx_state = 0u;
    g_rx_id = 0u;
    g_rx_index = 0u;
    g_rx_checksum = 0u;
}

void Bluetooth_ReceiveByte(uint8_t byte)
{
    switch (g_rx_state) {
        case 0u:
            if (byte == BLE_FRAME_HEAD) {
                g_rx_state = 1u;
                g_rx_checksum = 0u;
            }
            break;

        case 1u:
            g_rx_id = byte;
            g_rx_checksum = byte;
            g_rx_index = 0u;
            g_rx_state = 2u;
            break;

        case 2u:
            g_rx_float_bytes[g_rx_index++] = byte;
            g_rx_checksum = (uint8_t)(g_rx_checksum + byte);
            if (g_rx_index >= 4u) {
                g_rx_state = 3u;
            }
            break;

        case 3u:
            if (byte == g_rx_checksum) {
                g_rx_state = 4u;
            } else {
                g_rx_state = 0u;
            }
            break;

        case 4u:
            /* 参数帧格式：A5, 参数ID, float(4B), checksum, 5A。 */
            if (byte == BLE_FRAME_TAIL) {
                /* g_rx_id即参数ID，收到合法帧后写入对应PID。 */
                (void)PID_RemoteSetParameter(g_rx_id, BytesToFloat(g_rx_float_bytes));
            }
            g_rx_state = 0u;
            break;

        default:
            g_rx_state = 0u;
            break;
    }
}

void Bluetooth_SendTelemetry(void)
{
    PID_RemoteSnapshot pid;
    uint8_t frame[80];
    uint8_t payload_len = 0u;
    uint8_t checksum = 0u;
    uint8_t i;
//    int32_t qei_speed = Encoder_QEI_GetDeltaCount();
    int32_t gpio_speed = Encoder_GetSpeed(1);
    int32_t left_pwm = Motor_GetLastLeftOutput();
    int32_t right_pwm = Motor_GetLastRightOutput();

    PID_RemoteGetSnapshot(&pid);

    frame[0] = BLE_FRAME_HEAD;
    frame[1] = BLE_CMD_TELEMETRY;
    frame[2] = 0u; /* payload长度稍后填写 */

//    frame[3] = (uint8_t)(qei_speed >> 24);
//    frame[4] = (uint8_t)(qei_speed >> 16);
//    frame[5] = (uint8_t)(qei_speed >> 8);
//    frame[6] = (uint8_t)qei_speed;
    frame[7] = (uint8_t)(gpio_speed >> 24);
    frame[8] = (uint8_t)(gpio_speed >> 16);
    frame[9] = (uint8_t)(gpio_speed >> 8);
    frame[10] = (uint8_t)gpio_speed;
    frame[11] = (uint8_t)(left_pwm >> 8);
    frame[12] = (uint8_t)left_pwm;
    frame[13] = (uint8_t)(right_pwm >> 8);
    frame[14] = (uint8_t)right_pwm;
    payload_len = 12u;

    FloatToBytes(pid.speed_kp, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.speed_ki, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.speed_kd, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.yaw_angle_kp, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.yaw_angle_ki, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.yaw_angle_kd, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.yaw_rate_kp, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.yaw_rate_ki, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.yaw_rate_kd, &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.track_kp,    &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.track_ki,    &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.track_kd,    &frame[3u + payload_len]); payload_len += 4u;
    FloatToBytes(pid.control_dt,  &frame[3u + payload_len]); payload_len += 4u;

    frame[2] = payload_len;
    checksum = (uint8_t)(frame[1] + frame[2]);
    for (i = 0u; i < payload_len; i++) {
        checksum = (uint8_t)(checksum + frame[3u + i]);
    }
    frame[3u + payload_len] = checksum;
    frame[4u + payload_len] = BLE_FRAME_TAIL;
    UART1_TX_DMASend(frame, (uint16_t)(payload_len + 5u));
}

void Bluetooth_Process(void)
{
    UART1_TX_DMAPump();
}

void UART1_IRQHandler(void)
{
    while (!DL_UART_isRXFIFOEmpty(UART1_INST)) {
        Bluetooth_ReceiveByte(DL_UART_Main_receiveData(UART1_INST));
    }
}
