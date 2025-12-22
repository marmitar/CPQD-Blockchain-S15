#include <limits.h>
#include <stdio.h>

#include "../enclave.h"
#include "defines.h"
#include "enclave_t.h"

#pragma GCC diagnostic ignored "-Wtype-limits"

/** Minimum value for the password (inclusive). */
static constexpr unsigned MIN_PASSWORD = 0;
/** Maximum value for the password (inclusive). */
static constexpr unsigned MAX_PASSWORD = 99'999;

/**
 * Check if password value is in the expected range.
 */
#define IS_VALID(password) likely(MIN_PASSWORD <= (password) && (password) <= MAX_PASSWORD)

/**
 * An invalid password used for initialization.
 */
static constexpr unsigned UNINITIALIZED_PASSWORD = UINT_MAX;
static_assert(!IS_VALID(UNINITIALIZED_PASSWORD));

[[nodiscard("pure function"), gnu::const, gnu::cold]]
/**
 * Generate password from fixed seed. Returns `UNINITIALIZED_PASSWORD` on errors.
 */
static unsigned generate_password(void) {
    drbg_ctr128_t rng = drbg_seeded_init(2);

    uint128_t value = UINT128_MAX;
    const bool ok = drbg_rand_bounded(&rng, &value, MAX_PASSWORD - MIN_PASSWORD + 1);
    if unlikely (!ok) {
        return UNINITIALIZED_PASSWORD;
    }

    assume(value <= MAX_PASSWORD - MIN_PASSWORD);
    return MIN_PASSWORD + (unsigned) value;
}

[[nodiscard("effectively pure function"), gnu::const, gnu::hot]]
/**
 * Get expected password or generate from seed. Returns `UNINITIALIZED_PASSWORD` on errors.
 */
static unsigned get_password(void) {
    static unsigned cache = UNINITIALIZED_PASSWORD;
    // CONCURRENCY: although racy, the seed guarantees `generate_password` always return the same value,
    // so we always write the same value. This is also why this function can be safely marked as `const`.
    if unlikely (!IS_VALID(cache)) {
        cache = generate_password();
    }
    return cache;
}

[[nodiscard("error must be checked"), gnu::leaf, gnu::nothrow]]
/**
 * Challenge 2: Crack the Password
 * -------------------------------
 *
 * Returns 0 if the password is right, negative otherwise.
 *
 * HINT: the password is an integer between 0 and 99999.
 */
extern int ecall_verificar_senha(unsigned int senha) {
    const unsigned expected_password = get_password();
    if unlikely (!IS_VALID(expected_password)) {
#ifdef DEBUG
        printf("[ENCLAVE] ecall_verificar_senha: failed to generate password\n");
#endif
        return -2;
    }

    if unlikely (!IS_VALID(senha)) {
#ifdef DEBUG
        printf("[DEBUG] ecall_verificar_senha: invalid password=%u\n", senha);
#endif
        return -1;
    }

    if likely (senha != expected_password) {
        return -1;
    }

    printf("\n%s\n", SEPARATOR);
    printf("[ENCLAVE] DESAFIO 2 CONCLUIDO!! a senha é %u\n", expected_password);
    printf("%s\n", SEPARATOR);
    return 0;
}
