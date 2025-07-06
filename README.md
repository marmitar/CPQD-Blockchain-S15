# Intel SGX Challenges

Solution to selected challenges implemented in a secure SGX enclave.

## Challenges

### Challenge 1: Call the enclave

Use `ecall_verificar_aluno` to call the enclave with your name.

### Challenge 2: Crack the password

Find the password (between 0 and 99999 inclusive) for which `ecall_verificar_senha` returns 0.

### Challenge 3: Secret Sequence

Find the correct 20 letter string (with A-Z) for `ecall_palavra_secreta`.

### Challenge 4: Secret Polynomial

Use `ecall_polinomio_secreto` to find the coefficients for a polynomial $(a x^2 + b x + c) \text{ mod } p$, where $p$ is
[2147483647](https://en.wikipedia.org/wiki/2,147,483,647) and $-10^8 < a + b + c < 10^8$. Verify with
`ecall_verificar_polinomio`.

### Challenge 5: Rock, Paper, Scissors

Implement a `unsigned int ocall_pedra_papel_tesoura(unsigned int round)` that returns `0` (rock), `1` (paper) or `2`
(scissors) for each round, and beat `ecall_pedra_papel_tesoura` for all 20 rounds.

## Building

First make sure you have the latest [linux-sgx-sdk](https://github.com/intel/linux-sgx) installed, you can follow the
instructions on their github page.

To compile APP and/or setup the ENCLAVE, use one of the following options:

```sh
# Prepare meson
meson setup build

# Compile APP and ENCLAVE in Simulation+Release mode
meson compile -C build

# Compile ONLY the APP in Simulation+Release mode
meson compile -C build app

# Switch to Hardware+Debug mode
meson configure build --buildtype debugoptimized -D debug=true -D sgx_mode=hw

# Copy ENCLAVE
make compile -C build enclave.signed.so
```

Format the code using [clang-format](https://clang.llvm.org/docs/ClangFormat.html)

```sh
# CC=clang only
ninja -C build clang-format
```

Run the App

```sh
meson test -C build --verbose
```

Or manually

```console
> source /opt/intel/sgxsdk/environment
> build/app/app enclave/enclave.signed.so

------------------------------------------------

[ENCLAVE] DESAFIO 1 CONCLUIDO!! parabéns Tiago De Paula Alves!!

------------------------------------------------


------------------------------------------------

[ENCLAVE] DESAFIO 2 CONCLUIDO!! a senha é 47481

------------------------------------------------


------------------------------------------------

[ENCLAVE] DESAFIO 3 CONCLUIDO!! a palavra secreta é VASNVLIESBWTCTIHNYCO

------------------------------------------------

[ENCLAVE] a=368, b=2401, c=-3461 = 316784540

[ENCLAVE] a=368, b=2401, c=-3461 = 1389485725

[ENCLAVE] a=368, b=2401, c=-3461 = 327473577


------------------------------------------------

[ENCLAVE] DESAFIO 4 CONCLUIDO!! os polinomios são: A=368, B=2401, C=-3461

------------------------------------------------


------------------------------------------------

[ENCLAVE] DESAFIO 5 CONCLUIDO!! V (vitória), D (derrota) E (empate)
          ENCLAVE JOGADAS: 00010021202221110011
             SUAS JOGADAS: 11121102010002221122
                RESULTADO: VVVVVVVVVVVVVVVVVVVV

------------------------------------------------

Info: Enclave successfully returned.
```

### Development

Enable [pre-commit](https://pre-commit.com/):

```sh
pre-commit install
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

- `app/*`: Untrusted Component Code
  - `app.c`: Application entry point, register and calls the enclave.
  - `error.c`: Prints the
    [sgx_status_t](https://github.com/intel/linux-sgx/blob/sgx_2.26/common/inc/sgx_error.h#L37-L127) error message.
- `enclave/*`: Trusted Component Code
  <!-- - `enclave.c`: Enclave ECALLS implementation. -->
  - `enclave.edl`: Enclave Trusted and Untrusted input types boundaries, OCALLS and ECALLS definitions. (see
    [Enclave Definition Language - EDL](https://cdrdv2-public.intel.com/671446/input-types-and-boundary-checking-edl.pdf))
  <!-- - `enclave.lds` and `enclave_debug.lds`: Linkers for hardware and simulation mode, for more detals read the section
    [about enclave/\*.lds files](#about-enclavelds-files). -->
  - `enclave.config.xml`: XML file containing the user defined parameters of an enclave, for more detals read the
    section [Enclave XML Configuration File](#enclave-xml-configuration-file).
  - `enclave.signed.so`: Pre-compiled enclave file with challenges implemented.
- `build.sh`: Build script, do the same as `meson setup build -Dsgx_mode=sim`, but is easier to read and learn the
  compilation process step-by-step.

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

## About `enclave*.lds` files

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
