// clang-format off
#if __STDC_VERSION__ < 202300L
#error "This code is compliant with C23 or later only."
#endif
// clang-format on

#include <stdarg.h>
#include <stdio.h>  /* vsnprintf */
#include <string.h> /* vsnprintf */

#include "enclave.h"
#include "enclave_t.h" /* print_string */
#include "sgx_trts.h"

#define NAME_MAX_LEN 128

/* ecall_name_check:
 *   [string]
 */
int ecall_name_check(const char *name) {
    size_t len, i;
    char c;
    int start;
    start = NAME_MAX_LEN;
    for (i = 0; i < NAME_MAX_LEN; i++) {
        // word first char
        c = name[i];

        // First char of each word must be uppercase
        if (c < 'A' || c > 'Z') {
            return -1;
        }
        start = ++i;

        // Following letters must be lowercase
        for (; i < NAME_MAX_LEN; i++) {
            c = name[i];
            if (c == '\0') {
                goto end;
            }
            if (c == ' ') {
                break;
            }
            if (c < 'a' || c > 'z') {
                return -1;
            }
        }
        // The last char must be a lowercase letter.
        if ((i + 1) >= NAME_MAX_LEN || start == i) {
            return -2;
        }
    }
end:
    return start < i ? 0 : -1;
}

/*
 * printf:
 *   Invokes OCALL to display the enclave buffer to the terminal.
 */
int printf(const char *fmt, ...) {
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
    return 0;
}
