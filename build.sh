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
    '-std=c23'
)
declare -a APP_CFLAGS=(
   -fPIC -Wno-attributes -IApp "-I${SGX_SDK}/include" -DNDEBUG -UEDEBUG -UDEBUG
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

################
# SIGN ENCLAVE #
################

# STEP 4: Get signed ENCLAVE
# output files:
# - enclave.signed.so
cp -f enclave/enclave.signed.so .
