#include <inttypes.h>  // IWYU pragma: keep
#include <limits.h>
#include <pthread.h>
#include <sgx_error.h>
#include <sgx_tcrypto.h>
#include <sgx_trts.h>  // IWYU pragma: keep
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

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

[[nodiscard("error must be checked"), gnu::nonnull(1, 2, 3), gnu::cold, gnu::noinline, gnu::nothrow]]
/**
 * Acquire write lock and initialize seed, if not initialized already.
 */
static bool drbg_seed_init(pthread_rwlock_t *NONNULL lock, bool *NONNULL initialized, uint64_t *NONNULL seed) {
    // write step: check and initialize seed
    int rv = pthread_rwlock_wrlock(lock);
    if unlikely (rv != 0) {
#ifdef DEBUG
        printf("[DEBUG] drbg_seed: failed to acquire wrlock: %d\n", rv);
#endif
        return false;
    }

    sgx_status_t status = SGX_SUCCESS;
    // write new seed value, ensure write once semantics
    if likely (!*initialized) {
#if ENCLAVE_SEED < 0
        status = sgx_read_rand((uint8_t *) seed, sizeof(uint64_t));
#else
        *seed = ENCLAVE_SEED;
#endif
        *initialized = (status == SGX_SUCCESS);
    }

    rv = pthread_rwlock_unlock(lock);
    if unlikely (rv != 0) {
#ifdef DEBUG
        printf("[DEBUG] drbg_seed: failed to release wrlock: %d\n", rv);
#endif
        return false;
    }
    if unlikely (status != SGX_SUCCESS) {
#ifdef DEBUG
        printf("[DEBUG] drbg_seed: failed read rand: %04x\n", (unsigned) status);
#endif
        return false;
    }

#ifdef DEBUG
#    if ENCLAVE_SEED < 0
    printf("[DEBUG] drbg_seed: predefined %016" PRIx64 "\n", *seed);
#    else
    printf("[DEBUG] drbg_seed: generated %016" PRIx64 "\n", *seed);
#    endif
#endif
    return true;
}

[[nodiscard("error must be checked"), gnu::nonnull(1), gnu::hot, gnu::nothrow]]
/**
 * Acquire read lock and read seed, if initialized. Otherwise try to initialize it.
 */
static bool drbg_seed(uint64_t *NONNULL output) {
    static pthread_rwlock_t lock = PTHREAD_RWLOCK_INITIALIZER;
    static bool initialized = false;
    static uint64_t seed = 0;

    // read step: use seed if already initialized
    int rv = pthread_rwlock_rdlock(&lock);
    if unlikely (rv != 0) {
#ifdef DEBUG
        printf("[DEBUG] drbg_seed: failed to acquire rdlock: %d\n", rv);
#endif
        return false;
    }

    // safe to read, but not to write
    bool ok = initialized;

    rv = pthread_rwlock_unlock(&lock);
    if unlikely (rv != 0) {
#ifdef DEBUG
        printf("[DEBUG] drbg_seed: failed to release rdlock: %d\n", rv);
#endif
        return false;
    }

    if unlikely (!ok) {
        ok = drbg_seed_init(&lock, &initialized, &seed);
        if unlikely (!ok) {
            return false;
        }
    }

    // at this point, seed is already initialized, so
    // no other thread will write on it and it's safe to read
    *output = seed;
    return true;
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
    uint64_t seed = 0;
    const bool ok = drbg_seed(&seed);
    if unlikely (!ok) {
        abort();
    }

    return drbg_init(seed, stream);
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
