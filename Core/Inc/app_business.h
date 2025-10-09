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
#define REG_ROTATION_TIME_LOW       10    // 40010 转动时间低位
#define REG_ROTATION_TIME_HIGH      11    // 40011 转动时间高位
#define REG_TARGET_ANGLE_LOW_START  12    // 40012-40017 目标角度1-6低位
#define REG_TARGET_ANGLE_LOW_END    17
#define REG_TARGET_ANGLE_HIGH_START 18    // 40018-40023 目标角度1-6高位
#define REG_TARGET_ANGLE_HIGH_END   23
#define REG_CURRENT_ANGLE_LOW_START 24    // 40024-40029 当前角度1-6低位(只读)
#define REG_CURRENT_ANGLE_LOW_END   29
#define REG_CURRENT_ANGLE_HIGH_START 30   // 40030-40035 当前角度1-6高位(只读)
#define REG_CURRENT_ANGLE_HIGH_END  35
#define REG_ROTATION_TRIGGER        36    // 40036 转动触发
#define REG_READ_TRIGGER            37    // 40037 读取触发
#define REG_WEIGHT_LOW              38    // 40038 当前重量低位(只读)
#define REG_WEIGHT_HIGH             39    // 40039 当前重量高位(只读)
#define REG_WEIGHT_CONTROL          40    // 40040 控制位
#define REG_PUMP1_INIT_TRIGGER      41    // 40041 泵1初始化触发
#define REG_PUMP1_ABS_POSITION      42    // 40042 泵1绝对位置设置
#define REG_PUMP1_CONTROL_TRIGGER   43    // 40043 泵1控制触发
#define REG_PUMP1_STATUS            44    // 40044 泵1状态位(只读)
#define REG_PUMP1_CURRENT_POSITION  45    // 40045 泵1当前位置(只读)
#define REG_PUMP2_INIT_TRIGGER      46    // 40046 泵2初始化触发
#define REG_PUMP2_ABS_POSITION      47    // 40047 泵2绝对位置设置
#define REG_PUMP2_CONTROL_TRIGGER   48    // 40048 泵2控制触发
#define REG_PUMP2_STATUS            49    // 40049 泵2状态位(只读)
#define REG_PUMP2_CURRENT_POSITION  50    // 40050 泵2当前位置(只读)
#define REG_OBJECT_DETECTION        51    // 40051 物体检测状态(只读)

// 业务处理周期定义
#define BUSINESS_PROCESS_CYCLE_MS   100   // 100ms业务处理周期

/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/

/* Exported functions prototypes ---------------------------------------------*/
void app_business_init(void);
void app_business_process(void);

#ifdef __cplusplus
}
#endif

#endif /* __APP_BUSINESS_H__ */