#ifndef APP_CHALLENGES_H
/** Challenge soltions. */
#define APP_CHALLENGES_H

#include <sgx_eid.h>
#include <sgx_error.h>

[[gnu::nothrow, gnu::leaf]]
/**
 * Challenge 1: Call the enclave
 * -----------------------------
 *
 * Modify the project to call the `ecall_verificar_aluno` ecall from the `enclave-atividade.signed.so` enclave. It
 * receives a string as a parameter.
 */
sgx_status_t challenge_1(sgx_enclave_id_t eid);

[[gnu::nothrow, gnu::leaf]]
/**
 * Challenge 2: Crack the password
 * -------------------------------
 *
 * The `ecall_verificar_senha` ecall receives an integer as a parameter that represents a password. If you provide the
 * correct password, it will return `0`, or `-1` if you are incorrect.
 *
 * The password is a random integer of up to 5 digits, meaning a number between `0` and `99999`. Your task is to
 * discover the password.
 */
sgx_status_t challenge_2(sgx_enclave_id_t eid);

[[gnu::nothrow, gnu::leaf]]
/**
 * Challenge 3: Secret Sequence
 * ----------------------------
 *
 * The `ecall_palavra_secreta` ecall receives a `char[20]` array of 20 letters as a parameter. The enclave has a
 * **sequence of 20 secret letters** stored within it, composed only of **uppercase letters**. Every time you call
 * this enclave passing an array of letters, it will replace ALL the letters you got wrong in your array with the
 * character `-` (hyphen), but it will leave the letters in the positions you got right intact. Example:
 *
 * - You called `ecall_palavra_secreta` with the character array `ABCDEZHIKLMNOPQRSTVX`
 * - The enclave changed the array, and now it is equal to `---DE--I---N------V-`, which means you got 5 letters
 *  correct.
 * - You kept the letters you got right and now sent `QAEDESEIIUJNLUMMWDVJ`
 * - The enclave changed the array, and now it is equal to `---DE--I---N----W-V-`. You found a new letter!
 * - And so on, until you discover the entire secret phrase.
 */
sgx_status_t challenge_3(sgx_enclave_id_t eid);

[[gnu::nothrow, gnu::leaf]]
/**
 * Challenge 4: Secret Polynomial
 * ------------------------------
 *
 * The `ecall_polinomio_secreto` ecall receives a number `x` as a parameter and returns the result of the quadratic
 * equation `(ax^2 + bx + c) % p`, where `a`, `b`, and `c` are secret values and `p` is the prime number
 * [2147483647](https://en.wikipedia.org/wiki/2,147,483,647) (the largest 32-bit prime). You can call this ocall
 * with any value except zero; if you pass zero, the call will fail. Your task is to discover `a`, `b`, and `c`.
 */
sgx_status_t challenge_4(sgx_enclave_id_t eid);

[[gnu::nothrow, gnu::leaf]]
/**
 * Challenge 5: Rock, Paper, Scissors
 * ----------------------------------
 *
 * Replace the project's **enclave-atividade.signed.so** and **enclave-atividade.edl** with the ones provided above.
 * You do not need to create a new project. The enclave for this challenge has the same interface as the previous one;
 * it just adds two functions:
 *
 * - `ecall_pedra_papel_tesoura`
 * - `ocall_pedra_papel_tesoura`
 *
 * In the `app/app.c` code, create and implement the `ocall_pedra_papel_tesoura` ocall. This ocall will be called
 * by the enclave when you call the `ecall_pedra_papel_tesoura` ecall. The enclave will call your ocall 20 times,
 * and it must always return an integer between 0 and 2, where `rock = 0`, `paper = 1`, and `scissors = 2`.
 * (Note: if you do not implement this ocall, the execution will fail). At the end of the execution, the enclave will
 * return an integer representing how many matches you won, or a **negative value** if any problem occurred. You must
 * win all 20 matches to complete the challenge. The submission is all of the enclave's plays and your plays, which
 * will be printed to the terminal when the challenge is completed. Example:
 *
 *
 * ```raw
 * ------------------------------------------------
 * [ENCLAVE] DESAFIO 5 CONCLUIDO!!
 *           ENCLAVE JOGADAS: 11212201212200121120
 *              SUAS JOGADAS: 22020012020011202201
 * ------------------------------------------------
 * ```
 */
sgx_status_t challenge_5(sgx_enclave_id_t eid);

#endif  // APP_CHALLENGES_H
