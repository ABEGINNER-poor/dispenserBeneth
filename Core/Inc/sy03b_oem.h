#ifndef SY03B_OEM_H
#define SY03B_OEM_H

#include <stdint.h>

// 常量定义
#define STX 0x02  // 开始字符 ^B
#define ETX 0x03  // 结束字符 ^C
#define HOST_ADDR '0'  // 主机地址 0x30
#define PUMP_ADDR_MIN 0x31  // 泵地址范围 0x31-0x3F (单个泵)

// 错误代码
enum OEM_Error {
    ERR_NO_ERROR = 0,
    ERR_INIT_FAIL = 1,
    ERR_INVALID_CMD = 2,
    ERR_INVALID_OP = 3,
    ERR_EEPROM_FAIL = 6,
    ERR_NOT_INIT = 7,
    ERR_INTERNAL = 8,
    ERR_PLUNGER_OVERLOAD = 9,
    ERR_VALVE_OVERLOAD = 10,
    ERR_PLUNGER_NOT_ALLOW = 11,
    ERR_INTERNAL_FAULT = 12,
    ERR_ADC_FAIL = 14,
    ERR_CMD_OVERFLOW = 15
};

// 响应结构
typedef struct {
    char status;     // 状态位 (忙/闲 + 错误码)
    char data[256];  // 数据块
    uint8_t data_len;// 数据长度
} OEM_Response;

// 函数原型
int oem_send_command(char addr, uint8_t seq, const char* cmd, OEM_Response* resp);
uint8_t calculate_checksum(const char* buf, int len);
int is_pump_busy(char status);
enum OEM_Error get_error_code(char status);

#endif
