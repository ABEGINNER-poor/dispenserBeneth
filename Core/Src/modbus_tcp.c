#include "modbus_tcp.h"
#include <string.h>
// Holding Registers数组（根据PDF初始化默认值）
uint16_t holding_regs[1000] = {0}; // 示例：41003净重高低字、41005状态等

// 初始化TCP Server
void modbus_tcp_init(void) {
    struct tcp_pcb *tpcb = tcp_new();
    tcp_bind(tpcb, IP_ADDR_ANY, 502); // Modbus TCP端口502
    tpcb = tcp_listen(tpcb);
    tcp_accept(tpcb, modbus_tcp_accept);
}

// Accept回调
err_t modbus_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, modbus_tcp_recv);
    return ERR_OK;
}

// Recv回调：处理Modbus请求
err_t modbus_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (p == NULL) { // 连接关闭
        tcp_close(tpcb);
        return ERR_OK;
    }

    uint8_t buf[1024];
    uint16_t len = pbuf_copy_partial(p, buf, p->tot_len, 0);
    pbuf_free(p);

    if (len < 12) return ERR_VAL; // 最小Modbus TCP帧长

    uint16_t trans_id = (buf[0] << 8) | buf[1]; // 事务ID
    uint8_t fc = buf[7]; // 函数码

    uint8_t reply[1024];
    reply[0] = buf[0]; reply[1] = buf[1]; // 事务ID
    reply[2] = buf[2]; reply[3] = buf[3]; // 协议ID (0)
    uint16_t reply_len = 6; // 头长

    if (fc == FC_READ_HOLDING) {
        uint16_t addr = (buf[8] << 8) | buf[9]; // 起始地址
        uint16_t qty = (buf[10] << 8) | buf[11]; // 数量
        if (addr + qty > 1000) { // 错误：越界
            reply[7] = fc + 0x80; reply[8] = 0x02; reply_len = 3;
        } else {
            reply[7] = fc; reply[8] = qty * 2; // 字节数
            for (uint16_t i = 0; i < qty; i++) {
                reply[9 + i*2] = holding_regs[addr + i] >> 8;
                reply[10 + i*2] = holding_regs[addr + i] & 0xFF;
            }
            reply_len += 3 + qty * 2;
        }
    } else if (fc == FC_WRITE_SINGLE) {
        uint16_t addr = (buf[8] << 8) | buf[9];
        uint16_t val = (buf[10] << 8) | buf[11];
        if (addr >= 1000) { // 错误
            reply[7] = fc + 0x80; reply[8] = 0x02; reply_len = 3;
        } else {
            holding_regs[addr] = val;
            memcpy(&reply[6], &buf[6], 6); // 回显
            reply_len = 6;
        }
    } else if (fc == FC_WRITE_MULTIPLE) {
        uint16_t addr = (buf[8] << 8) | buf[9];
        uint16_t qty = (buf[10] << 8) | buf[11];
        uint8_t byte_cnt = buf[12];
        if (addr + qty > 1000 || byte_cnt != qty*2) { // 错误
            reply[7] = fc + 0x80; reply[8] = 0x02; reply_len = 3;
        } else {
            for (uint16_t i = 0; i < qty; i++) {
                holding_regs[addr + i] = (buf[13 + i*2] << 8) | buf[14 + i*2];
            }
            memcpy(&reply[6], &buf[6], 6); // 回显
            reply_len = 6;
        }
    } else { // 不支持函数码
        reply[7] = fc + 0x80; reply[8] = 0x01; reply_len = 3;
    }

    reply[4] = reply_len >> 8; reply[5] = reply_len & 0xFF; // 长度
    tcp_write(tpcb, reply, reply_len + 6, 1); // 发送回复
    tcp_output(tpcb);

    return ERR_OK;
}
