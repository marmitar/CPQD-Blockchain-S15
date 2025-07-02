#include <inttypes.h>
#include <limits.h>
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

[[nodiscard("error must be checked"), gnu::nonnull(2)]]
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

/**
 * Function to initialize the `answers` array (or part of it). It receives the current `position` being initialized
 * and the the `start` position for the current initialization process, and must return `0` (rock), `1` (paper), or
 * `2` (scissors) to be used and `answers` for that `position`.
 *
 * This function is used in the `greedy` hill climbing solution as a starting point, and is extremely important for
 * finding a good local maximum of high wins.
 */
typedef uint8_t (*init_function_t)(uint8_t position, uint8_t start);

[[nodiscard("error must be checked"), gnu::nonnull(2, 4)]]
/**
 * (Partial) Greedy Hill Climbing solution
 * ---------------------------------------
 *
 * This algorithm tries to find the solution for the Rock, Paper, Scissors by only testing one position at a time.
 * For each position, it tests the values `0` (rock), `1` (paper) and `2` (scissors), then sets on the one with
 * highest wins. This solution can be applied partially to subrange of the array, but choosing the correct `start`
 * index. The `init` function is used to set the starting point for hill climbing, see `init_function_t`. The greedy
 * seach starts from the end of the array.
 *
 * This implementation is considerably faster, but it's very unlikely to reach the global maximum (i.e. the actual
 * solution) by itself, but can be used to approximate how correct the unmodified positions (from `0` to `start`) are.
 * The `ecall_pedra_papel_tesoura` is called exatcly `2 * (ROUNDS - start) + 1` times. For example, at `start = 0`,
 * `ecall_pedra_papel_tesoura` will be called 41 times.
 *
 * Returns the total number of wins for all checked `answers`, or `UINT32_MAX` if a solution was found. In the case of
 * errors, `UINT32_MAX` is also returned to stop the solution and an error code is written to `status`.
 */
