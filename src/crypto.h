#ifndef CRYPTO_H
#define CRYPTO_H

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/sha.h>

// RSA密钥对生成
int generate_key_pair(const char* public_key_file, const char* private_key_file);

// 数字签名
unsigned char* sign_message(const char* message, const char* private_key_file, 
                          unsigned int* signature_len);

// 验证签名
int verify_signature(const char* message, unsigned char* signature, 
                    unsigned int signature_len, const char* public_key_file);

// 计算消息摘要
unsigned char* calculate_digest(const char* message, unsigned int* digest_len);

#endif // CRYPTO_H 