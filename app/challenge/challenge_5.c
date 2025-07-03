#define _GNU_SOURCE 1  // required for M_PI

#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <sgx_eid.h>
#include <sgx_error.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "../defines.h"
#include "./challenges.h"
#include "enclave_u.h"

/** Pre-defined number of rounds in each Rock, Paper, Scissors game. */
#define ROUNDS 20

/**
 * Answers for each round in Rock, Paper, Scissors game.
 *
 * These values will be returned by `ocall_pedra_papel_tesoura`.
 */
static uint8_t answers[ROUNDS] = {0};

/**
 * OCALL that will be invoked `ROUNDS` (20) times by the `ecall_pedra_papel_tesoura`. It receives the current round
 * number as its parameter (1 through `ROUNDS`). This function MUST return `0` (rock), `1` (paper), or `2` (scissors);
 * any other value makes the enclave abort immediately.
 *
 * TIP: use static variables if you need to keep state between calls.
 **/
extern unsigned int ocall_pedra_papel_tesoura(unsigned int round) {
    if unlikely (round < 1 || round > ROUNDS) {
        printf("Challenge 5: Invalid input round = %u\n", round);
        return UINT_MAX;
    }
    return answers[round - 1];
}

[[nodiscard("error must be checked"), gnu::nonnull(2), gnu::hot]]
/**
 * Runs `ecall_pedra_papel_tesoura` and validate its return value.
 *
 * Returns the number of wins for the current `answers`, or `UINT8_MAX` if a solution was found. In the case of errors,
 * `UINT8_MAX` is also returned to stop the solution and an error code is written to `status`.
 */
