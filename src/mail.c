#include "mail.h"
#include "base64.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_PAYLOAD_SIZE 1024*1024  // 1MB
#define CONFIG_LINE_MAX 256

// 用于存储邮件内容的结构体
typedef struct {
    char* data;
    size_t size;
} upload_ctx_t;

// libcurl回调函数，用于读取要发送的数据
static size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp) {
    upload_ctx_t* ctx = (upload_ctx_t*)userp;
    size_t available = ctx->size;
    size_t can_copy = size * nmemb;

    if (available < can_copy)
        can_copy = available;

    if (can_copy > 0) {
        memcpy(ptr, ctx->data, can_copy);
        ctx->data += can_copy;
        ctx->size -= can_copy;
        return can_copy;
    }

    return 0;
}

int mail_init() {
    return curl_global_init(CURL_GLOBAL_DEFAULT);
}

void mail_cleanup() {
    curl_global_cleanup();
}

// 生成MIME邮件内容
static char* generate_mime_message(const mail_content_t* content) {
    char* mime = malloc(MAX_PAYLOAD_SIZE);
    time_t now = time(NULL);
    char date[128];
    strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S %z", localtime(&now));

    // 构建MIME消息头
    int offset = snprintf(mime, MAX_PAYLOAD_SIZE,
        "Date: %s\r\n"
        "To: %s\r\n"
        "From: %s\r\n"
        "Subject: %s\r\n"
        "MIME-Version: 1.0\r\n"
        "Content-Type: multipart/mixed; boundary=\"boundary\"\r\n"
        "\r\n"
        "--boundary\r\n"
        "Content-Type: text/plain; charset=\"utf-8\"\r\n"
        "\r\n"
        "%s\r\n",
        date, content->to, content->from, content->subject, content->body);

    // 如果有签名文件，添加签名附件
    if (content->signature_file) {
        FILE* fp = fopen(content->signature_file, "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END);
            long sig_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            unsigned char* signature = malloc(sig_size);
            fread(signature, 1, sig_size, fp);
            fclose(fp);

            offset += snprintf(mime + offset, MAX_PAYLOAD_SIZE - offset,
                "\r\n--boundary\r\n"
                "Content-Type: application/octet-stream\r\n"
                "Content-Transfer-Encoding: base64\r\n"
                "Content-Disposition: attachment; filename=\"signature.bin\"\r\n"
                "\r\n");

            // Base64编码签名
            size_t base64_len;
            char* base64_sig = malloc(sig_size * 2);  // 足够大的空间
            base64_encode(signature, sig_size, base64_sig, &base64_len);

            offset += snprintf(mime + offset, MAX_PAYLOAD_SIZE - offset,
                "%s\r\n", base64_sig);

            free(signature);
            free(base64_sig);
        }
    }

    offset += snprintf(mime + offset, MAX_PAYLOAD_SIZE - offset,
        "\r\n--boundary--\r\n");

    return mime;
}

int send_signed_mail(const mail_config_t* config, const mail_content_t* content) {
    CURL* curl = curl_easy_init();
    if (!curl) return 0;

    // 生成MIME消息
    char* mime_message = generate_mime_message(content);
    upload_ctx_t upload_ctx = { mime_message, strlen(mime_message) };

    struct curl_slist* recipients = NULL;
    recipients = curl_slist_append(recipients, content->to);

    char url[256];
    snprintf(url, sizeof(url), "%s://%s:%d",
        config->use_ssl ? "smtps" : "smtp",
        config->smtp_server,
        config->port);

    // 设置CURL选项
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, content->from);
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
    curl_easy_setopt(curl, CURLOPT_USERNAME, config->username);
    curl_easy_setopt(curl, CURLOPT_PASSWORD, config->password);
    curl_easy_setopt(curl, CURLOPT_USE_SSL, config->use_ssl ? CURLUSESSL_ALL : CURLUSESSL_NONE);
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 
    // 发送邮件
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }


    // 清理
    curl_slist_free_all(recipients);
    curl_easy_cleanup(curl);
    free(mime_message);

    return (res == CURLE_OK);
}

int save_mail_config(const mail_config_t* config, const char* config_file) {
    FILE* fp = fopen(config_file, "w");
    if (!fp) return 0;

    fprintf(fp, "smtp_server=%s\n", config->smtp_server);
    fprintf(fp, "username=%s\n", config->username);
    fprintf(fp, "password=%s\n", config->password);
    fprintf(fp, "port=%d\n", config->port);
    fprintf(fp, "use_ssl=%d\n", config->use_ssl);
    fprintf(fp, "pop3_server=%s\n", config->pop3_server);
    fprintf(fp, "pop3_port=%d\n", config->pop3_port);
    fprintf(fp, "pop3_use_ssl=%d\n", config->pop3_use_ssl);

    fclose(fp);
    return 1;
}

int load_mail_config(mail_config_t* config, const char* config_file) {
    FILE* fp = fopen(config_file, "r");
    if (!fp) return 0;

    char line[CONFIG_LINE_MAX];
    char* value;

    while (fgets(line, sizeof(line), fp)) {
        value = strchr(line, '=');
        if (!value) continue;
        *value++ = '\0';
        value[strcspn(value, "\r\n")] = 0;

        if (strcmp(line, "smtp_server") == 0)
            config->smtp_server = strdup(value);
        else if (strcmp(line, "username") == 0)
            config->username = strdup(value);
        else if (strcmp(line, "password") == 0)
            config->password = strdup(value);
        else if (strcmp(line, "port") == 0)
            config->port = atoi(value);
        else if (strcmp(line, "use_ssl") == 0)
            config->use_ssl = atoi(value);
        else if (strcmp(line, "pop3_server") == 0)
            config->pop3_server = strdup(value);
        else if (strcmp(line, "pop3_port") == 0)
            config->pop3_port = atoi(value);
        else if (strcmp(line, "pop3_use_ssl") == 0)
            config->pop3_use_ssl = atoi(value);
    }

    fclose(fp);
    return 1;
} 