#ifndef MAIL_H
#define MAIL_H

#include <curl/curl.h>

// 邮件配置结构体
typedef struct {
    const char* smtp_server;
    const char* username;
    const char* password;
    int port;
    int use_ssl;
    // 添加POP3/IMAP配置
    const char* pop3_server;
    int pop3_port;
    int pop3_use_ssl;
} mail_config_t;

// 邮件内容结构体
typedef struct {
    const char* from;
    const char* to;
    const char* subject;
    const char* body;
    const char* signature_file;  // 签名文件路径
} mail_content_t;

// 邮件列表项结构体
typedef struct {
    char* uid;
    char* from;
    char* subject;
    char* date;
    int has_signature;  // 是否包含数字签名
    char* body;
    char* signature_file;
    size_t signature_file_len; 
} mail_item_t;

// 邮件列表结构体
typedef struct {
    mail_item_t* items;
    int count;
} mail_list_t;

// 初始化邮件系统
int mail_init();

// 清理邮件系统
void mail_cleanup();

// 发送签名邮件
int send_signed_mail(const mail_config_t* config, const mail_content_t* content);

// 接收邮件列表
mail_list_t* receive_mail_list(const mail_config_t* config);

// 接收指定邮件的完整内容
mail_content_t* receive_mail_content(const mail_config_t* config, const char* uid);

// 验证邮件签名
int verify_mail_signature(const mail_content_t* content, const char* public_key_file);

// 释放邮件列表内存
void free_mail_list(mail_list_t* list);

// 释放邮件内容内存
void free_mail_content(mail_content_t* content);

// 保存邮件配置
int save_mail_config(const mail_config_t* config, const char* config_file);

// 加载邮件配置
int load_mail_config(mail_config_t* config, const char* config_file);

#endif // MAIL_H 