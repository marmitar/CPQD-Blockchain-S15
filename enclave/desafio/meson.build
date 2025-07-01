# # # # # # # # # # # #
# CHANLLENGE ENCLAVE  #

untrusted_enclave_desafio = custom_target('untrusted-enclave-desafio',
    command: [
        sgx_edger8r,
        '--search-path', '@CURRENT_SOURCE_DIR@',
        '--search-path', SGX_INCLUDE,
        '--untrusted',
        '--untrusted-dir', '@OUTDIR@',
        '--header-only',
        '@INPUT@'
    ],
    input: files('enclave_desafio.edl'),
    output: ['enclave_desafio_u.h'],
)

fs = import('fs')

signed_enclave_desafio = fs.copyfile(
    files('enclave_desafio.signed.so'),
    'enclave.signed.so',
)
