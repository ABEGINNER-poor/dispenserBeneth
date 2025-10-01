# ethernetif.c 集成 PHY 芯片库说明

## 概述
本次修改将 YT8512C PHY 芯片库集成到 STM32F4 项目的 ethernetif.c 文件中，实现自动 PHY 芯片检测和链路状态管理。

## 主要修改内容

### 1. 创建的新文件
- `Drivers/BSP/Components/yt8512c/sys.h` - 为 ethernet_chip.h 提供基础类型定义

### 2. ethernetif.c 主要修改

#### 2.1 添加头文件包含
```c
#include "ethernet_chip.h"
```

#### 2.2 添加 PHY 对象和变量
```c
static eth_chip_object_t PHYchip;
static eth_chip_ioc_tx_t  PHYchip_io_ctx;
```

#### 2.3 实现 PHY IO 回调函数
- `PHY_IO_Init()` - PHY 初始化
- `PHY_IO_DeInit()` - PHY 反初始化
- `PHY_IO_ReadReg()` - 读取 PHY 寄存器
- `PHY_IO_WriteReg()` - 写入 PHY 寄存器
- `PHY_IO_GetTick()` - 获取系统时钟

#### 2.4 修改 low_level_init() 函数
- 注册 PHY IO 函数到芯片对象
- 初始化 PHY 芯片
- 禁用功耗下降模式
- 根据 PHY 链路状态配置 ETH 速度和双工模式
- 启动 ETH 中断传输和接收

#### 2.5 重写 ethernet_link_thread() 函数
- 定期检查 PHY 链路状态
- 根据链路状态自动配置 ETH 参数
- 动态启动/停止 ETH 接口
- 通知网络接口链路状态变化

## PHY 芯片支持
该库支持多种 PHY 芯片的自动检测：
- YT8512C
- RTL8201BL
- SR8201F
- LAN8720A

## 功能特性
1. **自动 PHY 芯片检测** - 通过读取寄存器自动识别 PHY 芯片类型
2. **智能链路监控** - 实时监控网络连接状态
3. **动态速度配置** - 自动配置 10M/100M 和半双工/全双工模式
4. **错误处理** - 完善的错误检测和处理机制
5. **低功耗支持** - 支持 PHY 功耗管理

## 使用说明
1. 确保项目包含 `ethernet_chip.c` 和 `ethernet_chip.h` 文件
2. 在项目设置中添加 BSP 路径到包含目录
3. 编译项目时会自动检测和配置 PHY 芯片
4. 网络连接状态会在 `ethernet_link_thread` 中自动管理

## 注意事项
1. 该实现基于 STM32 HAL 库
2. 需要确保 ETH 外设已正确配置
3. PHY 芯片的 SMI 接口需要正确连接
4. 建议在 FreeRTOS 环境下使用

## 兼容性
- STM32F4xx 系列微控制器
- LwIP 网络协议栈
- FreeRTOS 实时操作系统
- STM32CubeIDE 开发环境