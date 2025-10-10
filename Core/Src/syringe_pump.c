#include "syringe_pump.h"
#include "usart.h"  // STM32 USART头文件
#include "main.h"   // HAL头文件
#include "usbd_cdc_if.h"  // 用于调试输出

// 静态变量
static char pump_debug_buf[128];  // 调试信息缓冲区

// 内部函数声明
static void pump_debug_print(const char* message);

/**
  * @brief  泵驱动调试输出函数
  * @param  message: 调试信息字符串
  * @retval None
  */
static void pump_debug_print(const char* message) {
    int len = snprintf(pump_debug_buf, sizeof(pump_debug_buf), "[PUMP] %s\r\n", message);
    if (len > 0 && len < sizeof(pump_debug_buf)) {
        CDC_Transmit_FS((uint8_t*)pump_debug_buf, len);  // 暂时启用调试输出
    }
}

// 发送命令并读取响应 - STM32 USART3版本 - 简化的不定长接收
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
        snprintf(debug_msg, sizeof(debug_msg), "Invalid pump_id: %d", pump_id);
        pump_debug_print(debug_msg);
        return -1;
    }
    
    // 格式化命令: /地址命令CR+LF (根据手册需要CRLF结尾)
    snprintf(buffer, sizeof(buffer), "/%c%s\r\n", pump_address, cmd);
    
    // 调试信息
    snprintf(debug_msg, sizeof(debug_msg), "Pump%d TX: %s", pump_id, buffer);
    pump_debug_print(debug_msg);
    
    // 通过USART3发送命令
    status = HAL_UART_Transmit(&huart3, (uint8_t*)buffer, strlen(buffer), 1000);
    if (status != HAL_OK) {
        snprintf(debug_msg, sizeof(debug_msg), "Pump%d UART TX failed: %d", pump_id, status);
        pump_debug_print(debug_msg);
        return -1;
    }
    
    // 读取响应（如果需要）- 使用逐字节接收
    if (response && resp_size > 0) {
        memset(response, 0, resp_size);
        
        uint8_t rx_char;
        size_t received_count = 0;
        uint32_t start_time = HAL_GetTick();
        uint32_t timeout_ms = 200;  // 总超时200ms
        uint32_t char_timeout_ms = 50;  // 单字符超时50ms
        
        // 逐字节接收，直到遇到结束符或超时
        while (received_count < (resp_size - 1)) {
            status = HAL_UART_Receive(&huart3, &rx_char, 1, char_timeout_ms);
            
            if (status == HAL_OK) {
                response[received_count] = rx_char;
                received_count++;
                
                // 检查是否收到完整的结束符序列 (CRLF)
                // 只有当收到LF并且前一个字符是CR时才结束
                if (received_count >= 2 && 
                    rx_char == 0x0A && 
                    response[received_count-2] == 0x0D) {
                    break;
                }
                
                // 重置总超时计时器（收到数据说明设备在响应）
                start_time = HAL_GetTick();
            } else if (status == HAL_TIMEOUT) {
                // 单字符超时，检查是否已经接收到数据
                if (received_count > 0) {
                    // 已有数据，可能接收完成，等待一下看是否还有数据
                    HAL_Delay(5);  // 减少延时时间
                    // 给已收到数据情况下额外3次重试机会
                    static int retry_count = 0;
                    retry_count++;
                    if (retry_count > 3) {
                        retry_count = 0;
                        break;  // 重试次数用完，退出
                    }
                }
                
                // 检查总超时
                if (HAL_GetTick() - start_time > timeout_ms) {
                    break;  // 总超时，退出
                }
            } else {
                // 其他错误，退出
                break;
            }
        }
        
        // 确保字符串结束
        response[received_count] = '\0';
        
        if (received_count > 0) {
            // 调试信息：显示响应 (修复格式化问题)
            snprintf(debug_msg, sizeof(debug_msg), "Pump%d RX (%u bytes): [%s]", pump_id, (unsigned int)received_count, response);
            pump_debug_print(debug_msg);
            
            // 添加十六进制显示，帮助调试
            char hex_debug[256];
            snprintf(hex_debug, sizeof(hex_debug), "Pump%d RX_HEX: ", pump_id);
            for (size_t i = 0; i < received_count && i < 20; i++) {  // 只显示前20个字节
                char hex_byte[4];
                snprintf(hex_byte, sizeof(hex_byte), "%02X ", (unsigned char)response[i]);
                strcat(hex_debug, hex_byte);
            }
            pump_debug_print(hex_debug);
            
            return 0;  // 成功
        } else {
            snprintf(debug_msg, sizeof(debug_msg), "Pump%d RX: No data received", pump_id);
            pump_debug_print(debug_msg);
            return -1;  // 超时无数据
        }
    }

    return 0;  // 只发送，不接收
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
    char debug_msg[128];
    
    int result = send_command(pump_id, CMD_POSITION_QUERY, response, sizeof(response));
    
    // 添加详细的调试信息
    snprintf(debug_msg, sizeof(debug_msg), "Position query result: %d", result);
    pump_debug_print(debug_msg);
    
    if (result == 0 && position != NULL) {
        // 解析响应，从类似 "FF /0`3000 03 0D 0A" 格式中提取位置
        snprintf(debug_msg, sizeof(debug_msg), "Parsing response: [%s]", response);
        pump_debug_print(debug_msg);
        
        // 寻找 '`' 字符后的数字
        char* pos_start = strchr(response, '`');
        if (pos_start != NULL) {
            pos_start++; // 跳过 '`' 字符
            *position = atoi(pos_start);
            snprintf(debug_msg, sizeof(debug_msg), "Parsed position: %d", *position);
            pump_debug_print(debug_msg);
        } else {
            pump_debug_print("Error: No '`' character found in response");
            *position = -1;  // 解析失败
            return -1;
        }
    } else {
        snprintf(debug_msg, sizeof(debug_msg), "Position query failed: result=%d, position=%p", 
                result, (void*)position);
        pump_debug_print(debug_msg);
    }
    
    return result;
}

