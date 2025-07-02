#include <sgx_eid.h>
#include <sgx_error.h>
#include <stdio.h>

#include "../defines.h"
#include "./challenges.h"
#include "enclave_u.h"

/**
 * Challenge 1: Call the enclave
 * -----------------------------
 *
 * Just call `ecall_verificar_aluno` with my own name, everything capitalized, including the connective "de".
 */
extern sgx_status_t challenge_1(sgx_enclave_id_t eid) {
    const char name[] = "Tiago De Paula Alves";

#ifdef DEBUG
    printf("Challenge 1: name = '%s'\n", name);
#endif

    int rv = -1;
    const sgx_status_t status = ecall_verificar_aluno(eid, &rv, name);
    if unlikely (status != SGX_SUCCESS) {
        return status;
    }

    if unlikely (rv != 0) {
        printf("Challenge 1: Valid name not found\n");
        return SGX_ERROR_UNEXPECTED;
    }

    return SGX_SUCCESS;
}
