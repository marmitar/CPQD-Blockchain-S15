// clang-format off
#if __STDC_VERSION__ < 202300L
#error "This code is compliant with C23 or later only."
#endif
// clang-format on

#include <assert.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "enclave_u.h"
#include "error.h"
#include "sgx_defs.h"
#include "sgx_eid.h"
#include "sgx_error.h"
#include "sgx_urts.h"

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
    if (ret != SGX_SUCCESS) {
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

/* CHALLENGE 2 */

/** Brute-force all possible passwords for challenge 2. */
static unsigned desafio_2_senha(void) {
    enable_enclave_output = false;

    static const unsigned MAX_SENHA = 99'999;
    for (unsigned i = 0; i <= MAX_SENHA; i++) {
        int status = -1;
        sgx_status_t ret = ecall_verificar_senha(global_eid, &status, i);
        if (ret != SGX_SUCCESS) {
            print_error_message(ret);
            abort();
        }

        if (status == 0) {
            enable_enclave_output = true;
            return i;
        }
    }

    enable_enclave_output = true;
    printf("Info: No password matched.\n");
    return 0;
}

/* CHALLENGE 3 */

#define WORD 20

struct word {
    char s[WORD];
};

static const char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

[[gnu::const]]
/** Guess word with all positions set to 'A'. */
static struct word desafio_3_first_guess(void) {
    struct word w = {.s = ""};
    for (unsigned i = 0; i < WORD; i++) {
        w.s[i] = LETTERS[0];
    }
    return w;
}

static int desafio_3_update_guess(struct word *w, struct word t) {
    for (unsigned i = 0; i < WORD; i++) {
        if (t.s[i] != w->s[i]) {
            if (w->s[i] < 'Z') {
                w->s[i]++;
            } else {
                return -1;
            }
        }
    }
    return 0;
}

/** Increment all wrong letters until the solution is found. */
static struct word desafio_3_secret_word(void) {
    struct word w = desafio_3_first_guess();

    enable_enclave_output = false;
    while (true) {
        struct word t = w;

        int status = -1;
        sgx_status_t ret = ecall_palavra_secreta(global_eid, &status, t.s);
        if (ret != SGX_SUCCESS) {
            print_error_message(ret);
            abort();
        }

        if (status == 0) {
            enable_enclave_output = true;
            return w;
        }

        status = desafio_3_update_guess(&w, t);
        if (status != 0) {
            enable_enclave_output = true;
            return (struct word) {.s = "<not found>"};
        }
    }
}

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
    (void) round;
    return 0;
}

/* Application entry */
int SGX_CDECL main(void) {
    /* Initialize the enclave */
    if (initialize_enclave() != 0) {
        printf("Enter a character before exit ...\n");
        (void) getchar();
        return EXIT_FAILURE;
    }

    bool ok = true;

    /* DESAFIO 1: ecall_verificar_aluno */
    const char name[] = "Tiago De Paula Alves";
    int status = -1;
    sgx_status_t ret = ecall_verificar_aluno(global_eid, &status, name);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
    ok = ok && (status == 0);

    /* DESAFIO 2: ecall_verificar_senha */
    const unsigned password = desafio_2_senha();
    printf("Info: Password = %u\n", password);

    ret = ecall_verificar_senha(global_eid, &status, password);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
    ok = ok && (status == 0);

    /* DESAFIO 3: ecall_palavra_secreta */
    struct word secret = desafio_3_secret_word();
    printf("Info: Secret word = %20s\n", secret.s);

    ret = ecall_palavra_secreta(global_eid, &status, secret.s);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
    ok = ok && (status == 0);

    /* Destroy the enclave */
    ret = sgx_destroy_enclave(global_eid);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }

    printf("Info: Enclave successfully returned.\n");
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
