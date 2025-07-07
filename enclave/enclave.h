#ifndef ENCLAVE_H
#define ENCLAVE_H

#include <pcg_basic.h>
#include <stdint.h>

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

[[nodiscard("pure function"), gnu::const, gnu::cold, gnu::nothrow, gnu::leaf]]
/**
 * Initialize a PRNG using the seed file. The `stream_selector` allow selecting a completing different stream.
 * Note: each different PRNG should use a unique strea selector, since the seed is the same.
 */
pcg32_random_t seeded_pcg_rng(uint64_t stream_selector);

#endif /* ENCLAVE_H */
