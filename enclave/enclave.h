#ifndef ENCLAVE_H
#define ENCLAVE_H

#include <stdint.h>

#include "defines.h"

/** Challenge output separator. */
#define SEPARATOR "------------------------------------------------"

// IWYU pragma: always_keep
#if defined(__clang__) && __has_include(<__config>)
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

/** Unsigned 128-bit number. GCC and Clang extensions. */
typedef __uint128_t uint128_t;

/** Maximum value for `uint128_t`. */
[[maybe_unused]] static constexpr uint128_t UINT128_MAX = (uint128_t) -1;

/**
 * Deterministic Random Bit Generator (DRBG).
 *
 * Note: this implementation is not thread-safe.
 */
typedef struct drbg_ctr128 {
    /** 128-bit seed + stream selector */
    uint128_t key;
    /** 128-bit block counter */
    uint128_t ctr;
} drbg_ctr128_t;

[[nodiscard("pure function"), gnu::const, gnu::cold, gnu::nothrow, gnu::leaf]]
/**
 * Initialize the PRNG using the seed file. The `stream` selector allows picking a different generated stream.
 *
 * Note: each different PRNG should use a unique stream selector, since the seed is the same.
 */
drbg_ctr128_t drbg_seeded_init(uint64_t stream);

[[nodiscard("pure function"), gnu::const, gnu::hot, gnu::nothrow, gnu::leaf]]
/**
 * Replace the `stream` selector for the PRNG.
 *
 * Note: take care of keeping the stream selector unique throught the enclave.
 */
drbg_ctr128_t drbg_set_stream(drbg_ctr128_t drbg, uint64_t stream);

[[nodiscard("error must be checked"), gnu::nonnull(1, 2), gnu::hot, gnu::nothrow, gnu::leaf]]
/**
 * Generate a pseudo-random number in `[0,UINT128_MAX)` from the DRBG sequence.
 *
 * @return `true` on success, or `false` if AES CTR failed.
 */
bool drbg_rand(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output);

[[nodiscard("error must be checked"), gnu::nonnull(1, 2), gnu::hot, gnu::nothrow, gnu::leaf]]
/**
 * Generate a pseudo-random number in the range `[0,bound)` from the DRBG sequence.
 *
 * @return `true` on success, or `false` if AES CTR failed.
 */
bool drbg_rand_bounded(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output, uint128_t bound);

#endif /* ENCLAVE_H */