static uint32_t greedy(
    const sgx_enclave_id_t eid,
    sgx_status_t *NONNULL status,
    const uint8_t start,
    const NONNULL init_function_t init
) {
    assume(start <= 2 * ROUNDS);
    // we must initialize the positions, so previous values don't interfere with the results
    for (uint8_t i = start; i < ROUNDS; i++) {
        answers[i] = init(i, start);
    }

    // number of wins when: answers[i] = (init(i, start) + 0) % 3
    uint8_t wins0 = check_answers(eid, status);
    if unlikely (wins0 == UINT8_MAX) {
        return UINT32_MAX;
    }

    uint32_t total_wins = wins0;
    // we start from the end, so there's less interference of the pseudo-random Rock, Paper, Scissors sequence
    for (uint8_t i = ROUNDS; i > start; i--) {
        const uint8_t vi = answers[i - 1];

        // number of wins when: answers[i] = (init(i, start) + 1) % 3
        answers[i - 1] = (vi + 1) % 3;
        const uint8_t wins1 = check_answers(eid, status);
        if unlikely (wins1 == UINT8_MAX) {
            return UINT32_MAX;
        }

        // number of wins when: answers[i] = (init(i, start) + 2) % 3
        answers[i - 1] = (vi + 2) % 3;
        const uint8_t wins2 = check_answers(eid, status);
        if unlikely (wins2 == UINT8_MAX) {
            return UINT32_MAX;
        }

        // select the highest one, and save to `wins0`
        if (wins0 >= wins1 && wins0 >= wins2) {
            answers[i - 1] = (vi + 0) % 3;
            // SKIP: wins0 = wins0;
        } else if (wins1 > wins2) {
            answers[i - 1] = (vi + 1) % 3;
            wins0 = wins1;
        } else {
            // SKIP: answers[i-1] = (vi + 2) % 3;
            wins0 = wins2;
        }

        // account all checked results to the total wins
        total_wins += wins1;
        total_wins += wins2;
    }

    return total_wins;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * A `init_function_t` that sets all values to zero.
 */
static uint8_t init_zero(const uint8_t i, const uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    (void) i;
    (void) s;
    return 0;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * A `init_function_t` that sets all values to `i + s`.
 */
static uint8_t init_add(const uint8_t i, const uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i + s) % 3;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * A `init_function_t` that sets all values to `i * s` with `+ 1` for more mixing.
 */
static uint8_t init_mul(const uint8_t i, const uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i * s + 1) % 3;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * A `init_function_t` that sets all values to `i**2 + s**2`.
 */
static uint8_t init_square(const uint8_t i, const uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i * i + s * s) % 3;
}

[[nodiscard("error must be checked"), gnu::nonnull(2)]]
/**
 * Limited Depth-First Search solution
 * -----------------------------------
 *
 * This implementation tries to apply the DFS to find the correct `answers`. To avoid an exponential bloww up, the
 * search is limited to `depth` recursive calls, so `depth` positions set by DFS. The remaining positions are searched
 * using `greedy` hill climbing from different positions.
 *
 * The idea here is that `limited_dfs` selects a good value `answers[start]` by changing its value between `0` (rock),
 * `1` (paper) and `2` (scissors) and checking multiple distinct configurations for the other positions. The correct
 * value should, on average, result in higher wins.
 *
 * This implementation is possibly exponential, but should be able to reach the global maximum (i.e. the actual
 * solution) if unbounded. Nevertheless, the bounded implementation can abuse statistics to get a very good guess for
 * each position (usually the correct one). This function calls `ecall_pedra_papel_tesoura` a total of
 * `3**depth * 4 * (2 * (ROUNDS - (start + depth)) + 1)` times. For example, at `start = 0` and `depth = 2`, the
 * `ecall_pedra_papel_tesoura` will be called 1332 times.
 *
 * Returns the total number of wins for all checked `answers`, or `UINT32_MAX` if a solution was found. In the case of
 * errors, `UINT32_MAX` is also returned to stop the solution and an error code is written to `status`.
 */
static uint32_t limited_dfs(
    const sgx_enclave_id_t eid,
    sgx_status_t *NONNULL status,
    const uint8_t start,
    const uint8_t depth
) {
    if likely (depth == 0) {
        const NONNULL init_function_t init[] = {init_zero, init_mul, init_add, init_square};

        uint32_t total_wins = 0;
        for (unsigned i = 0; i < sizeof(init) / sizeof(init[0]); i++) {
            const uint32_t wins = greedy(eid, status, start, init[i]);
            if unlikely (wins == UINT32_MAX) {
                return UINT32_MAX;
            }
            total_wins += wins;
        }
        return total_wins;
    } else if (start >= ROUNDS) {
        return 0;
    }

    uint32_t wins[3] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
    for (uint8_t d = 0; d <= 2; d++) {
        answers[start] = d;
        wins[d] = limited_dfs(eid, status, start + 1, depth - 1);
        if unlikely (wins[d] == UINT32_MAX) {
            return UINT32_MAX;
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
 * the other two on average, assuming the remaining rounds are indistinguishable from random (i.e. it's a PRGN). In
 * total, up to 13_032 calls to `ecall_pedra_papel_tesoura` are made.
 */
extern sgx_status_t challenge_5(sgx_enclave_id_t eid) {
    for (uint8_t start = 0; start < ROUNDS; start++) {
        sgx_status_t status = SGX_SUCCESS;

        const size_t total_wins = limited_dfs(eid, &status, start, 2);
        if likely (total_wins == UINT32_MAX) {
            return status;
        }

#ifdef DEBUG
        printf("Challenge 5: answers[%" PRIu8 "] = %" PRIu8 "\n", start, answers[start]);
#endif
    }

    printf("Challenge 5: Winning sequence not found\n");
    return SGX_ERROR_UNEXPECTED;
}
