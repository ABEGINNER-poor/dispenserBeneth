#include "modbus_tcp.h"
#include <string.h>
#include "usbd_cdc_if.h"  // 包含CDC传输函数

// 简单的调试日志函数，输出到CDC
static void cdc_debug_log(const char* tag, const char* message)
{
    char debug_buf[128];
    uint32_t len = 0;
    
    // 手动拼接字符串，避免使用sprintf
    const char* ptr;
    
    // 复制tag
    ptr = tag;
    while (*ptr && len < 20) {
        debug_buf[len++] = *ptr++;
    }
    
    // 添加分隔符
    debug_buf[len++] = ':';
    debug_buf[len++] = ' ';
    
    // 复制message
    ptr = message;
    while (*ptr && len < 120) {
        debug_buf[len++] = *ptr++;
    }
    
    // 添加换行
    debug_buf[len++] = '\r';
    debug_buf[len++] = '\n';
    
    // 通过CDC发送
    if (len < sizeof(debug_buf)) {
        CDC_Transmit_FS((uint8_t*)debug_buf, len);
    }
}

// 简单的整数转字符串函数
static void int_to_str(int value, char* str, int max_len)
{
    int i = 0;
    int is_negative = 0;
    
    if (value < 0) {
        is_negative = 1;
        value = -value;
    }
    
    // 处理特殊情况0
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    
    // 转换数字
    while (value > 0 && i < max_len - 2) {
        str[i++] = '0' + (value % 10);
        value /= 10;
    }
    
    // 添加负号
    if (is_negative && i < max_len - 1) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    
    // 反转字符串
    int start = 0;
    int end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// 十六进制转换函数
static void hex_to_str(uint8_t value, char* str)
{
    const char hex_chars[] = "0123456789ABCDEF";
    str[0] = hex_chars[value >> 4];
    str[1] = hex_chars[value & 0x0F];
    str[2] = '\0';
}

// 数据转储函数 - 显示接收到的原始数据
static void dump_data(const uint8_t* data, uint16_t len)
{
    char dump_buf[128];
    char hex_str[3];
    uint16_t dump_len = 0;
    
    // 添加前缀
    const char* prefix = "Data: ";
    strcpy(dump_buf, prefix);
    dump_len = strlen(prefix);
    
    // 最多显示前16字节的数据
    uint16_t max_bytes = (len > 16) ? 16 : len;
    
    for (uint16_t i = 0; i < max_bytes && dump_len < 120; i++) {
        hex_to_str(data[i], hex_str);
        if (dump_len + 3 < sizeof(dump_buf)) {
            dump_buf[dump_len++] = hex_str[0];
            dump_buf[dump_len++] = hex_str[1];
            dump_buf[dump_len++] = ' ';
        }
    }
    
    if (len > 16) {
        const char* more = "...";
        strcat(dump_buf, more);
        dump_len += 3;
    }
    
    dump_buf[dump_len] = '\0';
    cdc_debug_log("MODBUS_TCP", dump_buf);
}

// Holding Registers数组（根据PDF初始化默认值）
uint16_t holding_regs[100]; // 100个寄存器足够使用

// 处理单个Modbus TCP请求
static err_t process_modbus_request(struct tcp_pcb *tpcb, uint8_t* buf, uint16_t offset)
{
    // 解析Modbus TCP帧头
    uint16_t trans_id = (buf[offset] << 8) | buf[offset+1];     // 事务ID
    uint16_t protocol_id = (buf[offset+2] << 8) | buf[offset+3];  // 协议ID (应该是0)
    uint16_t length = (buf[offset+4] << 8) | buf[offset+5];       // 长度字段
    
    // 验证协议ID
    if (protocol_id != 0) {
        cdc_debug_log("MODBUS_TCP", "Invalid protocol ID");
        return ERR_VAL;
    }
    
    // 验证最小长度（至少要有单元ID和功能码）
    if (length < 2) {
        cdc_debug_log("MODBUS_TCP", "Invalid length field");
        return ERR_VAL;
    }

    uint8_t unit_id = buf[offset+6];                       // 单元ID
    uint8_t fc = buf[offset+7];                            // 函数码

    // 添加调试信息
    char frame_info[80];
    strcpy(frame_info, "Frame: TID=");
    int_to_str(trans_id, frame_info + strlen(frame_info), 10);
    const char* len_prefix = " Len=";
    strcat(frame_info, len_prefix);
    int_to_str(length, frame_info + strlen(frame_info), 10);
    const char* fc_prefix = " FC=";
    strcat(frame_info, fc_prefix);
    int_to_str(fc, frame_info + strlen(frame_info), 10);
    cdc_debug_log("MODBUS_TCP", frame_info);

    uint8_t reply[1024];
    reply[0] = buf[offset]; reply[1] = buf[offset+1]; // 事务ID
    reply[2] = 0; reply[3] = 0;           // 协议ID (0)
    reply[6] = unit_id;                   // 单元ID
    uint16_t reply_data_len = 0;          // 数据部分长度（不包含单元ID）

    if (fc == FC_READ_HOLDING) {
        cdc_debug_log("MODBUS_TCP", "Processing read holding");
        
        // 检查数据长度是否足够
        if (length < 6) { // 单元ID + 功能码 + 起始地址 + 数量 = 1+1+2+2 = 6
            cdc_debug_log("MODBUS_TCP", "Read request too short");
            reply[7] = fc + 0x80;  // 异常函数码
            reply[8] = 0x03;       // 异常代码：非法数据值
            reply_data_len = 2;
        } else {
            uint16_t addr = (buf[offset+8] << 8) | buf[offset+9]; // 起始地址
            uint16_t qty = (buf[offset+10] << 8) | buf[offset+11]; // 数量
            
            char addr_info[50];
            strcpy(addr_info, "Read addr=");
            int_to_str(addr, addr_info + strlen(addr_info), 10);
            const char* qty_prefix = " qty=";
            strcat(addr_info, qty_prefix);
            int_to_str(qty, addr_info + strlen(addr_info), 10);
            cdc_debug_log("MODBUS_TCP", addr_info);
            
            // 验证数量范围
            if (qty == 0 || qty > 125) { // Modbus标准限制单次最多读125个寄存器
                cdc_debug_log("MODBUS_TCP", "Invalid quantity");
                reply[7] = fc + 0x80;  // 异常函数码
                reply[8] = 0x03;       // 异常代码：非法数据值
                reply_data_len = 2;
            } else if (addr + qty > 100) { // 错误：越界，现在只有100个寄存器
                cdc_debug_log("MODBUS_TCP", "Address out of bounds");
                reply[7] = fc + 0x80;  // 异常函数码
                reply[8] = 0x02;       // 异常代码：非法数据地址
                reply_data_len = 2;
            } else {
                reply[7] = fc;              // 函数码
                reply[8] = qty * 2;         // 字节数
                for (uint16_t i = 0; i < qty; i++) {
                    reply[9 + i*2] = holding_regs[addr + i] >> 8;
                    reply[10 + i*2] = holding_regs[addr + i] & 0xFF;
                }
                reply_data_len = 2 + qty * 2; // 单元ID + 函数码 + 字节数 + 数据
                cdc_debug_log("MODBUS_TCP", "Read success");
            }
        }
    } else if (fc == FC_WRITE_SINGLE) {
        cdc_debug_log("MODBUS_TCP", "Processing write single");
        
        // 检查数据长度是否足够：单元ID + 功能码 + 地址 + 值 = 1+1+2+2 = 6
        if (length < 6) {
            cdc_debug_log("MODBUS_TCP", "Write single request too short");
            reply[7] = fc + 0x80;  // 异常函数码
            reply[8] = 0x03;       // 异常代码：非法数据值
            reply_data_len = 2;
        } else {
            uint16_t addr = (buf[offset+8] << 8) | buf[offset+9];   // 寄存器地址
            uint16_t value = (buf[offset+10] << 8) | buf[offset+11]; // 要写入的值
            
            char write_info[60];
            strcpy(write_info, "Write addr=");
            int_to_str(addr, write_info + strlen(write_info), 10);
            const char* val_prefix = " val=0x";
            strcat(write_info, val_prefix);
            // 简单的十六进制转换
            char hex_str[5];
            hex_str[4] = '\0';
            for (int j = 3; j >= 0; j--) {
                int nibble = (value >> (j * 4)) & 0xF;
                hex_str[3-j] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
            }
            strcat(write_info, hex_str);
            cdc_debug_log("MODBUS_TCP", write_info);
            
            // 验证地址范围
            if (addr >= 100) {
                cdc_debug_log("MODBUS_TCP", "Write address out of bounds");
                reply[7] = fc + 0x80;  // 异常函数码
                reply[8] = 0x02;       // 异常代码：非法数据地址
                reply_data_len = 2;
            } else {
                // 写入寄存器
                holding_regs[addr] = value;
                
                // 回显请求（标准Modbus写单个寄存器响应）
                reply[7] = fc;                           // 函数码
                reply[8] = (addr >> 8) & 0xFF;          // 地址高字节
                reply[9] = addr & 0xFF;                 // 地址低字节
                reply[10] = (value >> 8) & 0xFF;        // 值高字节
                reply[11] = value & 0xFF;               // 值低字节
                reply_data_len = 5; // 功能码 + 地址 + 值 = 1+2+2 = 5
                cdc_debug_log("MODBUS_TCP", "Write single success");
            }
        }
    } else if (fc == FC_WRITE_MULTIPLE) {
        cdc_debug_log("MODBUS_TCP", "Processing write multiple");
        
        // 检查数据长度是否足够：单元ID + 功能码 + 起始地址 + 数量 + 字节数 = 1+1+2+2+1 = 7
        if (length < 7) {
            cdc_debug_log("MODBUS_TCP", "Write multiple request too short");
            reply[7] = fc + 0x80;  // 异常函数码
            reply[8] = 0x03;       // 异常代码：非法数据值
            reply_data_len = 2;
        } else {
            uint16_t addr = (buf[offset+8] << 8) | buf[offset+9];   // 起始地址
            uint16_t qty = (buf[offset+10] << 8) | buf[offset+11];  // 寄存器数量
            uint8_t byte_count = buf[offset+12];                    // 字节数
            
            char write_info[60];
            strcpy(write_info, "Write mult addr=");
            int_to_str(addr, write_info + strlen(write_info), 10);
            const char* qty_prefix = " qty=";
            strcat(write_info, qty_prefix);
            int_to_str(qty, write_info + strlen(write_info), 10);
            cdc_debug_log("MODBUS_TCP", write_info);
            
            // 验证参数
            if (qty == 0 || qty > 123 || byte_count != qty * 2) { // Modbus标准限制
                cdc_debug_log("MODBUS_TCP", "Invalid write multiple parameters");
                reply[7] = fc + 0x80;  // 异常函数码
                reply[8] = 0x03;       // 异常代码：非法数据值
                reply_data_len = 2;
            } else if (addr + qty > 100) { // 地址越界检查
                cdc_debug_log("MODBUS_TCP", "Write multiple address out of bounds");
                reply[7] = fc + 0x80;  // 异常函数码
                reply[8] = 0x02;       // 异常代码：非法数据地址
                reply_data_len = 2;
            } else if (length < 7 + byte_count) { // 检查是否有足够的数据
                cdc_debug_log("MODBUS_TCP", "Write multiple data incomplete");
                reply[7] = fc + 0x80;  // 异常函数码
                reply[8] = 0x03;       // 异常代码：非法数据值
                reply_data_len = 2;
            } else {
                // 写入多个寄存器
                for (uint16_t i = 0; i < qty; i++) {
                    uint16_t value = (buf[offset+13+i*2] << 8) | buf[offset+14+i*2];
                    holding_regs[addr + i] = value;
                }
                
                // 响应：功能码 + 起始地址 + 寄存器数量
                reply[7] = fc;                          // 函数码
                reply[8] = (addr >> 8) & 0xFF;         // 起始地址高字节
                reply[9] = addr & 0xFF;                // 起始地址低字节
                reply[10] = (qty >> 8) & 0xFF;         // 数量高字节
                reply[11] = qty & 0xFF;                // 数量低字节
                reply_data_len = 5; // 功能码 + 地址 + 数量 = 1+2+2 = 5
                cdc_debug_log("MODBUS_TCP", "Write multiple success");
            }
        }
    } else {
        // 其他功能码暂时返回不支持
        cdc_debug_log("MODBUS_TCP", "Unsupported function code");
        reply[7] = fc + 0x80;  // 异常函数码
        reply[8] = 0x01;       // 异常代码：非法函数码
        reply_data_len = 2;
    }

    // 设置长度字段 (包含单元ID + 数据部分)
    uint16_t total_length = reply_data_len + 1; // +1 for unit_id
    reply[4] = (total_length >> 8) & 0xFF; 
    reply[5] = total_length & 0xFF;
    
    cdc_debug_log("MODBUS_TCP", "Sending reply");
    
    // 发送完整的TCP帧 (6字节头 + 数据部分)
    uint16_t total_frame_len = 6 + total_length;
    
    // 检查TCP发送缓冲区是否有足够空间
    u16_t available_space = tcp_sndbuf(tpcb);
    if (available_space < total_frame_len) {
        char space_msg[50];
        strcpy(space_msg, "TCP buffer low: available=");
        int_to_str(available_space, space_msg + strlen(space_msg), 10);
        cdc_debug_log("MODBUS_TCP", space_msg);
        // 仍然尝试发送，但记录警告
    }
    
    err_t write_err = tcp_write(tpcb, reply, total_frame_len, TCP_WRITE_FLAG_COPY);
    if (write_err != ERR_OK) {
        char write_err_msg[40];
        strcpy(write_err_msg, "TCP write failed err=");
        int_to_str(write_err, write_err_msg + strlen(write_err_msg), 10);
        cdc_debug_log("MODBUS_TCP", write_err_msg);
        
        // 如果发送缓冲区满了，强制输出
        if (write_err == ERR_MEM) {
            tcp_output(tpcb);
        }
        return ERR_OK; // 即使写入失败也返回OK，避免连接中断
    }
    
    err_t output_err = tcp_output(tpcb);
    if (output_err != ERR_OK) {
        char output_err_msg[40];
        strcpy(output_err_msg, "TCP output failed err=");
        int_to_str(output_err, output_err_msg + strlen(output_err_msg), 10);
        cdc_debug_log("MODBUS_TCP", output_err_msg);
    } else {
        cdc_debug_log("MODBUS_TCP", "Reply sent successfully");
    }
    
    return ERR_OK;
}

// 初始化TCP Server
void modbus_tcp_init(void) {
    // 首先清零所有寄存器
    for (int i = 0; i < 100; i++) {
        holding_regs[i] = 0;
    }
    

    
    cdc_debug_log("MODBUS_TCP", "Clearing and initializing registers");
    
    // 验证寄存器初始值并输出调试信息
    char reg_info[80];
    for (int i = 0; i < 4; i++) {
        strcpy(reg_info, "Reg[");
        int_to_str(i, reg_info + strlen(reg_info), 10);
        strcat(reg_info, "]=0x");
        // 简单的十六进制转换
        uint16_t val = holding_regs[i];
        char hex_str[5];
        hex_str[4] = '\0';
        for (int j = 3; j >= 0; j--) {
            int nibble = (val >> (j * 4)) & 0xF;
            hex_str[3-j] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
        }
        strcat(reg_info, hex_str);
        strcat(reg_info, " (");
        int_to_str(val, reg_info + strlen(reg_info), 10);
        strcat(reg_info, ")");
        cdc_debug_log("MODBUS_TCP", reg_info);
    }
    
    cdc_debug_log("MODBUS_TCP", "Register initialization verified");
    
    struct tcp_pcb *tpcb = tcp_new();
    if (tpcb == NULL) {
        cdc_debug_log("MODBUS_TCP", "Failed to create TCP PCB");
        return;
    }
    
    err_t bind_err = tcp_bind(tpcb, IP_ADDR_ANY, 502); // Modbus TCP端口502
    if (bind_err != ERR_OK) {
        char err_msg[50];
        const char* prefix = "TCP bind failed: ";
        strcpy(err_msg, prefix);
        int_to_str(bind_err, err_msg + strlen(prefix), sizeof(err_msg) - strlen(prefix));
        cdc_debug_log("MODBUS_TCP", err_msg);
        tcp_close(tpcb);
        return;
    }
    
    tpcb = tcp_listen(tpcb);
    if (tpcb == NULL) {
        cdc_debug_log("MODBUS_TCP", "Failed to set TCP to listen mode");
        return;
    }
    
    tcp_accept(tpcb, modbus_tcp_accept);
    cdc_debug_log("MODBUS_TCP", "TCP server started on port 502");
}

// 发送确认回调
err_t modbus_tcp_sent(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    // 简单记录发送确认
    char sent_msg[40];
    strcpy(sent_msg, "TCP sent ");
    int_to_str(len, sent_msg + strlen(sent_msg), 10);
    strcat(sent_msg, " bytes");
    cdc_debug_log("MODBUS_TCP", sent_msg);
    return ERR_OK;
}

// 连接错误回调
void modbus_tcp_error(void *arg, err_t err) {
    char err_msg[40];
    strcpy(err_msg, "TCP error: ");
    int_to_str(err, err_msg + strlen(err_msg), 10);
    cdc_debug_log("MODBUS_TCP", err_msg);
}

// Accept回调
err_t modbus_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err) {
    if (err != ERR_OK || newpcb == NULL) {
        cdc_debug_log("MODBUS_TCP", "Accept failed");
        return err;
    }
    
    cdc_debug_log("MODBUS_TCP", "Client connected");
    
    // 设置连接参数
    tcp_setprio(newpcb, TCP_PRIO_MIN);
    tcp_recv(newpcb, modbus_tcp_recv);
    tcp_sent(newpcb, modbus_tcp_sent);  // 添加发送确认回调
    tcp_err(newpcb, modbus_tcp_error);
    
    // 设置保活参数，防止连接超时
    tcp_keepalive(newpcb);
    
    return ERR_OK;
}

