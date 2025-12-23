#ifndef ENCLAVE_H
#define ENCLAVE_H

#include <stdint.h>
#include <string.h>

#include "defines.h"
#include "enclave_config.h"

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

[[nodiscard("pure function"), gnu::const]]
/**
 * Initialize the PRNG using an input `seed` and a `stream` selector.
 */
static inline drbg_ctr128_t drbg_init(const uint64_t seed, const uint64_t stream) {
    drbg_ctr128_t drbg = {0};

    const uint64_t key[] = {seed, stream};
    static_assert(sizeof(key) == sizeof(drbg.key));

    memcpy(&(drbg.key), &key, sizeof(drbg.key));
    memset(&(drbg.ctr), 0, sizeof(drbg.ctr));
    return drbg;
}

[[nodiscard("pure function"), gnu::const, gnu::hot, gnu::nothrow]]
/**
 * Initialize the PRNG using the seed file. The `stream` selector allows picking a different generated stream.
 *
 * Note: each different PRNG should use a unique stream selector, since the seed is the same.
 */
static inline drbg_ctr128_t drbg_seeded_init(const uint64_t stream) {
    return drbg_init(ENCLAVE_SEED, stream);
}

[[nodiscard("pure function"), gnu::const, gnu::hot, gnu::nothrow]]
/**
 * Replace the `stream` selector for the PRNG.
 *
 * Note: take care of keeping the stream selector unique throught the enclave.
 */
static inline drbg_ctr128_t drbg_set_stream(drbg_ctr128_t drbg, const uint64_t stream) {
    uint64_t key[2] = {0, 0};
    static_assert(sizeof(key) == sizeof(drbg.key));

    memcpy(key, &(drbg.key), sizeof(drbg.key));
    key[1] = stream;
    memcpy(&(drbg.key), key, sizeof(drbg.key));

    return drbg;
}

[[nodiscard("error must be checked"), gnu::nonnull(1, 2), gnu::hot, gnu::nothrow, gnu::leaf]]
/**
 * Pick a pseudo-random number from the DRBG sequence if it's in the `[0,threshold)` range.
 *
 * @return `true` on success, or `false` if AES CTR failed.
 */
bool drbg_rand_threshold(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output, uint128_t threshold);

[[nodiscard("error must be checked"), gnu::nonnull(1, 2), gnu::hot, gnu::nothrow]]
/**
 * Generate a pseudo-random number in the range `[0,bound)` from the DRBG sequence.
 *
 * @return `true` on success, or `false` if AES CTR failed.
 */
static inline bool drbg_rand_bounded(drbg_ctr128_t *NONNULL drbg, uint128_t *NONNULL output, uint128_t bound) {
    assume(bound != 0);
    // this function is inlined so that the threshold can be constant folded,
    // since `bound` is always a constant in out code
    const uint128_t threshold = UINT128_MAX - UINT128_MAX % bound;

    uint128_t value = UINT128_MAX;
    const bool ok = drbg_rand_threshold(drbg, &value, threshold);
    if likely (ok) {
        *output = value % bound;
    }
    return ok;
}

#endif /* ENCLAVE_H */
