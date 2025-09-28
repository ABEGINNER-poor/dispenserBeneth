#include "sy03b_oem.h"
#include <string.h>
#include <stdio.h>  // 假设使用stdio模拟串口，实际替换为串口库

// 假设串口发送/接收函数 (实际需实现)
extern void serial_write(const char* buf, int len);
extern int serial_read(char* buf, int max_len, int timeout);

// 计算校验和 (地址 + 序列号 + 数据块 的XOR)
uint8_t calculate_checksum(const char* buf, int len) {
    uint8_t checksum = 0;
    for (int i = 0; i < len; i++) {
        checksum ^= (uint8_t)buf[i];
    }
    return checksum;
}

// 发送OEM命令并获取响应
int oem_send_command(char addr, uint8_t seq, const char* cmd, OEM_Response* resp) {
    int cmd_len = strlen(cmd);
    int pkt_len = 1 + 1 + cmd_len + 1;  // addr + seq + cmd + ETX
    char pkt[512];
    pkt[0] = addr;
    pkt[1] = seq + 0x30;  // 序列号0-7转为ASCII '0'-'7'
    memcpy(pkt + 2, cmd, cmd_len);
    pkt[2 + cmd_len] = ETX;
    uint8_t checksum = calculate_checksum(pkt, pkt_len);
    char full_pkt[513];
    full_pkt[0] = STX;
    memcpy(full_pkt + 1, pkt, pkt_len);
    full_pkt[1 + pkt_len] = checksum;

    // 发送
    serial_write(full_pkt, 2 + pkt_len);

    // 接收响应
    char rx_buf[512];
    int rx_len = serial_read(rx_buf, sizeof(rx_buf), 1000);
    if (rx_len < 5 || rx_buf[0] != STX || rx_buf[rx_len - 2] != ETX) {
        return -1;  // 无效响应
    }

    // 校验和验证
    uint8_t rx_checksum = rx_buf[rx_len - 1];
    uint8_t calc_checksum = calculate_checksum(rx_buf + 1, rx_len - 2);
    if (rx_checksum != calc_checksum) {
        return -2;  // 校验失败
    }

    // 解析
    resp->status = rx_buf[2];  // 状态/错误
    int data_start = 3;
    resp->data_len = rx_len - 5;  // STX + addr + status + ETX + checksum
    memcpy(resp->data, rx_buf + data_start, resp->data_len);

    return 0;
}

// 判断泵是否忙 (状态位第5位)
int is_pump_busy(char status) {
    return (status & 0x20) == 0;  // 第5位为0表示忙
}

// 获取错误码 (低4位)
enum OEM_Error get_error_code(char status) {
    return (enum OEM_Error)(status & 0x0F);
}
