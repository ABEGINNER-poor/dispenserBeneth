/**
  ******************************************************************************
  * @file    app_business.h
  * @brief   应用层业务逻辑处理头文件
  ******************************************************************************
  * @attention
  *
  * 此文件包含分配器设备的所有业务逻辑处理功能：
  * - 舵机控制
  * - 称重处理
  * - 泵控制
  * - 传感器数据更新
  *
  ******************************************************************************
  */

#ifndef __APP_BUSINESS_H__
#define __APP_BUSINESS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "modbus_tcp.h"

/* Private defines -----------------------------------------------------------*/
// Modbus寄存器地址定义 (相对于40001的偏移，实际数组索引)
#define REG_RESERVED_START          0     // 40000-40009 预留空点位
#define REG_RESERVED_END            9
#define REG_ROTATION_TIME           10    // 40010 转动时间 uint16 ms(0-65535)
#define REG_TARGET_ANGLE1           11    // 40011 目标角度1 uint16 位置(0-1000)
#define REG_TARGET_ANGLE2           12    // 40012 目标角度2 uint16 位置(0-1000)
#define REG_TARGET_ANGLE3           13    // 40013 目标角度3 uint16 位置(0-1000)
#define REG_TARGET_ANGLE4           14    // 40014 目标角度4 uint16 位置(0-1000)
#define REG_TARGET_ANGLE5           15    // 40015 目标角度5 uint16 位置(0-1000)
#define REG_TARGET_ANGLE6           16    // 40016 目标角度6 uint16 位置(0-1000)
#define REG_CURRENT_ANGLE1          17    // 40017 当前角度1 uint16 位置(0-1000) 只读
#define REG_CURRENT_ANGLE2          18    // 40018 当前角度2 uint16 位置(0-1000) 只读
#define REG_CURRENT_ANGLE3          19    // 40019 当前角度3 uint16 位置(0-1000) 只读
#define REG_CURRENT_ANGLE4          20    // 40020 当前角度4 uint16 位置(0-1000) 只读
#define REG_CURRENT_ANGLE5          21    // 40021 当前角度5 uint16 位置(0-1000) 只读
#define REG_CURRENT_ANGLE6          22    // 40022 当前角度6 uint16 位置(0-1000) 只读
#define REG_ROTATION_TRIGGER        23    // 40023 转动触发
#define REG_WEIGHT                  24    // 40024 当前重量 uint16 g signed int 只读
#define REG_WEIGHT_CONTROL          25    // 40025 控制位
#define REG_PUMP1_INIT_TRIGGER      26    // 40026 泵1初始化触发
#define REG_PUMP1_ABS_POSITION      27    // 40027 泵1绝对位置设置 uint16 步数(0-6000)
#define REG_PUMP1_CONTROL_TRIGGER   28    // 40028 泵1控制触发
#define REG_PUMP1_STATUS            29    // 40029 泵1状态位 只读 0-15=DT错误码, 999=通信失败/未初始化
#define REG_PUMP1_CURRENT_POSITION  30    // 40030 泵1当前位置 uint16 步数(0-6000) 只读
#define REG_PUMP2_INIT_TRIGGER      31    // 40031 泵2初始化触发
#define REG_PUMP2_ABS_POSITION      32    // 40032 泵2绝对位置设置 uint16 步数(0-6000)
#define REG_PUMP2_CONTROL_TRIGGER   33    // 40033 泵2控制触发
#define REG_PUMP2_STATUS            34    // 40034 泵2状态位 只读 0-15=DT错误码, 999=通信失败/未初始化
#define REG_PUMP2_CURRENT_POSITION  35    // 40035 泵2当前位置 uint16 步数(0-6000) 只读
#define REG_OBJECT_DETECTION        36    // 40036 物体检测状态 只读 1=无物体 2=有

// 业务处理周期定义
#define BUSINESS_PROCESS_CYCLE_MS   100   // 100ms业务处理周期

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void app_business_init(void);
void app_business_process(void);

// 调试函数声明
void cdc_debug_print(const char* message);

// 舵机接收线程通信函数
uint8_t get_servo_positions_from_recv_thread(uint16_t *positions);
void request_servo_read(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_BUSINESS_H__ */