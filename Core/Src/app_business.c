/**
  ******************************************************************************
  * @file    app_business.c
  * @brief   应用层业务逻辑处理实现文件
  ******************************************************************************
  * @attention
  *
  * 此文件实现分配器设备的所有业务逻辑处理功能
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_business.h"
#include "usbd_cdc_if.h"
#include "syringe_pump.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// 业务状态变量
static uint32_t last_rotation_trigger = 0;       // 上次转动触发状态
static uint32_t last_read_trigger = 0;           // 上次读取触发状态
static uint32_t last_weight_control = 0;         // 上次称重控制状态
static uint32_t last_pump1_init_trigger = 0;     // 上次泵1初始化触发状态
static uint32_t last_pump1_control_trigger = 0;  // 上次泵1控制触发状态
static uint32_t last_pump2_init_trigger = 0;     // 上次泵2初始化触发状态
static uint32_t last_pump2_control_trigger = 0;  // 上次泵2控制触发状态

static char debug_buf[128];                       // 调试信息缓冲区

// 泵控制相关状态变量
static uint8_t pump1_busy = 0;                    // 泵1忙碌状态 (0=空闲, 1=忙碌)
static uint8_t pump2_busy = 0;                    // 泵2忙碌状态 (0=空闲, 1=忙碌)
static uint16_t pump1_current_pos = 0;            // 泵1当前位置
static uint16_t pump2_current_pos = 0;            // 泵2当前位置
static uint32_t pump1_move_start_time = 0;        // 泵1开始移动时间（用于超时检测）
static uint32_t pump2_move_start_time = 0;        // 泵2开始移动时间（用于超时检测）

/* Private function prototypes -----------------------------------------------*/
static void process_servo_commands(void);
static void process_weight_commands(void);
static void process_pump_commands(void);
static void update_sensor_data(void);
static void cdc_debug_print(const char* message);

// 泵控制辅助函数
static int pump_send_command_uart(uint8_t pump_id, const char* cmd);
static int pump_init_device(uint8_t pump_id);
static int pump_move_absolute_device(uint8_t pump_id, uint16_t position);
static int pump_get_status_device(uint8_t pump_id);
static void pump_update_status(uint8_t pump_id);

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  初始化应用层业务逻辑
  * @retval None
  */
void app_business_init(void) {
    // 初始化寄存器默认值
    holding_regs[REG_CURRENT_ANGLE_LOW_START] = 500;     // 40024 当前角度1低位默认500
    holding_regs[REG_CURRENT_ANGLE_LOW_START+1] = 500;   // 40025 当前角度2低位默认500
    holding_regs[REG_CURRENT_ANGLE_LOW_START+2] = 500;   // 40026 当前角度3低位默认500
    holding_regs[REG_CURRENT_ANGLE_LOW_START+3] = 500;   // 40027 当前角度4低位默认500
    holding_regs[REG_CURRENT_ANGLE_LOW_START+4] = 500;   // 40028 当前角度5低位默认500
    holding_regs[REG_CURRENT_ANGLE_LOW_START+5] = 500;   // 40029 当前角度6低位默认500
    
    // 初始化泵相关寄存器
    holding_regs[REG_PUMP1_STATUS] = 1;                   // 40044 泵1状态：1=不忙
    holding_regs[REG_PUMP1_CURRENT_POSITION] = 0;        // 40045 泵1当前位置：0
    holding_regs[REG_PUMP2_STATUS] = 1;                   // 40049 泵2状态：1=不忙
    holding_regs[REG_PUMP2_CURRENT_POSITION] = 0;        // 40050 泵2当前位置：0
    
    // 初始化业务状态变量
    last_rotation_trigger = 0;
    last_read_trigger = 0;
    last_weight_control = 0;
    last_pump1_init_trigger = 0;
    last_pump1_control_trigger = 0;
    last_pump2_init_trigger = 0;
    last_pump2_control_trigger = 0;
    
    // 初始化泵状态变量
    pump1_busy = 0;
    pump2_busy = 0;
    pump1_current_pos = 0;
    pump2_current_pos = 0;
    pump1_move_start_time = 0;
    pump2_move_start_time = 0;
    
    cdc_debug_print("Business logic initialized");
}

