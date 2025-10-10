/**
  ******************************************************************************
  * @file    app_business.c
  * @brief   应用层业务逻辑处理实现文件
  ******************************************************************************
  * @attention
  *
  * 此文件实现分配器设备的所有业务逻辑处理功能    // 检查泵1初始化触发 (40026)
    if (holding_regs[REG_PUMP1_INIT_TRIGGER] == 1 && 
        (last_pump1_init_trigger == 0 || last_pump1_init_trigger == 2)) {  // 从0或2状态可以接受1
        cdc_debug_print("Pump1 initialization triggered");
        
        // 发送泵1初始化命令
        if (pump_init_device(1) == 0) {
            cdc_debug_print("Pump1 init command sent successfully");
            // 位置和状态将通过查询来更新，不在这里直接设置
        } else {
            cdc_debug_print("Pump1 init command failed");
        }
        
        // 发送后置状态为2
        holding_regs[REG_PUMP1_INIT_TRIGGER] = 2;
        last_pump1_init_trigger = 2;
    }**********************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "app_business.h"
#include "usbd_cdc_if.h"
#include "syringe_pump.h"
#include "bus_servo.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
// 业务状态变量
static uint32_t last_rotation_trigger = 0;       // 上次转动触发状态
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

// 舵机控制相关状态变量
static uint8_t servo_moving = 0;                  // 舵机移动状态 (0=空闲, 1=移动中)
static uint32_t servo_move_start_time = 0;        // 舵机开始移动时间
static uint16_t servo_current_positions[6];       // 6个舵机的当前位置
// static uint8_t servo_connected[6] = {0, 0, 0, 0, 0, 0};  // 舵机连接状态，0=未知，1=已连接，2=无响应 (暂时未使用)

/* Private function prototypes -----------------------------------------------*/
static void process_servo_commands(void);
static void process_weight_commands(void);
static void process_pump_commands(void);
static void update_sensor_data(void);
static void cdc_debug_print(const char* message);

// 泵控制辅助函数
// static int pump_send_command_uart(uint8_t pump_id, const char* cmd);  // 暂时未使用
static int pump_init_device(uint8_t pump_id);
static int pump_move_absolute_device(uint8_t pump_id, uint16_t position);
// static int pump_get_status_device(uint8_t pump_id);  // 暂时未使用
static void pump_update_status(uint8_t pump_id);

// 舵机控制辅助函数
static void servo_move_all(uint16_t* target_angles, uint32_t move_time);
static void servo_read_all_positions(void);
static void servo_update_status(void);
// static HAL_StatusTypeDef servo_test_connection(uint8_t servo_id);  // 暂时未使用

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  初始化应用层业务逻辑
  * @retval None
  */
void app_business_init(void) {
    // 初始化寄存器默认值
    
    // 初始化舵机相关寄存器
    holding_regs[REG_ROTATION_TRIGGER] = 3;               // 40023 转动触发：3=上电初始化值
    // 初始化当前角度寄存器为默认值500 (40017-40022)
    holding_regs[REG_CURRENT_ANGLE1] = 500;
    holding_regs[REG_CURRENT_ANGLE2] = 500;
    holding_regs[REG_CURRENT_ANGLE3] = 500;
    holding_regs[REG_CURRENT_ANGLE4] = 500;
    holding_regs[REG_CURRENT_ANGLE5] = 500;
    holding_regs[REG_CURRENT_ANGLE6] = 500;
    
    // 初始化泵相关寄存器
    holding_regs[REG_PUMP1_STATUS] = 999;                 // 40029 泵1状态：999=未知状态，等待查询
    holding_regs[REG_PUMP2_STATUS] = 999;                 // 40034 泵2状态：999=未知状态，等待查询

    
    // 初始化业务状态变量
    last_rotation_trigger = 3;    // 对应寄存器的初始值
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
    
    // 初始化舵机状态变量
    servo_moving = 0;
    servo_move_start_time = 0;
    for (int i = 0; i < 6; i++) {
        servo_current_positions[i] = 500;  // 默认位置500
    }
    
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
        CDC_Transmit_FS((uint8_t*)debug_buf, len);  // 暂时启用调试输出
    }
}

