#include "mail.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "crypto.h"

// 用于存储接收到的数据的结构体
typedef struct {
    char* data;
    size_t size;
} receive_ctx_t;

void trim_body(char** body) {
    if (*body == NULL) {
        return;
    }

    // 移动指针移去前导空行和横杠
    char* start = *body;
    while (*start) {
        if (*start == '\n' || *start == '\r' || *start == '-') {
            start++;
        } else {
            break;
        }
    }

    // 去除尾部空行和横杠
    char* end = start + strlen(start) - 1;
    while (end > start && (*end == '\n' || *end == '\r' || *end == '-')) {
        end--;
    }
    *(end + 1) = '\0';

    // 更新 body 指向去掉多余空行和分隔符的内容
    *body = strdup(start);
}

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

// 帮助函数：提取字段值，处理行匹配和字符串拷贝
static char* get_field_value(const char* line, const char* field_name) {
    size_t field_len = strlen(field_name);
    if (strncmp(line, field_name, field_len) == 0) {
        // 跳过字段名部分并去除多余的空格
        const char* value_start = line + field_len;
        while (*value_start == ' ' || *value_start == '\t') {
            value_start++;
        }
        // 返回字段值的副本
        return strdup(value_start);
    }
    return NULL;
}

// 核心解析函数
void parse_mail_list(mail_list_t* list, const char* data, int index) {
    // 确保有足够的空间
    if (index >= list->count) {
        list->items = realloc(list->items, sizeof(mail_item_t) * (index + 1));
        list->count = index + 1;
    }

    // 指向当前解析的邮件项
    mail_item_t* item = &list->items[index];
    
    item->uid = NULL;
    item->from = NULL;
    item->subject = NULL;
    item->date = NULL;
    item->has_signature = 0;
    item->body = NULL;
    item->signature_file = NULL;
    item->signature_file_len = 0;

    // 解析 UID
    const char* uid_tag = "\nMessage-ID: ";
    char* uid_start = strstr(data, uid_tag);
    if (uid_start) {
        uid_start += strlen(uid_tag);
        char* uid_end = strstr(uid_start, "\r\n");
        size_t uid_length = uid_end ? uid_end - uid_start : strlen(uid_start);
        item->uid = malloc(uid_length + 1);
        strncpy(item->uid, uid_start, uid_length);
        item->uid[uid_length] = '\0';
    } else {
        item->uid = strdup("No UID");
    }

    // 解析发件人
    const char* from_tag = "\nFrom: ";
    char* from_start = strstr(data, from_tag);
    if (from_start) {
        from_start += strlen(from_tag);
        char* from_end = strstr(from_start, "\r\n");
        size_t from_length = from_end ? from_end - from_start : strlen(from_start);
        item->from = malloc(from_length + 1);
        strncpy(item->from, from_start, from_length);
        item->from[from_length] = '\0';
    } else {
        item->from = strdup("No From");
    }

    // 解析日期
    const char* date_tag = "\nDate: ";
    char* date_start = strstr(data, date_tag);
    if (date_start) {
        date_start += strlen(date_tag);
        char* date_end = strstr(date_start, "\r\n");
        size_t date_length = date_end ? date_end - date_start : strlen(date_start);
        item->date = malloc(date_length + 1);
        strncpy(item->date, date_start, date_length);
        item->date[date_length] = '\0';
    } else {
        item->date = strdup("No Date");
    }

    // 解析主题
    const char* subject_tag = "\nSubject: ";
    char* subject_start = strstr(data, subject_tag);
    if (subject_start) {
        subject_start += strlen(subject_tag);
        char* subject_end = strstr(subject_start, "\r\n");
        size_t subject_length = subject_end ? subject_end - subject_start : strlen(subject_start);
        item->subject = malloc(subject_length + 1);
        strncpy(item->subject, subject_start, subject_length);
        item->subject[subject_length] = '\0';
    } else {
        item->subject = strdup("No Subject");
    }

    const char* body_start = strstr(data, "\r\n\r\n");
    if (body_start) {
        body_start += strlen("\r\n\r\n");
    } else {
        body_start = data;
    }

    const char* boundary_tag_start = strstr(data, "Content-Type: multipart/mixed; boundary=");
    if (boundary_tag_start != NULL) {
        boundary_tag_start += strlen("Content-Type: multipart/mixed; boundary=");
        char boundary[256];
        sscanf(boundary_tag_start, "\"%255[^\"]\"", boundary);

        char delimiter[256];
        snprintf(delimiter, sizeof(delimiter), "--%s", boundary);

        char* part_start = strstr(body_start, delimiter);
        while (part_start) {
            part_start += strlen(delimiter);
            if (strncmp(part_start, "--", 2) == 0) {
                break;
            }

            const char* text_start_tag = "Content-Type: text/plain;";
            if ((part_start = strstr(part_start, text_start_tag)) != NULL) {
                part_start = strstr(part_start, "\r\n\r\n");
                if (part_start) {
                    part_start += strlen("\r\n\r\n");
                    char* next_part_start = strstr(part_start, delimiter);
                    size_t part_length = next_part_start ? (next_part_start - part_start) : strlen(part_start);
                    item->body = malloc(part_length + 1);
                    strncpy(item->body, part_start, part_length);
                    item->body[part_length] = '\0';

                    trim_body(&item->body);
                }
            }

            const char* signature_tag = "Content-Disposition: attachment; filename=\"signature.bin\"\r\n\r\n";
            if ((part_start = strstr(part_start, signature_tag)) != NULL) {
                item->has_signature = 1;
                const char* signature_start = part_start + strlen(signature_tag);
                char* next_part_start = strstr(signature_start, delimiter);
                size_t signature_len = next_part_start ? (next_part_start - signature_start) : strlen(signature_start);

                // 去除末尾的空行和横杠
                char* signature_end = (char *)(signature_start + signature_len - 1);
                while (signature_end > signature_start && (*signature_end == '\n' || *signature_end == '\r' || *signature_end == '-' || *signature_end == ' ')) {
                    signature_end--;
                }
                *(signature_end + 1) = '\0';

                item->signature_file = strdup(signature_start);
                item->signature_file_len = strlen(item->signature_file);
            }

            part_start = strstr(part_start, delimiter);
        }
    } else {
        if (body_start) {
            item->body = strdup(body_start);
            trim_body(&item->body);
        } else {
            item->body = strdup("No Body");
        }
    }
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
    CURL* curl;
    CURLcode res;
    mail_list_t* list = malloc(sizeof(mail_list_t));
    list->count = 0;
    list->items = NULL; // 初始化为空

    for (size_t i = 1; i <= 8; ++i) {
        curl = curl_easy_init();
        if (!curl) {
            printf("CURL init failed!\n");
            free(list->items);
            free(list);
            return NULL;
        }

        receive_ctx_t ctx = { NULL, 0 };
        char url[256];
        snprintf(url, sizeof(url), "%s://%s:%d/%zu", // 获取第i封邮件
                 config->pop3_use_ssl ? "pop3s" : "pop3",
                 config->pop3_server,
                 config->pop3_port, i);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERNAME, config->username);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, config->password);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        // curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "RETR");

        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) {
            // printf("Failed to retrieve email %zu: %s\n", i, curl_easy_strerror(res));
            free(ctx.data);
            break;
        }

        // printf("%s\n",ctx.data);
        parse_mail_list(list,ctx.data, i - 1);
        printf("Date:%s Email #%zu: From:%s Subject: %s has_signature:%d body:%s\n", list->items[i-1].date,i,list->items[i-1].from, list->items[i - 1].subject,list->items[i-1].has_signature,list->items[i-1].body);

        free(ctx.data);
    }
    printf("请给出你想阅读的邮件序号(1-8,输入0则退出)：");
    int mail_num;
    scanf("%d",&mail_num);
    if(mail_num == 0)
    {
        return 1;
    }
    else if(mail_num>=1&&mail_num<=8)
    {
        printf("Email #%zu: Date:%s  From:%s Subject: %s\ncontent:%s\nsignature:%s\n",mail_num,list->items[mail_num-1].date,list->items[mail_num-1].from, list->items[mail_num-1].subject,list->items[mail_num-1].body,list->items[mail_num-1].signature_file);
        if(list->items[mail_num-1].has_signature)
        {
            if(!verify_signature(list->items[mail_num-1].body,list->items[mail_num-1].signature_file,list->items[mail_num-1].signature_file_len,"public.pem"))
                printf("签名验证失败！消息可能被篡改。\n");
        }
    }

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