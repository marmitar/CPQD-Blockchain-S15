#include <sgx_defs.h>
#include <sgx_eid.h>
#include <sgx_error.h>
#include <sgx_urts.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "./challenge/challenges.h"
#include "./error.h"
#include "defines.h"
#include "enclave_u.h"

/**
 * OCALL called by the enclave to print some text to the terminal.
 **/
extern void ocall_print_string(const char *NULLABLE str) {
    /* Proxy/Bridge will check the length and null-terminate
     * the input string to prevent buffer overflow.
     */
    puts(likely(str != NULL) ? str : "<null>");
}

/* Application entry */
extern int SGX_CDECL main(const int argc, const char *restrict NONNULL argv[NONNULL argc]) {
    const char *NONNULL enclave = "enclave-desafio-5.signed.so";
    // accept an optional argument for the enclave file
    if unlikely (argc == 2) {
        enclave = argv[1];
    } else if unlikely (argc > 2) {
        printf("Error: too many arguments\n");
        printf("%s: [SIGNED_ENCLAVE.SO]\n", argv[0]);
        return EXIT_FAILURE;
    }

    /* Global EID shared by multiple threads */
    sgx_enclave_id_t global_eid = (sgx_enclave_id_t) -1;

    /* Initialize the enclave */
    /* Debug Support: set 2nd parameter to 1 */
    sgx_status_t status = sgx_create_enclave(enclave, SGX_DEBUG_FLAG, NULL, NULL, &global_eid, NULL);
    if unlikely (status != SGX_SUCCESS) {
        print_error_message(status);
        return EXIT_FAILURE;
    }

    bool ok = true;

    /* CHALLENGE 1: Call the enclave */
    status = challenge_1(global_eid);
    if unlikely (status != SGX_SUCCESS) {
        print_error_message(status);
        ok = false;
    }

    /* CHALLENGE 2: Crack the password */
    status = challenge_2(global_eid);
    if unlikely (status != SGX_SUCCESS) {
        print_error_message(status);
        ok = false;
    }

    /* CHALLENGE 3: Secret Sequence */
    status = challenge_3(global_eid);
    if unlikely (status != SGX_SUCCESS) {
        print_error_message(status);
        ok = false;
    }

    /* CHALLENGE 4: Secret Polynomial */
    status = challenge_4(global_eid);
    if unlikely (status != SGX_SUCCESS) {
        print_error_message(status);
        ok = false;
    }

    /* CHALLENGE 5: Rock, Paper, Scissors */
    status = challenge_5(global_eid);
    if unlikely (status != SGX_SUCCESS) {
        print_error_message(status);
        ok = false;
    }

    /* Destroy the enclave */
    status = sgx_destroy_enclave(global_eid);
    if unlikely (status != SGX_SUCCESS) {
        print_error_message(status);
        return EXIT_FAILURE;
    }

    printf("Info: Enclave successfully returned.\n");
    return likely(ok) ? EXIT_SUCCESS : EXIT_FAILURE;
}
