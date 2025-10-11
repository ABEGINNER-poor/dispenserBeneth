#include "bus_servo.h"
#include "app_business.h"
#include <string.h>
#include <stdio.h>

// 发送通用指令
void BusServo_SendCmd(uint8_t cmd, uint8_t *params, uint8_t param_cnt) {
    uint8_t buf[256];  // 缓冲区，足够大
    uint8_t idx = 0;
    buf[idx++] = 0x55;  // 帧头
    buf[idx++] = 0x55;
    buf[idx++] = param_cnt + 2;  // Length = N + 2
    buf[idx++] = cmd;
    if (params && param_cnt > 0) {
        memcpy(&buf[idx], params, param_cnt);
        idx += param_cnt;
    }
    
    // 添加调试信息 - 显示发送的数据
    char debug_msg[200];
    int offset = snprintf(debug_msg, sizeof(debug_msg), "TX Servo: ");
    for (int i = 0; i < idx && i < 20; i++) {  // 最多显示20字节
        offset += snprintf(debug_msg + offset, sizeof(debug_msg) - offset, "%02X ", buf[i]);
    }
    cdc_debug_print(debug_msg);
    
    HAL_UART_Transmit(&huart6, buf, idx, HAL_MAX_DELAY);  // 发送
    
    // 发送完成后短暂延时，让舵机处理命令
    HAL_Delay(10);
}

// 控制单舵机转动（扩展多舵机可类似）
void BusServo_Move(uint8_t id, uint16_t position, uint16_t time) {
    uint8_t params[6];
    params[0] = 1;  // 舵机数
    params[1] = time & 0xFF;  // 时间低8位
    params[2] = (time >> 8) & 0xFF;  // 高8位
    params[3] = id;
    params[4] = position & 0xFF;  // 位置低8位
    params[5] = (position >> 8) & 0xFF;  // 高8位
    BusServo_SendCmd(CMD_SERVO_MOVE, params, 6);
}

// 控制多舵机同时转动
void BusServo_MultMove(uint8_t *ids, uint16_t *positions, uint8_t servo_count, uint16_t time) {
    uint8_t params[256];  // 足够大的缓冲区
    uint8_t idx = 0;
    
    params[idx++] = servo_count;    // 舵机数量
    params[idx++] = time & 0xFF;    // 时间低8位  
    params[idx++] = (time >> 8) & 0xFF;  // 时间高8位
    
    // 添加每个舵机的ID和位置
    for (uint8_t i = 0; i < servo_count; i++) {
        params[idx++] = ids[i];     // 舵机ID
        params[idx++] = positions[i] & 0xFF;        // 位置低8位
        params[idx++] = (positions[i] >> 8) & 0xFF; // 位置高8位
    }
    
    BusServo_SendCmd(CMD_SERVO_MOVE, params, idx);
}

// 运行动作组
void BusServo_ActionGroupRun(uint8_t group, uint16_t times) {
    uint8_t params[3];
    params[0] = group;
    params[1] = times & 0xFF;
    params[2] = (times >> 8) & 0xFF;
    BusServo_SendCmd(CMD_ACTION_GROUP_RUN, params, 3);
}

// 停止动作组
void BusServo_ActionGroupStop(void) {
    BusServo_SendCmd(CMD_ACTION_GROUP_STOP, NULL, 0);
}

// 调整动作组速度
void BusServo_ActionGroupSpeed(uint8_t group, uint16_t speed_percent) {
    uint8_t params[3];
    params[0] = group;
    params[1] = speed_percent & 0xFF;
    params[2] = (speed_percent >> 8) & 0xFF;
    BusServo_SendCmd(CMD_ACTION_GROUP_SPEED, params, 3);
}

