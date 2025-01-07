#include "gui.h"
#include "crypto.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static gui_config_t gui_config;
static int running = 1;

// 清屏函数
static void clear_screen() {
    printf("\033[2J\033[H");  // ANSI转义序列清屏并将光标移到开头
}

// 显示主菜单
static void show_main_menu() {
    clear_screen();
    printf("=== CryMail - 安全邮件客户端 ===\n\n");
    printf("1. 邮件配置\n");
    printf("2. 发送邮件\n");
    printf("3. 接收邮件\n");
    printf("0. 退出程序\n\n");
    printf("请选择操作 [0-3]: ");
}

// 显示配置页面
static void show_config_page() {
    clear_screen();
    printf("=== 邮件配置 ===\n\n");
    
    printf("SMTP服务器 [%s]: ", gui_config.smtp_server);
    char input[256];
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0) {
        strncpy(gui_config.smtp_server, input, sizeof(gui_config.smtp_server)-1);
    }
    
    printf("用户名 [%s]: ", gui_config.username);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0) {
        strncpy(gui_config.username, input, sizeof(gui_config.username)-1);
    }
    
    printf("密码 [%s]: ", gui_config.password);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0) {
        strncpy(gui_config.password, input, sizeof(gui_config.password)-1);
    }
    
    printf("端口 [%s]: ", gui_config.port);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0) {
        strncpy(gui_config.port, input, sizeof(gui_config.port)-1);
    }
    
    printf("使用SSL (1=是, 0=否) [%d]: ", gui_config.use_ssl);
    fgets(input, sizeof(input), stdin);
    input[strcspn(input, "\n")] = 0;
    if (strlen(input) > 0) {
        gui_config.use_ssl = atoi(input);
    }
    
    // 保存配置
    mail_config_t config;
    config.smtp_server = gui_config.smtp_server;
    config.username = gui_config.username;
    config.password = gui_config.password;
    config.port = atoi(gui_config.port);
    config.use_ssl = gui_config.use_ssl;
    
    if (save_mail_config(&config, "mail.conf")) {
        printf("\n配置已保存！按回车返回主菜单...");
    } else {
        printf("\n配置保存失败！按回车返回主菜单...");
    }
    getchar();
}

// 显示发送页面
static void show_send_page() {
    clear_screen();
    printf("=== 发送邮件 ===\n\n");
    
    printf("收件人: ");
    fgets(gui_config.to, sizeof(gui_config.to), stdin);
    gui_config.to[strcspn(gui_config.to, "\n")] = 0;
    
    printf("主题: ");
    fgets(gui_config.subject, sizeof(gui_config.subject), stdin);
    gui_config.subject[strcspn(gui_config.subject, "\n")] = 0;
    
    printf("内容 (输入单独的一行.结束):\n");
    char line[1024];
    gui_config.message[0] = '\0';
    while (fgets(line, sizeof(line), stdin)) {
        if (strcmp(line, ".\n") == 0) break;
        strncat(gui_config.message, line, sizeof(gui_config.message)-strlen(gui_config.message)-1);
    }
    
    // 发送邮件
    mail_config_t config;
    if (!load_mail_config(&config, "mail.conf")) {
        printf("\n无法加载邮件配置！按回车返回主菜单...");
        getchar();
        return;
    }
    
    // 签名消息
    unsigned int sig_len;
    unsigned char* signature = sign_message(gui_config.message, "private.pem", &sig_len);
    if (!signature) {
        printf("\n消息签名失败！按回车返回主菜单...");
        getchar();
        return;
    }
    
    // 保存签名到临时文件
    FILE* fp = fopen("temp_signature.bin", "wb");
    if (fp) {
        fwrite(signature, 1, sig_len, fp);
        fclose(fp);
    }
    free(signature);
    
    // 准备邮件内容
    mail_content_t content = {
        .from = config.username,
        .to = gui_config.to,
        .subject = gui_config.subject,
        .body = gui_config.message,
        .signature_file = "temp_signature.bin"
    };
    
    // 发送邮件
    if (send_signed_mail(&config, &content)) {
        printf("\n邮件发送成功！按回车返回主菜单...");
    } else {
        printf("\n邮件发送失败！按回车返回主菜单...");
    }
    
    remove("temp_signature.bin");
    getchar();
}

// 显示接收页面
static void show_receive_page() {
    clear_screen();
    printf("=== 接收邮件 ===\n\n");
    printf("正在实现中...\n\n");
    printf("按回车返回主菜单...");
    getchar();
}

int gui_init(int width, int height) {
    // 加载配置
    mail_config_t config;
    if (load_mail_config(&config, "mail.conf")) {
        strncpy(gui_config.smtp_server, config.smtp_server, sizeof(gui_config.smtp_server)-1);
        strncpy(gui_config.username, config.username, sizeof(gui_config.username)-1);
        strncpy(gui_config.password, config.password, sizeof(gui_config.password)-1);
        snprintf(gui_config.port, sizeof(gui_config.port), "%d", config.port);
        gui_config.use_ssl = config.use_ssl;
    }
    
    return 1;
}

void gui_run() {
    while (running) {
        show_main_menu();
        
        char choice[10];
        fgets(choice, sizeof(choice), stdin);
        int option = atoi(choice);
        
        switch (option) {
            case 0:
                running = 0;
                break;
            case 1:
                show_config_page();
                break;
            case 2:
                show_send_page();
                break;
            case 3:
                show_receive_page();
                break;
            default:
                printf("\n无效选项！按回车继续...");
                getchar();
                break;
        }
    }
}

void gui_cleanup() {
    // 命令行界面不需要特殊清理
} 