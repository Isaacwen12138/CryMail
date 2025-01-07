#include "mail.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

// 用于存储接收到的数据的结构体
typedef struct {
    char* data;
    size_t size;
} receive_ctx_t;

// libcurl写入回调函数
static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realsize = size * nmemb;
    receive_ctx_t* ctx = (receive_ctx_t*)userp;

    char* ptr = realloc(ctx->data, ctx->size + realsize + 1);
    if (!ptr) return 0;

    ctx->data = ptr;
    memcpy(&(ctx->data[ctx->size]), contents, realsize);
    ctx->size += realsize;
    ctx->data[ctx->size] = 0;

    return realsize;
}

// 解析邮件列表
static mail_list_t* parse_mail_list(const char* data) {
    mail_list_t* list = malloc(sizeof(mail_list_t));
    list->items = NULL;
    list->count = 0;

    // 这里需要根据POP3/IMAP协议解析邮件列表
    // 这是一个简化的示例
    char* line = strtok((char*)data, "\r\n");
    while (line) {
        if (strncmp(line, "+OK", 3) != 0) {  // 跳过POP3响应行
            list->count++;
            list->items = realloc(list->items, list->count * sizeof(mail_item_t));
            mail_item_t* item = &list->items[list->count - 1];

            // 解析邮件信息（这里需要根据实际协议响应格式调整）
            char* token = strtok(line, " ");
            item->uid = strdup(token);
            token = strtok(NULL, " ");
            item->from = strdup(token);
            token = strtok(NULL, " ");
            item->subject = strdup(token);
            token = strtok(NULL, " ");
            item->date = strdup(token);
            item->has_signature = 0;  // 需要检查附件来确定
        }
        line = strtok(NULL, "\r\n");
    }

    return list;
}

// 解析邮件内容
static mail_content_t* parse_mail_content(const char* data) {
    mail_content_t* content = malloc(sizeof(mail_content_t));
    
    // 这里需要根据POP3/IMAP协议解析邮件内容
    // 这是一个简化的示例
    content->from = strdup("");
    content->to = strdup("");
    content->subject = strdup("");
    content->body = strdup(data);
    content->signature_file = NULL;

    return content;
}

mail_list_t* receive_mail_list(const mail_config_t* config) {
    CURL* curl = curl_easy_init();
    if (!curl) return NULL;

    receive_ctx_t ctx = { 0 };
    char url[256];
    snprintf(url, sizeof(url), "%s://%s:%d",
        config->pop3_use_ssl ? "pop3s" : "pop3",
        config->pop3_server,
        config->pop3_port);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERNAME, config->username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config->password);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, config->pop3_use_ssl ? CURLUSESSL_ALL : CURLUSESSL_NONE);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(ctx.data);
        return NULL;
    }

    mail_list_t* list = parse_mail_list(ctx.data);
    free(ctx.data);
    return list;
}

mail_content_t* receive_mail_content(const mail_config_t* config, const char* uid) {
    CURL* curl = curl_easy_init();
    if (!curl) return NULL;

    receive_ctx_t ctx = { 0 };
    char url[256];
    snprintf(url, sizeof(url), "%s://%s:%d/RETR %s",
        config->pop3_use_ssl ? "pop3s" : "pop3",
        config->pop3_server,
        config->pop3_port,
        uid);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_USERNAME, config->username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config->password);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, config->pop3_use_ssl ? CURLUSESSL_ALL : CURLUSESSL_NONE);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        free(ctx.data);
        return NULL;
    }

    mail_content_t* content = parse_mail_content(ctx.data);
    free(ctx.data);
    return content;
}

void free_mail_list(mail_list_t* list) {
    if (!list) return;

    for (int i = 0; i < list->count; i++) {
        free(list->items[i].uid);
        free(list->items[i].from);
        free(list->items[i].subject);
        free(list->items[i].date);
    }
    free(list->items);
    free(list);
}

void free_mail_content(mail_content_t* content) {
    if (!content) return;

    free((void*)content->from);
    free((void*)content->to);
    free((void*)content->subject);
    free((void*)content->body);
    if (content->signature_file)
        free((void*)content->signature_file);
    free(content);
} 