/**
  * @brief  主业务逻辑处理函数 - 每100ms调用一次
  * @retval None
  */
void app_business_process(void) {
    // 1. 处理舵机相关命令
    process_servo_commands();
    
    // 2. 处理称重相关命令
    process_weight_commands();
    
    // 3. 处理泵相关命令
    process_pump_commands();
    
    // 4. 更新传感器数据 (只读寄存器)
    update_sensor_data();
}

/**
  * @brief  调试信息输出函数
  * @param  message: 要输出的调试信息
  * @retval None
  */
static void cdc_debug_print(const char* message) {
    int len = snprintf(debug_buf, sizeof(debug_buf), "[BUSINESS] %s\r\n", message);
    if (len > 0 && len < sizeof(debug_buf)) {
        CDC_Transmit_FS((uint8_t*)debug_buf, len);
    }
}

/**
  * @brief  处理舵机相关命令
  * @retval None
  */
static void process_servo_commands(void) {
    // 检查转动触发 (40036)
    if (holding_regs[REG_ROTATION_TRIGGER] == 1 && last_rotation_trigger != 1) {
        cdc_debug_print("Servo rotation triggered");
        
        // 获取转动时间 (40010-40011)
        uint32_t rotation_time = (holding_regs[REG_ROTATION_TIME_HIGH] << 16) | holding_regs[REG_ROTATION_TIME_LOW];
        
        // 获取目标角度 (40012-40023)
        uint16_t target_angles[6];
        for (int i = 0; i < 6; i++) {
            target_angles[i] = (holding_regs[REG_TARGET_ANGLE_HIGH_START + i] << 16) | 
                              holding_regs[REG_TARGET_ANGLE_LOW_START + i];
        }
        
        // TODO: 在这里添加舵机控制业务代码
        // 示例：发送舵机控制命令到各个舵机 (ID 1-6)
        /*
        for (int servo_id = 1; servo_id <= 6; servo_id++) {
            bus_servo_set_position(servo_id, target_angles[servo_id-1], rotation_time);
        }
        */
        
        // 立即置状态为2 (执行中)
        holding_regs[REG_ROTATION_TRIGGER] = 2;
        last_rotation_trigger = 2;
        
        cdc_debug_print("Servo rotation command sent");
    }
    
    // 检查读取触发 (40037)
    if (holding_regs[REG_READ_TRIGGER] == 1 && last_read_trigger != 1) {
        cdc_debug_print("Servo read triggered");
        
        // TODO: 在这里添加舵机位置读取业务代码
        // 示例：读取所有舵机当前位置
        /*
        for (int servo_id = 1; servo_id <= 6; servo_id++) {
            uint16_t current_pos = bus_servo_get_position(servo_id);
            // 更新当前角度寄存器 (40024-40035)
            holding_regs[REG_CURRENT_ANGLE_LOW_START + servo_id - 1] = current_pos & 0xFFFF;
            holding_regs[REG_CURRENT_ANGLE_HIGH_START + servo_id - 1] = (current_pos >> 16) & 0xFFFF;
        }
        */
        
        // 读取完成后置状态为2
        holding_regs[REG_READ_TRIGGER] = 2;
        last_read_trigger = 2;
        
        cdc_debug_print("Servo positions read");
    }
    
    // 更新上次状态
    last_rotation_trigger = holding_regs[REG_ROTATION_TRIGGER];
    last_read_trigger = holding_regs[REG_READ_TRIGGER];
}

/**
  * @brief  处理称重相关命令
  * @retval None
  */
static void process_weight_commands(void) {
    // 检查称重控制位 (40040)
    if (holding_regs[REG_WEIGHT_CONTROL] == 1 && last_weight_control != 1) {
        cdc_debug_print("Weight measurement started");
        
        // 置为忙状态
        holding_regs[REG_WEIGHT_CONTROL] = 2;
        
        // TODO: 在这里添加称重传感器业务代码
        // 示例：开始称重流程
        /*
        // 1. 清零称重传感器
        loadcell_driver_tare();
        
        // 2. 等待稳定 (这里可以用状态机实现)
        // 可以设置一个定时器或计数器，稳定后执行下一步
        
        // 3. 读取重量值并更新寄存器
        int32_t weight = loadcell_driver_read_weight();
        holding_regs[REG_WEIGHT_LOW] = weight & 0xFFFF;
        holding_regs[REG_WEIGHT_HIGH] = (weight >> 16) & 0xFFFF;
        
        // 4. 称重完成，置状态为3
        holding_regs[REG_WEIGHT_CONTROL] = 3;
        */
        
        cdc_debug_print("Weight measurement in progress");
    }
    
    // 更新上次状态
    last_weight_control = holding_regs[REG_WEIGHT_CONTROL];
}

