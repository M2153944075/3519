#ifndef __TRACK_H
#define __TRACK_H

#include <stdint.h>

#define TRACK_TURN_ANGLE_DEG          30.0f
/* 目标轮速单位与速度 PID 使用的单位一致。 */
#define TRACK_BASE_SPEED              120u

void Track_Init(void);
void Track_Update(void);
void Track_OnLostLine(void);
void Track_Control(uint32_t base_speed);

/* 状态值：0=正常循迹，1=标记直行，2=左转，3=右转。 */
uint8_t Track_GetState(void);
/* 侧别值：0=无，1=左标记，2=右标记。 */
uint8_t Track_GetMarkerSide(void);

#endif
