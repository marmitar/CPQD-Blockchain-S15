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
 * Generate password from fixed seed.
 */
static unsigned password(void) {
    drbg_ctr128_t rng = drbg_seeded_init(2);

    static constexpr uint128_t LENGTH = (uint128_t) (MAX_PASSWORD - MIN_PASSWORD);
    static constexpr uint128_t THRESHOLD = UINT128_MAX - UINT128_MAX % LENGTH;

    while (true) {
        uint128_t value = UINT128_MAX;
        const bool ok = drbg_rand(&rng, &value);
        assume(ok);

        if likely (value < THRESHOLD) {
            return MIN_PASSWORD + (unsigned) (value % LENGTH);
        }
    }
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
    static unsigned expected_password = 0;
    static bool initialized = false;
    if unlikely (!initialized) {
        expected_password = password();
        initialized = true;
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
