# # # # # # # # # # # # # #
# COMPILING THE USER APP  #

threads = dependency('threads')

app = executable('app',
    files('app.c', 'error.c'),
    untrusted_enclave,
    dependencies: [sgx_urts, threads],
    link_args: ['-Wl,-z,pack-relative-relocs'],
)
