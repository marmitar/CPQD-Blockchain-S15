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
    dependencies: [sgx_urts, math],
    link_args: ['-Wl,-z,pack-relative-relocs'],
)

custom_target('disassembly',
  input: app,
  output: 'app.s',
  command: ['objdump', '-d', '-S', '--no-show-raw-insn', '--no-addresses', '@INPUT@'],
  capture: true,
  build_by_default: true
)
