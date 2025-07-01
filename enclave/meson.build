# # # # # # # # # # # #
# SGX TOOLS AND LIBS  #

SGX_SDK = get_option('sgx_sdk')
SGX_PATH = [SGX_SDK / 'bin', SGX_SDK / 'bin/x64']
SGX_INCLUDE = SGX_SDK / 'include'
SGX_PKGCONFIG = SGX_SDK / 'pkgconfig'
SGX_LDLIBRARY = SGX_SDK / 'sdk_libs'
SGX_LIBDIR = SGX_SDK / 'lib64'

if get_option('sgx_mode') == 'auto' and not meson.is_cross_build()
    fs = import('fs')
    if fs.read('/proc/cpuinfo', encoding: 'utf-8').split().contains('sgx')
        SGX_MODE = 'hw'
    else
        SGX_MODE = 'sim'
    endif
elif get_option('sgx_mode') == 'hw'
    SGX_MODE = 'hw'
else
    SGX_MODE = 'sim'
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
        SGX_MODE == 'hw' ? f'@SGX_LIBDIR@/libsgx_trts.a' : f'@SGX_LIBDIR@/libsgx_trts_sim.a',
        '-Wl,--no-whole-archive',
        '-Wl,--start-group',
        f'@SGX_LIBDIR@/libsgx_tstdc.a',
        f'@SGX_LIBDIR@/libsgx_tcxx.a',
        f'@SGX_LIBDIR@/libsgx_tcrypto.a',
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
        SGX_MODE == 'hw' ? '-lsgx_urts' : '-lsgx_urts_sim',
        SGX_MODE == 'hw' ? '-lsgx_uae_service' : '-lsgx_uae_service_sim',
    ],
).as_system('system')

# # # # # # # # # # # # #
# COMPILING THE ENCLAVE #

trusted_enclave = custom_target('trusted-enclave',
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

untrusted_enclave = custom_target('untrusted-enclave',
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

enclave_lds = files(debugging_enabled ? 'enclave_debug.lds' : 'enclave.lds')

enclave = shared_library('enclave',
    files('enclave.c'),
    trusted_enclave,
    link_depends: [enclave_lds],
    link_args: [
        '-Wl,--version-script=@0@'.format(enclave_lds[0].full_path()),
    ],
    dependencies: [sgx_trts],
    name_prefix: '',
    name_suffix: 'so',
)

enclave_pem = custom_target(
    command: ['openssl', 'genrsa', '-out', '@OUTPUT@', '-3', '3072'],
    output: 'enclave_test.pem',
)

signed_enclave = custom_target('signed-enclave',
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
