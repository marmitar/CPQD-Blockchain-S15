fs = import('fs')

# # # # # # # # # # # #
# SGX TOOLS AND LIBS  #

SGX_SDK = get_option('sgx_sdk')
SGX_PATH = [SGX_SDK / 'bin', SGX_SDK / 'bin/x64']
SGX_INCLUDE = SGX_SDK / 'include'
SGX_PKGCONFIG = SGX_SDK / 'pkgconfig'
SGX_LDLIBRARY = SGX_SDK / 'sdk_libs'
SGX_LIBDIR = SGX_SDK / 'lib64'

if get_option('sgx_mode') == 'auto' and not meson.is_cross_build()
    if fs.read('/proc/cpuinfo', encoding: 'utf-8').split().contains('sgx')
        SGX_SIM = ''
    else
        SGX_SIM = '_sim'
    endif
elif get_option('sgx_mode') == 'hw'
    SGX_SIM = ''
else
    SGX_SIM = '_sim'
endif

sgx_edger8r = find_program('sgx_edger8r', dirs: SGX_PATH)
sgx_sign = find_program('sgx_sign', dirs: SGX_PATH)

sgx_trts = declare_dependency(
    include_directories: [
        SGX_INCLUDE,
        SGX_INCLUDE / 'libcxx',
        SGX_INCLUDE / 'tlibc',
    ],
    compile_args: [
        '-nostdinc',
        '-fvisibility=hidden',
        '-fpie',
        '-fstack-protector',
        '-fno-builtin-printf',
        '-fwrapv', # SGX doesn't support trapping math
    ],
    link_args: [
        '-Wl,-z,relro,-z,now,-z,noexecstack',
        '-nostdlib',
        '-nodefaultlibs',
        '-nostartfiles',
        '-Wl,--whole-archive',
        f'@SGX_LIBDIR@/libsgx_trts@SGX_SIM@.a',
        '-Wl,--no-whole-archive',
        '-Wl,--start-group',
        f'@SGX_LIBDIR@/libsgx_tstdc.a',
        f'@SGX_LIBDIR@/libsgx_tcxx.a',
        f'@SGX_LIBDIR@/libsgx_tcrypto.a',
        f'@SGX_LIBDIR@/libsgx_tservice@SGX_SIM@.a',
        '-Wl,--end-group',
        '-Wl,-Bstatic',
        '-Wl,-Bsymbolic',
        '-Wl,--no-undefined',
        '-Wl,-pie,-eenclave_entry',
        '-Wl,--export-dynamic',
        '-Wl,--defsym,__ImageBase=0',
    ],
).as_system('system')

sgx_urts = declare_dependency(
    include_directories: [
        SGX_INCLUDE,
    ],
    link_args: [
        f'-L@SGX_LIBDIR@',
        f'-lsgx_urts@SGX_SIM@',
        f'-lsgx_uae_service@SGX_SIM@',
    ],
).as_system('system')

# # # # # # # # # # #
# ENCLAVE INTERFACE #

trusted_enclave = custom_target('enclave_t',
    command: [
        sgx_edger8r,
        '--search-path', '@CURRENT_SOURCE_DIR@',
        '--search-path', SGX_INCLUDE,
        '--trusted',
        '--trusted-dir', '@OUTDIR@',
        '@INPUT@'
    ],
    input: files('enclave.edl'),
    output: ['enclave_t.c', 'enclave_t.h'],
)

untrusted_enclave = custom_target('enclave_u',
    command: [
        sgx_edger8r,
        '--search-path', '@CURRENT_SOURCE_DIR@',
        '--search-path', SGX_INCLUDE,
        '--untrusted',
        '--untrusted-dir', '@OUTDIR@',
        '@INPUT@'
    ],
    input: files('enclave.edl'),
    output: ['enclave_u.c', 'enclave_u.h'],
)

# # # # # # # # # #
# ENCLAVE BINARY  #

student_name = '{'
foreach name : get_option('student_name').split()
    student_name += '"@0@@1@",'.format(name[0].to_upper(), name.substring(1).to_lower())
endforeach
student_name += '}'

write_integer_to_file = '''
import sys, struct
with open(sys.argv[1], 'wb') as file:
    file.write(struct.pack('<Q', int(sys.argv[2])))
'''

enclave_seed = custom_target('enclave_seed.bin',
    command: get_option('enclave_seed') < 0
        ? ['openssl', 'rand', '-out', '@OUTPUT@', '8']
        : ['python3', '-c', write_integer_to_file, '@OUTPUT@', '@0@'.format(get_option('enclave_seed'))],
    output: 'enclave_seed.bin',
    build_always_stale: true,
)

enclave_cfg_data = configuration_data()
enclave_cfg_data.set(
    'MAX_STRING_LENGTH', 4096,
    description: 'Maximum input string length to check',
)
enclave_cfg_data.set(
    'STUDENT_NAME', student_name,
    description: 'Name of the student to be matched.',
)
enclave_cfg_data.set_quoted(
    'SEED_FILE', enclave_seed.full_path(),
    description: 'Seed used for all challenges.',
)

configure_file(
    output: 'enclave_config.h',
    configuration: enclave_cfg_data,
)

challenges = files(
    'challenge/challenge_1.c',
    'challenge/challenge_2.c',
)

enclave_lds = files(debugging_enabled ? 'enclave_debug.lds' : 'enclave.lds')

enclave = shared_library('enclave',
    files('enclave.c'),
    challenges,
    trusted_enclave,
    include_directories: include,
    dependencies: [sgx_trts, pcg],
    link_depends: [enclave_lds],
    link_args: [
        '-Wl,--version-script=@0@'.format(enclave_lds[0].full_path()),
    ],
    name_prefix: '',
    name_suffix: 'so',
)

# # # # # # # # # # #
# ENCLAVE SIGNATURE #

enclave_pem = custom_target(
    command: ['openssl', 'genrsa', '-out', '@OUTPUT@', '-3', '3072'],
    output: 'enclave.pem',
)

generated_enclave = custom_target('enclave.signed.so',
    command: [
        sgx_sign, 'sign',
        '-key', enclave_pem,
        '-config', files('enclave.config.xml'),
        '-enclave', '@INPUT@',
        '-out', '@OUTPUT@',
    ],
    input: enclave,
    output: 'enclave.signed.so',
)
