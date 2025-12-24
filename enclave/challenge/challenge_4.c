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

/**
 * Smallest possible value for the sum of all coefficients (inclusive).
 *
 * NOTE: each coefficient is also bounded by this value for faster pseudo-random generation.
 */
static constexpr int MIN_VALUE = -100'000'000;
/**
 * Largest possible value for the sum of all coefficients (inclusive).
 *
 * NOTE: each coefficient is also bounded by this value for faster pseudo-random generation.
 */
static constexpr int MAX_VALUE = +100'000'000;

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/** Promote to `int64_t` to avoid overflow. */
static inline int64_t i64(int x) {
    return (int64_t) x;
}

/** Check if a value is in the define `MIN_VALUE` to `MAX_VALUE` range (both inclusive). */
#define IN_RANGE(value) likely(MIN_VALUE <= (value) && (value) <= MAX_VALUE)
static_assert(INT_MIN < MIN_VALUE);
static_assert(MIN_VALUE < MAX_VALUE);
static_assert(MAX_VALUE < INT_MAX);

/**
 * The coefficients, promoted to `int64_t` to avoid overflow during multiplication.
 */
typedef struct coefficients {
    int64_t a;
    int64_t b;
    int64_t c;
} coefficients_t;

/**
 * Check if a given coefficient set is in the expected range.
 */
#define IS_VALID(poly) IN_RANGE((poly).a + (poly).b + (poly).c)

/**
 * An invalid set of coefficients used for initialization.
 */
static constexpr coefficients_t UNINITIALIZED_COEFFICIENTS = {
    .a = INT_MIN,
    .b = INT_MIN,
    .c = INT_MIN,
};
static_assert(!IS_VALID(UNINITIALIZED_COEFFICIENTS));

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/**
 * Generate pseudo-random polynomial coefficients from fixed seed. Returns `UNINITIALIZED_COEFFICIENTS` on errors.
 */
static coefficients_t generate_coefficients(void) {
    drbg_ctr128_t rng = drbg_seeded_init(4);

    while (true) {
        static constexpr uint64_t FULL_WIDTH = (uint64_t) (MAX_VALUE - MIN_VALUE) + 1;
        static_assert(FULL_WIDTH <= INT_MAX);

        uint128_t ua = UINT128_MAX;
        const bool ok1 = drbg_rand_bounded(&rng, &ua, FULL_WIDTH);

        uint128_t ub = UINT128_MAX;
        const bool ok2 = drbg_rand_bounded(&rng, &ub, FULL_WIDTH);

        uint128_t uc = UINT128_MAX;
        const bool ok3 = drbg_rand_bounded(&rng, &uc, FULL_WIDTH);
        if unlikely (!ok1 || !ok2 || !ok3) {
            return UNINITIALIZED_COEFFICIENTS;
        }

        assume(ua < FULL_WIDTH && ub < FULL_WIDTH && uc < FULL_WIDTH);
        const int ca = (int) (ua % FULL_WIDTH) + MIN_VALUE;
        const int cb = (int) (ub % FULL_WIDTH) + MIN_VALUE;
        const int cc = (int) (uc % FULL_WIDTH) + MIN_VALUE;

        static_assert(3 * MIN_VALUE >= INT_MIN);
        static_assert(3 * MAX_VALUE <= INT_MAX);
        if unlikely (!IN_RANGE(ca + cb + cc)) {
            continue;
        }

        return (coefficients_t) {
            .a = i64(ca),
            .b = i64(cb),
            .c = i64(cc),
        };
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
int ecall_polinomio_secreto(const int x) {
    const coefficients_t poly = generate_coefficients();
    if unlikely (!IS_VALID(poly)) {
#ifdef DEBUG
        printf("[DEBUG] ecall_polinomio_secreto: failed to generate coefficients\n");
#endif
        abort();
    }

    if unlikely (x == 0) {
#ifdef DEBUG
        printf("[DEBUG] ecall_polinomio_secreto: invalid x=%d\n", x);
#endif
        abort();
    }

    static_assert(P <= INT_MAX);
    return (int) ((((((poly.a * i64(x)) % P + poly.b) % P) * i64(x)) % P + poly.c) % P);
}

[[nodiscard("error must be checked"), gnu::leaf, gnu::nothrow]]
/**
 * Challenge 4: Secret Polynomial
 * ------------------------------
 *
 * Verify the polynomial coefficients.
 *
 * HINT: -10^8 < (a + b + c) < 10^8.
 * HINT: the function is deliberately hard to brute-force.
 */
int ecall_verificar_polinomio(int a, int b, int c) {
    const coefficients_t poly = generate_coefficients();
    if unlikely (!IS_VALID(poly)) {
#ifdef DEBUG
        printf("[DEBUG] ecall_polinomio_secreto: failed to generate coefficients\n");
#endif
        abort();
    }

    if likely (i64(a) != poly.a || i64(b) != poly.b || i64(c) != poly.c) {
        return (int) false;
    }

    printf("\n%s\n", SEPARATOR);
    printf(
        "[ENCLAVE] DESAFIO 4 CONCLUIDO!! os polinomios são: A=%" PRIi64 ", B=%" PRIi64 ", C=%" PRIi64 "\n",
        poly.a,
        poly.b,
        poly.c
    );
    printf("%s\n", SEPARATOR);
    return (int) true;
}
