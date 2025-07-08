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
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Play 20 rounds of rock-paper-scissors against the enclave. You must win all 20 rounds.
 *
 * How it works:
 *   1. The enclave picks rock (0), paper (1) or scissors (2).
 *   2. It ALWAYS plays the same move in round 1.
 *   3. It calls `ocall_pedra_papel_tesoura`, passing the current round number, counting 1, 2, 3... up to 20.
 *   4. It compares the moves; if you win, it increments your win count.
 *   5. The enclave's moves are deterministic, but the result of the previous round INFLUENCES its next move.
 *   6. After round 20 the enclave returns how many times YOU won. If the return value is 20 the challenge is
 *      complete and the console prints every round and outcome.
 *
 * - Returns -1 if your OCALL returns anything other than 0, 1 or 2.
 * - The enclave aborts if your OCALL fails or aborts.
 *
 * HINT: the strategy is deterministic; as long as the sequence of previous results is the same, the enclave
 *  plays the same moves.
 **/
extern int ecall_pedra_papel_tesoura(void) {
    return -1;
}

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
