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

[[nodiscard("pure function"), gnu::const]]
static word_t empty_word(void) {
    word_t empty = {0};
    memset(&empty, 0, sizeof(empty));
    return empty;
}

[[nodiscard("useless call otherwise"), gnu::nonnull(1)]]
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
 * Generate secret word from fixed seed.
 */
static word_t secret_word(void) {
    drbg_ctr128_t rng = drbg_seeded_init(3);

    word_t secret = empty_word();
    for (size_t i = 0; i < WORD_LEN; i++) {
        const char ch = generate_letter(&rng);
        if unlikely (ch == EOF) {
            return empty_word();
        }
        secret.data[i] = ch;
    }
    return secret;
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
    static word_t secret = {0};
    static bool initialized = false;
    if unlikely (!initialized) {
        secret = secret_word();
        if unlikely (secret.data[0] == '\0') {
            return -2;
        }
        initialized = true;
    }

    if unlikely (palavra == NULL) {
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
    printf(
        // clang-format off
        "\n"
        "------------------------------------------------\n"
        "\n"
        "DESAFIO 3 CONCLUIDO!! a palavra secreta é %*s\n"
        "\n"
        "------------------------------------------------\n"
        "\n",
        // clang-format on
        (int) WORD_LEN,
        secret.data
    );
    return 0;
}
