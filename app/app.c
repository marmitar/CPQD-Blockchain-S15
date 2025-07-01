// clang-format off
#if __STDC_VERSION__ < 202300L
#error "This code is compliant with C23 or later only."
#endif
// clang-format on

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_PATH FILENAME_MAX

#include "app.h"
#include "enclave_u.h"
#include "error.h"
#include "sgx_defs.h"
#include "sgx_eid.h"
#include "sgx_error.h"
#include "sgx_urts.h"

/* Global EID shared by multiple threads */
sgx_enclave_id_t global_eid = 0;

/* Initialize the enclave:
 *   Call sgx_create_enclave to initialize an enclave instance
 */
int initialize_enclave(void) {
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;

    /* Call sgx_create_enclave to initialize an enclave instance */
    /* Debug Support: set 2nd parameter to 1 */
    ret = sgx_create_enclave(ENCLAVE_FILENAME, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
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
int SGX_CDECL main(int argc, char *argv[]) {
    int status;
    const char *name;
    sgx_status_t ret = SGX_ERROR_UNEXPECTED;
    (void) (argc);
    (void) (argv);

    /* Initialize the enclave */
    if (initialize_enclave() < 0) {
        printf("Enter a character before exit ...\n");
        (void) getchar();
        return -1;
    }

    /* ecall_name_check */
    name = "Joao Da Silva";
    status = 0;
    ret = ecall_name_check(global_eid, &status, name);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }
    if (status != 0) {
        printf("the name '%s' is invalid\n", name);
    } else {
        printf("the name '%s' is valid\n", name);
    }

    /* Destroy the enclave */
    ret = sgx_destroy_enclave(global_eid);
    if (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }

    printf("Info: Enclave successfully returned.\n");
    return 0;
}
