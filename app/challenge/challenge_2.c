#include <sgx_eid.h>
#include <sgx_error.h>
#include <stdio.h>

#include "./challenges.h"
#include "defines.h"
#include "enclave_u.h"

/**
 * Challenge 2: Crack the password
 * -------------------------------
 *
 * Brute force all possible passwords, from `0` to `99_999`, and find the correct one. Up to 100 thousand
 * calls to `ecall_verificar_senha` are required.
 */
extern sgx_status_t challenge_2(sgx_enclave_id_t eid) {
    static constexpr unsigned MIN_PASSWORD = 0;
    static constexpr unsigned MAX_PASSWORD = 99'999;

    for (unsigned password = MIN_PASSWORD; password <= MAX_PASSWORD; password++) {
        int rv = -1;
        const sgx_status_t status = ecall_verificar_senha(eid, &rv, password);
        if unlikely (status != SGX_SUCCESS) {
            return status;
        }

        if (rv == 0) {
#ifdef DEBUG
            printf("Challenge 2: password = %u\n", password);
#endif
            return SGX_SUCCESS;
        }
    }

    printf("Challenge 2: Password not found\n");
    return SGX_ERROR_UNEXPECTED;
}
