#!/usr/bin/env bash
set -e

# Make sure we are in the same folder
cd "$(dirname "$0")"

# Verify if SGX-SDK environment variables are set
if [ -z "${SGX_SDK}" ]; then
    # Check if sgx-sdk is installed in the default directory
    if [ ! -d /opt/intel/sgxsdk ]; then
        printf 'sgx-sdk not found\n'
        exit 1
    fi
    # Load SGX-SDK variables and dev tools
    # shellcheck source=/dev/null
    source /opt/intel/sgxsdk/environment
fi

############
# Settings #
############

CC=/usr/bin/gcc

# Flags and dependencies used to compile the App and Enclave.
declare -a SGX_CFLAGS=(
    -m64 -O0 -g
    -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type
    -Waddress -Wsequence-point -Wformat-security
    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow
    -Wcast-align -Wcast-qual -Wconversion -Wredundant-decls
    -Wjump-misses-init -Wstrict-prototypes -Wunsuffixed-float-constants
    '-std=c99'
)
declare -a APP_CFLAGS=(
   -fPIC -Wno-attributes -IApp "-I${SGX_SDK}/include" -DNDEBUG -UEDEBUG -UDEBUG
)
declare -a ENCLAVE_CFLAGS=(
    '-nostdinc'
    '-fvisibility=hidden'
    '-fpie'
    '-fstack-protector'
    '-fno-builtin-printf'
    '-Ienclave'
    "-I${SGX_SDK}/include"
    "-I${SGX_SDK}/include/tlibc"
)
declare -a ENCLAVE_LINK_FLAGS=(
    '-Wl,-z,relro,-z,now,-z,noexecstack'
    '-Wl,--no-undefined'
    -nostdlib -nodefaultlibs -nostartfiles "-L${SGX_SDK}/lib64"
	'-Wl,--whole-archive'
    -lsgx_trts_sim
    '-Wl,--no-whole-archive'
	'-Wl,--start-group'
    -lsgx_tstdc -lsgx_tcrypto -lsgx_trts_sim
    '-Wl,--end-group'
	'-Wl,-Bstatic'
    '-Wl,-Bsymbolic'
    '-Wl,--no-undefined'
	'-Wl,-pie,-eenclave_entry'
    '-Wl,--export-dynamic'
	'-Wl,--defsym,__ImageBase=0'
	'-Wl,--version-script=enclave/enclave_debug.lds'
)
# artifacts generated during compilation
declare -a GENERATED_FILES=(
    '.config_*'
    'main'
    'enclave.so'
    'enclave.signed.so'
    'app/*.o'
    'app/enclave_u.*'
    'enclave/*.o'
    'enclave/enclave_t.*'
)

###############
# COMPILE APP #
###############

# STEP 1: remove auto-generated files.
rm -f "${GENERATED_FILES[@]}"

# STEP 2: Generates interfaces between the untrusted components and enclaves.
# output files:
# - app/enclave_u.c
# - app/enclave_u.h
pushd app
sgx_edger8r --untrusted ../enclave/enclave.edl --search-path ../enclave --search-path "${SGX_SDK}/include"
popd

# STEP 3: Compilar APP.
# output files:
# - main
$CC "${SGX_CFLAGS[@]}" "${APP_CFLAGS[@]}" -c app/enclave_u.c -o app/enclave_u.o
$CC "${SGX_CFLAGS[@]}" "${APP_CFLAGS[@]}" -c app/error.c -o app/error.o
$CC "${SGX_CFLAGS[@]}" "${APP_CFLAGS[@]}" -c app/app.c -o app/app.o
$CC app/enclave_u.o app/error.o app/app.o -o main "-L${SGX_SDK}/lib64" -lsgx_urts_sim -lpthread

###################
# COMPILE ENCLAVE #
###################

# STEP 4: Generates interfaces from enclaves to untrusted components.
# output files:
# - enclave/enclave_t.c
# - enclave/enclave_t.h
pushd enclave
sgx_edger8r --trusted ../enclave/enclave.edl --search-path ../enclave --search-path "${SGX_SDK}/include"
popd

# STEP 5: Compile ENCLAVE
# output files:
# - enclave.so
$CC "${SGX_CFLAGS[@]}" "${ENCLAVE_CFLAGS[@]}" -c enclave/enclave_t.c -o enclave/enclave_t.o
$CC "${SGX_CFLAGS[@]}" "${ENCLAVE_CFLAGS[@]}" -c enclave/enclave.c -o enclave/enclave.o
$CC enclave/enclave_t.o enclave/enclave.o -o enclave.so "${ENCLAVE_LINK_FLAGS[@]}"

################
# SIGN ENCLAVE #
################

# STEP 6: Generate test signing key
# output files:
# - enclave/enclave_private_test.pem
if [ ! -e enclave/enclave_private_test.pem ]; then
    openssl genrsa -out enclave/enclave_private_test.pem -3 3072
fi

# STEP 7: Sign ENCLAVE
# output files:
# - enclave.signed.so
sgx_sign sign \
    -key enclave/enclave_private_test.pem \
    -enclave enclave.so \
    -out enclave.signed.so \
    -config enclave/enclave.config.xml