// 获取电池电压（返回mV，需要接收响应）
uint16_t BusServo_GetBatteryVoltage(void) {
    BusServo_SendCmd(CMD_GET_BATTERY_VOLTAGE, NULL, 0);
    uint8_t rx_buf[6];
    
    // 使用较短超时时间 (200ms) 而不是HAL_MAX_DELAY
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart6, rx_buf, 6, 200);
    
    if (status == HAL_OK) {
        // 检查响应帧头和命令码
        if (rx_buf[0] == 0x55 && rx_buf[1] == 0x55 && rx_buf[3] == CMD_GET_BATTERY_VOLTAGE) {
            return (rx_buf[5] << 8) | rx_buf[4];  // 高低8位
        }
    }
    
    return 0;  // 超时或错误返回0
}

// 多舵机卸力
void BusServo_MultUnload(uint8_t *ids, uint8_t id_cnt) {
    uint8_t params[256];
    params[0] = id_cnt;
    memcpy(&params[1], ids, id_cnt);
    BusServo_SendCmd(CMD_MULT_SERVO_UNLOAD, params, id_cnt + 1);
}

// 舵机状态机相关变量
static uint8_t servo_rx_buf[256];
static uint8_t servo_expected_len = 0;
static uint32_t servo_start_time = 0;
static uint8_t servo_reading_flag = 0;
static uint8_t *servo_ids_ptr = NULL;
static uint8_t servo_id_count = 0;
static uint16_t *servo_positions_ptr = NULL;

// 批量读取舵机相关变量
static uint8_t batch_reading_flag = 0;
static uint32_t batch_start_time = 0;
static uint16_t batch_positions[6];  // 存储6个舵机的位置
static uint8_t batch_received_count = 0;  // 已接收到的舵机数量

// 读取多舵机位置（非阻塞版本）
void BusServo_MultPosRead(uint8_t *ids, uint8_t id_cnt, uint16_t *positions) {
    uint8_t params[256];
    params[0] = id_cnt;
    memcpy(&params[1], ids, id_cnt);
    
    // 保存参数供后续处理使用
    servo_ids_ptr = ids;
    servo_id_count = id_cnt;
    servo_positions_ptr = positions;
    
    // 初始化位置数组为默认值9999（表示读取失败）
    for (uint8_t i = 0; i < id_cnt; i++) {
        positions[i] = 9999;  // 默认位置9999代表读取失败
    }
    
    // 发送读取命令
    BusServo_SendCmd(CMD_MULT_SERVO_POS_READ, params, id_cnt + 1);
    
    // 简单策略：固定接收舵机数据
    servo_expected_len = id_cnt * 3 + 5;  // 按请求的舵机数量计算最大长度
    
    char len_msg[80];
    snprintf(len_msg, sizeof(len_msg), "Expecting %d bytes for %d servos", servo_expected_len, id_cnt);
    cdc_debug_print(len_msg);
    
    // 清零接收缓冲区
    memset(servo_rx_buf, 0, sizeof(servo_rx_buf));
    
    // 确保UART DMA处于空闲状态
    if (huart6.RxState != HAL_UART_STATE_READY) {
        HAL_UART_DMAStop(&huart6);
        HAL_Delay(5);
    }
    
    // 启动DMA接收固定长度，允许较长的超时时间
    if (HAL_UART_Receive_DMA(&huart6, servo_rx_buf, servo_expected_len) == HAL_OK) {
        servo_start_time = HAL_GetTick();
        servo_reading_flag = 1;
        cdc_debug_print("Servo: Started DMA receive with long timeout");
    } else {
        cdc_debug_print("Servo: Failed to start DMA receive");
        servo_reading_flag = 0;
    }
}

