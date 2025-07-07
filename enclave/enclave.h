#ifndef ENCLAVE_H
#define ENCLAVE_H

#include "defines.h"

// IWYU pragma: always_keep
#if defined(__clang__)
/** __config.h is malformed with clang */
#    undef __clang__
#    include <__config>  // IWYU pragma: keep
#    define __clang__ 1  // NOLINT(bugprone-reserved-identifier,cert-dcl37-c)
#endif

[[gnu::format(printf, 1, 2), gnu::nonnull(1), gnu::nothrow, gnu::leaf]]
/**
 * `printf`-like function for the enclave.
 *
 * @returns The number of bytes written to stdout.
 */
int printf(const char *NONNULL fmt, ...);

#endif /* ENCLAVE_H */
