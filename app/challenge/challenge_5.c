#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <math.h>
#include <pcg_basic.h>
#include <sgx_eid.h>
#include <sgx_error.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "./challenges.h"
#include "defines.h"
#include "enclave_u.h"

[[gnu::const, nodiscard("pure function")]]
/**
 * P-Quantile function of the Standard Normal Distribution. Acklam's approximation.
 *
 * @see https://en.wikipedia.org/wiki/Quantile_function
 * @see https://web.archive.org/web/20150910044729/http://home.online.no/~pjacklam/notes/invnorm/
 */
static double invnorm(const double p) {
    if unlikely (islessequal(p, 0)) {
        return -INFINITY;
    } else if unlikely (isgreaterequal(p, 1)) {
        return +INFINITY;
    } else if unlikely (isnan(p)) {
        return NAN;
    }
    assume(isless(0, p) && isless(p, 1));

    // coefficients in rational approximations
    static constexpr double a1 = -3.969683028665376e+01;
    static constexpr double a2 = 2.209460984245205e+02;
    static constexpr double a3 = -2.759285104469687e+02;
    static constexpr double a4 = 1.383577518672690e+02;
    static constexpr double a5 = -3.066479806614716e+01;
    static constexpr double a6 = 2.506628277459239e+00;

    static constexpr double b1 = -5.447609879822406e+01;
    static constexpr double b2 = 1.615858368580409e+02;
    static constexpr double b3 = -1.556989798598866e+02;
    static constexpr double b4 = 6.680131188771972e+01;
    static constexpr double b5 = -1.328068155288572e+01;

    static constexpr double c1 = -7.784894002430293e-03;
    static constexpr double c2 = -3.223964580411365e-01;
    static constexpr double c3 = -2.400758277161838e+00;
    static constexpr double c4 = -2.549732539343734e+00;
    static constexpr double c5 = 4.374664141464968e+00;
    static constexpr double c6 = 2.938163982698783e+00;

    static constexpr double d1 = 7.784695709041462e-03;
    static constexpr double d2 = 3.224671290700398e-01;
    static constexpr double d3 = 2.445134137142996e+00;
    static constexpr double d4 = 3.754408661907416e+00;

    // break-points
    static constexpr double p_low = 0.02425;
    static constexpr double p_high = 1 - p_low;

    double x = NAN;
    // rational approximation for lower region
    if (isless(0, p) && isless(p, p_low)) {
        const double q = sqrt(-2.0 * log(p));
        x = (((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1);
    }
    // rational approximation for central region
    else if (islessequal(p_low, p) && islessequal(p, p_high)) {
        const double q = p - 0.5;
        const double r = q * q;
        x = (((((a1 * r + a2) * r + a3) * r + a4) * r + a5) * r + a6) * q
            / (((((b1 * r + b2) * r + b3) * r + b4) * r + b5) * r + 1);
    }
    // rational approximation for upper region
    else if (isless(p_high, p) && isless(p, 1)) {
        const double q = sqrt(-2.0 * log(1.0 - p));
        x = -(((((c1 * q + c2) * q + c3) * q + c4) * q + c5) * q + c6) / ((((d1 * q + d2) * q + d3) * q + d4) * q + 1);
    }

    assume(isless(0, p) && isless(p, 1));
    static constexpr double PI = 3.14159265358979323846;
    // The relative error of the approximation has
    // absolute value less than 1.15 × 10^−9.  One iteration of
    // Halley's rational method (third order) gives full machine precision.
    const double e = 0.5 * erfc(-x / sqrt(2)) - p;
    const double u = e * sqrt(2 * PI) * exp((x * x) / 2.0);
    x = x - u / (1 + x * u / 2);

    return x;
}

/** Pre-defined number of rounds in each Rock, Paper, Scissors game. */
static constexpr size_t ROUNDS = 20;

/**
 * Answers for each round in Rock, Paper, Scissors game.
 *
 * These values will be returned by `ocall_pedra_papel_tesoura`.
 */
static uint8_t answers[ROUNDS] = {0};

/**
 * Number of successful calls to `ecall_pedra_papel_tesoura`.
 */
static size_t games_played = 0;

/**
 * OCALL that will be invoked `ROUNDS` (20) times by the `ecall_pedra_papel_tesoura`. It receives the current round
 * number as its parameter (1 through `ROUNDS`). This function MUST return `0` (rock), `1` (paper), or `2` (scissors);
 * any other value makes the enclave abort immediately.
 *
 * TIP: use static variables if you need to keep state between calls.
 **/
unsigned int ocall_pedra_papel_tesoura(unsigned int round) {
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
    static_assert(ROUNDS <= INT_MAX);
    int wins = INT_MIN;

    sgx_status_t rstatus = ecall_pedra_papel_tesoura(eid, &wins);
    if unlikely (rstatus != SGX_SUCCESS) {
        *status = rstatus;
        return UINT8_MAX;
    } else if unlikely (wins < 0 || wins > (int) ROUNDS) {
        printf("Challenge 5: Invalid ecall_pedra_papel_tesoura wins = %d\n", wins);
        *status = SGX_ERROR_UNEXPECTED;
        return UINT8_MAX;
    } else {
        *status = SGX_SUCCESS;
    }

    games_played++;
    return likely(wins != ROUNDS) ? (uint8_t) wins : UINT8_MAX;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Initialize a PRNG state with pre-defined seeds.
 */
static pcg32_random_t seed_random_state(void) {
    /** Randomly generated fixed seed (`openssl rand -hex 16`). */
    constexpr uint64_t SEED[2] = {0x4b'3b'71'75'60'aa'68'8b, 0x9b'13'2b'73'f3'91'a8'a0};

    pcg32_random_t state = {0};
    pcg32_srandom_r(&state, SEED[0], SEED[1]);
    return state;
}

[[nodiscard("useless call otherwise"), gnu::hot]]
/**
 * Generate an evenly distributed guess between `0` (rock), `1` (paper), or `2` (scissors) via rejection sampling.
 */
static uint8_t random_guess(pcg32_random_t *NONNULL random_state) {
    return (uint8_t) pcg32_boundedrand_r(random_state, 3) % 3;
}

[[gnu::hot]]
/**
 * Populate the last `ROUNDS - start` positions in `answers` with random guesses.
 */
static void generate_random_answers_from(pcg32_random_t *NONNULL random_state, const size_t start) {
    for (size_t i = start; i < ROUNDS; i++) {
        answers[i] = random_guess(random_state);
    }
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
 * Estimate sample size required for the given `confidence` and `power` based on classical inference using
 * two-sided tests.
 */
static double two_sided_sample_size(
    /**
     * 1 - α, or the likelihood that a Type-I error does not occur.
     */
    const double confidence,
    /**
     * 1 - β, or the likelihood that a Type-II error does not occur.
     */
    const double power,
    /**
     * (Assumed) Standard deviation of the sample.
     */
    const double sigma,
    /**
     * Expected gap between the correct choice and competitors.
     */
    const double delta
) {
    // Bonferroni-safe significance when split over three choices
    const double alpha = (1 - confidence) / 3;
    const double z1a = invnorm(1 - alpha);
    const double z1b = invnorm(power);

    return (2 * square(z1a + z1b) * square(sigma)) / square(delta);
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Estimate sample size required for the given `confidence` and `power` when choosing the correct value for a position.
 */
static size_t sample_size(const double confidence, const double power, const size_t start) {
    /** Correct choice always scores, and drawing or losing never does. So 1 score higher is expected. */
    static constexpr double DELTA = 1;
    /** The probability of winning in a single round. */
    static constexpr double P = 1.0 / 3.0;
    /* Standard deviation for the Bernoulli distribution. */
    const double sigma = sqrt(P * (1 - P));

    // sample size multiplier
    const double sn = two_sided_sample_size(confidence, power, sigma, DELTA);
    // we abuse the fact that Var[n bernoulli] = n Var[bernoulli]
    const size_t n = (size_t) ceil((double) (ROUNDS - start) * sn);
    // sample size can't be zero
    return unlikely(n <= 0) ? 1 : n;
}

[[nodiscard("error must be checked"), gnu::nonnull(2), gnu::hot]]
/**
 * Estimate the correct play for position `position` with 80% confidence.
 *
 * For each of the three possible values, `0` (rock), `1` (paper), or `2` (scissors), this function generates `n`
 * random sub-sequences after `position` and selects the value with most wins in total. The correct value is expected to
 * produce 1 more win on average than the other two possibilities, resulting in an expected `n` more wins on the
 * aggregate.
 *
 * The sample size `n` is estimated following a two-sided test of `ROUNDS - position - 1` guesses with 1/3 win
 * probability. This value is at most `n = 35`, for `position = 0` and 20% significance value. In total, `3 * n`
 * calls to `ecall_pedra_papel_tesoura` are made.
 *
 * Returns the total number of wins for all checked `answers`, or `UINT32_MAX` if a solution was found. In the case of
 * errors, `UINT32_MAX` is also returned to stop the solution and an error code is written to `status`
 */
static uint32_t pick_position(
    const sgx_enclave_id_t eid,
    sgx_status_t *NONNULL status,
    pcg32_random_t *NONNULL random_state,
    const size_t position
) {
    /** 20% chance of assuming a value is better when all are equal. */
    static constexpr double CONFIDENCE = 0.80;
    /** 30% chance of not picking the best value when there's one. */
    static constexpr double POWER = 0.70;

    const size_t n = sample_size(CONFIDENCE, POWER, position + 1);

    uint32_t wins[3] = {0, 0, 0};
    for (uint8_t d = 0; d < 3; d++) {
        answers[position] = d;
        for (size_t k = 0; k < n; k++) {
            generate_random_answers_from(random_state, position + 1);

            const uint8_t current_wins = check_answers(eid, status);
            if unlikely (current_wins == UINT8_MAX) {
                return UINT32_MAX;
            }
            wins[d] += current_wins;
        }
    }

    if (wins[0] >= wins[1] && wins[0] >= wins[2]) {
        answers[position] = 0;
    } else if (wins[1] >= wins[2]) {
        answers[position] = 1;
    } else {
        answers[position] = 2;
    }

    return wins[0] + wins[1] + wins[2];
}

[[nodiscard("error must be checked")]]
/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Search for a winning rock, paper, scissors sequence using randomly generated guesses and statistical inference.
 *
 * For each select position, all three values are tested in multiple different configurations, and the one with highest
 * total wins is selected. This is likely to be the correct result, because each correct position will yield more wins
 * then the other two on average, assuming the remaining rounds are indistinguishable from random (i.e. it's a PRNG).
 *
 * In total, up to 1068 calls to `ecall_pedra_papel_tesoura` are made:
 *
 *     Σ_{i=0}^19 3 sample_size(i) = 3 Σ_{i=0}^19 ⌈(20-i) × 2(z_{1-α}²+z_{1-β}²) σ²/Δ²⌉
 *                                 = 3 Σ_{i=0}^19 ⌈(20-i) × 2(z_{0.8}²+z_{0.7}² 2/9⌉
 *                                 = 3 × (35 + 33 + ... + 2 + 1) = 1068
 *
 * This solution is stochastic and has a 45.89% chance of finding the correct sequence in 20 rounds. See
 * `docs/probabilities.py` for more details on the probabilities. For my enclave, the solution was found after
 * 1064 games.
 */
static sgx_status_t challenge_5_stochastic(const sgx_enclave_id_t eid) {
    pcg32_random_t random_state = seed_random_state();

    for (size_t position = 0; position < ROUNDS; position++) {
        sgx_status_t status = SGX_SUCCESS;

        const uint32_t total_wins = pick_position(eid, &status, &random_state, position);
        if likely (total_wins == UINT32_MAX) {
            return status;
        }

#ifdef DEBUG
        printf(
            "Challenge 5: answers[%zu] = %" PRIu8 ", total wins = %" PRIu32 "\n",
            position,
            answers[position],
            total_wins
        );
#endif
    }

    // solution not found
    return SGX_ERROR_UNEXPECTED;
}

[[nodiscard("error must be checked")]]
/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Uses dynamic programming to find the largest prefix with the correct number of wins. At each iteration, the prefix
 * length is refined to how many wins the current configuration gets, then the next configuration is tested.
 *
 * This implementation has an upper bound of `2**n - 2` calls to `ecall_pedra_papel_tesoura`, so 1_048_574 for
 * `n = 20`. It should be much better on average, though, assuming a pseudo-random sequence is used. For my enclave,
 * the solution was found after 2807 games.
 */
static sgx_status_t challenge_5_exact(const sgx_enclave_id_t eid) {
    memset(answers, 0, ROUNDS * sizeof(uint8_t));

    while (true) {
        sgx_status_t status = SGX_SUCCESS;
        const uint8_t wins = check_answers(eid, &status);
        if unlikely (wins == UINT8_MAX) {
            return status;
        }
        assume(wins < ROUNDS);

        // we need all positions to be correct, but since we got `wins < ROUNDS`,
        // we assume the first `wins` positions are correct, so we update the next position
        uint8_t i = wins + 1;
        // if the next position is at maximum (i.e. we tried all values), we reduce the prefix length
        while (likely(i > 0) && unlikely(answers[i - 1] >= 2)) {
            i--;
        }

        // no prefix length matched
        if unlikely (i == 0) {
            // solution not found
            return SGX_ERROR_UNEXPECTED;
        }

        // when we finally find a prefix with next position open for increment,
        // we update that and reset all other positions to zero
        answers[i - 1] = (uint8_t) ((answers[i - 1] + 1) % 3);
        memset(answers + i, 0, (ROUNDS - i) * sizeof(uint8_t));
    }
}

/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Run an stochastic solution first, then the exact solution as fallback. The stochastic implementation is not
 * guaranteed to find a solution, but it runs faster. At 2800 calls (the same number as exact implementation), the
 * statistical one has 98% probability of finding the solution. Additionally, the stochastic solution allows
 * for extreme parallelization.
 */
sgx_status_t challenge_5(sgx_enclave_id_t eid) {
    games_played = 0;
    sgx_status_t status = challenge_5_stochastic(eid);
    const size_t stochastic_games = games_played;

    if likely (status == SGX_SUCCESS) {
#ifdef DEBUG
        printf("Challenge 5: Stochastic solution successful after %zu games.\n", stochastic_games);
#else
        return SGX_SUCCESS;
#endif
    }

    games_played = 0;
    status = challenge_5_exact(eid);
    const size_t exact_games = games_played;

    if likely (status == SGX_SUCCESS) {
#ifdef DEBUG
        printf("Challenge 5: Exact solution successful after %zu games.\n", exact_games);
#endif
        return SGX_SUCCESS;
    }

    printf("Challenge 5: Winning sequence not found after %zu games.\n", stochastic_games + exact_games);
    return status;
}
