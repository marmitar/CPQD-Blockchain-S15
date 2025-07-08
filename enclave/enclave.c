#include <limits.h>
#include <sgx_error.h>
#include <sgx_tcrypto.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "./enclave.h"
#include "defines.h"
#include "enclave_config.h"
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

[[nodiscard("pure function"), gnu::const]]
/**
 * Initialize the PRNG using an input `seed` and a `stream` selector.
 */
static drbg_ctr128_t drbg_init(const uint64_t seed, const uint64_t stream) {
    drbg_ctr128_t drbg = {0};

    const uint64_t key[] = {seed, stream};
    static_assert(sizeof(key) == sizeof(drbg.key));

    memcpy(&(drbg.key), &key, sizeof(drbg.key));
    memset(&(drbg.ctr), 0, sizeof(drbg.ctr));
    return drbg;
}

/**
 * Initialize the PRNG using the seed file and a `stream` selector.
 */
drbg_ctr128_t drbg_seeded_init(const uint64_t stream) {
    return drbg_init(ENCLAVE_SEED, stream);
}

/**
 * Generate a pseudo-random number from the DRBG sequence.
 */
bool drbg_rand(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output) {
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
bool drbg_rand_bounded(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output, uint128_t bound) {
    const uint128_t threshold = UINT128_MAX - UINT128_MAX % bound;

    while (true) {
        uint128_t value = UINT128_MAX;
        const bool ok = drbg_rand(drbg, &value);
        if unlikely (!ok) {
            return false;
        }

        if likely (value < threshold) {
            *output = value % bound;
            return true;
        }
    }
}
