#include <string.h>
#include <stdbool.h>
            
#include <utils.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
    
// ----------------- XXXX --------------------

unsigned char *ssl_keygen(char *password)
{
    // should use a randomly generated salt unique to each user/file
    const unsigned char salt[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
    int salt_len = sizeof(salt);

    // output Buffer (32 bytes = 256 bits for AES-256)
    static unsigned char derived_key[32];
    int key_len = sizeof(derived_key);

    // configuration
    int iterations = 10000;              // High number prevents brute-force attacks
    const EVP_MD *digest = EVP_sha256(); // Internal PRF (Pseudo-Random Function)

    // call the OpenSSL Key Derivation function
    int status = PKCS5_PBKDF2_HMAC(
        password,
        strlen(password),
        salt,
        salt_len,
        iterations,
        digest,
        key_len,
        derived_key);
    if (status != 1) {
        //ERROR("PKCS5_PBKDF2_HMAC failed\n"); //xxx review error prints
        return NULL;
    }

    // debug print the derived key
    char str[100], *p=str;
    for (int i = 0; i < key_len; i++) {
        p += sprintf(p, "%02x", derived_key[i]);
    }
    //printf("got derived_key '%s'\n", str);  // xxx check these prints

    // return derived_key
    return derived_key;
}

int ssl_encrypt(unsigned char *key, char *plaintext_arg, ssl_payload_t *payload)
{
    EVP_CIPHER_CTX *ctx = NULL;
    int             len, ciphertext_len;
    bool            succ;
    char           *errstr = NULL;
    char            plaintext[OPENSSL_TEXTLEN];

    // xxx copy plaintext
    // xxx check len
    strncpy(plaintext, plaintext_arg, OPENSSL_TEXTLEN);
    plaintext[OPENSSL_TEXTLEN-1] = '\0';

    // zero payload
    memset(payload, 0, sizeof(ssl_payload_t));

    // alloc ctx
    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        errstr = "EVP_CIPHER_CTX_new";
        goto error;
    }

    // initialize cipher
    succ = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    if (!succ) {
        errstr = "EVP_EncryptInit_ex";
        goto error;
    }

    // initialize nonce to random value
    RAND_bytes(payload->nonce, sizeof(payload->nonce));

    // set length of nonce (aka IV)
    succ = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL);
    if (!succ) {
        errstr = "EVP_CIPHER_CTX_ctrl";
        goto error;
    }

    // set Key and Nonce
    succ = EVP_EncryptInit_ex(ctx, NULL, NULL, key, payload->nonce);
    if (!succ) {
        errstr = "EVP_EncryptInit_ex";
        goto error;
    }

    // encrypt plaintext
    succ = EVP_EncryptUpdate(ctx, payload->ciphertext, &len, (unsigned char *)plaintext, OPENSSL_TEXTLEN);
    if (!succ) {
        errstr = "EVP_EncryptUpdate";
        goto error;
    }
    ciphertext_len = len;

    // finalize encryption
    succ = EVP_EncryptFinal_ex(ctx, payload->ciphertext + len, &len);
    if (!succ) {
        errstr = "EVP_EncryptFinal";
        goto error;
    }
    ciphertext_len += len;

    // validate ciphertext_len
    if (ciphertext_len != OPENSSL_TEXTLEN) {
        errstr = "invalid ciphertext_len";
        goto error;
    }

    // get the Authentication Tag
    succ = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, payload->tag);
    if (!succ) {
        errstr = "EVP_CIPHER_CTX_ctrl";
        goto error;
    }

    // free ctx, and return success
    EVP_CIPHER_CTX_free(ctx);
    return 0;

    // error return path
error:
    if (ctx) {
        EVP_CIPHER_CTX_free(ctx);
    }
    printf("ERROR: %s failed\n", errstr);
    return -1;
}

int ssl_decrypt(unsigned char *key, ssl_payload_t *payload, char **plaintext_arg)
{
    bool            succ;
    EVP_CIPHER_CTX *ctx = NULL;
    int             len;
    char           *errstr = NULL;
    static char     plaintext[OPENSSL_TEXTLEN];

    // xxx
    *plaintext_arg = NULL;

    // alloc ctx
    ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        errstr = "EVP_CIPHER_CTX_new";
        goto error;
    }

    // initialize cipher
    succ = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL);
    if (!succ) {
        errstr = "EVP_DecryptInit_ex";
        goto error;
    }

    // set nonce length
    succ = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL);
    if (!succ) {
        errstr = "EVP_CIPHER_CTX_ctrl";
        goto error;
    }

    // set key and nonce
    succ = EVP_DecryptInit_ex(ctx, NULL, NULL, key, payload->nonce);
    if (!succ) {
        errstr = "EVP_DecryptInit_ex";
        goto error;
    }

    // decrypt
    succ = EVP_DecryptUpdate(ctx, (unsigned char*)plaintext, &len, payload->ciphertext, OPENSSL_TEXTLEN);
    if (!succ) {
        errstr = "EVP_DecryptUpdate";
        goto error;
    }

    // set the expected tag
    succ = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, payload->tag);
    if (!succ) {
        errstr = "EVP_CIPHER_CTX_ctrl";
        goto error;
    }

    // finalize decryption; this checks the tag integrity.
    succ = EVP_DecryptFinal_ex(ctx, (unsigned char*)plaintext + len, &len);
    if (!succ) {
        errstr = "EVP_DecryptFinal_ex";
        goto error;
    }

    // free ctx, and return success
    EVP_CIPHER_CTX_free(ctx);
    *plaintext_arg = plaintext;
    //printf("decrypt success '%s'\n", plaintext);
    return 0;

    // error return path
error:
    if (ctx) {
        EVP_CIPHER_CTX_free(ctx);
    }
    printf("ERROR: %s failed\n", errstr);
    return -1;
}
