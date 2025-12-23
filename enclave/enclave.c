#include <limits.h>
#include <sgx_error.h>
#include <sgx_tcrypto.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

#include "./enclave.h"
#include "defines.h"
#include "enclave_t.h"

/**
 * `printf`-like function for the enclave. Buffer limited to `BUFSIZ` (8192) bytes.
 */
int printf(const char *NONNULL fmt, ...) {
    char buf[BUFSIZ] = "";

    va_list ap;
    va_start(ap, fmt);
    const int written = vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);

    if unlikely (written <= 0) {
        return written;
    }

    const sgx_status_t status = ocall_print_string(buf);
    if unlikely (status != SGX_SUCCESS) {
        return -1;
    }

    constexpr int MAX_BYTES = likely(BUFSIZ > 0) ? BUFSIZ - 1 : 0;
    return likely(written < MAX_BYTES) ? written : MAX_BYTES;
}

[[nodiscard("error must be checked"), gnu::nonnull(1, 2), gnu::hot, gnu::nothrow]]
/**
 * Generate a pseudo-random number in `[0,UINT128_MAX)` from the DRBG sequence.
 *
 * @return `true` on success, or `false` if AES CTR failed.
 */
static bool drbg_rand(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output) {
    // randomized plaintext is useless in CTR mode
    const uint128_t PLAINTEXT = 0;

    const sgx_status_t status = sgx_aes_ctr_encrypt(
        (const sgx_aes_ctr_128bit_key_t *) &(drbg->key),
        (const uint8_t *) &PLAINTEXT,
        sizeof(PLAINTEXT),
        (uint8_t *) &(drbg->ctr),
        sizeof(drbg->ctr) * CHAR_BIT,
        (uint8_t *) output
    );

    if unlikely (status != SGX_SUCCESS) {
        printf("[ENCLAVE] drbg_rand failed: status=0x%04x\n", status);
        return false;
    }
    return true;
}

/**
 * Generate a pseudo-random number from 0 up to (but not including) `bound`.
 */
bool drbg_rand_threshold(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output, uint128_t threshold) {
    assume(threshold > 0);

    while (true) {
        uint128_t value = UINT128_MAX;
        const bool ok = drbg_rand(drbg, &value);
        if unlikely (!ok) {
            return false;
        }

        if likely (value < threshold) {
            *output = value;
            return true;
        }
    }
}