/**
  * @brief  处理泵相关命令
  * @retval None
  */
static void process_pump_commands(void) {
    // 处理泵1命令
    // 检查泵1初始化触发 (40041)
    if (holding_regs[REG_PUMP1_INIT_TRIGGER] == 1 && last_pump1_init_trigger != 1) {
        cdc_debug_print("Pump1 initialization triggered");
        
        // 发送泵1初始化命令
        if (pump_init_device(1) == 0) {
            cdc_debug_print("Pump1 init command sent successfully");
            pump1_current_pos = 0;  // 初始化后位置归零
            holding_regs[REG_PUMP1_CURRENT_POSITION] = 0;
            holding_regs[REG_PUMP1_STATUS] = 1;  // 设置为不忙状态
        } else {
            cdc_debug_print("Pump1 init command failed");
        }
        
        // 发送后置状态为2
        holding_regs[REG_PUMP1_INIT_TRIGGER] = 2;
        last_pump1_init_trigger = 2;
    }
    
    // 检查泵1控制触发 (40043)
    if (holding_regs[REG_PUMP1_CONTROL_TRIGGER] == 1 && last_pump1_control_trigger != 1) {
        if (!pump1_busy) {  // 只有在泵不忙时才处理移动命令
            cdc_debug_print("Pump1 control triggered");
            
            // 获取目标位置 (40042)
            uint16_t target_position = holding_regs[REG_PUMP1_ABS_POSITION];
            
            // 验证目标位置范围 (0-6000)
            if (target_position <= 6000) {
                // 发送泵1移动命令
                if (pump_move_absolute_device(1, target_position) == 0) {
                    pump1_busy = 1;  // 设置忙状态
                    pump1_move_start_time = HAL_GetTick();  // 记录开始时间
                    holding_regs[REG_PUMP1_STATUS] = 2;  // 设置为忙状态
                    cdc_debug_print("Pump1 move command sent successfully");
                    
                    // 调试信息：显示目标位置
                    char pos_msg[50];
                    snprintf(pos_msg, sizeof(pos_msg), "Pump1 moving to position: %d", target_position);
                    cdc_debug_print(pos_msg);
                } else {
                    cdc_debug_print("Pump1 move command failed");
                }
            } else {
                cdc_debug_print("Pump1 target position out of range (0-6000)");
            }
        } else {
            cdc_debug_print("Pump1 is busy, ignoring move command");
        }
        
        // 根据40042发送移动命令后置状态为2
        holding_regs[REG_PUMP1_CONTROL_TRIGGER] = 2;
        last_pump1_control_trigger = 2;
    }
    
    // 处理泵2命令 (类似泵1)
    // 检查泵2初始化触发 (40046)
    if (holding_regs[REG_PUMP2_INIT_TRIGGER] == 1 && last_pump2_init_trigger != 1) {
        cdc_debug_print("Pump2 initialization triggered");
        
        // 发送泵2初始化命令
        if (pump_init_device(2) == 0) {
            cdc_debug_print("Pump2 init command sent successfully");
            pump2_current_pos = 0;  // 初始化后位置归零
            holding_regs[REG_PUMP2_CURRENT_POSITION] = 0;
            holding_regs[REG_PUMP2_STATUS] = 1;  // 设置为不忙状态
        } else {
            cdc_debug_print("Pump2 init command failed");
        }
        
        // 发送后置状态为2
        holding_regs[REG_PUMP2_INIT_TRIGGER] = 2;
        last_pump2_init_trigger = 2;
    }
    
    // 检查泵2控制触发 (40048)
    if (holding_regs[REG_PUMP2_CONTROL_TRIGGER] == 1 && last_pump2_control_trigger != 1) {
        if (!pump2_busy) {  // 只有在泵不忙时才处理移动命令
            cdc_debug_print("Pump2 control triggered");
            
            // 获取目标位置 (40047)
            uint16_t target_position = holding_regs[REG_PUMP2_ABS_POSITION];
            
            // 验证目标位置范围 (0-6000)
            if (target_position <= 6000) {
                // 发送泵2移动命令
                if (pump_move_absolute_device(2, target_position) == 0) {
                    pump2_busy = 1;  // 设置忙状态
                    pump2_move_start_time = HAL_GetTick();  // 记录开始时间
                    holding_regs[REG_PUMP2_STATUS] = 2;  // 设置为忙状态
                    cdc_debug_print("Pump2 move command sent successfully");
                    
                    // 调试信息：显示目标位置
                    char pos_msg[50];
                    snprintf(pos_msg, sizeof(pos_msg), "Pump2 moving to position: %d", target_position);
                    cdc_debug_print(pos_msg);
                } else {
                    cdc_debug_print("Pump2 move command failed");
                }
            } else {
                cdc_debug_print("Pump2 target position out of range (0-6000)");
            }
        } else {
            cdc_debug_print("Pump2 is busy, ignoring move command");
        }
        
        // 根据40047发送移动命令后置状态为2
        holding_regs[REG_PUMP2_CONTROL_TRIGGER] = 2;
        last_pump2_control_trigger = 2;
    }
    
    // 更新泵状态（检查是否完成移动）
    pump_update_status(1);  // 更新泵1状态
    pump_update_status(2);  // 更新泵2状态
    
    // 更新上次状态
    last_pump1_init_trigger = holding_regs[REG_PUMP1_INIT_TRIGGER];
    last_pump1_control_trigger = holding_regs[REG_PUMP1_CONTROL_TRIGGER];
    last_pump2_init_trigger = holding_regs[REG_PUMP2_INIT_TRIGGER];
    last_pump2_control_trigger = holding_regs[REG_PUMP2_CONTROL_TRIGGER];
}

