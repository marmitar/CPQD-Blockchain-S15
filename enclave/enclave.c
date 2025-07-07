#include <sgx_error.h>
#include <stdarg.h>
#include <stdio.h>

#include "./enclave.h"
#include "defines.h"
#include "enclave_t.h"

/**
 * Example code.
 */
extern int ecall_name_check(const char *NULLABLE name) {
    (void) name;
    return -1;
}

/**
 * Challenge 1: Call the Enclave
 * -----------------------------
 *
 * Just call this function passing your full name.
 */
extern int ecall_verificar_aluno(const char *NULLABLE nome) {
    (void) nome;
    return -1;
}

/**
 * Challenge 2: Crack the Password
 * -------------------------------
 *
 * Returns 0 if the password is right, negative otherwise.
 *
 * HINT: the password is an integer between 0 and 99999.
 */
extern int ecall_verificar_senha(unsigned int senha) {
    (void) senha;
    return -1;
}

/** Number of characters for the secret word. */
#define WORD_LEN 20

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
    for (unsigned i = 0; i < WORD_LEN; i++) {
        palavra[i] = '-';
    }
    return -1;
}

/**
 * Challenge 4: Secret Polynomial
 * ------------------------------
 *
 * this function returns ((x*x*a) + (x*b) + c) % 2147483647
 * Assumption: -10^8 < (a + b + c) < 10^8
 *
 * Use it to help you discover the polynomial before calling `ecall_verificar_polinomio`.
 * NOTE: this ECALL aborts if you pass zero.
 *
 * HINT: the prime 2147483647 is irrelevant except when you supply *       a very large x.
 */
extern int ecall_polinomio_secreto(int x) {
    (void) x;
    return 0;
}

/**
 * Challenge 4: Secret Polynomial
 * ------------------------------
 *
 * Verify the polynomial coefficients.
 *
 * HINT: -10^8 < (a + b + c) < 10^8.
 * HINT: the function is deliberately hard to brute-force.
 */
extern int ecall_verificar_polinomio(int a, int b, int c) {
    (void) (a + b + c);
    return 0;
}

/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Play 20 rounds of rock-paper-scissors against the enclave. You must win all 20 rounds.
 *
 * How it works:
 *   1. The enclave picks rock (0), paper (1) or scissors (2).
 *   2. It ALWAYS plays the same move in round 1.
 *   3. It calls `ocall_pedra_papel_tesoura`, passing the current round number, counting 1, 2, 3... up to 20.
 *   4. It compares the moves; if you win, it increments your win count.
 *   5. The enclave's moves are deterministic, but the result of the previous round INFLUENCES its next move.
 *   6. After round 20 the enclave returns how many times YOU won. If the return value is 20 the challenge is
 *      complete and the console prints every round and outcome.
 *
 * - Returns -1 if your OCALL returns anything other than 0, 1 or 2.
 * - The enclave aborts if your OCALL fails or aborts.
 *
 * HINT: the strategy is deterministic; as long as the sequence of previous results is the same, the enclave
 *  plays the same moves.
 **/
extern int ecall_pedra_papel_tesoura(void) {
    return -1;
}

/**
 * `printf`-like function for the enclave. Buffer limited to `BUFSIZ` (8192) bytes.
 */
int printf(const char *NONNULL fmt, ...) {
    char buf[BUFSIZ] = "";

    va_list ap;
    va_start(ap, fmt);
    const int written = vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);

    if unlikely (written <= 0) {
        return written;
    }

    const sgx_status_t status = ocall_print_string(buf);
    if unlikely (status != SGX_SUCCESS) {
        return -1;
    }

    constexpr int MAX_BYTES = likely(BUFSIZ > 0) ? BUFSIZ - 1 : 0;
    return likely(written < MAX_BYTES) ? written : MAX_BYTES;
}