// 检查舵机读取状态（需要在主循环中定期调用）
uint8_t BusServo_CheckReadStatus(void) {
    if (!servo_reading_flag) {
        return 0;  // 没有正在读取
    }
    
    uint32_t elapsed = HAL_GetTick() - servo_start_time;
    uint32_t received = servo_expected_len - huart6.RxXferCount;
    
    // 检查DMA是否完成（接收到期望的全部数据）
    if (huart6.RxXferCount == 0) {
        HAL_UART_DMAStop(&huart6);
        char complete_msg[80];
        snprintf(complete_msg, sizeof(complete_msg), "Receive complete in %ldms", elapsed);
        cdc_debug_print(complete_msg);
        
        // 打印接收数据
        char hex_msg[200];
        int offset = snprintf(hex_msg, sizeof(hex_msg), "RX data: ");
        for (int i = 0; i < servo_expected_len && i < 30; i++) {
            offset += snprintf(hex_msg + offset, sizeof(hex_msg) - offset, "%02X ", servo_rx_buf[i]);
        }
        cdc_debug_print(hex_msg);
        
        BusServo_ParseResponse();
        servo_reading_flag = 0;
        return 1;  // 成功完成
    }
    
    // 如果接收到部分数据且已等待5ms，处理已接收的数据
    if (received > 0 && elapsed >= 5) {
        HAL_UART_DMAStop(&huart6);
        
        char partial_msg[80];
        snprintf(partial_msg, sizeof(partial_msg), "Partial receive: %ld/%d bytes in %ldms", 
                received, servo_expected_len, elapsed);
        cdc_debug_print(partial_msg);
        
        // 打印已接收的数据
        char hex_msg[200];
        int offset = snprintf(hex_msg, sizeof(hex_msg), "RX partial: ");
        for (int i = 0; i < received && i < 30; i++) {
            offset += snprintf(hex_msg + offset, sizeof(hex_msg) - offset, "%02X ", servo_rx_buf[i]);
        }
        cdc_debug_print(hex_msg);
        
        // 调整期望长度为实际接收长度，然后解析
        servo_expected_len = received;
        BusServo_ParseResponse();
        servo_reading_flag = 0;
        return 1;  // 部分完成
    }
    
    // 总体超时检查 - 500ms超时
    if (elapsed > 500) {
        HAL_UART_DMAStop(&huart6);
        
        char timeout_msg[80];
        snprintf(timeout_msg, sizeof(timeout_msg), "Timeout after %ldms, received %ld/%d bytes", 
                elapsed, received, servo_expected_len);
        cdc_debug_print(timeout_msg);
        
        servo_reading_flag = 0;
        return 2;  // 超时
    }
    
    return 3;  // 正在接收中
}

// 解析舵机响应数据
void BusServo_ParseResponse(void) {
    // 首先打印接收到的所有数据用于调试
    char all_data_msg[200];
    int offset = snprintf(all_data_msg, sizeof(all_data_msg), "Parse RX: ");
    for (int i = 0; i < 16 && i < sizeof(servo_rx_buf); i++) {  // 显示前16字节
        offset += snprintf(all_data_msg + offset, sizeof(all_data_msg) - offset, "%02X ", servo_rx_buf[i]);
    }
    cdc_debug_print(all_data_msg);
    
    // 检查帧头和命令码
    if (servo_rx_buf[0] == 0x55 && servo_rx_buf[1] == 0x55 && 
        servo_rx_buf[3] == CMD_MULT_SERVO_POS_READ) {
        
        uint8_t data_length = servo_rx_buf[2];  // 数据长度字段
        uint8_t returned_count = servo_rx_buf[4];  // 返回的舵机个数
        
        char debug_msg[100];
        snprintf(debug_msg, sizeof(debug_msg), "Valid frame: data_len=%d, returned_count=%d, expected=%d", 
                 data_length, returned_count, servo_id_count);
        cdc_debug_print(debug_msg);
        
        // 解析实际返回的舵机数据
        for (uint8_t i = 0; i < returned_count; i++) {
            uint8_t offset = 5 + i * 3;  // 数据从servo_rx_buf[5]开始，每个舵机3字节
            
            // 检查数据边界
            if (offset + 2 <= data_length + 2) {  // 确保不超出数据长度
                uint8_t servo_id = servo_rx_buf[offset];      // 舵机ID
                uint8_t pos_low = servo_rx_buf[offset + 1];   // 位置低位
                uint8_t pos_high = servo_rx_buf[offset + 2];  // 位置高位
                uint16_t position = (pos_high << 8) | pos_low;
                
                char parse_msg[100];
                snprintf(parse_msg, sizeof(parse_msg), "Parse: ID=%d, low=0x%02X, high=0x%02X, pos=%d", 
                         servo_id, pos_low, pos_high, position);
                cdc_debug_print(parse_msg);
                
                // 找到对应的舵机ID在请求数组中的位置并更新
                for (uint8_t j = 0; j < servo_id_count; j++) {
                    if (servo_ids_ptr[j] == servo_id) {
                        servo_positions_ptr[j] = position;
                        char update_msg[80];
                        snprintf(update_msg, sizeof(update_msg), "Updated: positions[%d] = %d for servo ID %d", 
                                 j, position, servo_id);
                        cdc_debug_print(update_msg);
                        break;
                    }
                }
            } else {
                char boundary_msg[80];
                snprintf(boundary_msg, sizeof(boundary_msg), "Data boundary exceeded at servo %d, offset=%d", i, offset);
                cdc_debug_print(boundary_msg);
            }
        }
    } else {
        char debug_msg[100];
        snprintf(debug_msg, sizeof(debug_msg), "Invalid frame: h1=0x%02X h2=0x%02X len=0x%02X cmd=0x%02X", 
                 servo_rx_buf[0], servo_rx_buf[1], servo_rx_buf[2], servo_rx_buf[3]);
        cdc_debug_print(debug_msg);
    }
}

