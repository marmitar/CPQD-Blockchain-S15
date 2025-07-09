#include <assert.h>
#include <limits.h>
#include <stdio.h>

#include "../enclave.h"
#include "defines.h"
#include "enclave_t.h"

/** Pre-defined number of rounds in each Rock, Paper, Scissors game. */
static constexpr size_t ROUNDS = 20;

[[nodiscard("generated value"), gnu::nonnull(1)]]
/**
 * Generate a random play: `0` (rock), `1` (paper), or `2` (scissors). Returns `UINT8_MAX` on errors.
 */
static uint8_t random_play(drbg_ctr128_t *NONNULL rng) {
    uint128_t value = UINT128_MAX;
    const bool ok = drbg_rand_bounded(rng, &value, 3);
    if unlikely (!ok) {
        return UINT8_MAX;
    }

    assume(value < 3);
    return (uint8_t) (value % 3);
}

[[nodiscard("do not throw away user calls")]]
/**
 * Call app for answer on a specific round.
 */
static uint8_t ocall_play(uint8_t round) {
    assume(0 < round && round <= ROUNDS);

    unsigned play = UINT_MAX;
    const sgx_status_t status = ocall_pedra_papel_tesoura(&play, round);
    if unlikely (status != SGX_SUCCESS) {
        printf("[ENCLAVE] ocall_pedra_papel_tesoura failed: status=0x%04x\n", status);
        return UINT8_MAX;
    }

    if unlikely (play >= 3) {
#ifdef DEBUG
        printf("[DEBUG] ocall_pedra_papel_tesoura: invalid answer=%u\n", play);
#endif
        return UINT8_MAX;
    }

    return (uint8_t) (play % 3);
}

typedef enum [[gnu::packed]] round_result {
    DRAW = 0,
    WIN = 1,
    LOSE = 2,
} round_result_t;

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/**
 * Check the result of this round for the app.
 */
static round_result_t result(uint8_t enclave_play, uint8_t app_play) {
    assume(enclave_play < 3);
    assume(app_play < 3);

    const uint8_t difference = (uint8_t) ((app_play + 3 - enclave_play) % 3);
    assume(difference < 3);
    switch (difference) {
        case 0:
            return DRAW;
        case 1:
            return WIN;
        case 2:
            return LOSE;
        default:
            return (round_result_t) difference;
    }
}

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/**
 * Display a single play for debug output.
 */
static char display_play(const uint8_t play) {
    assume(play < 3);
    return (char) ('0' + play % 3);
}

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/**
 * Display a single result for debug output.
 */
static char display_result(const round_result_t result) {
    switch (result) {
        case DRAW:
            return 'E';
        case WIN:
            return 'V';
        case LOSE:
            return 'D';
        default:
            return '?';
    }
}

/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Play 20 rounds of rock-paper-scissors against the enclave. You must win all 20 rounds.
 *
 * How it works:
 *   1. The enclave picks rock (0), paper (1) or scissors (2).
 *   2. It ALWAYS plays the same move in round 1.
 *   3. It calls `ocall_pedra_papel_tesoura`, passing the current round number, counting 1, 2, 3... up to 20.
 *   4. It compares the moves; if you win, it increments your win count.
 *   5. The enclave's moves are deterministic, but the result of the previous round INFLUENCES its next move.
 *   6. After round 20 the enclave returns how many times YOU won. If the return value is 20 the challenge is
 *      complete and the console prints every round and outcome.
 *
 * - Returns -1 if your OCALL returns anything other than 0, 1 or 2.
 * - The enclave aborts if your OCALL fails or aborts.
 *
 * HINT: the strategy is deterministic; as long as the sequence of previous results is the same, the enclave
 *  plays the same moves.
 **/
extern int ecall_pedra_papel_tesoura(void) {
    drbg_ctr128_t rng = drbg_seeded_init(5);
    uint8_t user_wins = 0;

    char enclave_sequence[ROUNDS + 1] = "";
    char app_sequence[ROUNDS + 1] = "";
    char results[ROUNDS + 1] = "";

    uint8_t last_app_play = 0;

    static_assert(ROUNDS < UINT8_MAX);
    for (uint8_t i = 0; i < ROUNDS; i++) {
        uint8_t enclave_play = random_play(&rng);
        if unlikely (enclave_play == UINT8_MAX) {
            return -2;
        }
        // FIXME: insecure implementation
        enclave_play = (enclave_play + last_app_play) % 3;

        const uint8_t app_play = ocall_play(i + 1);
        if unlikely (app_play == UINT8_MAX) {
            return -1;
        }
        const round_result_t res = result(enclave_play, app_play);
        user_wins += res == WIN;

        enclave_sequence[i] = display_play(enclave_play);
        app_sequence[i] = display_play(app_play);
        results[i] = display_result(res);
        last_app_play = app_play;
    }

    enclave_sequence[ROUNDS] = '\0';
    app_sequence[ROUNDS] = '\0';
    results[ROUNDS] = '\0';

    assume(user_wins <= ROUNDS);
    if unlikely (user_wins >= ROUNDS) {
        printf("\n%s\n", SEPARATOR);
        printf(
            // clang-format off
            "[ENCLAVE] DESAFIO 5 CONCLUIDO!! V (vitória), D (derrota) E (empate)\n"
            "          ENCLAVE JOGADAS: %s\n"
            "             SUAS JOGADAS: %s\n"
            "                RESULTADO: %s\n",
            // clang-format on
            enclave_sequence,
            app_sequence,
            results
        );
        printf("%s\n", SEPARATOR);
    }
    return (int) user_wins;
}
