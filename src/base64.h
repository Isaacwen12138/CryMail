#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>

// Base64编码
extern int base64_encode(const unsigned char* input, size_t input_len, 
                 char* output, size_t* output_len);

// Base64解码
extern int base64_decode(const char* input, size_t input_len,
                 unsigned char* output, size_t* output_len);

#endif // BASE64_H 