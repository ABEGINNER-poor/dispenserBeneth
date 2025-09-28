#ifndef BUS_SERVO_H
#define BUS_SERVO_H

#include "stm32f4xx_hal.h"  // 根据你的STM32系列修改，例如stm32f4xx_hal.h

extern UART_HandleTypeDef huart1;  // 假设使用USART1，根据你的配置修改
extern UART_HandleTypeDef huart6;
// 指令定义
#define CMD_SERVO_MOVE          0x03
#define CMD_ACTION_GROUP_RUN    0x06
#define CMD_ACTION_GROUP_STOP   0x07
#define CMD_ACTION_GROUP_SPEED  0x0B
#define CMD_GET_BATTERY_VOLTAGE 0x0F
#define CMD_MULT_SERVO_UNLOAD   0x14
#define CMD_MULT_SERVO_POS_READ 0x15

// 函数原型
void BusServo_SendCmd(uint8_t cmd, uint8_t *params, uint8_t param_cnt);
void BusServo_Move(uint8_t id, uint16_t position, uint16_t time);  // 单舵机示例
void BusServo_ActionGroupRun(uint8_t group, uint16_t times);
void BusServo_ActionGroupStop(void);
void BusServo_ActionGroupSpeed(uint8_t group, uint16_t speed_percent);
uint16_t BusServo_GetBatteryVoltage(void);
void BusServo_MultUnload(uint8_t *ids, uint8_t id_cnt);
void BusServo_MultPosRead(uint8_t *ids, uint8_t id_cnt, uint16_t *positions);

#endif
