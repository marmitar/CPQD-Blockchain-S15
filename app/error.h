#ifndef APP_SGXERROR_H
#define APP_SGXERROR_H

#include "sgx_error.h"

typedef struct sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

/* Print error from enclave status */
void print_error_message(sgx_status_t ret);

#endif  // APP_SGXERROR_H
