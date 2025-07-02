// clang-format off
#if __STDC_VERSION__ < 202300L
#error "This code is compliant with C23 or later only."
#endif
// clang-format on

#include <assert.h>
#include <limits.h>
#include <sgx_defs.h>
#include <sgx_eid.h>
#include <sgx_error.h>
#include <sgx_urts.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "./challenge/challenges.h"
#include "./defines.h"
#include "./error.h"
#include "enclave_u.h"

#if !defined(ENCLAVE_FILENAME)
#    define ENCLAVE_FILENAME "enclave.signed.so"
#endif

/* Global EID shared by multiple threads */
static sgx_enclave_id_t global_eid = 0;

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
static int initialize_enclave(void) {
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    sgx_status_t ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

/** Allow ecall output temporarily. */
static bool enable_enclave_output = true;

/* OCall functions */
extern void ocall_print_string(const char *str) {
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    if (enable_enclave_output) {
        printf("%s", str);
    }
}

/* CHALLENGE 4 */

struct coeff {
    int a;
    int b;
    int c;
};

/** Evaluate the polynomial on x=1, x=2 and x=3, then solve the linear equation for the coefficients. */
static struct coeff desafio_4_coefficients(void) {
    enable_enclave_output = false;

    // a + b + c = y1
    int y1 = 0;
    sgx_status_t ret = ecall_polinomio_secreto(global_eid, &y1, /*x=*/1);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }

    // 4a + 2b + c = y2
    int y2 = 0;
    ret = ecall_polinomio_secreto(global_eid, &y2, /*x=*/2);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }

    // 9a + 3b + c = y3
    int y3 = 0;
    ret = ecall_polinomio_secreto(global_eid, &y3, /*x=*/3);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }

    // 3a + b = y2 - y1
    // 5a + b = y3 - y2
    // 2a = y3 - y2 - (y2 - y1) = y3 - 2 y2 + y1
    int a = (y3 - 2 * y2 + y1) / 2;
    // 5a + b = y3 - y2
    const int mb = 5;
    int b = y3 - y2 - mb * a;
    // a + b + c = y1
    int c = y1 - a - b;

    enable_enclave_output = true;
    return (struct coeff) {.a = a, .b = b, .c = c};
}

/* CHALLENGE 5 */

#define ROUNDS 20

static uint8_t desafio_5_answers[ROUNDS] = {0};

/**
 * OCALL que será chamada 20x pela ecall `ecall_pedra_papel_tesoura`,
 * recebe como parametro o round atual, contando a partir do 1, até 20.
 * Essa função DEVE retornar 0 (pedra), 1 (papel) ou 2 (tesoura), caso
 * contrário o enclave aborta imediatamente.
 *
 * DICA: utilize variáveis estáticas se precisar persistir um estado entre
 *       chamadas a essa função.
 **/
extern unsigned ocall_pedra_papel_tesoura(unsigned int round) {
    if unlikely (round < 1 || round > ROUNDS) {
        (void) fprintf(stderr, "Error: invalid input round: %u\n", round);
        abort();
    }
    return desafio_5_answers[round - 1];
}

static uint8_t desafio_5_wins(void) {
    int wins = -1;

    sgx_status_t ret = ecall_pedra_papel_tesoura(global_eid, &wins);
    if unlikely (ret != SGX_SUCCESS || wins < 0 || wins >= UINT8_MAX) {
        print_error_message(ret);
        abort();
    }

    if likely (wins >= 0 && wins <= UINT8_MAX) {
        return (uint8_t) wins;
    }

    (void) fprintf(stderr, "Error: unexpected result from ecall_pedra_papel_tesoura: %d\n", wins);
    abort();
}

