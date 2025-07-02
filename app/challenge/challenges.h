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

#endif  // APP_CHALLENGES_H
