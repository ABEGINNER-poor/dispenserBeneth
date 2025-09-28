#include "bus_servo.h"
#include <string.h>

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
    HAL_UART_Transmit(&huart6, buf, idx, HAL_MAX_DELAY);  // 发送
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
    HAL_UART_Receive(&huart6, rx_buf, 6, HAL_MAX_DELAY);  // 接收响应
    if (rx_buf[0] == 0x55 && rx_buf[1] == 0x55 && rx_buf[3] == CMD_GET_BATTERY_VOLTAGE) {
        return (rx_buf[5] << 8) | rx_buf[4];  // 高低8位
    }
    return 0;  // 错误返回0
}

// 多舵机卸力
void BusServo_MultUnload(uint8_t *ids, uint8_t id_cnt) {
    uint8_t params[256];
    params[0] = id_cnt;
    memcpy(&params[1], ids, id_cnt);
    BusServo_SendCmd(CMD_MULT_SERVO_UNLOAD, params, id_cnt + 1);
}

// 读取多舵机位置（positions数组需预分配空间）
void BusServo_MultPosRead(uint8_t *ids, uint8_t id_cnt, uint16_t *positions) {
    uint8_t params[256];
    params[0] = id_cnt;
    memcpy(&params[1], ids, id_cnt);
    BusServo_SendCmd(CMD_MULT_SERVO_POS_READ, params, id_cnt + 1);

    // 接收响应：Length = id_cnt*3 + 3
    uint8_t rx_len = id_cnt * 3 + 5;  // 总长度：头2 + len1 + cmd1 + data
    uint8_t rx_buf[256];
    HAL_UART_Receive(&huart6, rx_buf, rx_len, HAL_MAX_DELAY);
    if (rx_buf[0] == 0x55 && rx_buf[1] == 0x55 && rx_buf[3] == CMD_MULT_SERVO_POS_READ) {
        for (uint8_t i = 0; i < id_cnt; i++) {
            uint8_t offset = 4 + i * 3;  // 数据从rx_buf[4]开始
            positions[i] = (rx_buf[offset + 2] << 8) | rx_buf[offset + 1];
        }
    }
}
