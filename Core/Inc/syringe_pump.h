#ifndef SYRINGE_PUMP_H
#define SYRINGE_PUMP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"  // STM32 HAL头文件

// 定义常量
#define PUMP_ADDRESS '0'  // 地址，ASCII '0' (0x30)

/* DT协议状态字节定义（根据错误代码和状态字节表）*/
/* 忙碌状态（0x4X系列）*/
typedef enum {
    PUMP_STATUS_BUSY_NO_ERROR = 0x40,           // '@' - 忙碌+无误
    PUMP_STATUS_BUSY_INITIALIZED = 0x41,        // 'A' - 忙碌+初始化
    PUMP_STATUS_BUSY_NO_COMMAND = 0x42,         // 'B' - 忙碌+无效指令
    PUMP_STATUS_BUSY_INVALID_PARAM = 0x43,      // 'C' - 忙碌+无效参数
    PUMP_STATUS_BUSY_EEPROM_FAULT = 0x46,       // 'F' - 忙碌+EEPROM故障
    PUMP_STATUS_BUSY_DEVICE_NOT_INIT = 0x47,    // 'G' - 忙碌+设备未初始化
    PUMP_STATUS_BUSY_OVERLOAD = 0x49,           // 'I' - 忙碌+柱塞过载
    PUMP_STATUS_BUSY_VALVE_OVERLOAD = 0x4A,     // 'J' - 忙碌+阀过载
    PUMP_STATUS_BUSY_PLUNGER_MOVE_NOT_ALLOWED = 0x4B, // 'K' - 忙碌+不支持柱移动
    PUMP_STATUS_BUSY_INTERNAL_FAULT = 0x4C,     // 'L' - 忙碌+内部故障
    PUMP_STATUS_BUSY_AD_FAULT = 0x4E,           // 'N' - 忙碌+A/D转换器故障
    PUMP_STATUS_BUSY_CMD_OVERFLOW = 0x4F,       // 'O' - 忙碌+指令溢出

    /* 空闲状态（0x6X系列）*/
    PUMP_STATUS_IDLE_NO_ERROR = 0x60,           // '`' - 空闲+无误
    PUMP_STATUS_IDLE_INITIALIZED = 0x61,        // 'a' - 空闲+初始化
    PUMP_STATUS_IDLE_NO_COMMAND = 0x62,         // 'b' - 空闲+无效指令
    PUMP_STATUS_IDLE_INVALID_PARAM = 0x63,      // 'c' - 空闲+无效参数
    PUMP_STATUS_IDLE_EEPROM_FAULT = 0x66,       // 'f' - 空闲+EEPROM故障
    PUMP_STATUS_IDLE_DEVICE_NOT_INIT = 0x67,    // 'g' - 空闲+设备未初始化
    PUMP_STATUS_IDLE_OVERLOAD = 0x69,           // 'i' - 空闲+柱塞过载
    PUMP_STATUS_IDLE_VALVE_OVERLOAD = 0x6A,     // 'j' - 空闲+阀过载
    PUMP_STATUS_IDLE_PLUNGER_MOVE_NOT_ALLOWED = 0x6B, // 'k' - 空闲+不支持柱移动
    PUMP_STATUS_IDLE_INTERNAL_FAULT = 0x6C,     // 'l' - 空闲+内部故障
    PUMP_STATUS_IDLE_AD_FAULT = 0x6E,           // 'n' - 空闲+A/D转换器故障
    PUMP_STATUS_IDLE_CMD_OVERFLOW = 0x6F        // 'o' - 空闲+指令溢出
} PumpStatusByte_t;

/* DT协议错误编号定义 */
typedef enum {
    PUMP_ERROR_NO_ERROR = 0,            // 无误
    PUMP_ERROR_INITIALIZED = 1,         // 初始化
    PUMP_ERROR_NO_COMMAND = 2,          // 无效指令
    PUMP_ERROR_INVALID_PARAM = 3,       // 无效参数
    PUMP_ERROR_EEPROM_FAULT = 6,        // EEPROM故障
    PUMP_ERROR_DEVICE_NOT_INIT = 7,     // 设备未初始化
    PUMP_ERROR_OVERLOAD = 9,            // 柱塞过载
    PUMP_ERROR_VALVE_OVERLOAD = 10,     // 阀过载
    PUMP_ERROR_PLUNGER_MOVE_NOT_ALLOWED = 11, // 不支持柱移动
    PUMP_ERROR_INTERNAL_FAULT = 12,     // 内部故障
    PUMP_ERROR_AD_FAULT = 14,           // A/D转换器故障
    PUMP_ERROR_CMD_OVERFLOW = 15        // 指令溢出（忙）
} PumpErrorCode_t;

/* 泵忙碌状态定义 */
typedef enum {
    PUMP_STATE_IDLE = 0,                // 空闲
    PUMP_STATE_BUSY = 1                 // 忙碌
} PumpBusyState_t;

// 常见指令宏（基于DT协议）
#define CMD_INIT "ZR"      // 初始化活塞
#define CMD_ABS_MOVE "A%dR" // 绝对位置移动，%d为步数
#define CMD_REL_PICK "P%dR" // 相对抽取，%d为步数
#define CMD_REL_DISP "D%dR" // 相对分配，%d为步数
#define CMD_SET_SPEED "V%dR" // 设置速度，%d为脉冲/秒
#define CMD_STOP "TR"       // 终止运动
#define CMD_STATUS "?"      // 查询状态
#define CMD_ERROR_QUERY "QR"  // 查询错误码
#define CMD_POSITION_QUERY "?4R"  // 查询当前活塞位置

// 函数声明 - 适配STM32版本
int send_command(int pump_id, const char* cmd, char* response, size_t resp_size);
int pump_init(int pump_id);
int pump_move_absolute(int pump_id, int position);
int pump_pick_relative(int pump_id, int steps);
int pump_dispense_relative(int pump_id, int steps);
int pump_set_speed(int pump_id, int speed);
int pump_stop(int pump_id);
int pump_get_status(int pump_id, char* status);
int pump_query_error(int pump_id, char* error_code);  // 新增：查询错误码
int pump_query_position(int pump_id, int* position); // 新增：查询当前位置

// 辅助函数：从状态字节转换为错误编号
PumpErrorCode_t pump_parse_status_byte(uint8_t status_byte);
// 辅助函数：从状态字节解析忙碌状态
PumpBusyState_t pump_parse_busy_state(uint8_t status_byte);

#endif // SYRINGE_PUMP_H