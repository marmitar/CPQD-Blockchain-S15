#!/bin/bash
set -e

DEBUG="${DEBUG:-0}"
if [ -z "${SGX_SDK_DIR}" ]; then
    # SGX-SDK default directory
    SGX_SDK_DIR="/opt/intel"
fi
if [ -z ${USE_OPT_LIBS+x} ]; then
    # Use IPP crypto by default
    USE_OPT_LIBS=1
fi

# Check if intell SGX-SDK is already installed.
if [ -d "${SGX_SDK_DIR}/sgxsdk" ]; then
    printf 'SGX-SDK is already installed at "%s"\n' "${SGX_SDK_DIR}/sgxsdk"
    exit 0
fi

# Update system
sudo apt update -y && sudo apt upgrade -y

# Install essential tools
sudo apt install -y vim curl wget git-all build-essential

# Install dependencies needed for compile SGX-SDK
sudo apt install -y cmake flex bison gnupg2 libelf-dev \
  libncurses5-dev libssl-dev pahole autoconf gperf autopoint \
  texinfo texi2html gettext libtool libreadline-dev libbz2-dev \
  libsqlite3-dev liblzma-dev libffi-dev libcurl4-openssl-dev \
  ocaml ocamlbuild automake python-is-python3 git perl protobuf-compiler \
  libprotobuf-dev debhelper reprepro unzip pkgconf libboost-dev  \
  libboost-system-dev libboost-thread-dev lsb-release libsystemd0 \
  indent cpuid

# Get SGX-SDK source code
if [[ ! -d ./linux-sgx ]]; then
    git clone https://github.com/intel/linux-sgx.git
fi
# Enter in linux-sgx folder
cd linux-sgx

# Make sure we are doing a clean install
make clean

# Download project depedencies
make preparation

# Use external tools for building intel SGX-SDK.
# export PATH_BACKUP="${PATH}"
# export PATH="${PWD}/external/toolset/ubuntu20.04:${PATH}"

# Compile o SGX-SDK
#   USE_OPT_LIBS=0 — Compile SGXSSL and String/Math
#   USE_OPT_LIBS=1 — Compile IPP crypto and String/Math
#   USE_OPT_LIBS=2 — Compile SGXSSL without mitigation and use an optimized String/Math
#   USE_OPT_LIBS=3 — Compile IPP crypto without mitigation and use an optimized String/Math
#   DEBUG=1        — Enable Debug.
sudo make sdk -j"$(nproc)" DEBUG="${DEBUG}" USE_OPT_LIBS="${USE_OPT_LIBS}"

# Compile Intel SGX installer
sudo make sdk_install_pkg -j"$(nproc)" DEBUG="${DEBUG}"

# Restore the PATH
# export PATH="${PATH_BACKUP}"
# unset PATH_BACKUP

# Extract SGX-SDK version
export SGX_SDK_VERSION="$(cat ./common/inc/internal/se_version.h | grep '#define STRFILEVER' | awk  '{print $3}' | tr -d \")"
printf "SGX SDK version: %s\n" "${SGX_SDK_VERSION}"

# Exec the installer at ./linux/installer/bin/sgx_linux_x64_sdk_XXXXXXXX.bin
# Install SGX-SDK at the directory defined by "SGX_SDK_DIR"
sudo mkdir "${SGX_SDK_DIR}"
printf "n\n%s\n" "${SGX_SDK_DIR}" | sudo ./linux/installer/bin/"sgx_linux_x64_sdk_${SGX_SDK_VERSION}.bin"

# Load SGX-SDK environment variables
source "${SGX_SDK_DIR}/sgxsdk/environment"

# Compile and run example code using Simulation Mode
printf "Compiling example project...\n"
cd SampleCode/LocalAttestation
make SGX_MODE=SIM
cd bin
./app

printf '\nSGX SDK installed succesfully!\n\n'
printf 'if you wish to load sgx-sdk automatically, run following command:\n'
printf "echo \"source '${SGX_SDK_DIR}/sgxsdk/environment'\" >> ~/.bashrc\n"