/**
  * @brief  更新传感器数据 (只读寄存器)
  * @retval None
  */
static void update_sensor_data(void) {
    // TODO: 在这里添加传感器数据更新业务代码
    
    // 示例1：更新舵机当前角度 (40024-40035)
    /*
    static uint32_t servo_read_counter = 0;
    if (servo_read_counter % 10 == 0) {  // 每1秒更新一次舵机位置
        for (int servo_id = 1; servo_id <= 6; servo_id++) {
            uint16_t current_pos = bus_servo_get_position(servo_id);
            holding_regs[REG_CURRENT_ANGLE_LOW_START + servo_id - 1] = current_pos & 0xFFFF;
            holding_regs[REG_CURRENT_ANGLE_HIGH_START + servo_id - 1] = (current_pos >> 16) & 0xFFFF;
        }
    }
    servo_read_counter++;
    */
    
    // 示例2：更新称重数据 (40038-40039)
    /*
    static uint32_t weight_read_counter = 0;
    if (weight_read_counter % 5 == 0) {  // 每500ms更新一次重量
        if (holding_regs[REG_WEIGHT_CONTROL] == 3) {  // 称重完成状态才更新
            int32_t weight = loadcell_driver_read_weight();
            holding_regs[REG_WEIGHT_LOW] = weight & 0xFFFF;
            holding_regs[REG_WEIGHT_HIGH] = (weight >> 16) & 0xFFFF;
        }
    }
    weight_read_counter++;
    */
    
    // 示例3：更新泵状态和位置 (40044-40045, 40049-40050)
    // 注意：泵状态已在process_pump_commands中实时更新，这里主要做周期性验证
    static uint32_t pump_read_counter = 0;
    if (pump_read_counter % 50 == 0) {  // 每5秒进行一次状态验证
        // 这里可以添加泵状态的周期性查询验证
        // 当前的状态更新主要在pump_update_status中处理
        
        // 可选：发送状态查询命令来验证泵的实际状态
        /*
        pump_get_status_device(1);
        pump_get_status_device(2);
        */
    }
    pump_read_counter++;
    
    // 示例4：更新物体检测状态 (40051)
    /*
    static uint32_t detection_read_counter = 0;
    if (detection_read_counter % 2 == 0) {  // 每200ms更新一次检测状态
        uint8_t object_detected = gpio_read_object_sensor();
        holding_regs[REG_OBJECT_DETECTION] = object_detected;
    }
    detection_read_counter++;
    */
}

