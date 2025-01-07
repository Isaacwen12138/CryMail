#ifndef GUI_H
#define GUI_H

#include "mail.h"

// GUI配置结构体
typedef struct {
    int current_page;  // 0=主页面, 1=配置页面, 2=发送页面, 3=接收页面
    
    // 输入框
    char smtp_server[256];
    char username[256];
    char password[256];
    char port[32];
    int use_ssl;
    char pop3_server[256];
    char pop3_port[32];
    int pop3_use_ssl;
    
    // 发送邮件
    char to[256];
    char subject[256];
    char message[1024];
    
    // 状态信息
    char status[256];
} gui_config_t;

// 初始化GUI
int gui_init(int width, int height);

// 运行GUI主循环
void gui_run();

// 清理GUI资源
void gui_cleanup();

#endif // GUI_H 