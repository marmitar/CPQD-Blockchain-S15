#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../enclave.h"
#include "defines.h"
#include "enclave_config.h"
#include "enclave_t.h"

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/**
 * Check if this byte is a space character.
 */
static bool is_whitespace(const char ch) {
    return isspace((unsigned char) ch);
}

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/**
 * Check if this byte is an uppercase letter.
 */
static bool is_uppercase_letter(const char ch) {
    return isalpha((unsigned char) ch) && isupper((unsigned char) ch);
}

[[nodiscard("pure function"), gnu::const, gnu::hot]]
/**
 * Check if this byte is an lowercase letter.
 */
static bool is_lowercase_letter(const char ch) {
    return isalpha((unsigned char) ch) && islower((unsigned char) ch);
}

/**
 * NUL-terminated byte string. Cannot be null.
 */
typedef const char *NONNULL string_t;

/**
 * NUL-terminated byte string that doesn't alias to something else.
 */
typedef const char *restrict NONNULL unique_string_t;

[[nodiscard("pure function"), gnu::pure]]
/**
 * Advance string until a non-whitespace character is found.
 *
 * @returns The adanced string, or `NULL` if `stop` is reached.
 */
static const char *NULLABLE skip_whitespace(string_t str, const string_t stop) {
    while (unlikely(is_whitespace(*str))) {
        str++;
        if unlikely (str == stop) {
#ifdef DEBUG
            printf("[DEBUG] skip_whitespace: stop reached, str=%s\n", str);
#endif
            return NULL;
        }
    }
    return str;
}

[[nodiscard("pure function"), gnu::pure]]
/**
 * Advance string to the next byte after the current name. A name starts with an uppercase letter, and is followed
 * by one or more lowercase letters.
 *
 * @returns The adanced string, or `NULL` if the name is invalid or `stop` is reached.
 */
static const char *NULLABLE consume_name(string_t str, const string_t stop) {
    // start with uppercase letter
    const string_t start = str;
    if unlikely (!is_uppercase_letter(*str)) {
#ifdef DEBUG
        printf("[DEBUG] consume_name: does not start with uppercase letter, str=%s\n", str);
#endif
        return NULL;
    }
    str++;

    // followed by lowercase letters
    while (likely(is_lowercase_letter(*str))) {
        str++;
        if unlikely (str == stop) {
#ifdef DEBUG
            printf("[DEBUG] consume_name: stop reached, str=%s\n", str);
#endif
            return NULL;
        }
    }

    // at least one lowercase letter is required
    if unlikely (str == start + 1) {
#ifdef DEBUG
        printf("[DEBUG] consume_name: single letter name, start=%s\n", start);
#endif
        return NULL;
    }

    return str;
}

[[nodiscard("pure function"), gnu::pure]]
/**
 * Check if string matches the expected name at position `i`. Ignored if no expected name is given.
 */
static bool name_matches_position(
    const char *NULLABLE str,
    size_t i,
    size_t n,
    const unique_string_t expected[const NULLABLE n]
) {
    if unlikely (expected == NULL) {
        return true;
    }

    if unlikely (i >= n) {
#ifdef DEBUG
        printf("[DEBUG] name_matches_position: nothing to match at i=%zu: str=%s\n", i, str);
#endif
        return false;
    }

    const size_t len = strlen(expected[i]);
    if unlikely (strncmp(str, expected[i], strlen(expected[i])) != 0) {
#ifdef DEBUG
        printf("[DEBUG] name_matches_position: does not match i=%zu: expected=%s, str=%s\n", i, expected[i], str);
#endif
        return false;
    }

    if unlikely (str[len] != '\0' && !is_whitespace(str[len])) {
#ifdef DEBUG
        printf("[DEBUG] name_matches_position: length does not match i=%zu: longer than %zu\n", i, len);
#endif
        return false;
    }

    return true;
}

[[nodiscard("pure function"), gnu::pure]]
/**
 * Returns `true` if `str` is a valid name and all words matches the `expected` name, ignoring whitespace.
 */
static bool match_name(const char *NULLABLE str, const size_t n, const unique_string_t expected[const NULLABLE]) {
    assume(n > 1);
    if unlikely (str == NULL) {
#ifdef DEBUG
        printf("[DEBUG] match_name: string is null\n");
#endif
        return false;
    }

    const string_t stop = str + MAX_STRING_LENGTH;

    str = skip_whitespace(str, stop);
    if unlikely (str == NULL) {
        return false;
    }

    // first name
    const char *const end_first = consume_name(str, stop);
    if unlikely (end_first == NULL) {
        return false;
    }

    size_t i = 0;
    if unlikely (!name_matches_position(str, i++, n, expected)) {
        return false;
    }
    str = end_first;

    // second or more names
    while (true) {
        const string_t start = str;

        str = skip_whitespace(str, stop);
        if unlikely (str == NULL) {
            return false;
        }

        // if no space found, then there isn't another word
        if unlikely (str == start) {
            break;
        }

        // no other name found, end of string reached
        const char *const end = consume_name(str, stop);
        if unlikely (end == NULL) {
            break;
        }

        if unlikely (!name_matches_position(str, i++, n, expected)) {
            return false;
        }

        str = end;
    }

    if unlikely (*str != '\0') {
#ifdef DEBUG
        printf("[DEBUG] match_name: unmatched content: str=%s\n", str);
#endif
        return false;
    }

    if unlikely (expected != NULL && i != n) {
#ifdef DEBUG
        printf("[DEBUG] match_name: does not match expected name: i=%zu, n=%zu\n", i, n);
#endif
    }

    return true;
}

[[nodiscard("error must be checked"), gnu::leaf, gnu::nothrow]]
/**
 * Example code.
 */
extern int ecall_name_check(const char *NULLABLE name) {
    const bool ok = match_name(name, SIZE_MAX, NULL);
    return likely(ok) ? 0 : -1;
}

[[nodiscard("error must be checked"), gnu::leaf, gnu::nothrow]]
/**
 * Challenge 1: Call the Enclave
 * -----------------------------
 *
 * Just call this function passing your full name.
 */
extern int ecall_verificar_aluno(const char *NULLABLE nome) {
    static const unique_string_t EXPECTED_NAME[] = STUDENT_NAME;
    const size_t len = sizeof(EXPECTED_NAME) / sizeof(EXPECTED_NAME[0]);

    const bool ok = match_name(nome, len, EXPECTED_NAME);
    if unlikely (!ok) {
        return -1;
    }

    printf("\n%s\n", SEPARATOR);
    printf("[ENCLAVE] DESAFIO 1 CONCLUIDO!! parabéns %s!!\n", nome);
    printf("%s\n", SEPARATOR);
    return 0;
}
