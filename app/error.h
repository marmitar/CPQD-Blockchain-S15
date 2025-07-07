#ifndef APP_ERROR_H
#define APP_ERROR_H

#include <sgx_error.h>

[[gnu::cold, gnu::nothrow, gnu::leaf]]
/* Print error from enclave status */
void print_error_message(sgx_status_t ret);

#endif  // APP_ERROR_H
