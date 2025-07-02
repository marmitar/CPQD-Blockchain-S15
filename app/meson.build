# # # # # # # # # # # # # #
# COMPILING THE USER APP  #

threads = dependency('threads')

challenges = files(
    'challenge/challenge_1.c',
    'challenge/challenge_2.c',
    'challenge/challenge_3.c',
    'challenge/challenge_4.c',
    'challenge/challenge_5.c',
)

app = executable('app',
    files('app.c', 'error.c'),
    challenges,
    untrusted_enclave,
    dependencies: [sgx_urts, threads],
    link_args: ['-Wl,-z,pack-relative-relocs'],
)
