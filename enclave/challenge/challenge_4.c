#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "../enclave.h"
#include "defines.h"
#include "enclave_t.h"

/**
 * The prime base of the polynomial, used for modular arithmetic.
 */
static constexpr int64_t P = 2'147'483'647;

// The coefficients, promoted to `int64_t` to avoid overflow during multiplication.
static int64_t A = INT_MIN;
static int64_t B = INT_MIN;
static int64_t C = INT_MIN;

// Must be checked before using the coefficients.
static bool initialized = false;

[[nodiscard("pure function"), gnu::const, gnu::cold]]
/**
 * Generate random polynomial coefficients and populates `A`, `B` and `C` with them. Returns `false` on errors.
 */
static bool generate_coefficients(void) {
    drbg_ctr128_t rng = drbg_seeded_init(4);

    while (true) {
        static constexpr int MIN_VALUE = -100'000'000;
        static constexpr int MAX_VALUE = +100'000'000;
        static constexpr uint64_t WIDTH = (uint64_t) (MAX_VALUE - MIN_VALUE - 1);
        static_assert(WIDTH <= INT_MAX);

        uint128_t ua = UINT128_MAX;
        const bool ok1 = drbg_rand_bounded(&rng, &ua, WIDTH);
        uint128_t ub = UINT128_MAX;
        const bool ok2 = drbg_rand_bounded(&rng, &ub, WIDTH);
        uint128_t uc = UINT128_MAX;
        const bool ok3 = drbg_rand_bounded(&rng, &uc, WIDTH);
        if unlikely (!ok1 || !ok2 || !ok3) {
            return false;
        }

        assume(ua < WIDTH && ub < WIDTH && uc < WIDTH);
        const int ca = (int) (ua % WIDTH) + MIN_VALUE;
        const int cb = (int) (ub % WIDTH) + MIN_VALUE;
        const int cc = (int) (uc % WIDTH) + MIN_VALUE;

        static_assert(3 * MIN_VALUE >= INT_MIN);
        static_assert(3 * MAX_VALUE <= INT_MAX);
        if unlikely (ca + cb + cc <= MIN_VALUE || ca + cb + cc >= MAX_VALUE) {
            continue;
        }

        A = (int64_t) ca;
        B = (int64_t) cb;
        C = (int64_t) cc;
        return initialized = true;
    }
}

/**
 * Challenge 4: Secret Polynomial
 * ------------------------------
 *
 * this function returns ((x*x*a) + (x*b) + c) % 2147483647
 * Assumption: -10^8 < (a + b + c) < 10^8
 *
 * Use it to help you discover the polynomial before calling `ecall_verificar_polinomio`.
 * NOTE: this ECALL aborts if you pass zero.
 *
 * HINT: the prime 2147483647 is irrelevant except when you supply *       a very large x.
 */
extern int ecall_polinomio_secreto(const int x) {
    if unlikely (!initialized) {
        const bool ok = generate_coefficients();
        if unlikely (!ok) {
            abort();
        }
    }

    if unlikely (x == 0) {
#ifdef DEBUG
        printf("[DEBUG] ecall_polinomio_secreto: invalid x=%d\n", x);
#endif
        abort();
    }

    static_assert(P <= INT_MAX);
    const int64_t X = (int64_t) x;
    return (int) ((((((A * X) % P + B) % P) * x) % P + C) % P);
}

/**
 * Challenge 4: Secret Polynomial
 * ------------------------------
 *
 * Verify the polynomial coefficients.
 *
 * HINT: -10^8 < (a + b + c) < 10^8.
 * HINT: the function is deliberately hard to brute-force.
 */
extern int ecall_verificar_polinomio(int a, int b, int c) {
    if unlikely (!initialized) {
        const bool ok = generate_coefficients();
        if unlikely (!ok) {
            abort();
        }
    }

    if likely (a != A || b != B || c != C) {
        return 0;
    }

    printf("\n%s\n", SEPARATOR);
    printf("[ENCLAVE] DESAFIO 4 CONCLUIDO!! os polinomios são: A=%" PRIi64 ", B=%" PRIi64 ", C=%" PRIi64 "\n", A, B, C);
    printf("%s\n", SEPARATOR);
    return 1;
}
