#ifndef SYRINGE_PUMP_H
#define SYRINGE_PUMP_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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

// 函数声明
int send_command(int fd, const char* cmd, char* response, size_t resp_size);
int pump_init(int fd);
int pump_move_absolute(int fd, int position);
int pump_pick_relative(int fd, int steps);
int pump_dispense_relative(int fd, int steps);
int pump_set_speed(int fd, int speed);
int pump_stop(int fd);
int pump_get_status(int fd, char* status);

#endif // SYRINGE_PUMP_H