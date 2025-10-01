# LwIP网络调试功能使用指南

## 概述
在lwip.c中新增了多个网络调试和测试函数，帮助诊断网络连接问题。

## 新增功能

### 1. 调试输出函数
```c
static void lwip_cdc_debug_printf(const char *msg);
static void lwip_cdc_debug_log(const char *prefix, const char *msg);
```
- 通过USB CDC输出调试信息
- 包含时间戳的格式化日志输出

### 2. 网络状态检查
```c
void lwip_get_interface_status(void);
```
- 显示网络接口状态（UP/DOWN）
- 显示链路状态（UP/DOWN）
- 显示IP地址、子网掩码、网关

### 3. 网络统计信息
```c
void lwip_print_stats(void);
```
- 显示RX/TX数据包统计
- 显示错误和丢包统计
- 帮助识别网络传输问题

### 4. Ping测试
```c
void lwip_ping(const char* target_ip);
```
- 向指定IP地址发送ping测试
- 目前为通知功能，实际ping需要ICMP实现

### 5. 网络测试套件
```c
void lwip_network_test(void);
```
- 综合网络连接测试
- 检查接口状态、统计信息
- 测试常见IP地址连通性

### 6. 定期状态检查
```c
void lwip_periodic_status_check(void);
```
- 每10秒自动检查网络状态
- 适合在主循环中调用

## 调试输出示例

### 初始化阶段
```
[1234] LWIP_INIT: IP: 192.168.10.88
[1235] LWIP_INIT: Netmask: 255.255.255.0
[1236] LWIP_INIT: Gateway: 0.0.0.0
[1237] LWIP_INIT: Initializing TCP/IP stack
[1245] LWIP_INIT: TCP/IP stack initialized
[1250] LWIP_INIT: Adding network interface
[1260] LWIP_INIT: Setting default network interface
[1261] LWIP_INIT: Bringing network interface up
[1262] LWIP_INIT: Setting link callback
[1265] LWIP_INIT: Creating Ethernet link handler thread
[1270] LWIP_INIT: Ethernet link handler thread created
[1271] LWIP_INIT: LwIP initialization completed successfully
```

### 链路状态变化
```
[2000] LINK_STATUS: Network interface is UP
[2001] LINK_STATUS: Interface UP - IP: 192.168.10.88
```

### 网络测试输出
```
[5000] NET_TEST: === Network Connectivity Test ===
[7001] NET_STATUS: Interface: UP, Link: UP, IP: 192.168.10.88, Netmask: 255.255.255.0, Gateway: 0.0.0.0
[7002] NET_STATS: === Network Statistics ===
[7003] NET_STATS: RX Packets: 0, TX Packets: 0
[7004] NET_STATS: RX Errors: 0, TX Errors: 0
[7005] NET_STATS: RX Drops: 0, TX Drops: 0
[7010] PING: Attempting to ping 192.168.1.1
[7011] PING: Attempting to ping 8.8.8.8
[7012] PING: Attempting to ping 192.168.10.1
[7013] NET_TEST: Network test completed
```

## 使用方法

### 在main函数中使用
```c
int main(void)
{
  // ... HAL初始化 ...
  
  MX_LWIP_Init();
  
  // ... 其他初始化 ...
  
  // 启动网络测试
  lwip_network_test();
  
  while (1)
  {
    // 定期检查网络状态
    lwip_periodic_status_check();
    
    // ... 其他业务逻辑 ...
    
    HAL_Delay(100);
  }
}
```

### 在FreeRTOS任务中使用
```c
void NetworkMonitorTask(void *argument)
{
  // 等待网络初始化完成
  vTaskDelay(pdMS_TO_TICKS(3000));
  
  // 执行网络测试
  lwip_network_test();
  
  while(1)
  {
    // 每30秒检查一次网络状态
    lwip_get_interface_status();
    lwip_print_stats();
    
    vTaskDelay(pdMS_TO_TICKS(30000));
  }
}
```

## 故障排除

### 常见问题及解决方案

1. **接口DOWN**
   - 检查PHY芯片初始化
   - 检查硬件连接
   - 查看ethernetif.c中的PHY调试信息

2. **链路DOWN**
   - 检查网线连接
   - 检查PHY芯片配置
   - 查看链路监控线程的调试信息

3. **IP配置错误**
   - 检查lwip.c中的IP地址设置
   - 确认网络配置与局域网匹配

4. **无数据传输**
   - 检查MAC配置
   - 查看TX/RX统计信息
   - 检查以太网中断是否正常

## 调试建议

1. **按顺序检查**：
   - 首先确认LWIP初始化成功
   - 然后检查PHY芯片状态
   - 最后检查链路和数据传输

2. **使用CDC输出**：
   - 连接USB CDC查看实时调试信息
   - 所有调试信息都有时间戳，便于分析

3. **结合ethernetif.c调试**：
   - 同时查看ethernetif.c中的PHY和ETH调试信息
   - 形成完整的诊断链条

## 配置要求

- 确保USB CDC已正确配置
- 确保FreeRTOS堆栈足够大
- 确保所有相关头文件已包含