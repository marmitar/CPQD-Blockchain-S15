#include <limits.h>
#include <sgx_eid.h>
#include <sgx_error.h>
#include <stdint.h>

#include "../defines.h"
#include "./challenges.h"
#include "enclave_u.h"

#define ROUNDS 20

static uint8_t answers[ROUNDS] = {0};

/**
 * OCALL que será chamada 20x pela ecall `ecall_pedra_papel_tesoura`,
 * recebe como parametro o round atual, contando a partir do 1, até 20.
 * Essa função DEVE retornar 0 (pedra), 1 (papel) ou 2 (tesoura), caso
 * contrário o enclave aborta imediatamente.
 *
 * DICA: utilize variáveis estáticas se precisar persistir um estado entre
 *       chamadas a essa função.
 **/
extern unsigned int ocall_pedra_papel_tesoura(unsigned int round) {
    // TODO:
    // if unlikely (round < 1 || round > ROUNDS) {
    //     (void) fprintf(stderr, "Error: invalid input round: %u\n", round);
    //     abort();
    // }
    return answers[round - 1];
}

[[gnu::nonnull(2), nodiscard("useless call if unused")]]
static uint8_t check_answers(sgx_enclave_id_t eid, sgx_status_t *NONNULL status) {
    int wins = -1;

    sgx_status_t rstatus = ecall_pedra_papel_tesoura(eid, &wins);
    if unlikely (rstatus != SGX_SUCCESS || wins < 0 || wins >= UINT8_MAX) {
        // print_error_message(ret);
        // abort();
        *status = rstatus;
    }

    if likely (wins >= 0 && wins <= UINT8_MAX) {
        return (uint8_t) wins;
    }

    // TODO:
    // (void) fprintf(stderr, "Error: unexpected result from ecall_pedra_papel_tesoura: %d\n", wins);
    // abort();
    return UINT8_MAX;
}

typedef uint8_t init_function_t(uint8_t position, uint8_t start);

[[gnu::nonnull(2)]]
static uint32_t greedy(sgx_enclave_id_t eid, sgx_status_t *NONNULL status, uint8_t start, init_function_t init) {
    assume(start <= 2 * ROUNDS);
    for (uint8_t i = start; i < ROUNDS; i++) {
        answers[i] = init(i, start);
    }

    uint8_t w0 = check_answers(eid, status);

    uint32_t total = w0;
    for (uint8_t i = ROUNDS; i > start; i--) {
        const uint8_t vi = answers[i - 1];

        answers[i - 1] = (vi + 1) % 3;
        const uint8_t w1 = check_answers(eid, status);

        answers[i - 1] = (vi + 2) % 3;
        const uint8_t w2 = check_answers(eid, status);

        if (w0 >= w1 && w0 >= w2) {
            answers[i - 1] = (vi + 0) % 3;
            // wins = wins;
        } else if (w1 > w2) {
            answers[i - 1] = (vi + 1) % 3;
            w0 = w1;
        } else {
            // desafio_5_answers[i-1] = (v + 2) % 3;
            w0 = w2;
        }

        total += w0;
        total += w1;
        total += w2;
    }
    return total;
}

[[gnu::const, nodiscard("pure function")]]
static uint8_t init_zero(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    (void) i;
    (void) s;
    return 0;
}

[[gnu::const, nodiscard("pure function")]]
static uint8_t init_add(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i + s - 1) % 3;
}

[[gnu::const, nodiscard("pure function")]]
static uint8_t init_mul(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i * s + 1) % 3;
}

[[gnu::const, nodiscard("pure function")]]
static uint8_t init_square(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i * i + s * s) % 3;
}

[[gnu::nonnull(2)]]
static uint32_t limited_dfs(sgx_enclave_id_t eid, sgx_status_t *NONNULL status, uint8_t start, uint8_t depth) {
    if likely (depth == 0) {
        return greedy(eid, status, start, init_zero) + greedy(eid, status, start, init_add)
            + greedy(eid, status, start, init_mul) + greedy(eid, status, start, init_square);
    }

    uint32_t total_wins[3] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
    for (uint8_t d = 0; d <= 2; d++) {
        answers[start] = d;  // FIXME: possible out-of-bounds access
        total_wins[d] = limited_dfs(eid, status, start + 1, depth - 1);
    }

    if (total_wins[0] >= total_wins[1] && total_wins[0] >= total_wins[2]) {
        answers[start] = 0;
    } else if (total_wins[1] >= total_wins[2]) {
        answers[start] = 1;
    } else {
        answers[start] = 2;
    }

    return total_wins[0] + total_wins[1] + total_wins[2];
}

/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * TODO
 */
extern sgx_status_t challenge_5(sgx_enclave_id_t eid) {
    for (uint8_t start = 0; start < ROUNDS; start++) {
        sgx_status_t status = SGX_SUCCESS;
        limited_dfs(eid, &status, start, 2);
        if unlikely (status != SGX_SUCCESS) {
            return status;
        }
    }

    int rv = -1;
    const sgx_status_t status = ecall_pedra_papel_tesoura(eid, &rv);
    if unlikely (status != SGX_SUCCESS) {
        return status;
    }

    if unlikely (rv != ROUNDS) {
        // TODO: print error?
        return SGX_ERROR_UNEXPECTED;
    }

    return SGX_SUCCESS;
}
