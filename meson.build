project('trabalho-pratico-intel-sgx', 'c',
    version: '1.0.0',
    license: 'AGPL-3.0-or-later',
    meson_version: '>= 1.4',
    default_options: {
        'c_std': 'c23',
        'buildtype': 'release',
        'warning_level': '2',
        'default_library': 'static',
        'prefer_static': true,
        'b_asneeded': true,
        'b_lundef': true,
        'b_lto': true,
        'b_lto_threads': 0,
        'b_lto_mode': 'default',
        'b_ndebug': 'if-release',
        'b_pie': true,
        'b_staticpic': true,
    },
)

warning_level = get_option('warning_level') == 'everything' ? 5 : get_option('warning_level').to_int()
optimizations_enabled = get_option('optimization') in ['2', '3', 's', 'z']
debugging_enabled = get_option('debug')

cc = meson.get_compiler('c')

# # # # # # # # # #
# COMPILER FLAGS  #

warnings = []
# Safety warnings
if warning_level >= 1
    warnings += cc.get_supported_arguments(
        '-Wconversion',
        '-Wsign-conversion',
        '-Wformat',
        '-Wformat=2',
        '-Wformat-signedness',
        '-Winit-self',
        '-Wpointer-arith',
        '-Wreturn-type',
        '-Waddress',
        '-Wsequence-point',
        '-Wformat-security',
        '-Wmissing-include-dirs',
        '-Wfloat-equal',
        '-Wundef',
        '-Wshadow',
        '-Wcast-align',
        '-Wcast-qual',
        '-Wconversion',
        '-Wredundant-decls',
        '-Wstrict-prototypes',
        cc.get_supported_arguments('-Wformat-truncation=2').get(0, '-Wformat-truncation'),
        cc.get_supported_arguments('-Wformat-overflow=2').get(0, '-Wformat-overflow'),
        checked: 'warn',
    ) + cc.get_supported_arguments(
        '-Wjump-misses-init', # GCC only
        '-Wstringop-overflow=4', # GCC only
        '-Wno-nullability-extension', # Clang only
        checked: 'off',
    )
endif
# Extra warnings
if warning_level >= 2
    warnings += cc.get_supported_arguments(
        '-Wunused-result',
        checked: 'warn',
    )
endif
# Pedantic warnings
if warning_level >= 3
    warnings += cc.get_supported_arguments(
        optimizations_enabled ? '-Winline' : [],
        checked: 'warn',
    ) + cc.get_supported_arguments(
        optimizations_enabled ? '-Rpass=.*' : [], # Clang only
        optimizations_enabled ? '-Rpass-missed=.*' : [], # Clang only
        checked: 'off',
    )
endif
# Disable some bugprone GCC warnings
if cc.get_id() == 'gcc'
    warnings += cc.get_supported_arguments(
        '-Wno-abi',
        '-Wno-nonnull-compare',
        '-Wno-unused-function',
        checked: 'warn',
    )
endif

optimizations = []
if optimizations_enabled
    optimizations += cc.get_supported_arguments(
        not meson.is_cross_build() ? '-march=native' : [],
        not meson.is_cross_build() ? '-mtune=native' : [],
        '-fdata-sections',
        '-ffunction-sections',
        '-fno-exceptions',
        '-fno-math-errno',
        '-fno-trapping-math',
        checked: 'warn',
    ) + cc.get_supported_arguments(
        get_option('b_lto_mode') == 'default' ? '-flto=full' : [], # Clang only
        '-fno-signaling-nans', # GCC only
        '-fallow-store-data-races', # GCC only
        '-fwhole-program', # GCC only
        checked: 'off',
    )
endif

general_codegen = cc.get_supported_arguments(
    '-pipe',
    '-fvisibility=hidden',
    debugging_enabled ? '-fstrict-overflow' : '-fno-strict-overflow',
    # _FORTIFY_SOURCE=2+ breaks static analysis on some stdlib functions for Clang
    '-D_FORTIFY_SOURCE=@0@'.format(debugging_enabled and cc.get_id() == 'gcc' ? 3 : 1),
    checked: 'warn',
)

debugging = []
if debugging_enabled
    debugging += cc.get_supported_arguments(
        '-DDEBUG',
        '-ggdb',
        '-g3',
        '-fstack-clash-protection',
        '-fcf-protection',
        '-ftrapv',
        '-fno-exceptions',
        '-fno-omit-frame-pointer',
        '-mno-omit-leaf-frame-pointer',
        checked: 'warn',
    )
endif

add_global_arguments(
    warnings,
    optimizations,
    general_codegen,
    debugging,
    language: ['c', 'cpp'],
)

linker_options = cc.get_supported_link_arguments(
    optimizations_enabled ? '-Wl,-O1' : [],
    '-Wl,--gc-sections',
    '-Wl,--sort-common',
    '-Wl,--as-needed',
    '-Wl,-z,relro',
    '-Wl,-z,now',
    '-Wl,-z,noexecstack',
)

add_global_link_arguments(
    linker_options,
    language: ['c', 'cpp'],
)

# # # # # #
# TARGETS #

subdir('enclave')
subdir('app')

# # # # #
# TESTS #

challenge_enclave = files('docs/enclave-desafio-5.signed.so')

test('challenge-enclave',
    app,
    args: [challenge_enclave],
    env: {
        'LD_LIBRARY_PATH': SGX_LDLIBRARY,
    },
    suite: ['challenge'],
)

test('generated-enclave',
    app,
    args: [generated_enclave],
    env: {
        'LD_LIBRARY_PATH': SGX_LDLIBRARY,
    },
    suite: ['generated'],
)
