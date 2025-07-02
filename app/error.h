#ifndef APP_SGXERROR_H
#define APP_SGXERROR_H

#include <sgx_error.h>

[[gnu::nothrow, gnu::leaf]]
/* Print error from enclave status */
void print_error_message(sgx_status_t ret);

#endif  // APP_SGXERROR_H
