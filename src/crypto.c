#include "crypto.h"
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define RSA_KEY_BITS 2048
#define DIGEST_ALG EVP_sha256()

int generate_key_pair(const char* public_key_file, const char* private_key_file) {
    EVP_PKEY_CTX *ctx = NULL;
    EVP_PKEY *pkey = NULL;
    BIO *bp_public = NULL, *bp_private = NULL;
    int ret = 0;

    // 创建RSA密钥生成上下文
    ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) goto cleanup;

    // 初始化密钥生成
    if (EVP_PKEY_keygen_init(ctx) <= 0) goto cleanup;

    // 设置RSA密钥长度
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, RSA_KEY_BITS) <= 0) goto cleanup;

    // 生成密钥对
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) goto cleanup;

    // 保存公钥
    bp_public = BIO_new_file(public_key_file, "w+");
    if (!bp_public || !PEM_write_bio_PUBKEY(bp_public, pkey)) goto cleanup;

    // 保存私钥
    bp_private = BIO_new_file(private_key_file, "w+");
    if (!bp_private || !PEM_write_bio_PrivateKey(bp_private, pkey, NULL, NULL, 0, NULL, NULL)) goto cleanup;

    ret = 1;

cleanup:
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    BIO_free_all(bp_public);
    BIO_free_all(bp_private);

    return ret;
}

unsigned char* calculate_digest(const char* message, unsigned int* digest_len) {
    EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
    unsigned char *digest = malloc(EVP_MAX_MD_SIZE);
    
    EVP_DigestInit_ex(mdctx, DIGEST_ALG, NULL);
    EVP_DigestUpdate(mdctx, message, strlen(message));
    EVP_DigestFinal_ex(mdctx, digest, digest_len);
    
    EVP_MD_CTX_free(mdctx);
    return digest;
}

unsigned char* sign_message(const char* message, const char* private_key_file, 
                          unsigned int* signature_len) {
    FILE* fp = fopen(private_key_file, "r");
    if (!fp) return NULL;

    EVP_PKEY* pkey = PEM_read_PrivateKey(fp, NULL, NULL, NULL);
    fclose(fp);
    if (!pkey) return NULL;

    // 计算消息摘要
    unsigned int digest_len;
    unsigned char* digest = calculate_digest(message, &digest_len);

    // 创建签名上下文
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    unsigned char* signature = NULL;
    size_t sig_len;

    if (EVP_DigestSignInit(ctx, NULL, DIGEST_ALG, NULL, pkey) <= 0) goto cleanup;
    
    // 计算签名长度
    if (EVP_DigestSign(ctx, NULL, &sig_len, digest, digest_len) <= 0) goto cleanup;
    
    // 分配签名缓冲区
    signature = malloc(sig_len);
    
    // 生成签名
    if (EVP_DigestSign(ctx, signature, &sig_len, digest, digest_len) <= 0) {
        free(signature);
        signature = NULL;
        goto cleanup;
    }

    *signature_len = sig_len;

cleanup:
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    free(digest);
    
    return signature;
}

int verify_signature(const char* message, unsigned char* signature,
                    unsigned int signature_len, const char* public_key_file) {
    FILE* fp = fopen(public_key_file, "r");
    if (!fp) return 0;

    EVP_PKEY* pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
    fclose(fp);
    if (!pkey) return 0;

    // 计算消息摘要
    unsigned int digest_len;
    unsigned char* digest = calculate_digest(message, &digest_len);

    // 创建验证上下文
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    int ret = 0;

    if (EVP_DigestVerifyInit(ctx, NULL, DIGEST_ALG, NULL, pkey) <= 0) goto cleanup;
    
    // 验证签名
    ret = (EVP_DigestVerify(ctx, signature, signature_len, digest, digest_len) == 1);

cleanup:
    EVP_MD_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    free(digest);
    
    return ret;
} 