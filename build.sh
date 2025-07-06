#!/usr/bin/bash
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

make SGX_MODE=SIM SGX_DEBUG=0 all
