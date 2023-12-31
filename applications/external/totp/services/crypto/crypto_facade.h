#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "../../types/crypto_settings.h"
#include "common_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Checks whether key slot can be used for encryption purposes
 * @param key_slot key slot index
 * @return \c true if key slot can be used for encryption; \c false otherwise
 */
bool totp_crypto_check_key_slot(uint8_t key_slot);

/**
 * @brief Encrypts plain data using built-in certificate and given initialization vector (IV)
 * @param plain_data plain data to be encrypted
 * @param plain_data_length plain data length
 * @param crypto_settings crypto settings
 * @param[out] encrypted_data_length encrypted data length
 * @return Encrypted data
 */
uint8_t* totp_crypto_encrypt(
    const uint8_t* plain_data,
    const size_t plain_data_length,
    const CryptoSettings* crypto_settings,
    size_t* encrypted_data_length);

/**
 * @brief Decrypts encrypted data using built-in certificate and given initialization vector (IV)
 * @param encrypted_data encrypted data to be decrypted
 * @param encrypted_data_length encrypted data length
 * @param crypto_settings crypto settings
 * @param[out] decrypted_data_length decrypted data length
 * @return Decrypted data
 */
uint8_t* totp_crypto_decrypt(
    const uint8_t* encrypted_data,
    const size_t encrypted_data_length,
    const CryptoSettings* crypto_settings,
    size_t* decrypted_data_length);

/**
 * @brief Seed initialization vector (IV) using user's PIN
 * @param crypto_settings crypto settings
 * @param pin user's PIN
 * @param pin_length user's PIN length
 * @return Results of seeding IV
 */
CryptoSeedIVResult
    totp_crypto_seed_iv(CryptoSettings* crypto_settings, const uint8_t* pin, uint8_t pin_length);

/**
 * @brief Verifies whether cryptographic information (certificate + IV) is valid and can be used for encryption and decryption
 * @param crypto_settings crypto settings
 * @return \c true if cryptographic information is valid; \c false otherwise
 */
bool totp_crypto_verify_key(const CryptoSettings* crypto_settings);

#ifdef __cplusplus
}
#endif