/**
  * @brief  解析DT协议状态字节，转换为错误编号
  * @param  status_byte: 从泵响应中提取的状态字节（HEX值）
  * @retval 对应的错误编号
  * @note   根据DT协议状态字节表进行映射，忙碌和空闲状态的错误编号相同
  */
PumpErrorCode_t pump_parse_status_byte(uint8_t status_byte) {
    // 提取低4位作为错误编号
    uint8_t error_bits = status_byte & 0x0F;
    
    switch (error_bits) {
        case 0x00:  // 0000 - 无误
            return PUMP_ERROR_NO_ERROR;         // 0
            
        case 0x01:  // 0001 - 初始化
            return PUMP_ERROR_INITIALIZED;      // 1
            
        case 0x02:  // 0010 - 无效指令
            return PUMP_ERROR_NO_COMMAND;       // 2
            
        case 0x03:  // 0011 - 无效参数
            return PUMP_ERROR_INVALID_PARAM;    // 3
            
        case 0x06:  // 0110 - EEPROM故障
            return PUMP_ERROR_EEPROM_FAULT;     // 6
            
        case 0x07:  // 0111 - 设备未初始化
            return PUMP_ERROR_DEVICE_NOT_INIT;  // 7
            
        case 0x09:  // 1001 - 柱塞过载
            return PUMP_ERROR_OVERLOAD;         // 9
            
        case 0x0A:  // 1010 - 阀过载
            return PUMP_ERROR_VALVE_OVERLOAD;   // 10
            
        case 0x0B:  // 1011 - 不支持柱移动
            return PUMP_ERROR_PLUNGER_MOVE_NOT_ALLOWED; // 11
            
        case 0x0C:  // 1100 - 内部故障
            return PUMP_ERROR_INTERNAL_FAULT;   // 12
            
        case 0x0E:  // 1110 - A/D转换器故障
            return PUMP_ERROR_AD_FAULT;         // 14
            
        case 0x0F:  // 1111 - 指令溢出
            return PUMP_ERROR_CMD_OVERFLOW;     // 15
            
        default:
            // 未知错误码，默认返回内部故障
            return PUMP_ERROR_INTERNAL_FAULT;
    }
}

/**
  * @brief  解析DT协议状态字节，提取忙碌状态
  * @param  status_byte: 从泵响应中提取的状态字节（HEX值）
  * @retval 忙碌状态 (PUMP_STATE_IDLE 或 PUMP_STATE_BUSY)
  * @note   高4位表示忙碌状态：0x4X=忙碌，0x6X=空闲
  */
PumpBusyState_t pump_parse_busy_state(uint8_t status_byte) {
    // 检查高4位
    uint8_t busy_bits = status_byte & 0xF0;
    
    if (busy_bits == 0x40) {
        return PUMP_STATE_BUSY;   // 0x4X = 忙碌
    } else if (busy_bits == 0x60) {
        return PUMP_STATE_IDLE;   // 0x6X = 空闲
    } else {
        // 未知状态，默认为忙碌（安全考虑）
        return PUMP_STATE_BUSY;
    }
}