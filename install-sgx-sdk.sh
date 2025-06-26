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
  printf '  %s... ' "$1"
  local cmdOutput
  if eval "cmdOutput=\$( { $2 ;} 2>&1 )" > /dev/null; then
    # Success
    echo "${bold_color}${green_color}OK${reset_color}"
  else
    # Failure
    echo "${bold_color}${error_color}FAILED${reset_color}"
    print_failed_command "$2" "${cmdOutput}"
    if [ "${1}" != 'test' ]; then
        exit 1
    fi
  fi
}

# Return the resolved physical path
#
# We need it rather than SCRIPTPATH=`pwd` in order to properly handle spaces and symlinks.
# The inclusion of output redirection (>/dev/null 2>&1) handles the rare(?) case where cd might produce output that 
# would interfere with the surrounding $( ... ) capture. Such as cd being overridden to also ls a directory after
# switching to it: https://unix.stackexchange.com/questions/20396/make-cd-automatically-ls/20413#20413
#
# Reference: https://stackoverflow.com/questions/4774054/reliable-way-for-a-bash-script-to-get-the-full-path-to-itself
absolute_dir(){
    var="$1"
    test -e "$var" || die "'$var' - file doesn't exist"
    test -d "$var" || var="$(dirname "$var")"
    var="$( cd -- "$var" >/dev/null 2>&1 || exit $? ; pwd -P )"
    test "$?" != 0 && exit 1
    echo "${var}"
}

# Convert a string to lowercase
tolower(){
    echo "$@" | tr ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz
}

# Make sure `SGX_SDK_DIR` is an absolute path
SGX_SDK_DIR="$(absolute_dir "${SGX_SDK_DIR}")"

# Check if intell SGX-SDK is already installed.
if [ -d "${SGX_SDK_DIR}/sgxsdk" ]; then
    echo "${bold_color}${green_color}SGX-SDK already installed:${reset_color} ${SGX_SDK_DIR}/sgxsdk"
    exit 0
fi

if ! command -v apt >/dev/null 2>&1
then
    die 'This script only works on debian/ubuntu'
fi

# Update system
sudo apt update -y && sudo apt upgrade -y

# Install essential tools development tools
sudo apt install -y curl wget git-all build-essential

# Install dependencies needed for compile SGX-SDK
sudo apt install -y cmake flex bison gnupg2 libelf-dev \
  libncurses5-dev libssl-dev pahole autoconf gperf autopoint \
  texinfo texi2html gettext libtool libreadline-dev libbz2-dev \
  libsqlite3-dev liblzma-dev libffi-dev libcurl4-openssl-dev \
  ocaml ocamlbuild automake python-is-python3 perl protobuf-compiler \
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

# Compile o SGX-SDK
#   USE_OPT_LIBS=0 — Compile SGXSSL and String/Math
#   USE_OPT_LIBS=1 — Compile IPP crypto and String/Math
#   USE_OPT_LIBS=2 — Compile SGXSSL without mitigation and use an optimized String/Math
#   USE_OPT_LIBS=3 — Compile IPP crypto without mitigation and use an optimized String/Math
#   DEBUG=1        — Enable Debug.
sudo make sdk -j"$(nproc)" DEBUG="${DEBUG}" USE_OPT_LIBS="${USE_OPT_LIBS}"

# Compile Intel SGX installer
sudo make sdk_install_pkg -j"$(nproc)" DEBUG="${DEBUG}"

# Extract SGX-SDK version
export SGX_SDK_VERSION="$(cat ./common/inc/internal/se_version.h | grep '#define STRFILEVER' | awk  '{print $3}' | tr -d \")"
printf "SGX SDK version: %s\n" "${SGX_SDK_VERSION}"

# Exec the installer at ./linux/installer/bin/sgx_linux_x64_sdk_XXXXXXXX.bin
# Install SGX-SDK at the directory defined by "SGX_SDK_DIR"
if [ ! -d "${SGX_SDK_DIR}" ]; then
    sudo mkdir -v "${SGX_SDK_DIR}" || die "cannot create directory '${SGX_SDK_DIR}'"
fi
printf "n\n%s\n" "${SGX_SDK_DIR}" | sudo ./linux/installer/bin/"sgx_linux_x64_sdk_${SGX_SDK_VERSION}.bin"

# Load SGX-SDK environment variables
source "${SGX_SDK_DIR}/sgxsdk/environment"

# Compile and run example code using Simulation Mode
printf "Compiling example project...\n"
cd SampleCode/LocalAttestation
make SGX_MODE=SIM || die 'failed to compile SampleCode/LocalAttestation'
cd bin
./app || die 'failed to run SampleCode/LocalAttestation'

printf "\n${bold_color}SGX SDK installed succesfully!${reset_color}\n\n"
printf 'if you wish to load sgx-sdk automatically, run following command:\n'
printf "${bold_color}echo '%s'${reset_color}\n" "source '${SGX_SDK_DIR}/sgxsdk/environment' >> ~/.bashrc"
