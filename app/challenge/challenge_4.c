#include <assert.h>
#include <limits.h>
#include <sgx_eid.h>
#include <sgx_error.h>
#include <stdint.h>
#include <stdio.h>

#include "../defines.h"
#include "./challenges.h"
#include "enclave_u.h"

/**
 * The prime base of the polynomial, used for modular arithmetic.
 *
 * @see https://en.wikipedia.org/wiki/2,147,483,647
 */
static constexpr const uint32_t P = 2'147'483'647;
// we assume multiplication P doesn't overflow uint64_t
static_assert(P <= INT32_MAX);

[[gnu::const, nodiscard("pure function")]]
/**
 * Convert integer to range [0, P).
 */
static uint32_t toP(const int n) {
    static const int64_t Pi = (int64_t) P;
    const int64_t nn = (int64_t) n;
    return ((uint32_t) (nn % Pi + Pi)) % P;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Converts a value from the modular field [0, P) to its smallest signed integer representation.
 */
static int fromP(const uint32_t n) {
    if likely (n <= P / 2) {
        return (int) n;
    }
    return ((int) (n % P)) - ((int) P);
}

[[gnu::const, nodiscard("pure function"), gnu::hot]]
/**
 * Does `(a + b) % P` without overflowing or underflowing.
 */
static uint32_t addP(const uint32_t a, const uint32_t b) {
    const uint64_t aa = (uint64_t) a;
    const uint64_t bb = (uint64_t) b;
    return (uint32_t) ((aa + bb) % P);
}

[[gnu::const, nodiscard("pure function"), gnu::hot]]
/**
 * Does `(a - b) % P` without overflowing or underflowing.
 */
static uint32_t subP(const uint32_t a, const uint32_t b) {
    const uint64_t aa = (uint64_t) a;
    const uint64_t bb = (uint64_t) b % P;
    return (uint32_t) ((aa + P - bb) % P);
}

[[gnu::const, nodiscard("pure function"), gnu::hot]]
/**
 * Does `(a * b) % P` without overflowing or underflowing.
 */
static uint32_t mulP(const uint32_t a, const uint32_t b) {
    const uint64_t aa = (uint64_t) a;
    const uint64_t bb = (uint64_t) b;
    return (uint32_t) ((aa * bb) % P);
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Fast modular exponentiation `(a ** n) % P`.
 */
static uint32_t expP(const uint32_t a, uint32_t n) {
    uint32_t base = a;
    uint32_t result = 1;
    while (n > 0) {
        if likely (n % 2 != 0) {
            result = mulP(result, base);
        }
        base = mulP(base, base);
        n /= 2;
    }
    return result;
}

/**
 * Polynomial coeffiecients for `(a * x**2 + b * x + c) % p`.
 */
struct coefficients {
    /** First coefficient, order 2. */
    int a;
    /** Second coefficient, order 1. */
    int b;
    /** Last coefficient, order 0. */
    int c;
};

[[gnu::const, nodiscard("pure function")]]
/*
 * Quadratic interpolation in F_p
 *
 * We have three distinct points `(x1,y1)`, `(x2,y2)`, `(x3,y3)` in the prime field `F_p` with `p = 2_147_483_647`.
 * They must be distinct mod p or `D` collapses to 0 and the system is singular.
 *
 * The parabola y ≡ a·x² + b·x + c (mod p) is unique, so we solve the 3×3 Vandermonde system with Cramer's rule,
 * still mod p:
 *
 *     D  = (x1-x2)(x1-x3)(x2-x3)                         // determinant
 *     D⁻¹ = D^(p-2) mod p                               // Fermat inverse
 *
 *     Na =  x1(y3-y2) + x2(y1-y3) + x3(y2-y1)
 *     Nb =  x1²(y2-y3) + x2²(y3-y1) + x3²(y1-y2)
 *     Nc =  x1²(x2y3-x3y2) + x2²(x3y1-x1y3) + x3²(x1y2-x2y1)
 *
 *     a ≡ Na · D⁻¹  (mod p)
 *     b ≡ Nb · D⁻¹  (mod p)
 *     c ≡ Nc · D⁻¹  (mod p)
 *
 * Returned coefficients are canonicalised to signed ints via `fromP()`.
 */
static struct coefficients solve_polynomial_coefficients(
    const uint32_t x1,
    const uint32_t y1,
    const uint32_t x2,
    const uint32_t y2,
    const uint32_t x3,
    const uint32_t y3
) {
    // points must be distinct
    assume(x1 != x2);
    assume(x2 != x3);
    assume(x3 != x1);

    const uint32_t D = mulP(mulP(subP(x1, x2), subP(x1, x3)), subP(x2, x3));
    const uint32_t iD = expP(D, P - 2);

    // first coefficient
    const uint32_t Na = addP(addP(mulP(x1, subP(y3, y2)), mulP(x2, subP(y1, y3))), mulP(x3, subP(y2, y1)));
    const uint32_t a = mulP(Na, iD);

    // second coefficient
    uint32_t Nb = addP(
        addP(mulP(mulP(x1, x1), subP(y2, y3)), mulP(mulP(x2, x2), subP(y3, y1))),
        mulP(mulP(x3, x3), subP(y1, y2))
    );
    uint32_t b = mulP(Nb, iD);

    // last coefficient
    uint32_t Nc = addP(
        addP(
            mulP(mulP(x1, x1), subP(mulP(x2, y3), mulP(x3, y2))),
            mulP(mulP(x2, x2), subP(mulP(x3, y1), mulP(x1, y3)))
        ),
        mulP(mulP(x3, x3), subP(mulP(x1, y2), mulP(x2, y1)))
    );
    uint32_t c = mulP(Nc, iD);

    return (struct coefficients) {.a = fromP(a), .b = fromP(b), .c = fromP(c)};
}

/**
 * Challenge 4: Secret Polynomial
 * ------------------------------
 *
 * Evaluate the polynomial on `x = 10000`, `x = 22222` and `x = 303030`, then solve the linear equation to find the
 * coefficients for the secret polynomial. Only 3 calls to `ecall_polinomio_secreto` and 1 call to
 * `ecall_verificar_polinomio` are made.
 */
extern sgx_status_t challenge_4(sgx_enclave_id_t eid) {
    const int x[3] = {10'000, 22'222, 303'030};
    int y[3] = {INT_MIN, INT_MIN, INT_MIN};

    // collect some points for the linear solution
    for (size_t i = 0; i < 3; i++) {
        const sgx_status_t status = ecall_polinomio_secreto(eid, &(y[i]), x[i]);
        if unlikely (status != SGX_SUCCESS) {
            return status;
        }
#ifdef DEBUG
        printf("Challenge 4: x%zu = %d, y%zu = %d\n", i + 1, x[i], i + 1, y[i]);
#endif
    }

    const struct coefficients poly =
        solve_polynomial_coefficients(toP(x[0]), toP(y[0]), toP(x[1]), toP(y[1]), toP(x[2]), toP(y[2]));

#ifdef DEBUG
    printf("Challenge 4: a = %d, b = %d, c = %d\n", poly.a, poly.b, poly.c);
#endif

    int rv = 0;
    const sgx_status_t status = ecall_verificar_polinomio(eid, &rv, poly.a, poly.b, poly.c);
    if unlikely (status != SGX_SUCCESS) {
        return status;
    }

    if unlikely (rv == 0) {
        printf("Challenge 4: Coefficients not found\n");
        return SGX_ERROR_UNEXPECTED;
    }

    return SGX_SUCCESS;
}
