#ifndef APP_H
#define APP_H

#include <assert.h>
#include <stdarg.h>

#include "sgx_eid.h" /* sgx_enclave_id_t */

#ifndef TRUE
#    define TRUE 1
#endif

#ifndef FALSE
#    define FALSE 0
#endif

#if defined(__GNUC__)
#    define ENCLAVE_FILENAME "enclave.signed.so"
#endif

extern sgx_enclave_id_t global_eid; /* global enclave id */

#if defined(__cplusplus)
extern "C" {
#endif

    void ecall_libcxx_functions(void);

#if defined(__cplusplus)
}
#endif

#endif /* !APP_H */
