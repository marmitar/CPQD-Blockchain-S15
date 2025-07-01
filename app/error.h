#ifndef _APP_SGXERROR_H_
#define _APP_SGXERROR_H_

#include "sgx_error.h"

typedef struct _sgx_errlist_t {
    sgx_status_t err;
    const char *msg;
    const char *sug; /* Suggestion */
} sgx_errlist_t;

#if defined(__cplusplus)
extern "C" {
#endif

    /* Print error from enclave status */
    void print_error_message(sgx_status_t ret);

#if defined(__cplusplus)
}
#endif

#endif  // _APP_SGXERROR_H_
