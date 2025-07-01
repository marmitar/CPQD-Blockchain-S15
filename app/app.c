// clang-format off
#if __STDC_VERSION__ < 202300L
#error "This code is compliant with C23 or later only."
#endif
// clang-format on

#include <assert.h>
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
sgx_enclave_id_t global_eid = 0;

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void) {
    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    sgx_status_t ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        return -1;
    }

    return 0;
}

/* OCall functions */
void ocall_print_string(const char *str) {
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    printf("%s", str);
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
unsigned ocall_pedra_papel_tesoura(unsigned int round) {
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

    /*  DESAFIO 1: ecall_verificar_aluno */
    const char name[] = "Tiago De Paula Alves";
    int status = -1;
    sgx_status_t ret = ecall_verificar_aluno(global_eid, &status, name);
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
