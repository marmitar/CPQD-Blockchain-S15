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

#endif  // APP_CHALLENGES_H