// Recv回调：处理Modbus请求
err_t modbus_tcp_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    if (err != ERR_OK) {
        cdc_debug_log("MODBUS_TCP", "Receive error");
        if (p != NULL) {
            pbuf_free(p);
        }
        return err;
    }
    
    if (p == NULL) { // 连接关闭
        cdc_debug_log("MODBUS_TCP", "Client disconnected");
        tcp_close(tpcb);
        return ERR_OK;
    }

    // 获取数据长度并通知TCP栈已接收
    uint16_t data_len = p->tot_len;
    tcp_recved(tpcb, data_len); // 重要：通知LwIP已处理数据
    
    // 添加更详细的接收数据信息
    char recv_info[60];
    const char* prefix = "Received data len=";
    strcpy(recv_info, prefix);
    int_to_str(data_len, recv_info + strlen(prefix), sizeof(recv_info) - strlen(prefix));
    cdc_debug_log("MODBUS_TCP", recv_info);

    uint8_t buf[1024];
    uint16_t len = pbuf_copy_partial(p, buf, data_len, 0);
    pbuf_free(p);

    // 转储接收到的原始数据用于调试
    dump_data(buf, len);

    // 检查最小帧长度
    if (len < 8) { // Modbus TCP最小长度: 6字节头 + 2字节最小数据
        cdc_debug_log("MODBUS_TCP", "Frame too short");
        return ERR_VAL;
    }

    // 处理可能包含多个Modbus请求的数据包
    uint16_t offset = 0;
    uint16_t processed_frames = 0;
    
    while (offset < len && processed_frames < 10) { // 最多处理10个帧，防止无限循环
        // 检查是否还有足够的数据读取帧头
        if (offset + 6 > len) {
            cdc_debug_log("MODBUS_TCP", "No more complete frames");
            break;
        }
        
        // 获取长度字段
        uint16_t frame_length = (buf[offset+4] << 8) | buf[offset+5];
        uint16_t total_frame_size = 6 + frame_length; // 6字节头 + 数据部分
        
        // 检查是否有完整的帧
        if (offset + total_frame_size > len) {
            char incomplete_msg[60];
            strcpy(incomplete_msg, "Incomplete frame at offset=");
            int_to_str(offset, incomplete_msg + strlen(incomplete_msg), 10);
            cdc_debug_log("MODBUS_TCP", incomplete_msg);
            break;
        }
        
        // 处理这个帧
        char frame_msg[50];
        strcpy(frame_msg, "Processing frame ");
        int_to_str(processed_frames + 1, frame_msg + strlen(frame_msg), 10);
        cdc_debug_log("MODBUS_TCP", frame_msg);
        
        process_modbus_request(tpcb, buf, offset);
        
        // 移动到下一个帧
        offset += total_frame_size;
        processed_frames++;
        
        // 如果只有一个标准的12字节请求，直接退出
        if (len == 12 && processed_frames == 1) {
            break;
        }
    }
    
    char summary_msg[60];
    strcpy(summary_msg, "Processed ");
    int_to_str(processed_frames, summary_msg + strlen(summary_msg), 10);
    const char* frames_suffix = " frames from ";
    strcat(summary_msg, frames_suffix);
    int_to_str(len, summary_msg + strlen(summary_msg), 10);
    const char* bytes_suffix = " bytes";
    strcat(summary_msg, bytes_suffix);
    cdc_debug_log("MODBUS_TCP", summary_msg);

    return ERR_OK;
}