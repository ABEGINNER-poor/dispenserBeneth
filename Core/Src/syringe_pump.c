#include "syringe_pump.h"

// 发送命令并读取响应
int send_command(int fd, const char* cmd, char* response, size_t resp_size) {
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "/%c%s\r", PUMP_ADDRESS, cmd);  // 格式: /地址命令CR

    // 发送
    if (write(fd, buffer, strlen(buffer)) < 0) {
        perror("send_command: write failed");
        return -1;
    }

    // 读取响应（如果需要）
    if (response) {
        memset(response, 0, resp_size);
        int n = read(fd, response, resp_size - 1);
        if (n > 0) {
            response[n] = '\0';
            return n;
        } else {
            perror("send_command: read failed");
            return -1;
        }
    }
    return 0;
}

// 初始化泵
int pump_init(int fd) {
    return send_command(fd, CMD_INIT, NULL, 0);
}

// 绝对位置移动
int pump_move_absolute(int fd, int position) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_ABS_MOVE, position);
    return send_command(fd, cmd, NULL, 0);
}

// 相对抽取
int pump_pick_relative(int fd, int steps) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_REL_PICK, steps);
    return send_command(fd, cmd, NULL, 0);
}

// 相对分配
int pump_dispense_relative(int fd, int steps) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_REL_DISP, steps);
    return send_command(fd, cmd, NULL, 0);
}

// 设置速度
int pump_set_speed(int fd, int speed) {
    char cmd[32];
    snprintf(cmd, sizeof(cmd), CMD_SET_SPEED, speed);
    return send_command(fd, cmd, NULL, 0);
}

// 停止
int pump_stop(int fd) {
    return send_command(fd, CMD_STOP, NULL, 0);
}

// 获取状态
int pump_get_status(int fd, char* status) {
    return send_command(fd, CMD_STATUS, status, 256);
}