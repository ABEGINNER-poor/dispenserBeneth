# ETH调试信息集成说明

## 概述
在ethernetif.c中集成了通过CDC（USB虚拟串口）输出调试信息的功能，帮助监控以太网连接的各个环节。

## 添加的调试功能

### 1. 调试函数
- `cdc_debug_printf()` - 发送调试消息到CDC
- `cdc_debug_log()` - 带时间戳的格式化调试输出

### 2. ETH中断回调调试
在以下回调函数中添加了调试信息：

#### HAL_ETH_RxCpltCallback()
- 记录RX完成中断触发

#### HAL_ETH_TxCpltCallback()
- 记录TX完成中断触发

#### HAL_ETH_ErrorCallback()
- 记录DMA错误信息
- 显示具体错误码
- 特别标记RX缓冲区不可用错误

### 3. ETH初始化调试
在`low_level_init()`函数中添加：
- HAL_ETH_Init 调用状态
- PHY芯片初始化各步骤状态
- PHY地址检测结果
- 链路状态检测和配置
- MAC配置状态
- ETH启动状态

### 4. 数据传输调试
#### TX (发送) - low_level_output()
- 记录发送的数据包长度
- 传输成功确认
- TX忙碌状态警告
- 传输错误信息和错误码

#### RX (接收) - low_level_input()
- 记录接收的数据包长度
- RX分配错误警告

### 5. 链路监控调试
在`ethernet_link_thread()`中添加：
- 线程启动确认
- 定期链路检查计数（每5秒输出一次状态）
- 详细的链路状态信息（10M/100M，半双工/全双工）
- 链路UP/DOWN状态变化
- MAC配置更新状态
- ETH启动/停止操作结果
- PHY读取错误检测

### 6. PHY IO操作调试
在PHY寄存器读写函数中添加：
- 读操作失败时显示设备地址和寄存器地址
- 写操作失败时显示设备地址、寄存器地址和写入值

## 调试输出格式
所有调试信息按以下格式输出：
```
[时间戳] 模块名称: 消息内容\r\n
```

### 模块标识
- `ETH_INIT` - 以太网初始化
- `PHY_INIT` - PHY芯片初始化
- `ETH_RX` - 接收相关
- `ETH_TX` - 发送相关
- `ETH_ERROR` - 错误信息
- `LINK_THREAD` - 链路监控线程
- `PHY_IO` - PHY寄存器操作

## 使用说明
1. 通过USB CDC连接到PC
2. 使用串口调试工具（如PuTTY、串口助手等）
3. 配置波特率：根据USB CDC配置（通常为115200）
4. 观察实时调试输出

## 调试信息示例
```
[1000] ETH_INIT: HAL_ETH_Init called
[1050] PHY_INIT: Starting PHY chip initialization
[1100] PHY_INIT: PHY chip initialized, address: 0
[1150] PHY_INIT: Configured: 100M Full Duplex
[1200] PHY_INIT: ETH started with interrupt successfully
[1250] LINK_THREAD: Ethernet link monitoring started
[1300] ETH_RX: RX Complete callback triggered
[1350] ETH_RX: Received packet, length: 64
[1400] ETH_TX: TX: Sending packet, length: 128
[1450] ETH_TX: Packet transmitted successfully
```

## 注意事项
1. 大量调试输出可能影响性能，特别是在高网络流量时
2. 链路检查信息每50次检查（约5秒）输出一次，避免信息泛滥
3. CDC传输可能有延迟，不要依赖调试输出的精确时序
4. 在生产环境中可以通过宏定义禁用调试输出

## 故障排除
通过调试信息可以诊断：
- PHY芯片检测和初始化问题
- 链路连接状态
- 数据包传输状态
- DMA错误和缓冲区问题
- 以太网配置错误