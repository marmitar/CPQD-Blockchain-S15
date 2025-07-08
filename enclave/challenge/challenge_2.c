#include <limits.h>
#include <stdio.h>

#include "../enclave.h"
#include "defines.h"
#include "enclave_t.h"

/** Minimum value for the password (inclusive). */
static constexpr unsigned MIN_PASSWORD = 0;
/** Maximum value for the password (inclusive). */
static constexpr unsigned MAX_PASSWORD = 99'999;

[[nodiscard("pure function"), gnu::const, gnu::cold]]
/**
 * Generate password from fixed seed. Returns `UINT_MAX` on errors.
 */
static unsigned password(void) {
    drbg_ctr128_t rng = drbg_seeded_init(2);

    uint128_t value = UINT128_MAX;
    const bool ok = drbg_rand_bounded(&rng, &value, MAX_PASSWORD - MIN_PASSWORD + 1);
    if unlikely (!ok) {
        return UINT_MAX;
    }

    assume(value <= MAX_PASSWORD - MIN_PASSWORD);
    return MIN_PASSWORD + (unsigned) value;
}

[[nodiscard("error must be checked"), gnu::const, gnu::leaf, gnu::nothrow]]
/**
 * Challenge 2: Crack the Password
 * -------------------------------
 *
 * Returns 0 if the password is right, negative otherwise.
 *
 * HINT: the password is an integer between 0 and 99999.
 */
extern int ecall_verificar_senha(unsigned int senha) {
    static unsigned expected_password = UINT_MAX;
    static bool initialized = false;
    if unlikely (!initialized) {
        expected_password = password();
        if unlikely (expected_password == UINT_MAX) {
            return -2;
        }
        initialized = true;
    }
    assume(MIN_PASSWORD <= expected_password && expected_password <= MAX_PASSWORD);

    if unlikely (senha < MIN_PASSWORD || senha > MAX_PASSWORD) {
#ifdef DEBUG
        printf("[DEBUG] ecall_verificar_senha: invalid password=%u\n", senha);
#endif
        return -1;
    }

    if likely (senha != expected_password) {
        return -1;
    }

    printf(
        // clang-format off
        "\n"
        "------------------------------------------------\n"
        "\n"
        "DESAFIO 2 CONCLUIDO!! a senha é %u\n"
        "\n"
        "------------------------------------------------\n"
        "\n",
        // clang-format on
        expected_password
    );
    return 0;
}
