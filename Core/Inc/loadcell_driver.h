#ifndef LOADCELL_DRIVER_H
#define LOADCELL_DRIVER_H

#include <mb.h>  // 假设使用libmodbus库
#include <stdint.h>

// 默认从机地址
#define LOADCELL_SLAVE_ADDR 15

// 寄存器地址宏（基于PDF寄存器列表，地址形式4XXXX，实际通讯地址=4XXXX-1）
#define REG_TARE_VALUE          41001  // 读/写皮重值 (float32, 2 regs)
#define REG_NET_WEIGHT          41003  // 读净重值 (float32, 2 regs)
#define REG_STATUS_WORD         41005  // 读状态字 (uint16_t, 1 reg)
#define REG_WEIGHT_OP           41006  // 读/写称重操作 (uint16_t, 1 reg)
#define REG_TARE_STATUS         40023  // 读去皮状态 (uint16_t, 1 reg)
#define REG_ZERO_STATUS         40025  // 读清零状态 (uint16_t, 1 reg)
#define REG_POWERON_ZERO_MODE   40209  // 读/写开机清零模式 (uint16_t, 1 reg)
#define REG_WEIGH_MODE          40210  // 读/写称重模式 (float32, 2 regs)
#define REG_ENV_COND            40212  // 读/写环境条件 (float32, 2 regs)
#define REG_WEIGH_DIR           49204  // 读/写称重方向 (uint16_t, 1 reg)
#define REG_CANCEL_CMD          40401  // 写取消命令 (uint16_t, 1 reg)
#define REG_CAL_STATUS          40402  // 读校正状态 (uint16_t, 1 reg)
#define REG_EXT_CAL_WEIGHT      40403  // 读/写外校砝码重量 (float32, 2 regs)
#define REG_TIMEOUT             40405  // 读/写超时时间 (uint16_t, 1 reg)
#define REG_START_2PT_CAL       40438  // 启动两点外校 (uint16_t, 1 reg)
#define REG_REQ_CAL_WEIGHT      40444  // 读需要加载校正重量 (float32, 2 regs)
#define REG_ONECLICK_PARAM      40451  // 读/写一键校正参数 (float32*2, 4 regs)
#define REG_START_3PT_CAL       40456  // 启动三点外校 (uint16_t, 1 reg)
#define REG_CALFREE             49203  // 启动免标定 (uint16_t, 1 reg)
#define REG_DISPLAY_STEP        40204  // 读显示分度值 (float32, 2 regs)
#define REG_MAX_CAPACITY        40206  // 读最大容量 (float32, 2 regs)
#define REG_AUTO_ZERO_TRACK     40208  // 读/写自动零跟踪 (uint16_t, 1 reg)
#define REG_UNIT                40226  // 读/写称重单位 (uint16_t, 1 reg)
#define REG_ZERO_STAB_TIME      40228  // 读/写清零判稳时间 (float32, 2 regs)
#define REG_ZERO_STAB_DEV       40230  // 读/写清零判稳偏差 (float32, 2 regs)
#define REG_TARE_STAB_TIME      40232  // 读/写去皮判稳时间 (float32, 2 regs)
#define REG_TARE_STAB_DEV       40234  // 读/写去皮判稳偏差 (float32, 2 regs)
#define REG_WEIGH_STAB_TIME     40236  // 读/写称重判稳时间 (float32, 2 regs)
#define REG_WEIGH_STAB_DEV      40238  // 读/写称重判稳偏差 (float32, 2 regs)
#define REG_STEP_COEFF          40240  // 读/写显示分度值系数 (int16_t, 1 reg)
#define REG_WEIGHT_TYPE         40241  // 读/写净重显示类型 (uint16_t, 1 reg)
#define REG_BAUD_RATE           40217  // 读/写串口波特率 (uint16_t, 1 reg)
#define REG_SERIAL_PARAM        40218  // 读/写串口参数 (uint16_t, 1 reg)
#define REG_MODBUS_ADDR         40219  // 读/写Modbus从机地址 (uint16_t, 1 reg)
#define REG_BYTE_ORDER          40991  // 读/写Modbus字节序 (uint16_t, 1 reg)
#define REG_SN                  40030  // 读设备序列号 (string, 10 regs)
#define REG_SW_VER              40070  // 读软件版本 (string, 10 regs)
#define REG_DEVICE_DATA         40126  // 读设备数据 (string, 32 regs)
#define REG_GEO_CODE            40200  // 读/写GEO代码 (float32, 1 reg)  // 注意: PDF中为1 reg，但类型float32应2 regs，可能PDF笔误
#define REG_CURRENT_TIME        40220  // 读/写当前时间 (struct, 6 regs)
#define REG_RESET_USER          40997  // 重置用户设置 (uint16_t, 1 reg)
#define REG_RESTART             40999  // 重启设备 (uint16_t, 1 reg)