/**
  * @brief  处理舵机相关命令
  * @retval None
  */
static void process_servo_commands(void) {
    // 检查转动触发 (40023)
    // 状态机逻辑：
    // 1 = 正在执行任务（忙）
    // 2 = 任务完成，可重新写入1开始新任务
    // 3 = 上电初始化值，可接受写入1
    // 当写入1时执行移动，执行期间保持状态1，完成后置为状态2
    if (holding_regs[REG_ROTATION_TRIGGER] == 1 && 
        (last_rotation_trigger == 2 || last_rotation_trigger == 3)) {  // 只有从状态2或3才能接受状态1
        if (!servo_moving) {  // 只有在舵机不在移动时才处理新命令
            cdc_debug_print("Servo rotation triggered, status 1 (busy)");
            
            // 获取转动时间 (40010) - 现在只用一个寄存器
            uint16_t rotation_time = holding_regs[REG_ROTATION_TIME];
            
            // 验证转动时间范围 (1-65535ms)
            if (rotation_time == 0) {
                rotation_time = 1000;  // 默认1秒
                cdc_debug_print("Using default rotation time: 1000ms");
            }
            
            // 获取目标角度 (40011-40016，每个角度占用1个寄存器)
            uint16_t target_angles[6];
            target_angles[0] = holding_regs[REG_TARGET_ANGLE1];
            target_angles[1] = holding_regs[REG_TARGET_ANGLE2];
            target_angles[2] = holding_regs[REG_TARGET_ANGLE3];
            target_angles[3] = holding_regs[REG_TARGET_ANGLE4];
            target_angles[4] = holding_regs[REG_TARGET_ANGLE5];
            target_angles[5] = holding_regs[REG_TARGET_ANGLE6];
            
            // 验证角度范围 (0-1000)
            for (int i = 0; i < 6; i++) {
                if (target_angles[i] > 1000) {
                    char warning_msg[60];
                    snprintf(warning_msg, sizeof(warning_msg), "Servo%d angle limited to 1000 (was %d)", i+1, target_angles[i]);
                    cdc_debug_print(warning_msg);
                    target_angles[i] = 1000;
                }
            }
            
            // 调试信息：显示所有目标角度
            char angles_msg[128];
            snprintf(angles_msg, sizeof(angles_msg), "Target angles: [%d,%d,%d,%d,%d,%d] Time:%dms", 
                    target_angles[0], target_angles[1], target_angles[2], 
                    target_angles[3], target_angles[4], target_angles[5], rotation_time);
            cdc_debug_print(angles_msg);
            
            // 发送舵机控制命令到各个舵机 (ID 1-6)
            servo_move_all(target_angles, rotation_time);
            
            // 设置移动状态
            servo_moving = 1;
            servo_move_start_time = HAL_GetTick();
            
            // 状态保持为1表示正在执行任务（忙）
            // holding_regs[REG_ROTATION_TRIGGER] = 1;  // 状态已经是1，不需要改变
            last_rotation_trigger = 1;
            
            cdc_debug_print("Servo rotation commands sent, status remains 1 (busy)");
        } else {
            cdc_debug_print("Servos are busy, ignoring rotation command");
        }
    } else if (holding_regs[REG_ROTATION_TRIGGER] == 1 && last_rotation_trigger == 1) {
        // 如果连续写入1，给出警告（系统正忙）
        cdc_debug_print("Warning: Servo rotation command ignored - system busy (status 1)");
    }
    
    // 更新舵机状态（检查是否完成移动）
    servo_update_status();
    
    // 更新上次状态
    last_rotation_trigger = holding_regs[REG_ROTATION_TRIGGER];
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
    // 检查泵1初始化触发 (40026) - 只有当错误码为0且不忙时才允许
    if (holding_regs[REG_PUMP1_INIT_TRIGGER] == 1 && 
        holding_regs[REG_PUMP1_STATUS] == 0 && pump1_busy == 0) {  // 错误码为0且不忙
        cdc_debug_print("Pump1 initialization triggered (idle & no error)");
        
        // 发送泵1初始化命令
        if (pump_init_device(1) == 0) {
            cdc_debug_print("Pump1 init command sent successfully");
            // 发送后立即置状态为2
            holding_regs[REG_PUMP1_INIT_TRIGGER] = 2;
            last_pump1_init_trigger = 2;
        } else {
            cdc_debug_print("Pump1 init command failed");
            // 发送失败也置状态为2
            holding_regs[REG_PUMP1_INIT_TRIGGER] = 2;
        }
    } else if (holding_regs[REG_PUMP1_INIT_TRIGGER] == 1 && 
               (holding_regs[REG_PUMP1_STATUS] != 0 || pump1_busy == 1)) {
        char reject_msg[80];
        snprintf(reject_msg, sizeof(reject_msg), "Pump1 init rejected: error=%d, busy=%d", 
                holding_regs[REG_PUMP1_STATUS], pump1_busy);
        cdc_debug_print(reject_msg);
        // 立即置初始化位为2，表示无法执行
        holding_regs[REG_PUMP1_INIT_TRIGGER] = 2;
    }
    
    // 检查泵1控制触发 (40028) - 只有当错误码为0且不忙时才允许
    if (holding_regs[REG_PUMP1_CONTROL_TRIGGER] == 1 && 
        holding_regs[REG_PUMP1_STATUS] == 0 && pump1_busy == 0) {  // 错误码为0且不忙
        cdc_debug_print("Pump1 control triggered (idle & no error)");
        
        // 获取目标位置 (40027)
        uint16_t target_position = holding_regs[REG_PUMP1_ABS_POSITION];
        
        // 验证目标位置范围 (0-6000)
        if (target_position <= 6000) {
            // 发送泵1移动命令
            if (pump_move_absolute_device(1, target_position) == 0) {
                pump1_move_start_time = HAL_GetTick();  // 记录开始时间
                cdc_debug_print("Pump1 move command sent successfully");
                
                // 调试信息：显示目标位置
                char pos_msg[50];
                snprintf(pos_msg, sizeof(pos_msg), "Pump1 moving to position: %d", target_position);
                cdc_debug_print(pos_msg);
                
                // 发送后立即置状态为2
                holding_regs[REG_PUMP1_CONTROL_TRIGGER] = 2;
                last_pump1_control_trigger = 2;
            } else {
                cdc_debug_print("Pump1 move command failed");
                // 发送失败也置状态为2
                holding_regs[REG_PUMP1_CONTROL_TRIGGER] = 2;
            }
        } else {
            cdc_debug_print("Pump1 target position out of range (0-6000)");
            // 参数错误也置状态为2
            holding_regs[REG_PUMP1_CONTROL_TRIGGER] = 2;
        }
    } else if (holding_regs[REG_PUMP1_CONTROL_TRIGGER] == 1 && 
               (holding_regs[REG_PUMP1_STATUS] != 0 || pump1_busy == 1)) {
        char reject_msg[80];
        snprintf(reject_msg, sizeof(reject_msg), "Pump1 control rejected: error=%d, busy=%d", 
                holding_regs[REG_PUMP1_STATUS], pump1_busy);
        cdc_debug_print(reject_msg);
        // 立即置控制位为2，表示无法执行
        holding_regs[REG_PUMP1_CONTROL_TRIGGER] = 2;
    }
    
    // 处理泵2命令 (类似泵1)
    // 检查泵2初始化触发 (40031) - 只有当错误码为0且不忙时才允许
    if (holding_regs[REG_PUMP2_INIT_TRIGGER] == 1 && 
        holding_regs[REG_PUMP2_STATUS] == 0 && pump2_busy == 0) {  // 错误码为0且不忙
        cdc_debug_print("Pump2 initialization triggered (idle & no error)");
        
        // 发送泵2初始化命令
        if (pump_init_device(2) == 0) {
            cdc_debug_print("Pump2 init command sent successfully");
            // 发送后立即置状态为2
            holding_regs[REG_PUMP2_INIT_TRIGGER] = 2;
            last_pump2_init_trigger = 2;
        } else {
            cdc_debug_print("Pump2 init command failed");
            // 发送失败也置状态为2
            holding_regs[REG_PUMP2_INIT_TRIGGER] = 2;
        }
    } else if (holding_regs[REG_PUMP2_INIT_TRIGGER] == 1 && 
               (holding_regs[REG_PUMP2_STATUS] != 0 || pump2_busy == 1)) {
        char reject_msg[80];
        snprintf(reject_msg, sizeof(reject_msg), "Pump2 init rejected: error=%d, busy=%d", 
                holding_regs[REG_PUMP2_STATUS], pump2_busy);
        cdc_debug_print(reject_msg);
        // 立即置初始化位为2，表示无法执行
        holding_regs[REG_PUMP2_INIT_TRIGGER] = 2;
    }
    
    // 检查泵2控制触发 (40033) - 只有当错误码为0且不忙时才允许
    if (holding_regs[REG_PUMP2_CONTROL_TRIGGER] == 1 && 
        holding_regs[REG_PUMP2_STATUS] == 0 && pump2_busy == 0) {  // 错误码为0且不忙
            cdc_debug_print("Pump2 control triggered (idle & no error)");
            
            // 获取目标位置 (40032)
            uint16_t target_position = holding_regs[REG_PUMP2_ABS_POSITION];
            
            // 验证目标位置范围 (0-6000)
            if (target_position <= 6000) {
                // 发送泵2移动命令
                if (pump_move_absolute_device(2, target_position) == 0) {
                    pump2_move_start_time = HAL_GetTick();  // 记录开始时间
                    cdc_debug_print("Pump2 move command sent successfully");
                    
                    // 调试信息：显示目标位置
                    char pos_msg[50];
                    snprintf(pos_msg, sizeof(pos_msg), "Pump2 moving to position: %d", target_position);
                    cdc_debug_print(pos_msg);
                    
                    // 发送后立即置状态为2
                    holding_regs[REG_PUMP2_CONTROL_TRIGGER] = 2;
                    last_pump2_control_trigger = 2;
                } else {
                    cdc_debug_print("Pump2 move command failed");
                    // 发送失败也置状态为2
                    holding_regs[REG_PUMP2_CONTROL_TRIGGER] = 2;
                }
            } else {
                cdc_debug_print("Pump2 target position out of range (0-6000)");
                // 参数错误也置状态为2
                holding_regs[REG_PUMP2_CONTROL_TRIGGER] = 2;
            }
    } else if (holding_regs[REG_PUMP2_CONTROL_TRIGGER] == 1 && 
               (holding_regs[REG_PUMP2_STATUS] != 0 || pump2_busy == 1)) {
        char reject_msg[80];
        snprintf(reject_msg, sizeof(reject_msg), "Pump2 control rejected: error=%d, busy=%d", 
                holding_regs[REG_PUMP2_STATUS], pump2_busy);
        cdc_debug_print(reject_msg);
        // 立即置控制位为2，表示无法执行
        holding_regs[REG_PUMP2_CONTROL_TRIGGER] = 2;
    }
    
    // 更新泵状态（检查是否完成移动）
    pump_update_status(1);  // 更新泵1状态
    pump_update_status(2);  // 更新泵2状态
}

