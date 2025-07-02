#include <sgx_eid.h>
#include <sgx_error.h>

#include "../defines.h"
#include "./challenges.h"
#include "enclave_u.h"

/**
 * Challenge 2: Crack the password
 * -------------------------------
 *
 * Brute force all possible passwords, from `0` to `99_999`, and find the correct one.
 */
extern sgx_status_t challenge_2(sgx_enclave_id_t eid) {
    static const unsigned MIN_PASSWORD = 0;
    static const unsigned MAX_PASSWORD = 99'999;

    for (unsigned password = MIN_PASSWORD; password <= MAX_PASSWORD; password++) {
        int rv = -1;

        sgx_status_t status = ecall_verificar_senha(eid, &rv, password);
        if unlikely (status != SGX_SUCCESS) {
            return status;
        }

        if (rv == 0) {
            return SGX_SUCCESS;
        }
    }

    // TODO: print error?
    return SGX_ERROR_UNEXPECTED;
}
