#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "../enclave.h"
#include "defines.h"
#include "enclave_t.h"

/** Number of characters for the secret word. */
static constexpr size_t WORD_LEN = 20;

/**
 * The secret word, not NUL-terminated.
 */
typedef struct word {
#if __has_c_attribute(nonstring)
    [[gnu::nonstring]]  // GCC 8 and Clang 21
#endif
    /** The word content, without the NUL byte (`\0`). */
    char data[WORD_LEN];
} word_t;

/**
 * Word with all positions set to `\0`, used for initialization.
 */
static constexpr word_t EMPTY_WORD = {
    .data = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

/**
 * Check for an unitialized secret word.
 */
#define IS_EMPTY(word) unlikely((word).data[0] == '\0')

[[nodiscard("useless call otherwise"), gnu::nonnull(1)]]
/**
 * Generate a single letter from the specified list. Returns `EOF` on errors.
 */
static char generate_letter(drbg_ctr128_t *NONNULL rng) {
    constexpr char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const size_t N_LETTERS = strlen(LETTERS);

    uint128_t index = 0;
    const bool ok = drbg_rand_bounded(rng, &index, N_LETTERS);
    if unlikely (!ok) {
        return EOF;
    }

    assume(index < N_LETTERS);
    return LETTERS[index % N_LETTERS];
}

[[nodiscard("pure function"), gnu::const, gnu::cold]]
/**
 * Generate secret word from fixed seed. Returns `EMPTY_WORD` on errors.
 */
static word_t generate_secret_word(void) {
    drbg_ctr128_t rng = drbg_seeded_init(3);

    word_t secret = EMPTY_WORD;
    for (size_t i = 0; i < WORD_LEN; i++) {
        const char ch = generate_letter(&rng);
        if unlikely (ch == EOF) {
            return EMPTY_WORD;
        }
        secret.data[i] = ch;
    }
    return secret;
}

[[nodiscard("effectively pure function"), gnu::const, gnu::hot]]
/**
 * Get secret word or generate from seed. Returns `EMPTY_WORD` on errors.
 */
static word_t get_secret_word(void) {
    static word_t cache = EMPTY_WORD;
    // CONCURRENCY: although racy, the seed guarantees `generate_secret_word` always return the same value,
    // so we always write the same value. This is also why this function can be safely marked as `const`.
    if unlikely (IS_EMPTY(cache)) {
        cache = generate_secret_word();
    }
    return cache;
}

/**
 * Challenge 3: Find the Secret Word
 * ---------------------------------
 *
 * The enclave replaces wrong letters with '-' and keeps the letters you guessed correctly.
 * Returns 0 on success, negative otherwise.
 *
 * HINT: the secret word contains only uppercase letters, no spaces, diacritics or digits.
 */
extern int ecall_palavra_secreta(char palavra[NULLABLE WORD_LEN]) {
    const word_t secret = get_secret_word();
    if unlikely (IS_EMPTY(secret)) {
#ifdef DEBUG
        printf("[ENCLAVE] ecall_palavra_secreta: failed to generate secret word\n");
#endif
        return -2;
    }

    if unlikely (palavra == NULL) {
#ifdef DEBUG
        printf("[DEBUG] ecall_palavra_secreta: input is null\n");
#endif
        return -1;
    }

    bool matches_secret = true;
    for (size_t i = 0; i < WORD_LEN; i++) {
        if likely (palavra[i] != secret.data[i]) {
            palavra[i] = '-';
            matches_secret = false;
        }
    }

    if likely (!matches_secret) {
        return -1;
    }

    static_assert(WORD_LEN <= INT_MAX);
    printf("\n%s\n", SEPARATOR);
    printf("[ENCLAVE] DESAFIO 3 CONCLUIDO!! a palavra secreta é %.*s\n", (int) WORD_LEN, secret.data);
    printf("%s\n", SEPARATOR);
    return 0;
}