/**
  * @brief  更新传感器数据 (只读寄存器)
  * @retval None
  */
static void update_sensor_data(void) {
    // TODO: 在这里添加传感器数据更新业务代码
    
    // 示例1：更新舵机当前角度 (40017-40022，每个角度占用1个寄存器)
    // 注意：舵机位置已在servo_update_status中更新，这里做周期性验证
    static uint32_t servo_read_counter = 0;
    if (servo_read_counter % 10 == 0 && !servo_moving) {  // 每1秒更新一次，且不在移动时
        // 周期性读取舵机位置进行验证
        servo_read_all_positions();
        // 更新当前角度寄存器 (40017-40022)
        holding_regs[REG_CURRENT_ANGLE1] = servo_current_positions[0];
        holding_regs[REG_CURRENT_ANGLE2] = servo_current_positions[1];
        holding_regs[REG_CURRENT_ANGLE3] = servo_current_positions[2];
        holding_regs[REG_CURRENT_ANGLE4] = servo_current_positions[3];
        holding_regs[REG_CURRENT_ANGLE5] = servo_current_positions[4];
        holding_regs[REG_CURRENT_ANGLE6] = servo_current_positions[5];
    }
    servo_read_counter++;
    
    // 示例2：更新称重数据 (40024)
    /*
    static uint32_t weight_read_counter = 0;
    if (weight_read_counter % 5 == 0) {  // 每500ms更新一次重量
        if (holding_regs[REG_WEIGHT_CONTROL] == 2) {  // 称重完成状态才更新
            int32_t weight = loadcell_driver_read_weight();
            // 现在重量只用一个寄存器，如果需要负数支持，可以用signed int16_t
            holding_regs[REG_WEIGHT] = (uint16_t)weight;  // 假设重量在 -32768 到 32767g 范围内
        }
    }
    weight_read_counter++;
    */
    
    // 示例3：更新泵状态和位置 (40029-40030, 40034-40035)
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
    
    // 示例4：更新物体检测状态 (40036)
    /*
    static uint32_t detection_read_counter = 0;
    if (detection_read_counter % 2 == 0) {  // 每200ms更新一次检测状态
        uint8_t object_detected = gpio_read_object_sensor();
        holding_regs[REG_OBJECT_DETECTION] = object_detected ? 2 : 1;  // 1=无物体；2=有
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
/*
// 暂时未使用的泵控制函数
static int pump_send_command_uart(uint8_t pump_id, const char* cmd) {
    // 直接使用syringe_pump.c中的send_command函数
    // 注意：现在所有泵都通过USART3通信，pump_id用于调试信息
    return send_command(pump_id, cmd, NULL, 0);
}
*/

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

/*
// 暂时未使用的泵状态查询函数
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
*/

/**
  * @brief  更新泵状态
  * @param  pump_id: 泵ID (1或2)
  * @retval None
  */
/**
  * @brief  更新泵状态 - 使用实际的DT协议查询
  * @param  pump_id: 泵ID (1或2)
  * @retval None
  */
static void pump_update_status(uint8_t pump_id) {
    static uint32_t last_update_time1 = 0;  // 泵1的时间控制
    static uint32_t last_update_time2 = 0;  // 泵2的时间控制
    uint32_t current_time = HAL_GetTick();
    
    // 根据泵ID选择对应的时间控制变量
    uint32_t* last_time_ptr = (pump_id == 1) ? &last_update_time1 : &last_update_time2;
    
    // 降低查询频率，每1秒查询一次
    if (current_time - *last_time_ptr < 1000) {
        return;
    }
    *last_time_ptr = current_time;
    
    char error_response[256];
    int current_position;
    
    if (pump_id == 1) {
        // 查询错误码 (/1QR)
        if (pump_query_error(1, error_response) == 0) {
            // 从响应中解析状态字节
            // 响应格式：FF /0`3000 03 0D 0A
            // 第3个字节（'`'字符）是状态字节的HEX值
            char* status_byte_ptr = strchr(error_response, '`');
            if (status_byte_ptr != NULL) {
                // 提取状态字节
                uint8_t status_byte = (uint8_t)(*status_byte_ptr);
                
                // 使用枚举解析状态字节，转换为错误编号
                PumpErrorCode_t error_code = pump_parse_status_byte(status_byte);
                PumpBusyState_t busy_state = pump_parse_busy_state(status_byte);
                
                // 直接将错误码写入状态寄存器
                holding_regs[REG_PUMP1_STATUS] = (uint16_t)error_code;
                
                // 更新内部忙状态逻辑
                pump1_busy = (busy_state == PUMP_STATE_BUSY) ? 1 : 0;
                
                char debug_msg[100];
                snprintf(debug_msg, sizeof(debug_msg), "Pump1: byte=0x%02X, error=%d, busy=%s", 
                        status_byte, error_code, (busy_state == PUMP_STATE_BUSY) ? "YES" : "NO");
                cdc_debug_print(debug_msg);
            }
        } else {
            cdc_debug_print("Pump1 query error failed");
            // 通信失败，设置为通信错误状态
            holding_regs[REG_PUMP1_STATUS] = 999;  // 999 = 通信失败
            pump1_busy = 1;  // 通信失败时认为忙碌（安全考虑）
        }
        
        // 查询当前活塞位置 (/1?4R)
        if (pump_query_position(1, &current_position) == 0) {
            pump1_current_pos = current_position;
            holding_regs[REG_PUMP1_CURRENT_POSITION] = current_position;
            
            char pos_msg[50];
            snprintf(pos_msg, sizeof(pos_msg), "Pump1 position: %d", current_position);
            cdc_debug_print(pos_msg);
        } else {
            cdc_debug_print("Pump1 position query failed");
        }
        
    } else if (pump_id == 2) {
        // 查询错误码 (/2QR)
        if (pump_query_error(2, error_response) == 0) {
            // 从响应中解析状态字节
            // 响应格式：FF /0`3000 03 0D 0A
            // 第3个字节（'`'字符）是状态字节的HEX值
            char* status_byte_ptr = strchr(error_response, '`');
            if (status_byte_ptr != NULL) {
                // 提取状态字节
                uint8_t status_byte = (uint8_t)(*status_byte_ptr);
                
                // 使用枚举解析状态字节，转换为错误编号
                PumpErrorCode_t error_code = pump_parse_status_byte(status_byte);
                PumpBusyState_t busy_state = pump_parse_busy_state(status_byte);
                
                // 直接将错误码写入状态寄存器
                holding_regs[REG_PUMP2_STATUS] = (uint16_t)error_code;
                
                // 更新内部忙状态逻辑
                pump2_busy = (busy_state == PUMP_STATE_BUSY) ? 1 : 0;
                
                char debug_msg[100];
                snprintf(debug_msg, sizeof(debug_msg), "Pump2: byte=0x%02X, error=%d, busy=%s", 
                        status_byte, error_code, (busy_state == PUMP_STATE_BUSY) ? "YES" : "NO");
                cdc_debug_print(debug_msg);
            }
        } else {
            cdc_debug_print("Pump2 UART RX timeout");
            // 通信失败，设置为通信错误状态
            holding_regs[REG_PUMP2_STATUS] = 999;  // 999 = 通信失败
            pump2_busy = 1;  // 通信失败时认为忙碌（安全考虑）
        }
        
        // 查询当前活塞位置 (/2?4R)
        if (pump_query_position(2, &current_position) == 0) {
            pump2_current_pos = current_position;
            holding_regs[REG_PUMP2_CURRENT_POSITION] = current_position;
            
            char pos_msg[50];
            snprintf(pos_msg, sizeof(pos_msg), "Pump2 position: %d", current_position);
            cdc_debug_print(pos_msg);
        } else {
            cdc_debug_print("Pump2 position query failed");
        }
    }
}

/* 舵机控制辅助函数实现 --------------------------------------------------------*/

/**
  * @brief  控制所有舵机移动到目标位置
  * @param  target_angles: 6个舵机的目标角度数组
  * @param  move_time: 移动时间(ms)
  * @retval None
  */
static void servo_move_all(uint16_t* target_angles, uint32_t move_time) {
    // 使用多舵机同时移动命令 - 更高效的方式
    uint8_t servo_ids[6] = {1, 2, 3, 4, 5, 6};
    BusServo_MultMove(servo_ids, target_angles, 6, (uint16_t)move_time);
    
    // 调试信息：显示发送的命令
    char move_msg[128];
    snprintf(move_msg, sizeof(move_msg), "MultMove sent: [%d,%d,%d,%d,%d,%d] time:%lums", 
            target_angles[0], target_angles[1], target_angles[2], 
            target_angles[3], target_angles[4], target_angles[5], move_time);
    cdc_debug_print(move_msg);
    
    // 备用方法：单独控制每个舵机（如果多舵机命令有问题可以启用）
    /*
    for (int servo_id = 1; servo_id <= 6; servo_id++) {
        BusServo_Move(servo_id, target_angles[servo_id-1], (uint16_t)move_time);
        
        // 小延时确保命令发送完成
        HAL_Delay(10);
        
        // 调试信息 - 记录发送的命令
        char move_msg[80];
        snprintf(move_msg, sizeof(move_msg), "Servo%d -> %d (time:%lums) sent", 
                servo_id, target_angles[servo_id-1], move_time);
        cdc_debug_print(move_msg);
    }
    */
}

/**
  * @brief  读取所有舵机的当前位置
  * @retval None
  */
static void servo_read_all_positions(void) {
    uint8_t servo_ids[6] = {1, 2, 3, 4, 5, 6};
    uint16_t positions[6];
    
    // 使用多舵机位置读取函数（非阻塞版本）
    BusServo_MultPosRead(servo_ids, 6, positions);
    
    // 更新内部状态并记录调试信息
    char pos_debug[128];
    snprintf(pos_debug, sizeof(pos_debug), "Raw read result: [%d,%d,%d,%d,%d,%d]", 
            positions[0], positions[1], positions[2], positions[3], positions[4], positions[5]);
    cdc_debug_print(pos_debug);
    
    // 更新内部状态
    for (int i = 0; i < 6; i++) {
        // 只有当读取到的值在合理范围内时才更新，否则保持原值
        if (positions[i] <= 1000) {
            servo_current_positions[i] = positions[i];
        } else {
            // 如果读取值异常，记录警告但保持原值
            char warning[60];
            snprintf(warning, sizeof(warning), "Servo%d invalid position %d, keeping %d", 
                    i+1, positions[i], servo_current_positions[i]);
            cdc_debug_print(warning);
        }
    }
    
    cdc_debug_print("Servo positions read with timeout protection");
}

/**
  * @brief  更新舵机状态
  * @retval None
  */
static void servo_update_status(void) {
    if (servo_moving) {
        uint32_t current_time = HAL_GetTick();
        uint32_t elapsed_time = current_time - servo_move_start_time;
        
        // 获取当前设置的移动时间
        uint32_t expected_move_time = holding_regs[REG_ROTATION_TIME];
        
        if (expected_move_time == 0) {
            expected_move_time = 1000;  // 默认1秒
        }
        
        // 添加500ms缓冲时间，确保舵机完全到位
        uint32_t timeout_time = expected_move_time + 500;
        
        if (elapsed_time >= timeout_time) {
            servo_moving = 0;
            
            // 读取当前位置并更新寄存器
            servo_read_all_positions();
            // 更新当前角度寄存器 (40017-40022)
            holding_regs[REG_CURRENT_ANGLE1] = servo_current_positions[0];
            holding_regs[REG_CURRENT_ANGLE2] = servo_current_positions[1];
            holding_regs[REG_CURRENT_ANGLE3] = servo_current_positions[2];
            holding_regs[REG_CURRENT_ANGLE4] = servo_current_positions[3];
            holding_regs[REG_CURRENT_ANGLE5] = servo_current_positions[4];
            holding_regs[REG_CURRENT_ANGLE6] = servo_current_positions[5];
            
            // 转动时间到达后，置状态为2（完成状态，可以重新写1进行新转动）
            holding_regs[REG_ROTATION_TRIGGER] = 2;
            last_rotation_trigger = 2;
            
            cdc_debug_print("Servo movement completed, status set to 2 (ready for new command)");
            
            // 调试信息：显示最终位置
            char final_pos_msg[128];
            snprintf(final_pos_msg, sizeof(final_pos_msg), "Final positions: [%d,%d,%d,%d,%d,%d]", 
                    servo_current_positions[0], servo_current_positions[1], servo_current_positions[2],
                    servo_current_positions[3], servo_current_positions[4], servo_current_positions[5]);
            cdc_debug_print(final_pos_msg);
        }
    }
}

/*
// 暂时未使用的测试函数
static HAL_StatusTypeDef servo_test_connection(uint8_t servo_id) {
    uint16_t test_position = BusServo_ReadSinglePosition(servo_id);
    // 如果返回值为默认值500或合理范围内，认为连接正常
    if (test_position <= 1000) {
        return HAL_OK;
    } else {
        return HAL_TIMEOUT;
    }
}
*/