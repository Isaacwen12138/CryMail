#include "base64.h"
#include <string.h>

static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_encode(const unsigned char* input, size_t input_len, 
                 char* output, size_t* output_len) {
    size_t i, j;

    // 计算输出长度
    *output_len = 4 * ((input_len + 2) / 3);

    for (i = 0, j = 0; i < input_len;) {
        unsigned int octet_a = i < input_len ? input[i++] : 0;
        unsigned int octet_b = i < input_len ? input[i++] : 0;
        unsigned int octet_c = i < input_len ? input[i++] : 0;

        unsigned int triple = (octet_a << 16) + (octet_b << 8) + octet_c;

        output[j++] = base64_chars[(triple >> 18) & 0x3F];
        output[j++] = base64_chars[(triple >> 12) & 0x3F];
        output[j++] = base64_chars[(triple >> 6) & 0x3F];
        output[j++] = base64_chars[triple & 0x3F];
    }

    // 添加填充
    if (input_len % 3 == 1) {
        output[*output_len - 1] = '=';
        output[*output_len - 2] = '=';
    }
    else if (input_len % 3 == 2) {
        output[*output_len - 1] = '=';
    }

    output[*output_len] = '\0';
    return 1;
}

static int char_to_index(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
}

int base64_decode(const char* input, size_t input_len,
                 unsigned char* output, size_t* output_len) {
    size_t i, j;

    if (input_len % 4 != 0) return 0;

    *output_len = input_len / 4 * 3;
    if (input[input_len - 1] == '=') (*output_len)--;
    if (input[input_len - 2] == '=') (*output_len)--;

    for (i = 0, j = 0; i < input_len;) {
        unsigned int sextet_a = input[i] == '=' ? 0 : char_to_index(input[i]); i++;
        unsigned int sextet_b = input[i] == '=' ? 0 : char_to_index(input[i]); i++;
        unsigned int sextet_c = input[i] == '=' ? 0 : char_to_index(input[i]); i++;
        unsigned int sextet_d = input[i] == '=' ? 0 : char_to_index(input[i]); i++;

        unsigned int triple = (sextet_a << 18) + (sextet_b << 12) +
                            (sextet_c << 6) + sextet_d;

        if (j < *output_len) output[j++] = (triple >> 16) & 0xFF;
        if (j < *output_len) output[j++] = (triple >> 8) & 0xFF;
        if (j < *output_len) output[j++] = triple & 0xFF;
    }

    return 1;
} 