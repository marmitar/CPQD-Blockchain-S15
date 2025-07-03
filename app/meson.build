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

math = cc.find_library('m', required : false)

app = executable('app',
    files('app.c', 'error.c'),
    challenges,
    untrusted_enclave,
    dependencies: [sgx_urts, threads, math],
    link_args: ['-Wl,-z,pack-relative-relocs'],
)

custom_target('disassembly',
  input: app,
  output: 'app.s',
  command: ['objdump', '-d', '-S', '--no-show-raw-insn', '--no-addresses', '@INPUT@'],
  capture: true,
  build_by_default: true
)
