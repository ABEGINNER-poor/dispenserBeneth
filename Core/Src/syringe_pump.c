#include "syringe_pump.h"
#include "usart.h"  // STM32 USART头文件
#include "main.h"   // HAL头文件
#include "usbd_cdc_if.h"  // 用于调试输出

// 发送命令并读取响应 - STM32 USART3版本
int send_command(int pump_id, const char* cmd, char* response, size_t resp_size) {
    char buffer[256];
    char debug_msg[128];
    char pump_address;
    HAL_StatusTypeDef status;
    
    // 根据pump_id选择地址
    if (pump_id == 1) {
        pump_address = '1';  // 泵1地址为'1'
    } else if (pump_id == 2) {
        pump_address = '2';  // 泵2地址为'2'
    } else {
        snprintf(debug_msg, sizeof(debug_msg), "Invalid pump_id: %d\r\n", pump_id);
        CDC_Transmit_FS((uint8_t*)debug_msg, strlen(debug_msg));
        return -1;
    }
    
    // 格式化命令: /地址命令CR
    snprintf(buffer, sizeof(buffer), "/%c%s\r", pump_address, cmd);
    
    // 调试信息
    snprintf(debug_msg, sizeof(debug_msg), "Pump%d TX: %s", pump_id, buffer);
    CDC_Transmit_FS((uint8_t*)debug_msg, strlen(debug_msg));
    
    // 通过USART3发送命令
    status = HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
    if (status != HAL_OK) {
        snprintf(debug_msg, sizeof(debug_msg), "Pump%d UART TX failed: %d\r\n", pump_id, status);
        CDC_Transmit_FS((uint8_t*)debug_msg, strlen(debug_msg));
        return -1;
    }
    
    // 读取响应（如果需要）
    if (response && resp_size > 0) {
        memset(response, 0, resp_size);
        
        // 等待响应，超时时间200ms
        status = HAL_UART_Receive(&huart3, (uint8_t*)response, resp_size - 1, 200);
        if (status == HAL_OK) {
            // 找到实际接收到的数据长度
            size_t actual_len = strlen(response);
            if (actual_len > 0) {
                // 调试信息：显示响应
                snprintf(debug_msg, sizeof(debug_msg), "Pump%d RX: %s", pump_id, response);
                CDC_Transmit_FS((uint8_t*)debug_msg, strlen(debug_msg));
                return actual_len;
            }
        } else if (status == HAL_TIMEOUT) {
            snprintf(debug_msg, sizeof(debug_msg), "Pump%d UART RX timeout\r\n", pump_id);
            CDC_Transmit_FS((uint8_t*)debug_msg, strlen(debug_msg));
            return -2;  // 超时错误码
        } else {
            snprintf(debug_msg, sizeof(debug_msg), "Pump%d UART RX failed: %d\r\n", pump_id, status);
            CDC_Transmit_FS((uint8_t*)debug_msg, strlen(debug_msg));
            return -1;
        }
    }
    
    return 0;  // 成功发送，无需响应
}

// 初始化泵
int pump_init(int pump_id) {
    return send_command(pump_id, CMD_INIT, NULL, 0);
}

// 绝对位置移动
int pump_move_absolute(int pump_id, int position) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_ABS_MOVE, position);
    return send_command(pump_id, cmd, NULL, 0);
}

// 相对抽取
int pump_pick_relative(int pump_id, int steps) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_REL_PICK, steps);
    return send_command(pump_id, cmd, NULL, 0);
}

// 相对分配
int pump_dispense_relative(int pump_id, int steps) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_REL_DISP, steps);
    return send_command(pump_id, cmd, NULL, 0);
}

// 设置速度
int pump_set_speed(int pump_id, int speed) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_SET_SPEED, speed);
    return send_command(pump_id, cmd, NULL, 0);
}

// 停止
int pump_stop(int pump_id) {
    return send_command(pump_id, CMD_STOP, NULL, 0);
}

// 获取状态
int pump_get_status(int pump_id, char* status) {
    return send_command(pump_id, CMD_STATUS, status, 256);
}

// 查询错误码
int pump_query_error(int pump_id, char* error_code) {
    return send_command(pump_id, CMD_ERROR_QUERY, error_code, 256);
}

// 查询当前活塞位置
int pump_query_position(int pump_id, int* position) {
    char response[256];
    int result = send_command(pump_id, CMD_POSITION_QUERY, response, sizeof(response));
    
    if (result == 0 && position != NULL) {
        // 解析响应，从类似 "FF /0`3000 03 0D 0A" 格式中提取位置
        // 寻找 '`' 字符后的数字
        char* pos_start = strchr(response, '`');
        if (pos_start != NULL) {
            pos_start++; // 跳过 '`' 字符
            *position = atoi(pos_start);
        } else {
            *position = -1;  // 解析失败
            return -1;
        }
    }
    
    return result;
}