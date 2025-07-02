#include <sgx_eid.h>
#include <sgx_error.h>
#include <stddef.h>
#include <string.h>

#include "../defines.h"
#include "./challenges.h"
#include "enclave_u.h"

/** Number of characters for the secret word. */
#define WORD_LEN 20

/**
 * A contender for the secret word, not NUL-terminated.
 */
typedef struct word {
#if __has_c_attribute(nonstring)
    [[gnu::nonstring]]  // GCC 8 and Clang 21
#endif
    /** The word content, without the NUL byte (`\0`). */
    char data[WORD_LEN];
} word_t;

[[gnu::const, nodiscard("pure function")]]
/**
 * Create a word formed by repeating `letter` in all `WORD_LEN` positions.
 */
static word_t make_word(char letter) {
    word_t word = {0};
    memset(word.data, letter, WORD_LEN);
    return word;
}

[[gnu::const, nodiscard("pure function")]]
/**
 * Replace `secret` with the given `letter` in all positions modified in `guess`, returning the updated `secret`.
 */
static word_t update_secret(word_t secret, const word_t guess, const char letter) {
    for (unsigned i = 0; i < WORD_LEN; i++) {
        if unlikely (guess.data[i] != secret.data[i]) {
            secret.data[i] = letter;
        }
    }
    return secret;
}

/**
 * Challenge 3: Secret Sequence
 * ----------------------------
 *
 * Test all valid letters in each position, until the correct letter is found. This is similar to brute-force,
 * except that each position is tested independently, allowing for per letter "parallelism". In total, only 26
 * calls to `ecall_palavra_secreta` or less are required.
 */
extern sgx_status_t challenge_3(sgx_enclave_id_t eid) {
    static const char LETTERS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static const size_t N_LETTERS = strlen(LETTERS);

    word_t secret = make_word(LETTERS[0]);

    for (size_t i = 0; i < N_LETTERS; i++) {
        word_t guess = secret;

        int rv = -1;
        const sgx_status_t status = ecall_palavra_secreta(eid, &rv, guess.data);
        if unlikely (status != SGX_SUCCESS) {
            return status;
        }

        if (rv == 0) {
            return SGX_SUCCESS;
        }

        if likely (i + 1 < N_LETTERS) {
            secret = update_secret(secret, guess, LETTERS[i + 1]);
        }
    }

    // TODO: print error?
    return SGX_ERROR_UNEXPECTED;
}
