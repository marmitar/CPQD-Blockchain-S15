SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_DEBUG ?= 0

# meson options are lowercase
_SGX_MODE := $(shell echo $(SGX_MODE) | tr '[:upper:]' '[:lower:]')
# debug uses true or false string
_SGX_DEBUG := $(if $(subst 0,,$(SGX_DEBUG)),true,false)

.PHONY: all run

all: main enclave.signed.so

main: build/app/app
	ln -sf $< ./$@

enclave.signed.so: build/enclave/enclave.signed.so
	ln -sf $< ./$@

build/%: .config_$(_SGX_MODE)_$(_SGX_DEBUG) build
	@meson compile -C build

run: all
	@meson test -C build --verbose

build: meson.build
	@meson setup $@

.config_$(_SGX_MODE)_$(_SGX_DEBUG): build
	@rm -f .config_*  main enclave.signed.so
	@meson configure $< -Ddebug=$(_SGX_DEBUG) -Dsgx_sdk=$(SGX_SDK) -Dsgx_mode=$(_SGX_MODE)
	@touch $@

.PHONY: clean format

clean:
	@rm -f .config_* main enclave.signed.so
	@rm -rf build/

format: build
	@ninja -C build clang-format
