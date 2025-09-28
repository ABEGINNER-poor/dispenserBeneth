#include "mb.h"         // FreeModbus 核心头文件
#include "port.h"       // STM32 平台相关头文件
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx_hal.h"
// 宏定义（根据 Mettler Toledo 手册和图片调整）
#define LOADCELL_SLAVE_ADDR 1      // 从机地址
#define REG_NET_WEIGHT       1000   // 净重寄存器地址 (40001 + 偏移)
#define REG_WEIGHT_OP        1001   // 重量操作寄存器地址
#define REG_ONECLICK_PARAM   1002   // 一键校正参数寄存器
#define REG_UNIT             1003   // 单位寄存器地址

// 操作码定义
#define OP_ZERO_IMM       1    // 立即清零
#define OP_ZERO_STABLE    2    // 稳态清零
#define OP_TARE_IMM       3    // 立即去皮
#define OP_TARE_STABLE    4    // 稳态去皮
#define OP_CLEAR_TARE     5    // 清皮
#define OP_ONECLICK_CAL   6    // 一键校正

//句柄声明
extern TIM_HandleTypeDef htim7;
extern UART_HandleTypeDef hlpuart1; // 替换为实际使用的 UART
// 寄存器缓冲区
USHORT usRegHoldingBuf[64]; // 保持寄存器缓冲区，足够容纳所有寄存器

// 初始化 Modbus RTU 上下文
int loadcell_init(void) {
    eMBErrorCode status = eMBInit(MB_RTU, LOADCELL_SLAVE_ADDR, 0, 115200, MB_PAR_NONE, 1); // 添加 ucStopBits = 1
    if (status != MB_ENOERR) return -1; // 检查初始化是否成功
    status = eMBEnable();
    if (status != MB_ENOERR) return -1; // 检查启用是否成功
    return 0;
}

// 读净重和状态
int loadcell_read_net_weight(float *weight, uint16_t *status) {
    if (weight == NULL || status == NULL) return -1;
    // 假设净重占两个寄存器 (REG_NET_WEIGHT 和 REG_NET_WEIGHT+1)，状态占一个
    *weight = *(float*)&usRegHoldingBuf[REG_NET_WEIGHT - 1000]; // 偏移到数组索引
    *status = usRegHoldingBuf[REG_NET_WEIGHT - 1000 + 2];       // 状态寄存器
    return 0;
}

// 清零
int loadcell_zero(int immediate) {
    uint16_t op = immediate ? OP_ZERO_IMM : OP_ZERO_STABLE;
    usRegHoldingBuf[REG_WEIGHT_OP - 1000] = op; // 写入操作码到缓冲区
    // 轮询检查操作完成
    while (usRegHoldingBuf[REG_WEIGHT_OP - 1000] != 0) {
        eMBPoll(); // 处理 Modbus 请求
    }
    return 0;
}

// 去皮
int loadcell_tare(int immediate) {
    uint16_t op = immediate ? OP_TARE_IMM : OP_TARE_STABLE;
    usRegHoldingBuf[REG_WEIGHT_OP - 1000] = op;
    while (usRegHoldingBuf[REG_WEIGHT_OP - 1000] != 0) {
        eMBPoll();
    }
    return 0;
}

// 清皮
int loadcell_clear_tare(void) {
    usRegHoldingBuf[REG_WEIGHT_OP - 1000] = OP_CLEAR_TARE;
    while (usRegHoldingBuf[REG_WEIGHT_OP - 1000] != 0) {
        eMBPoll();
    }
    return 0;
}

// 一键校正
int loadcell_calibrate(float cal_weight, float read_weight) {
    USHORT params[4];
    memcpy(params, &cal_weight, sizeof(float));    // 校正重量
    memcpy(params + 2, &read_weight, sizeof(float)); // 读取重量
    memcpy(&usRegHoldingBuf[REG_ONECLICK_PARAM - 1000], params, sizeof(params));
    usRegHoldingBuf[REG_WEIGHT_OP - 1000] = OP_ONECLICK_CAL;
    while (usRegHoldingBuf[REG_WEIGHT_OP - 1000] != 0) {
        eMBPoll();
    }
    return 0;
}

// 设置单位
int loadcell_set_unit(uint16_t unit) {
    usRegHoldingBuf[REG_UNIT - 1000] = unit;
    return 0;
}

// 关闭 Modbus 上下文
int loadcell_close(void) {
    eMBDisable(); // 禁用 Modbus
    return 0;
}

// Modbus 寄存器回调函数 (需在主程序中注册)
eMBErrorCode eMBRegHoldingCB(UCHAR *pucRegBuffer, USHORT usAddress, USHORT usNRegs, eMBRegisterMode eMode) {
    if (eMode == MB_REG_WRITE) {
        // 写入寄存器
        memcpy(&usRegHoldingBuf[usAddress - 1000], pucRegBuffer, usNRegs * 2);
    } else if (eMode == MB_REG_READ) {
        // 读取寄存器
        memcpy(pucRegBuffer, &usRegHoldingBuf[usAddress - 1000], usNRegs * 2);
    }
    return MB_ENOERR;
}


