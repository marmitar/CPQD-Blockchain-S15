#include "error.h"
#include "sgx_error.h"
#include <stdio.h>

struct sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
};

/* Error code returned by sgx_create_enclave */
static const struct sgx_errlist_t sgx_errlist[] = {
    {
     .err = SGX_ERROR_UNEXPECTED,
     .msg = "Unexpected error occurred.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_INVALID_PARAMETER,
     .msg = "Invalid parameter.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_OUT_OF_MEMORY,
     .msg = "Out of memory.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_ENCLAVE_LOST,
     .msg = "Power transition occurred.",
     .sug = "Please refer to the sample \"PowerTransition\" for details.",
     },
    {
     .err = SGX_ERROR_INVALID_ENCLAVE,
     .msg = "Invalid enclave image.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_INVALID_ENCLAVE_ID,
     .msg = "Invalid enclave identification.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_INVALID_SIGNATURE,
     .msg = "Invalid enclave signature.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_OUT_OF_EPC,
     .msg = "Out of EPC memory.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_NO_DEVICE,
     .msg = "Invalid SGX device.",
     .sug = "Please make sure SGX module is enabled in the BIOS, and install SGX driver afterwards.",
     },
    {
     .err = SGX_ERROR_MEMORY_MAP_CONFLICT,
     .msg = "Memory map conflicted.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_INVALID_METADATA,
     .msg = "Invalid enclave metadata.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_DEVICE_BUSY,
     .msg = "SGX device was busy.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_INVALID_VERSION,
     .msg = "Enclave version was invalid.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_INVALID_ATTRIBUTE,
     .msg = "Enclave was not authorized.",
     .sug = NULL,
     },
    {
     .err = SGX_ERROR_ENCLAVE_FILE_ACCESS,
     .msg = "Can't open enclave file.",
     .sug = NULL,
     },
    {.err = SGX_ERROR_NDEBUG_ENCLAVE,
     .msg = "The enclave is signed as product enclave, and can not be created as debuggable enclave.",
     .sug = NULL},
    {
     .err = SGX_ERROR_MEMORY_MAP_FAILURE,
     .msg = "Failed to reserve memory for the enclave.",
     .sug = NULL,
     },
};

/* Check error conditions for loading enclave */
extern void print_error_message(sgx_status_t ret) {
    const size_t ttl = sizeof(sgx_errlist) / sizeof(sgx_errlist[0]);

    for (size_t idx = 0; idx < ttl; idx++) {
        if (ret == sgx_errlist[idx].err) {
            if (NULL != sgx_errlist[idx].sug) {
                printf("Info: %s\n", sgx_errlist[idx].sug);
            }
            printf("Error: %s\n", sgx_errlist[idx].msg);
            return;
        }
    }

    printf("Error: Unexpected error occurred.\n");
}