// 诊断寄存器（可选）
#define REG_SMART5_L5_CODE      48001  // 读SMART5 Level5报警 (uint32_t, 2 regs)
#define REG_SMART5_L4_CODE      48003  // 读SMART5 Level4报警 (uint32_t, 2 regs)
#define REG_SMART5_L3_CODE      48005  // 读SMART5 Level3报警 (uint32_t, 2 regs)
#define REG_SMART5_L2_CODE      48007  // 读SMART5 Level2报警 (uint32_t, 2 regs)
#define REG_OVERLOAD_100_LOG    48442  // 读载荷超100%日志 (struct, 41 regs)
#define REG_TEMP_OVER_LOG       48483  // 读温度超范围次数 (float32, 2 regs)
#define REG_HUMI_OVER_LOG       48491  // 读湿度超范围日志 (struct, 41 regs)
#define REG_LOAD_TIMES_LOG      48534  // 读加载次数 (float32, 2 regs)
#define REG_TEMP_GRAD_LOG       48583  // 读温度梯度超范围 (float32, 2 regs)
#define REG_VOLT_OVER_LOG       48636  // 读激励电压超范围 (float32, 2 regs)
#define REG_OVERLOAD_150_LOG    48642  // 读载荷超150%日志 (struct, 41 regs)
#define REG_UNDERLOAD_LOG       48683  // 读欠载次数 (float32, 2 regs)
#define REG_CAL_FAIL_LOG        48685  // 读校正失败次数 (float32, 2 regs)
#define REG_ZERO_FAIL_LOG       48687  // 读清零失败次数 (float32, 2 regs)

// 称重操作码（用于REG_WEIGHT_OP）
#define OP_TARE_STABLE     0x01  // 稳态去皮
#define OP_CLEAR_TARE      0x02  // 清皮
#define OP_ZERO_STABLE     0x04  // 稳态清零
#define OP_TARE_IMM        0x10  // 立即去皮
#define OP_ZERO_IMM        0x20  // 立即清零
#define OP_ONECLICK_CAL    0x40  // 一键校正

// 单位码（用于REG_UNIT）
#define UNIT_G  0
#define UNIT_KG 1
#define UNIT_MG 3
#define UNIT_LB 7

// 函数声明
int loadcell_init(modbus_t *ctx, const char *device, int baudrate);  // 初始化Modbus上下文
int loadcell_read_net_weight(modbus_t *ctx, float *weight, uint16_t *status);  // 读净重和状态
int loadcell_zero(modbus_t *ctx, int immediate);  // 清零（立即或稳态）
int loadcell_tare(modbus_t *ctx, int immediate);  // 去皮（立即或稳态）
int loadcell_clear_tare(modbus_t *ctx);  // 清皮
int loadcell_calibrate(modbus_t *ctx, float cal_weight, float read_weight);  // 一键校正
int loadcell_set_unit(modbus_t *ctx, uint16_t unit);  // 设置单位
int loadcell_close(modbus_t *ctx);  // 关闭上下文

#endif