static uint8_t check_answers(const sgx_enclave_id_t eid, sgx_status_t *NONNULL status) {
    int wins = INT_MIN;

    sgx_status_t rstatus = ecall_pedra_papel_tesoura(eid, &wins);
    if unlikely (rstatus != SGX_SUCCESS || wins < 0 || wins >= UINT8_MAX) {
        *status = rstatus;
        return UINT8_MAX;
    } else if unlikely (wins < 0 || wins > ROUNDS) {
        printf("Challenge 5: Invalid ecall_pedra_papel_tesoura wins = %d\n", wins);
        *status = SGX_ERROR_UNEXPECTED;
        return UINT8_MAX;
    }

    return likely(wins != ROUNDS) ? (uint8_t) wins : UINT8_MAX;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Sergei Winitzki's approximation to inverse error function ($erf^{-1}$). Used as starting point for `erfinv`.
 *
 * @see https://www.scribd.com/document/82414963/Winitzki-Approximation-to-Error-Function
 */
static double erfinv_approx(const double x) {
    assume(isgreater(x, -1) && isless(x, 1));

    const double xa = fabs(x);
    const double lxa = log(1 - xa * xa);

    static const double a = (8 * (M_PI - 3)) / (3 * M_PI * (4 - M_PI));

    const double v = 2 / (M_PI * a) + (1.0 / 2) * lxa;
    const double y = -v + sqrt(v * v - (1.0 / a) * lxa);

    return copysign(y, x);
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Newton-Raphson's method for finding the inverse error function ($erf^{-1}$).
 *
 * @see https://en.wikipedia.org/wiki/Newton%27s_method
 */
static double erfinv(const double x) {
    if unlikely (islessequal(x, -1)) {
        return -INFINITY;
    } else if unlikely (isgreaterequal(x, 1)) {
        return +INFINITY;
    }

    static const size_t ITERATIONS = 6;
    const double M_SQRTPI = sqrt(M_PI);

    double y = erfinv_approx(x);
    for (size_t i = 0; i < ITERATIONS; i++) {
        const double f = erf(y) - x;
        const double fp = (2 / M_SQRTPI) * exp(-y * y);
        y -= f / fp;
    }
    return y;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * P-Quantile function of the Standard Normal Distribution.
 *
 * @see https://en.wikipedia.org/wiki/Quantile_function
 */
static double z(const double p) {
    return sqrt(2) * erfinv(2 * p - 1);
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Calculate `x**2`.
 */
static double square(double x) {
    return x * x;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Estimate sample size required for the given `confidence` and `power` when choosing the correct value for a position.
 * This is based on classical inference using two-sided tests.
 */
static size_t sample_size(
    /**
     * 1 - α, or the likelihood that a Type-I error does not occur.
     */
    const double confidence,  // NOLINT(bugprone-easily-swappable-parameters)
    /**
     * 1 - β, or the likelihood that a Type-II error does not occur.
     */
    const double power,  // NOLINT(bugprone-easily-swappable-parameters)
    /**
     * (Assumed) Standard deviation of the sample.
     */
    const double sigma,
    /**
     * Expected gap between the correct choice and competitors.
     */
    const double delta
) {
    // Bonferroni-safe significance
    const double alpha = (1 - confidence) / 2;
    const double z1a = z(1 - alpha);
    const double z1b = z(power);

    const double n = (2 * square(z1a + z1b) * square(sigma)) / square(delta);
    return (size_t) ceil(n);
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Estimate standard deviation for the games assuming binomial distribution for the sequence.
 */
static double sigma(const size_t n) {
    /** Winning probability for each position. */
    const double p = 1.0 / 3.0;

    return sqrt((double) n * (p * (1.0 - p)));
}

[[nodiscard("useless call otherwise")]]
/**
 * Generate an evenly distributed guess between `0` (rock), `1` (paper), or `2` (scissors).
 */
static uint8_t random_guess(void) {
    static const int THRESHOLD = RAND_MAX - RAND_MAX % 3;

    while (true) {
        const int value = rand();  // NOLINT(cert-msc30-c, concurrency-mt-unsafe)
        assume(value >= 0);

        if likely (value < THRESHOLD) {
            return (uint8_t) (value % 3);
        }
    }
}

[[gnu::hot]]
/**
 * Populate the last `ROUNDS - start` positions in `answers` with random guesses.
 */
static void generate_random_answers_from(const size_t start) {
    for (size_t i = start; i < ROUNDS; i++) {
        answers[i] = random_guess();
    }
}

[[nodiscard("error must be checked"), gnu::nonnull(2), gnu::hot]]
/**
 * Estimate the correct play for position `start` with 95% confidence.
 *
 * For each of the three possible values, `0` (rock), `1` (paper), or `2` (scissors), this function generates `n`
 * random sub-sequences after `start` and selects the value with most wins in total. The correct value is expected to
 * produce 1 more win on average than the other two possibilities, resulting in an expected `n` more wins on the
 * aggregate.
 *
 * The sample size `n` is estimated following a two-sided test of `ROUNDS - start` guesses with 1/3 win probability.
 * This value is at most `n = 94`, for `start = 0` and 5% significance value. In total, `3 * n` calls to
 * `ecall_pedra_papel_tesoura` are made.
 *
 * Returns the total number of wins for all checked `answers`, or `UINT32_MAX` if a solution was found. In the case of
 * errors, `UINT32_MAX` is also returned to stop the solution and an error code is written to `status`
 */
static uint32_t pick_position(const sgx_enclave_id_t eid, sgx_status_t *NONNULL status, const size_t start) {
    /** 5% chance of assuming a value is better when all are equal. */
    static const double CONFIDENCE = 0.95;
    /** 10% chance of not picking the best value when there's one. */
    static const double POWER = 0.90;
    /** Correct choice always scores, and wrong ones never does. So 1 score higher is expected. */
    static const double DELTA = 1;

    const size_t n = sample_size(CONFIDENCE, POWER, sigma(ROUNDS - start), DELTA);

    uint32_t wins[3] = {0, 0, 0};
    for (uint8_t d = 0; d < 3; d++) {
        answers[start] = d;
        for (size_t k = 0; k < n; k++) {
            generate_random_answers_from(start + 1);

            const uint8_t current_wins = check_answers(eid, status);
            if unlikely (current_wins == UINT8_MAX) {
                return UINT32_MAX;
            }
            wins[d] += current_wins;
        }
    }

    if (wins[0] >= wins[1] && wins[0] >= wins[2]) {
        answers[start] = 0;
    } else if (wins[1] >= wins[2]) {
        answers[start] = 1;
    } else {
        answers[start] = 2;
    }

    return wins[0] + wins[1] + wins[2];
}

/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Search for a winning rock, paper, scissors sequence using a mix of Depth Limited Search and Hill Climbing. For each
 * select position, all three values are tested in multiple different configurations, and the one with highest total
 * wins is selected. This is likely to be the correct result, because each correct position will yield more wins then
 * the other two on average, assuming the remaining rounds are indistinguishable from random (i.e. it's a PRNG). In
 * total, up to 2979 calls to `ecall_pedra_papel_tesoura` are made.
 */
extern sgx_status_t challenge_5(sgx_enclave_id_t eid) {
    /** Randomly generated fixed seed (`openssl rand -hex 4`). */
    static const unsigned SEED = 0x23'20'c5'da;
    srand(SEED);  // NOLINT(cert-msc32-c)

    for (size_t start = 0; start < ROUNDS; start++) {
        sgx_status_t status = SGX_SUCCESS;

        const uint32_t total_wins = pick_position(eid, &status, start);
        if likely (total_wins == UINT32_MAX) {
            return status;
        }

#ifdef DEBUG
        printf("Challenge 5: answers[%zu] = %" PRIu8 ", total wins = %" PRIu32 "\n", start, answers[start], total_wins);
#endif
    }

    printf("Challenge 5: Winning sequence not found\n");
    return SGX_ERROR_UNEXPECTED;
}
