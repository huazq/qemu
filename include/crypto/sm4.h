/**
 * \file sm4.h
 * \brief sm4 block cipher
 */

#ifndef QEMU_SM4_H
#define QEMU_SM4_H

#define SM4_BLOCK_SIZE 16

/**
 * \brief          SM4 key structure
 */
struct sm4_key_st
{
    uint32_t sk[32];			/*!<  SM4 subkeys       */
};
typedef struct sm4_key_st SM4_KEY;

void sm4_init( SM4_KEY *key );
void sm4_free( SM4_KEY *key );

/**
 * \brief          SM4 key schedule (128-bit, encryption)
 *
 * \param key      SM4 key to be initialized
 * \param key      16-byte secret key
 */
int sm4_set_encrypt_key( const unsigned char *userKey, SM4_KEY *key );

/**
 * \brief          SM4 key schedule (128-bit, decryption)
 *
 * \param key      SM4 key to be initialized
 * \param key      16-byte secret key
 */
int sm4_set_decrypt_key( const unsigned char *userKey, SM4_KEY *key );

/**
 * \brief          SM4-ECB block encryption/decryption
 * \param key      SM4 key
 * \param input    input block
 * \param output   output block
 * \param length   length of the input data
 */
int sm4_ecb_crypt( SM4_KEY *key,
                   const unsigned char *input,
                   unsigned char *output,
                   int length);

/**
 * \brief          SM4-CBC buffer encryption
 * \param key      SM4 key
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 * \param length   length of the input data
 */
void sm4_cbc_encrypt( SM4_KEY *key,
                     unsigned char *iv,
                     const unsigned char *input,
                     unsigned char *output,
                     int length );

/**
 * \brief          SM4-CBC buffer decryption
 * \param key      SM4 key
 * \param iv       initialization vector (updated after use)
 * \param input    buffer holding the input data
 * \param output   buffer holding the output data
 * \param length   length of the input data
 */
void sm4_cbc_decrypt( SM4_KEY *key,
                     unsigned char *iv,
                     const unsigned char *input,
                     unsigned char *output,
                     int length );

#endif /* sm4.h */
