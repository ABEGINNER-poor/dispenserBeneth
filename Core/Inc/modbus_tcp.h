#ifndef MODBUS_TCP_H
#define MODBUS_TCP_H

#include "lwip.h"
#include "lwip/tcp.h"
#include "lwip/err.h"
#include "lwip/pbuf.h"
// Modbus 函数码
#define FC_READ_HOLDING 0x03
#define FC_WRITE_SINGLE 0x06
#define FC_WRITE_MULTIPLE 0x10

// 寄存器数组（模拟PDF中Holding Registers，地址4xxxx -> 数组索引0起）
extern uint16_t holding_regs[100]; // 100个寄存器足够使用

void modbus_tcp_init(void);
err_t modbus_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err);
err_t modbus_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

#endif
