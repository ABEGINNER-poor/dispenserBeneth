#ifndef SYRINGE_PUMP_H
#define SYRINGE_PUMP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"  // STM32 HAL头文件

// 定义常量
#define PUMP_ADDRESS '0'  // 地址，ASCII '0' (0x30)

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

#endif // SYRINGE_PUMP_H