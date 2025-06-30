#!/usr/bin/env bash
set -e

DEBUG="${DEBUG:-0}"
if [ -z "${SGX_SDK_DIR}" ]; then
    if [ -z "${SGX_SDK}" ]; then
        SGX_SDK_DIR="$(dirname "${SGX_SDK}")"
    else
        # SGX-SDK default directory
        SGX_SDK_DIR="/opt/intel"
    fi
fi
if [ -z ${USE_OPT_LIBS+x} ]; then
    # Use IPP crypto by default
    USE_OPT_LIBS=3
fi

# Check if this terminal supports colors
# Setup console colors
if test -t 1 && command -v tput >/dev/null 2>&1; then
    ncolors=$(tput colors)
    if test -n "${ncolors}" && test "${ncolors}" -ge 8; then
        bold_color=$(tput bold)
        green_color=$(tput setaf 2)
        warn_color=$(tput setaf 3)
        error_color=$(tput setaf 1)
        reset_color=$(tput sgr0)
    fi
    # 72 used instead of 80 since that's the default of pr
    ncols=$(tput cols)
fi
: "${ncols:=72}"

# Print a warning message in yellow
warn(){
    echo "${warn_color}${bold_color}${*}${reset_color}"
}

# Print an error message in red then exit
die(){
    echo "${error_color}${bold_color}${*}${reset_color}"
    exit 1
}

# Print the command in yellow, and the output in red
print_failed_command() {
  printf >&2 "${bold_color}${warn_color}%s${reset_color}\n${error_color}%s${reset_color}\n" "$1" "$2"
}

# Execute a command and only print it if it fails
exec_cmd() {
  printf '%s ' "$1"
  local cmdOutput
  if eval "cmdOutput=\$( { $1 ;} 2>&1 )" > /dev/null; then
    # Success
    echo "${bold_color}${green_color}OK${reset_color}"
  else
    # Failure
    echo "${bold_color}${error_color}FAILED${reset_color}"
    print_failed_command "$1" "${cmdOutput}"
    exit 1
  fi
}

# Convert a string to lowercase
tolower(){
    echo "$@" | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz
}

# Display a message to load SGX-SDK environment if not set.
show_sgx_setup_cmd(){
    if [ -z "$SGX_SDK" ]; then
        echo "${bold_color}please run: ${warn_color}source ${SGX_SDK_DIR}/sgxsdk/environment${reset_color}"
    fi
}

# Make sure `SGX_SDK_DIR` is an absolute path
realpath "${SGX_SDK_DIR}" > /dev/null 2>&1  || die "invalid SGX_SDK_DIR: '${SGX_SDK_DIR}'"
SGX_SDK_DIR="$(realpath "${SGX_SDK_DIR}")"

# Check if intell SGX-SDK is already installed.
if [ -d "${SGX_SDK_DIR}/sgxsdk" ]; then
    echo "${bold_color}${green_color}SGX-SDK already installed:${reset_color} ${SGX_SDK_DIR}/sgxsdk"
    show_sgx_setup_cmd
    exit 0
fi
# Check if intell SGX-SDK is already installed.
if [ -f "/opt/intel/sgxsdk/environment" ]; then
    SGX_SDK_DIR='/opt/intel'
    echo "${bold_color}${green_color}SGX-SDK already installed:${reset_color} ${SGX_SDK_DIR}/sgxsdk"
    show_sgx_setup_cmd
    exit 0
fi

if ! command -v apt >/dev/null 2>&1
then
    die 'This script only works on debian/ubuntu'
fi

# Update system
exec_cmd 'set -eux; sudo apt-get update -y'
exec_cmd 'set -eux; sudo apt-get upgrade -y'

# Install essential development tools
exec_cmd 'set -eux; sudo apt-get install -y --no-install-recommends curl wget git-all build-essential indent cpuid'

# Install dependencies needed for compile SGX-SDK
exec_cmd \
    'sudo apt-get install -y --no-install-recommends \
    cmake flex bison gnupg2 libelf-dev libncurses5-dev libssl-dev \
    pahole autoconf gperf autopoint texinfo texi2html gettext \
    libtool libreadline-dev libbz2-dev ocaml libsqlite3-dev \
    liblzma-dev libffi-dev libcurl4-openssl-dev ocamlbuild automake \
    python-is-python3 perl protobuf-compiler libprotobuf-dev \
    debhelper reprepro unzip pkgconf libboost-dev libboost-system-dev \
    libboost-thread-dev lsb-release libsystemd0'

