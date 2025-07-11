#ifndef DEFINES_H
/** Utility macros. */
#define DEFINES_H

#include <assert.h>

// clang-format off
#if __STDC_VERSION__ < 202000L
#   error "This code is compliant with C2x/C23 or later only."
#endif
// clang-format on

#if defined(__clang__)
/**
 * Indicates that the pointer is possibly `NULL`.
 * @see https://clang.llvm.org/docs/AttributeReference.html#nullable
 */
#    define NULLABLE _Nullable
/**
 * Indicates that the pointer should never be `NULL`.
 * @see https://clang.llvm.org/docs/AttributeReference.html#nonnull
 */
#    define NONNULL _Nonnull
// For completeness.
#    define UNSPECIFIED _Null_unspecified
#else  // GCC
// These pointer modifiers are specific to Clang, and are left as comments for readers on GCC.
#    define NULLABLE
#    define NONNULL
#    define UNSPECIFIED
#endif

/**
 * Marker for a branch that should be taken often or should be optimized for. Usually the "happy" case.
 * Probability is assumed to be 90%.
 */
#define likely(x) (__builtin_expect((x), 1))
/**
 * Marker for a branch that should be taken rarely or should be optimized against. Usually the error path.
 * Probability is assumed to be 10%.
 */
#define unlikely(x) (__builtin_expect((x), 0))

/** Stringification macro. */
#define STR(...) STR_(__VA_ARGS__)
// https://gcc.gnu.org/onlinedocs/gcc-4.8.5/cpp/Stringification.html
#define STR_(...) #__VA_ARGS__

#if defined(__clang__)
/** Adds compiler hints. Checked on DEBUG builds. */
#    define assume(condition) ((assert(condition)), __builtin_assume(condition))
#else  // GCC
/** Adds compiler hints. Checked on DEBUG builds. */
#    define assume(condition) \
        assert(condition);    \
        [[gnu::assume(condition)]]
#endif

#endif  // DEFINES_H