// 批量发送6个舵机读取命令
void BusServo_BatchSendReadCommands(void) {
    uint8_t servo_ids[6] = {1, 2, 3, 4, 5, 6};
    
    // 初始化批量读取状态
    batch_reading_flag = 1;
    batch_start_time = HAL_GetTick();
    batch_received_count = 0;
    
    // 初始化位置数组为9999
    for (int i = 0; i < 6; i++) {
        batch_positions[i] = 9999;
    }
    
    cdc_debug_print("Batch: Sending 6 servo read commands...");
    
    // 连续发送6个读取命令
    for (uint8_t i = 0; i < 6; i++) {
        uint8_t params[2];
        params[0] = 1;  // 舵机数量为1
        params[1] = servo_ids[i]; // 舵机ID
        
        char send_msg[60];
        snprintf(send_msg, sizeof(send_msg), "Batch: Sending read command for servo %d", servo_ids[i]);
        cdc_debug_print(send_msg);
        
        BusServo_SendCmd(CMD_MULT_SERVO_POS_READ, params, 2);
        
        // 发送间隔，避免命令冲突
        HAL_Delay(50);
    }
    
    cdc_debug_print("Batch: All commands sent, starting receive polling");
    
    // 启动连续接收模式
    servo_expected_len = 8;  // 单个舵机响应长度
    memset(servo_rx_buf, 0, sizeof(servo_rx_buf));
    
    // 确保UART DMA处于空闲状态
    if (huart6.RxState != HAL_UART_STATE_READY) {
        HAL_UART_DMAStop(&huart6);
        HAL_Delay(5);
    }
    
    // 启动DMA接收
    if (HAL_UART_Receive_DMA(&huart6, servo_rx_buf, servo_expected_len) == HAL_OK) {
        cdc_debug_print("Batch: Started DMA receive for responses");
    } else {
        cdc_debug_print("Batch: Failed to start DMA receive");
        batch_reading_flag = 0;
    }
}

