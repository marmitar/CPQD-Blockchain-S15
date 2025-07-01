# Intel SGX Template Project

Minimal `C` project intended to be used as template project to get started with Intel SGX.

## Development

Enable [pre-commit](https://pre-commit.com/):

```sh
pre-commit install
```

## Building

First make sure you have the latest [linux-sgx-sdk](https://github.com/intel/linux-sgx) installed, you can follow the
instructions on their github page.

To compile APP and/or ENCLAVE, use one of the following options:

```sh
# Prepare meson
meson setup build

# Compile APP and ENCLAVE in Simulation+Release mode
meson compile -C build

# Compile ONLY the APP in Simulation+Release mode
meson compile -C build app

# Switch to Hardware+Debug mode
meson configure --buildtype debugoptimized --debug -D sgx_mode=hw build

# Compile ENCLAVE in Hardware+Debug Mode
make compile -C build signed-enclave
```

Format the code using [clang-format](https://clang.llvm.org/docs/ClangFormat.html)

```sh
ninja -C build clang-format
```

Run the App

```sh
meson test -C build
```

Or manually

```sh
ln -sf build/enclave/enclave.signed.so .
source /opt/intel/sgxsdk/environment
build/app/app
```

## Check SGX Hardware Support

Use [ark.intel](https://www.intel.com/content/www/us/en/ark/featurefilter.html?productType=873) to check if your Intel
CPU model is listed, but even if your CPU supports SGX it doesn't mean it is enabled, the SGX is usually disabled by
default and must be enabled in the computer BIOS.

One easy way to check if your linux machine have SGX enabled, is using dmesg:

```sh
sudo dmesg | grep -i sgx
```

But dmesg will show nothing if your machine doesn't support it, another alternative is CPUID.

```sh
# Install CPUID tool
sudo apt install cpuid -y

# Check if SGX is enabled.
sudo cpuid | grep -i sgx
```

## Project Structure

- **app/\*:** Untrusted Component Code
  - **app.c:** Application entry point, register and calls the enclave.
  - **error.c:** Prints the
    [sgx_status_t](https://github.com/intel/linux-sgx/blob/sgx_2.26/common/inc/sgx_error.h#L37-L127) error message.
- **enclave/\*:** Trusted Component Code
  - **enclave.c:** Enclave ECALLS implementation.
  - **enclave.edl:** Enclave Trusted and Untrusted input types boundaries, OCALLS and ECALLS definitions. (see
    [Enclave Definition Language - EDL](https://cdrdv2-public.intel.com/671446/input-types-and-boundary-checking-edl.pdf))
  - **enclave.lds** and **enclave_debug.lds**: Linkers for hardware and simulation mode, for more detals read the
    section [about enclave/\*.lds files](#about-enclavelds-files).
  - **enclave.config.xml:** XML file containing the user defined parameters of an enclave, for more detals read the
    section [Enclave XML Configuration File](#enclave-xml-configuration-file).
- **build.sh:** Build script, do the same as `make SGX_MODE=SIM`, but is easier to read and learn the compilation
  process step-by-step.
- **setup-sgx.sh:** Install SGX-SDK in a linux machine, obs: only tested on Ubuntu 24.04.
- **.vscode/c_cpp_properties.json:** intellisense
  [c_cpp_properties.json](https://code.visualstudio.com/docs/cpp/customize-cpp-settings) for vscode c/c++
  auto-completion.

## Enclave XML Configuration File

The enclave configuration file is an XML file containing the user defined parameters of an enclave. This XML file is a
part of the enclave project. A tool named sgx_sign uses this file as an input to create the signature and metadata for
the enclave. Here is an example of the configuration file.

```xml
<EnclaveConfiguration>
    <ProdID>100</ProdID>
    <ISVSVN>1</ISVSVN>
    <StackMaxSize>0x50000</StackMaxSize>
    <StackMinSize>0x2000</StackMinSize>
    <HeapMaxSize>0x100000</HeapMaxSize>
    <HeapMinSize>0x40000</HeapMinSize>
    <HeapInitSize>0x80000</HeapInitSize>
    <TCSNum>3</TCSNum>
    <TCSMaxNum>4</TCSMaxNum>
    <TCSMinPool>2</TCSMinPool>
    <TCSPolicy>1</TCSPolicy>
    <DisableDebug>0</DisableDebug>
    <MiscSelect>0</MiscSelect>
    <MiscMask>0xFFFFFFFF</MiscMask>
    <EnableKSS>1</EnableKSS>
    <ISVEXTPRODID_H>1</ISVEXTPRODID_H>
    <ISVEXTPRODID_L>2</ISVEXTPRODID_L>
    <ISVFAMILYID_H>3</ISVFAMILYID_H>
    <ISVFAMILYID_L>4</ISVFAMILYID_L>
</EnclaveConfiguration>
```

## About enclave\*.lds files

The symbol `enclave_entry` is the entry point to the enclave. The symbol `g_global_data_sim` comes from the **tRTS
simulation library** and is required to be exposed for running an enclave in the simulation mode since it distinguishes
between enclaves built to run on the simulator and on the hardware. The `sgx_emmt` tool relies on the symbol
`g_peak_heap_used` to determine the size of the heap that the enclave uses and relies on the symbol
`g_peak_rsrv_mem_committed` to determine the size of the reserved memory that the enclave uses. The symbol `__ImageBase`
is used by tRTS to compute the base address of the enclave.

```
// file: enclave/enclave.lds
enclave.so
{
    global:
        g_global_data_sim;
        g_global_data;
        enclave_entry;
    local:
        *;
};
```

For more details read the chapter **Setting up an Intel® Software Guard Extensions Project** at
[Intel SGX Developer Reference Linux 2.26 Open Source](https://download.01.org/intel-sgx/sgx-linux/2.26/docs/Intel_SGX_Developer_Reference_Linux_2.26_Open_Source.pdff).

### References

- [Intel SGX Developer Reference Linux](https://download.01.org/intel-sgx/sgx-linux/2.26/docs/Intel_SGX_Developer_Reference_Linux_2.26_Open_Source.pdf)
- [Enclave Definition Language - EDL](https://cdrdv2-public.intel.com/671446/input-types-and-boundary-checking-edl.pdf)
- [SGX LINUX 2.26 all docs](https://download.01.org/intel-sgx/sgx-linux/2.26/docs/)
