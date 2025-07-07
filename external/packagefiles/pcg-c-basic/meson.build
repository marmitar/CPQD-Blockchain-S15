project('pcg-c-basic', 'c',
	license: 'Apache 2.0',
	version: '1',
    meson_version: '>= 1.2',
)

install_headers('pcg_basic.h')

pcg_c_lib = static_library('pcg',
    extra_files: files('pcg_basic.h'),
    sources: files('pcg_basic.c'),
    c_args: ['-w'],
)

pcg_c_dep = declare_dependency(
    include_directories: include_directories('.'),
    extra_files: files('pcg_basic.h'),
    link_with: [pcg_c_lib],
)