/* 泵控制辅助函数实现 --------------------------------------------------------*/

/**
  * @brief  通过UART发送泵命令
  * @param  pump_id: 泵ID (1或2)
  * @param  cmd: 要发送的命令字符串
  * @retval 0: 成功, -1: 失败
  */
static int pump_send_command_uart(uint8_t pump_id, const char* cmd) {
    // 直接使用syringe_pump.c中的send_command函数
    // 注意：现在所有泵都通过USART3通信，pump_id用于调试信息
    return send_command(pump_id, cmd, NULL, 0);
}

/**
  * @brief  初始化指定泵
  * @param  pump_id: 泵ID (1或2)
  * @retval 0: 成功, -1: 失败
  */
static int pump_init_device(uint8_t pump_id) {
    return pump_init(pump_id);
}

/**
  * @brief  泵移动到绝对位置
  * @param  pump_id: 泵ID (1或2)
  * @param  position: 目标位置 (0-6000步)
  * @retval 0: 成功, -1: 失败
  */
static int pump_move_absolute_device(uint8_t pump_id, uint16_t position) {
    return pump_move_absolute(pump_id, position);
}

/**
  * @brief  查询泵状态
  * @param  pump_id: 泵ID (1或2)
  * @retval 0: 成功, -1: 失败
  */
static int pump_get_status_device(uint8_t pump_id) {
    char status_buffer[256];
    int result = pump_get_status(pump_id, status_buffer);
    
    if (result > 0) {
        // 调试信息：显示状态响应
        char debug_msg[128];
        snprintf(debug_msg, sizeof(debug_msg), "Pump%d status: %s", pump_id, status_buffer);
        cdc_debug_print(debug_msg);
    }
    
    return result;
}

/**
  * @brief  更新泵状态
  * @param  pump_id: 泵ID (1或2)
  * @retval None
  */
static void pump_update_status(uint8_t pump_id) {
    uint32_t current_time = HAL_GetTick();
    uint32_t timeout_ms = 30000;  // 30秒超时
    
    if (pump_id == 1) {
        if (pump1_busy) {
            // 检查超时
            if (current_time - pump1_move_start_time > timeout_ms) {
                pump1_busy = 0;
                holding_regs[REG_PUMP1_STATUS] = 1;  // 超时后设置为不忙
                cdc_debug_print("Pump1 movement timeout, setting to idle");
            } else {
                // 这里可以添加实际的状态查询逻辑
                // 目前简单地假设经过一定时间后移动完成
                // 实际应用中可以通过查询状态命令来确定是否完成
                
                // 模拟：2秒后移动完成
                if (current_time - pump1_move_start_time > 2000) {
                    pump1_busy = 0;
                    holding_regs[REG_PUMP1_STATUS] = 1;  // 设置为不忙
                    // 更新当前位置为目标位置
                    pump1_current_pos = holding_regs[REG_PUMP1_ABS_POSITION];
                    holding_regs[REG_PUMP1_CURRENT_POSITION] = pump1_current_pos;
                    cdc_debug_print("Pump1 movement completed");
                }
            }
        }
    } else if (pump_id == 2) {
        if (pump2_busy) {
            // 检查超时
            if (current_time - pump2_move_start_time > timeout_ms) {
                pump2_busy = 0;
                holding_regs[REG_PUMP2_STATUS] = 1;  // 超时后设置为不忙
                cdc_debug_print("Pump2 movement timeout, setting to idle");
            } else {
                // 模拟：2秒后移动完成
                if (current_time - pump2_move_start_time > 2000) {
                    pump2_busy = 0;
                    holding_regs[REG_PUMP2_STATUS] = 1;  // 设置为不忙
                    // 更新当前位置为目标位置
                    pump2_current_pos = holding_regs[REG_PUMP2_ABS_POSITION];
                    holding_regs[REG_PUMP2_CURRENT_POSITION] = pump2_current_pos;
                    cdc_debug_print("Pump2 movement completed");
                }
            }
        }
    }
}