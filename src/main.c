#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crypto.h"
#include "mail.h"
#include "base64.h"
#include "gui.h"

#define MAX_MESSAGE_LENGTH 1024
#define CONFIG_FILE "mail.conf"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

void print_usage() {
    printf("使用方法:\n");
    printf("1. 图形界面模式: ./crymail\n");
    printf("2. 生成密钥对: ./crymail -g\n");
    printf("3. 签名消息: ./crymail -s <消息>\n");
    printf("4. 验证签名: ./crymail -v <消息> <签名文件>\n");
    printf("5. 配置邮件: ./crymail -c\n");
    printf("6. 发送签名邮件: ./crymail -m <收件人> <主题> <消息>\n");
    printf("7. 接收邮件: ./crymail -l\n");
}

// 配置邮件设置
int configure_mail() {
    mail_config_t config;
    char buffer[256];

    printf("请输入SMTP服务器地址: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    config.smtp_server = strdup(buffer);

    printf("请输入用户名: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    config.username = strdup(buffer);

    printf("请输入密码: ");
    fgets(buffer, sizeof(buffer), stdin);
    buffer[strcspn(buffer, "\n")] = 0;
    config.password = strdup(buffer);

    printf("请输入端口号 (例如: 587): ");
    fgets(buffer, sizeof(buffer), stdin);
    config.port = atoi(buffer);

    printf("使用SSL? (1=是, 0=否): ");
    fgets(buffer, sizeof(buffer), stdin);
    config.use_ssl = atoi(buffer);

    printf("请输入pop3服务器地址：");
    fgets(buffer, sizeof(buffer), stdin);
    config.pop3_server = strdup(buffer);

    printf("请输入pop3端口号：");
    fgets(buffer, sizeof(buffer), stdin);
    config.pop3_port = atoi(buffer);

    printf("pop3服务器是否使用SSL?：(1=是,2=否)");
    fgets(buffer, sizeof(buffer), stdin);
    config.pop3_use_ssl = atoi(buffer);

    return save_mail_config(&config, CONFIG_FILE);
}

int main(int argc, char *argv[]) {
    printf("程序启动...\n");

    // 初始化OpenSSL
    OpenSSL_add_all_algorithms();
    ERR_load_crypto_strings();
    printf("OpenSSL初始化完成\n");

    // 如果没有参数，启动GUI模式
    if (argc == 1) {
        printf("启动GUI模式...\n");
        if (!gui_init(WINDOW_WIDTH, WINDOW_HEIGHT)) {
            fprintf(stderr, "GUI初始化失败！\n");
            return 1;
        }
        printf("GUI初始化成功，开始运行主循环\n");
        gui_run();
        printf("GUI主循环结束，开始清理\n");
        gui_cleanup();
        printf("程序正常退出\n");
        return 0;
    }

    if (strcmp(argv[1], "-g") == 0) {
        // 生成密钥对
        if (generate_key_pair("public.pem", "private.pem")) {
            printf("密钥对生成成功！\n");
        } else {
            printf("密钥对生成失败！\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "-s") == 0) {
        if (argc < 3) {
            printf("错误：请提供要签名的消息\n");
            return 1;
        }

        // 签名消息
        unsigned int sig_len;
        unsigned char* signature = sign_message(argv[2], "private.pem", &sig_len);
        
        if (signature) {
            // 将签名保存到文件
            FILE* fp = fopen("signature.bin", "wb");
            if (fp) {
                fwrite(signature, 1, sig_len, fp);
                fclose(fp);
                printf("消息签名成功，签名已保存到 signature.bin\n");
            }
            free(signature);
        } else {
            printf("签名失败！\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "-v") == 0) {
        if (argc < 3) {
            printf("错误：请提供消息和签名文件\n");
            return 1;
        }

        // 读取签名
        FILE* fp = fopen("signature.bin", "rb");
        if (!fp) {
            printf("无法打开签名文件！\n");
            return 1;
        }

        // 获取文件大小
        fseek(fp, 0, SEEK_END);
        unsigned int sig_len = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        unsigned char* signature = malloc(sig_len);
        fread(signature, 1, sig_len, fp);
        fclose(fp);

        // 验证签名
        if (verify_signature(argv[2], signature, sig_len, "public.pem")) {
            printf("签名验证成功！消息是真实的。\n");
        } else {
            printf("签名验证失败！消息可能被篡改。\n");
        }

        free(signature);
    }
    else if (strcmp(argv[1], "-c") == 0) {
        if (configure_mail()) {
            printf("邮件配置保存成功！\n");
        } else {
            printf("邮件配置保存失败！\n");
            return 1;
        }
    }
    else if (strcmp(argv[1], "-m") == 0) {
        if (argc < 5) {
            printf("错误：请提供收件人、主题和消息\n");
            return 1;
        }

        // 初始化邮件系统
        mail_init();

        // 加载邮件配置
        mail_config_t config;
        if (!load_mail_config(&config, CONFIG_FILE)) {
            printf("无法加载邮件配置，请先运行 -c 选项配置邮件\n");
            return 1;
        }

        // 签名消息
        unsigned int sig_len;
        unsigned char* signature = sign_message(argv[4], "private.pem", &sig_len);
        if (!signature) {
            printf("消息签名失败！\n");
            return 1;
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
            .to = argv[2],
            .subject = argv[3],
            .body = argv[4],
            .signature_file = "temp_signature.bin"
        };
        
        // 发送邮件
        if (send_signed_mail(&config, &content)) {
            printf("签名邮件发送成功！\n");
        } else {
            printf("签名邮件发送失败！\n");
            return 1;
        }

        // 清理
        remove("temp_signature.bin");
        mail_cleanup();
    }
    else if(strcmp(argv[1],"-l")==0){
         mail_init();

        // printf("test\n");

        // 加载邮件配置
        mail_config_t config;
        if (!load_mail_config(&config, CONFIG_FILE)) {
            printf("无法加载邮件配置，请先运行 -c 选项配置邮件\n");
            return 1;
        }
        mail_list_t* list = receive_mail_list(&config);

        if (list->items == NULL && list->count > 0) {
            printf("邮件列表项数据为空\n");
            free(list);
            return 1;
        }

        return 1;
    }
    else {
        print_usage();
        return 1;
    }

    // 清理OpenSSL
    EVP_cleanup();
    ERR_free_strings();
    printf("程序结束\n");

    return 0;
} 