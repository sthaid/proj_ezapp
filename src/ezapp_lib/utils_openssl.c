#include <std_hdrs.h>
            
#include <utils.h>
#include <private.h>

#include <openssl/evp.h>
    
// ----------------- XXXX --------------------

unsigned char *openssl_keygen(char *password)
{
    // xxx should use a randomly generated salt unique to each user/file
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
        ERROR("PKCS5_PBKDF2_HMAC failed\n");
        return NULL;
    }

    // debug print the derived key
    char str[100], *p=str;
    for (int i = 0; i < key_len; i++) {
        p += sprintf(p, "%02x", derived_key[i]);
    }
    INFO("got derived_key '%s'\n", str);

    // return derived_key
    return derived_key;
}