# Get SGX-SDK source code
if [[ ! -d ./linux-sgx ]]; then
    exec_cmd 'git clone https://github.com/intel/linux-sgx.git'
fi

# Enter in linux-sgx directory
pushd linux-sgx > /dev/null 2>&1

# Make sure we are doing a clean install
exec_cmd 'make clean'

# Download project depedencies
exec_cmd 'make preparation'

# USE_OPT_LIBS=0 — Compile SGXSSL and String/Math
# USE_OPT_LIBS=1 — Compile IPP crypto and String/Math
# USE_OPT_LIBS=2 — Compile SGXSSL without mitigation and use an optimized String/Math
# USE_OPT_LIBS=3 — Compile IPP crypto without mitigation and use an optimized String/Math
# DEBUG=1        — Enable Debug.
if [[ "${USE_OPT_LIBS}" == '0' || "${USE_OPT_LIBS}" == '1' ]]; then
    # Compile o SGX-SDK
    exec_cmd "sudo make sdk -j$(nproc) DEBUG='${DEBUG}' USE_OPT_LIBS='${USE_OPT_LIBS}'"
    
    # Compile Intel SGX installer
    # Obs: compiling with -j1, because protobuf sometimes fails when building in parallel.
    echo "sudo make sdk_install_pkg -j1 DEBUG='${DEBUG}'"
    sudo make sdk_install_pkg -j1 DEBUG="${DEBUG}"
else
    # Compile o SGX-SDK
    exec_cmd "make sdk_no_mitigation -j$(nproc) DEBUG='${DEBUG}' USE_OPT_LIBS='${USE_OPT_LIBS}'"
    
    # Compile Intel SGX installer
    # Obs: compiling with -j1, because protobuf sometimes fails when building in parallel.
    echo "make sdk_install_pkg -j1 DEBUG='${DEBUG}'"
    make sdk_install_pkg -j1 DEBUG="${DEBUG}"
fi

# Extract SGX-SDK version
export SGX_SDK_VERSION="$(cat ./common/inc/internal/se_version.h | grep '#define STRFILEVER' | awk  '{print $3}' | tr -d \")"
printf "${bold_color}SGX SDK version: ${green_color}%s${reset_color}\n" "${SGX_SDK_VERSION}"

# Exec the installer at ./linux/installer/bin/sgx_linux_x64_sdk_XXXXXXXX.bin
# Install SGX-SDK at the directory defined by "SGX_SDK_DIR"
if [ ! -d "${SGX_SDK_DIR}" ]; then
    exec_cmd "sudo mkdir -v '${SGX_SDK_DIR}'"
    # die "cannot create directory '${SGX_SDK_DIR}'"
fi
printf 'sudo %s\n' "./linux/installer/bin/sgx_linux_x64_sdk_${SGX_SDK_VERSION}.bin"
printf "n\n%s\n" "${SGX_SDK_DIR}" | sudo ./linux/installer/bin/"sgx_linux_x64_sdk_${SGX_SDK_VERSION}.bin"

# Load SGX-SDK environment variables
source "${SGX_SDK_DIR}/sgxsdk/environment" || die "failed to load sdk env: '${SGX_SDK_DIR}/sgxsdk/environment'"

# Compile and run example code using Simulation Mode
printf "Compiling example project...\n"
pushd SampleCode > /dev/null 2>&1 || die "directory not found: $(pwd)/SampleCode"
pushd LocalAttestation > /dev/null 2>&1 || die "sample project not found: $(pwd)/LocalAttestation"
exec_cmd 'make SGX_MODE=SIM'
pushd bin > /dev/null 2>&1 || die "directory not found: '$(pwd)/bin'"
./app > /dev/null 2>&1 || die "sgx example code failed: '$(pwd)/app'"

# SUCCESS, print configuration options
printf "\n${bold_color}${green_color}SGX SDK installed succesfully!${reset_color}\n\n"
printf 'if you wish to load sgx-sdk tools automatically, run following command:\n'
if [ -e ~/.zshenv ]; then
    SHELL_ENV_FILE='~/.zshenv'
elif [ -e ~/.zshrc ]; then
    SHELL_ENV_FILE='~/.zshrc'
else
    SHELL_ENV_FILE='~/.bashrc'
fi
printf "${bold_color}echo %s${reset_color}\n" \
    "\"[ -d '${SGX_SDK_DIR}/sgxsdk' ] && source '${SGX_SDK_DIR}/sgxsdk/environment'\" >> $SHELL_ENV_FILE"