static unsigned desafio_5_greedy_solution(uint8_t s, uint8_t init(uint8_t i, uint8_t s)) {
    assume(s <= 2 * ROUNDS);

    if likely (init != NULL) {
        for (uint8_t i = s; i < ROUNDS; i++) {
            desafio_5_answers[i] = init(i, s);
        }
    }
    uint8_t w0 = desafio_5_wins();

    unsigned total = w0;
    for (uint8_t i = ROUNDS; i > s; i--) {
        const uint8_t vi = desafio_5_answers[i - 1];

        desafio_5_answers[i - 1] = (vi + 1) % 3;
        const uint8_t w1 = desafio_5_wins();

        desafio_5_answers[i - 1] = (vi + 2) % 3;
        const uint8_t w2 = desafio_5_wins();

        if (w0 >= w1 && w0 >= w2) {
            desafio_5_answers[i - 1] = (vi + 0) % 3;
            // wins = wins;
        } else if (w1 > w2) {
            desafio_5_answers[i - 1] = (vi + 1) % 3;
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

static uint8_t zero(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    (void) i;
    (void) s;
    return 0;
}

static uint8_t add(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i + s - 1) % 3;
}

static uint8_t mul(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i * s + 1) % 3;
}

static uint8_t sq(uint8_t i, uint8_t s) {
    assume(i <= ROUNDS);
    assume(s <= ROUNDS);

    return (i * i + s * s) % 3;
}

static unsigned desafio_5_limited_search(uint8_t s, uint8_t depth) {
    if (depth == 0) {
        return desafio_5_greedy_solution(s, zero) + desafio_5_greedy_solution(s, add)
            + desafio_5_greedy_solution(s, mul) + desafio_5_greedy_solution(s, sq) + desafio_5_greedy_solution(s, NULL);
    }

    unsigned wt[3] = {0, 0, 0};
    for (uint8_t d = 0; d <= 2; d++) {
        desafio_5_answers[s] = d;  // FIXME: out-of-bounds access
        wt[d] = desafio_5_limited_search(s + 1, depth - 1);
    }

    if (wt[0] >= wt[1] && wt[0] >= wt[2]) {
        desafio_5_answers[s] = 0;
    } else if (wt[1] >= wt[2]) {
        desafio_5_answers[s] = 1;
    } else {
        desafio_5_answers[s] = 2;
    }

    return wt[0] + wt[1] + wt[2];
}

/** Test all possible values for each position, and chose the one that increase wins locally. */
static void desafio_5_find_solution(void) {
    enable_enclave_output = false;

    for (uint8_t s = 0; s < ROUNDS; s++) {
        desafio_5_limited_search(s, 2);
    }

    enable_enclave_output = true;
}

/* Application entry */
int SGX_CDECL main(void) {
    /* Initialize the enclave */
    if unlikely (initialize_enclave() != 0) {
        printf("Enter a character before exit ...\n");
        (void) getchar();
        return EXIT_FAILURE;
    }

    bool ok = true;

    /* CHALLENGE 1: Call the enclave */
    sgx_status_t ret = challenge_1(global_eid);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        ok = false;
    }

    /* CHALLENGE 2: Crack the password */
    ret = challenge_2(global_eid);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        ok = false;
    }

    /* CHALLENGE 3: Secret Sequence */
    ret = challenge_3(global_eid);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        ok = false;
    }

    /* DESAFIO 4: ecall_polinomio_secreto */
    struct coeff poly = desafio_4_coefficients();
    printf("Info: Secret polynomial: a=%d, b=%d, c=%d\n", poly.a, poly.b, poly.c);

    int status = -1;
    ret = ecall_verificar_polinomio(global_eid, &status, poly.a, poly.b, poly.c);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
    ok = likely(ok) && (status != 0);

    /* DESAFIO 5: ecall_pedra_papel_tesoura */
    desafio_5_find_solution();
    ret = ecall_pedra_papel_tesoura(global_eid, &status);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
    ok = likely(ok) && (status == ROUNDS);

    printf("Info: Pedra-papel-tesoura = [");
    for (unsigned i = 0; i < ROUNDS; i++) {
        if likely (i > 0) {
            printf(" ");
        }
        printf("%u", desafio_5_answers[i]);
    }
    printf("], wins = %d\n", status);

    /* Destroy the enclave */
    ret = sgx_destroy_enclave(global_eid);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }

    printf("Info: Enclave successfully returned.\n");
    return likely(ok) ? EXIT_SUCCESS : EXIT_FAILURE;
}
