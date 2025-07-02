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

    /* CHALLENGE 4: Secret Polynomial */
    ret = challenge_4(global_eid);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        ok = false;
    }

    /* CHALLENGE 5: Rock, Paper, Scissors */
    ret = challenge_5(global_eid);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        ok = false;
    }

    /* Destroy the enclave */
    ret = sgx_destroy_enclave(global_eid);
    if unlikely (ret != SGX_SUCCESS) {
        print_error_message(ret);
        abort();
    }

    printf("Info: Enclave successfully returned.\n");
    return likely(ok) ? EXIT_SUCCESS : EXIT_FAILURE;
}