// 检查批量读取状态，处理接收到的响应
uint8_t BusServo_CheckBatchReadStatus(void) {
    if (!batch_reading_flag) {
        return 0;  // 没有正在进行批量读取
    }
    
    uint32_t elapsed = HAL_GetTick() - batch_start_time;
    
    // 检查DMA是否接收到数据
    uint32_t received = servo_expected_len - huart6.RxXferCount;
    
    // 如果接收到至少5字节（最小响应长度）
    if (received >= 5) {
        // 停止当前DMA
        HAL_UART_DMAStop(&huart6);
        
        // 解析接收到的数据
        BusServo_ParseBatchResponse();
        
        // 如果还没有接收完所有舵机且未超时，继续接收下一个响应
        if (batch_received_count < 6 && elapsed < 3000) {
            // 清零缓冲区，准备接收下一个响应
            memset(servo_rx_buf, 0, sizeof(servo_rx_buf));
            
            // 短暂延时后重新启动DMA接收
            HAL_Delay(10);
            if (HAL_UART_Receive_DMA(&huart6, servo_rx_buf, servo_expected_len) == HAL_OK) {
                char continue_msg[60];
                snprintf(continue_msg, sizeof(continue_msg), "Batch: Continue receiving, count=%d/6", batch_received_count);
                cdc_debug_print(continue_msg);
            } else {
                cdc_debug_print("Batch: Failed to restart DMA receive");
                batch_reading_flag = 0;
                return 2;  // 接收失败
            }
        } else {
            // 所有响应接收完成或超时
            char complete_msg[80];
            snprintf(complete_msg, sizeof(complete_msg), "Batch: Complete, received %d/6 responses in %ldms", 
                    batch_received_count, elapsed);
            cdc_debug_print(complete_msg);
            
            batch_reading_flag = 0;
            return 1;  // 完成
        }
    }
    
    // 总体超时检查 - 3秒
    if (elapsed > 3000) {
        HAL_UART_DMAStop(&huart6);
        
        char timeout_msg[80];
        snprintf(timeout_msg, sizeof(timeout_msg), "Batch: Timeout after %ldms, received %d/6", 
                elapsed, batch_received_count);
        cdc_debug_print(timeout_msg);
        
        batch_reading_flag = 0;
        return 2;  // 超时
    }
    
    return 3;  // 正在接收中
}

// 解析批量接收的响应数据
void BusServo_ParseBatchResponse(void) {
    // 打印接收到的数据用于调试
    char hex_msg[100];
    int offset = snprintf(hex_msg, sizeof(hex_msg), "Batch RX: ");
    for (int i = 0; i < 8; i++) {
        offset += snprintf(hex_msg + offset, sizeof(hex_msg) - offset, "%02X ", servo_rx_buf[i]);
    }
    cdc_debug_print(hex_msg);
    
    // 检查是否是错误响应 55 55 03 15 00
    if (servo_rx_buf[0] == 0x55 && servo_rx_buf[1] == 0x55 && 
        servo_rx_buf[2] == 0x03 && servo_rx_buf[3] == 0x15 && servo_rx_buf[4] == 0x00) {
        
        char error_msg[60];
        snprintf(error_msg, sizeof(error_msg), "Batch: Received error response (servo disconnected/error)");
        cdc_debug_print(error_msg);
        
        batch_received_count++;
        return;  // 错误响应，对应舵机位置保持9999
    }
    
    // 检查是否是正常响应：55 55 06 15 01 xx xx yy
    if (servo_rx_buf[0] == 0x55 && servo_rx_buf[1] == 0x55 && 
        servo_rx_buf[2] == 0x06 && servo_rx_buf[3] == 0x15 && servo_rx_buf[4] == 0x01) {
        
        uint8_t servo_id = servo_rx_buf[5];         // 舵机ID
        uint8_t pos_low = servo_rx_buf[6];          // 位置低位
        uint8_t pos_high = servo_rx_buf[7];         // 位置高位
        uint16_t position = (pos_high << 8) | pos_low;
        
        // 验证舵机ID范围
        if (servo_id >= 1 && servo_id <= 6) {
            batch_positions[servo_id - 1] = position;  // 更新对应位置
            
            char success_msg[80];
            snprintf(success_msg, sizeof(success_msg), "Batch: Servo %d position = %d", servo_id, position);
            cdc_debug_print(success_msg);
        } else {
            char id_error_msg[60];
            snprintf(id_error_msg, sizeof(id_error_msg), "Batch: Invalid servo ID %d", servo_id);
            cdc_debug_print(id_error_msg);
        }
        
        batch_received_count++;
    } else {
        char format_error_msg[80];
        snprintf(format_error_msg, sizeof(format_error_msg), "Batch: Unknown response format");
        cdc_debug_print(format_error_msg);
        
        batch_received_count++;  // 仍然计数，避免死循环
    }
}

// 获取批量读取的结果
void BusServo_GetBatchResults(uint16_t *positions) {
    for (int i = 0; i < 6; i++) {
        positions[i] = batch_positions[i];
    }
}
