# # # # # # # # # # # # # #
# COMPILING THE USER APP  #

math = cc.find_library('m', required : false)

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
    include_directories: include,
    dependencies: [sgx_urts, math, pcg],
    link_args: ['-Wl,-z,pack-relative-relocs'],
)

custom_target('app.s',
  input: app,
  output: 'app.s',
  command: ['objdump', '-d', '-S', '--no-show-raw-insn', '--no-addresses', '@INPUT@'],
  capture: true,
  build_by_default: true,
)
