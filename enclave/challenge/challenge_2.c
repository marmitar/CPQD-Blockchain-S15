#include <stdio.h>

#include "../enclave.h"
#include "defines.h"
#include "enclave_config.h"
#include "enclave_t.h"

/** Minimum value for the password (inclusive). */
static constexpr unsigned MIN_PASSWORD = 0;
/** Maximum value for the password (inclusive). */
static constexpr unsigned MAX_PASSWORD = 99'999;

/** Randomly generated fixed password (`openssl rand -hex 8`). */
static constexpr unsigned PASSWORD = CHALLENGE_2_PASSWORD % MAX_PASSWORD;

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
    static_assert(MIN_PASSWORD <= PASSWORD && PASSWORD <= MAX_PASSWORD);

    if likely (senha != PASSWORD) {
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
        PASSWORD
    );

    return 0;
}
