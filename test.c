#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

typedef struct {
    const char* payload_text;
    size_t bytes_read;
} upload_ctx_t;

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp) {
    upload_ctx_t *upload_ctx = (upload_ctx_t *)userp;
    const char *data;
    size_t room = size * nmemb;

    if(upload_ctx->bytes_read == strlen(upload_ctx->payload_text)) {
        return 0;
    }
    
    data = upload_ctx->payload_text + upload_ctx->bytes_read;
    if(data) {
        size_t len = strlen(data);
        if(len > room)
            len = room;
        memcpy(ptr, data, len);
        upload_ctx->bytes_read += len;
        return len;
    }
    return 0;
}

int main(void) {
    CURL *curl;
    CURLcode res = CURLE_OK;
    
    const char *payload_text =
        "Date: Fri, 29 Sep 2023 21:54:29 +1100\r\n"
        "To: <19147167739@163.com>\r\n"
        "From: <m19147167739@126.com>\r\n"
        "Subject: Test mail from CURL\r\n"
        "\r\n"
        "This is a test email sent via libcurl in C.\r\n";

    upload_ctx_t upload_ctx;
    upload_ctx.payload_text = payload_text;
    upload_ctx.bytes_read = 0;

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "smtps://smtp.126.com:994");
        curl_easy_setopt(curl, CURLOPT_USERNAME, "m19147167739@126.com");
        curl_easy_setopt(curl, CURLOPT_PASSWORD, "QLTQidezFWLULGnw");
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, "<m19147167739@126.com>");

        struct curl_slist *recipients = NULL;
        recipients = curl_slist_append(recipients, "<19147167739@163.com>");
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
        
        curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_TLSv1_2);

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        // curl_easy_setopt(curl, CURLOPT_CAINFO, "/root/CryMail/cacert.pem"); // optionally set your own CA cert

        res = curl_easy_perform(curl);
        
        if(res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        else
            printf("Success!\n");

        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
    }

    return (int)res;
}
