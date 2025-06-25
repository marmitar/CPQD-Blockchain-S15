######## SGX SDK Settings ########

SGX_SDK ?= /opt/intel/sgxsdk
SGX_MODE ?= HW
SGX_ARCH ?= x64
SGX_DEBUG ?= 1

ifeq ($(shell getconf LONG_BIT), 32)
    SGX_ARCH := x86
else ifeq ($(findstring -m32, $(CXXFLAGS)), -m32)
    SGX_ARCH := x86
endif

ifeq ($(SGX_ARCH), x86)
    SGX_CFLAGS := -m32
    SGX_LIBRARY_PATH := $(SGX_SDK)/lib
    SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x86/sgx_sign
    SGX_EDGER8R := $(SGX_SDK)/bin/x86/sgx_edger8r
else
    SGX_CFLAGS := -m64
    SGX_LIBRARY_PATH := $(SGX_SDK)/lib64
    SGX_ENCLAVE_SIGNER := $(SGX_SDK)/bin/x64/sgx_sign
    SGX_EDGER8R := $(SGX_SDK)/bin/x64/sgx_edger8r
endif

ifeq ($(SGX_DEBUG), 1)
ifeq ($(SGX_PRERELEASE), 1)
$(error Cannot set SGX_DEBUG and SGX_PRERELEASE at the same time!!)
endif
endif

ifeq ($(SGX_DEBUG), 1)
        SGX_CFLAGS += -O0 -g
else
        SGX_CFLAGS += -O2
endif
SGX_CFLAGS += -Wall -Wextra -Winit-self -Wpointer-arith -Wreturn-type \
                    -Waddress -Wsequence-point -Wformat-security \
                    -Wmissing-include-dirs -Wfloat-equal -Wundef -Wshadow \
                    -Wcast-align -Wcast-qual -Wconversion -Wredundant-decls \
					-Wjump-misses-init -Wstrict-prototypes -Wunsuffixed-float-constants \
					-std=c99

######## App Settings ########

ifeq ($(SGX_MODE), HW)
    URTS_LIBRARY_NAME := sgx_urts
else
    URTS_LIBRARY_NAME := sgx_urts_sim
endif

APP_NAME := main
APP_SRCS := app/app.c app/error.c
APP_HEADERS := app/app.h app/error.h
APP_OBJS := $(APP_SRCS:.c=.o)
APP_INCLUDES := -Iapp -I$(SGX_SDK)/include
APP_CFLAGS := -fPIC -Wno-attributes $(APP_INCLUDES)
APP_LINK_FLAGS := -L$(SGX_LIBRARY_PATH) -l$(URTS_LIBRARY_NAME) -lpthread

# Three configuration modes - Debug, prerelease, release
#   Debug - Macro DEBUG enabled.
#   Prerelease - Macro NDEBUG and EDEBUG enabled.
#   Release - Macro NDEBUG enabled.
ifeq ($(SGX_DEBUG), 1)
        APP_CFLAGS += -DDEBUG -UNDEBUG -UEDEBUG
else ifeq ($(SGX_PRERELEASE), 1)
        APP_CFLAGS += -DNDEBUG -DEDEBUG -UDEBUG
else
        APP_CFLAGS += -DNDEBUG -UEDEBUG -UDEBUG
endif

######## Enclave Settings ########

ENCLAVE_VERSION_SCRIPT := enclave/enclave_debug.lds
ifeq ($(SGX_MODE), HW)
ifneq ($(SGX_DEBUG), 1)
ifneq ($(SGX_PRERELEASE), 1)
    # Choose to use 'enclave.lds' for HW release mode
    ENCLAVE_VERSION_SCRIPT = enclave/enclave.lds 
endif
endif
endif

ifeq ($(SGX_MODE), HW)
    TRTS_LIBRARY_NAME := sgx_trts
    SERVICE_LIBRARY_NAME := sgx_tservice
else
    TRTS_LIBRARY_NAME := sgx_trts_sim
    SERVICE_LIBRARY_NAME := sgx_tservice_sim
endif
CRYPTO_LIBRARY_NAME := sgx_tcrypto

ENCLAVE_SRCS := enclave/enclave.c
ENCLAVE_OBJS := $(ENCLAVE_SRCS:.c=.o)
ENCLAVE_HEADERS := enclave/enclave.h
ENCLAVE_INCLUDE_PATHS := -Ienclave -I$(SGX_SDK)/include -I$(SGX_SDK)/include/tlibc 

ENCLAVE_CFLAGS := -nostdinc -fvisibility=hidden -fpie -fstack-protector -fno-builtin-printf $(ENCLAVE_INCLUDE_PATHS)

# Enable the security flags
ENCLAVE_SECURITY_LINK_FLAGS := -Wl,-z,relro,-z,now,-z,noexecstack

# To generate a proper enclave, it is recommended to follow below guideline to link the trusted libraries:
#    1. Link sgx_trts with the `--whole-archive' and `--no-whole-archive' options,
#       so that the whole content of trts is included in the enclave.
#    2. For other libraries, you just need to pull the required symbols.
#       Use `--start-group' and `--end-group' to link these libraries.
# Do NOT move the libraries linked with `--start-group' and `--end-group' within `--whole-archive' and `--no-whole-archive' options.
# Otherwise, you may get some undesirable errors.
ENCLAVE_LINK_FLAGS := $(ENCLAVE_SECURITY_LINK_FLAGS) \
    -Wl,--no-undefined -nostdlib -nodefaultlibs -nostartfiles -L$(SGX_LIBRARY_PATH) \
	-Wl,--whole-archive -l$(TRTS_LIBRARY_NAME) -Wl,--no-whole-archive \
	-Wl,--start-group -lsgx_tstdc -l$(CRYPTO_LIBRARY_NAME) -l$(SERVICE_LIBRARY_NAME) -Wl,--end-group \
	-Wl,-Bstatic -Wl,-Bsymbolic -Wl,--no-undefined \
	-Wl,-pie,-eenclave_entry -Wl,--export-dynamic  \
	-Wl,--defsym,__ImageBase=0 \
	-Wl,--version-script=$(ENCLAVE_VERSION_SCRIPT)

ENCLAVE_NAME := enclave.so
SIGNED_ENCLAVE_NAME := enclave.signed.so
ENCLAVE_CONFIG_FILE := enclave/enclave.config.xml
ENCLAVE_TEST_KEY := enclave/enclave_private_test.pem

ifeq ($(SGX_MODE), HW)
ifeq ($(SGX_DEBUG), 1)
    BUILD_MODE = HW_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
    BUILD_MODE = HW_PRERELEASE
else
    BUILD_MODE = HW_RELEASE
endif
else
ifeq ($(SGX_DEBUG), 1)
    BUILD_MODE = SIM_DEBUG
else ifeq ($(SGX_PRERELEASE), 1)
    BUILD_MODE = SIM_PRERELEASE
else
    BUILD_MODE = SIM_RELEASE
endif
endif


.PHONY: all run target
all: .config_$(BUILD_MODE)_$(SGX_ARCH)
	@$(MAKE) target

ifeq ($(BUILD_MODE), HW_RELEASE)
target: $(APP_NAME) $(ENCLAVE_NAME)
	@echo "The project has been built in release hardware mode."
	@echo "Please sign the $(ENCLAVE_NAME) first with your signing key before you run the $(APP_NAME) to launch and access the enclave."
	@echo "To sign the enclave use the command:"
	@echo "   $(SGX_ENCLAVE_SIGNER) sign -key <your key> -enclave $(ENCLAVE_NAME) -out <$(SIGNED_ENCLAVE_NAME)> -config $(ENCLAVE_CONFIG_FILE)"
	@echo "You can also sign the enclave using an external signing tool."
	@echo "To build the project in simulation mode set SGX_MODE=SIM. To build the project in prerelease mode set SGX_PRERELEASE=1 and SGX_MODE=HW."
else
target: $(APP_NAME) $(SIGNED_ENCLAVE_NAME)
ifeq ($(BUILD_MODE), HW_DEBUG)
	@echo "The project has been built in debug hardware mode."
else ifeq ($(BUILD_MODE), SIM_DEBUG)
	@echo "The project has been built in debug simulation mode."
else ifeq ($(BUILD_MODE), HW_PRERELEASE)
	@echo "The project has been built in pre-release hardware mode."
else ifeq ($(BUILD_MODE), SIM_PRERELEASE)
	@echo "The project has been built in pre-release simulation mode."
else
	@echo "The project has been built in release simulation mode."
endif
endif

run: all
ifneq ($(BUILD_MODE), HW_RELEASE)
	@$(CURDIR)/$(APP_NAME)
	@echo "RUN  =>  $(APP_NAME) [$(SGX_MODE)|$(SGX_ARCH), OK]"
endif

.config_$(BUILD_MODE)_$(SGX_ARCH):
	@rm -f .config_* $(APP_NAME) $(ENCLAVE_NAME) $(SIGNED_ENCLAVE_NAME) $(APP_OBJS) app/enclave_u.* $(ENCLAVE_OBJS) enclave/enclave_t.*
	@touch .config_$(BUILD_MODE)_$(SGX_ARCH)

######## App Objects ########
app/enclave_u.h: $(SGX_EDGER8R) enclave/enclave.edl
	@cd app && $(SGX_EDGER8R) --untrusted ../enclave/enclave.edl --search-path ../enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

app/enclave_u.c: app/enclave_u.h

app/enclave_u.o: app/enclave_u.c
	@$(CC) $(SGX_CFLAGS) $(APP_CFLAGS) -c $< -o $@
	@echo "CC   <=  $<"

app/%.o: app/%.c app/enclave_u.h
	@$(CC) $(SGX_CFLAGS) $(APP_CFLAGS) -c $< -o $@
	@echo "CC  <=  $<"

$(APP_NAME): app/enclave_u.o $(APP_OBJS)
	@$(CC) $^ -o $@ $(APP_LINK_FLAGS)
	@echo "LINK =>  $@"

######## Enclave Objects ########

enclave/enclave_t.h: $(SGX_EDGER8R) enclave/enclave.edl
	@cd enclave && $(SGX_EDGER8R) --trusted ../enclave/enclave.edl --search-path ../enclave --search-path $(SGX_SDK)/include
	@echo "GEN  =>  $@"

enclave/enclave_t.c: enclave/enclave_t.h

enclave/enclave_t.o: enclave/enclave_t.c
	@$(CC) $(SGX_CFLAGS) $(ENCLAVE_CFLAGS) -c $< -o $@
	@echo "CC   <=  $<"

enclave/%.o: enclave/%.c
	@$(CC) $(SGX_CFLAGS) $(ENCLAVE_CFLAGS) -c $< -o $@
	@echo "CC  <=  $<"

$(ENCLAVE_OBJS): enclave/enclave_t.h

$(ENCLAVE_NAME): enclave/enclave_t.o $(ENCLAVE_OBJS)
	@$(CC) $^ -o $@ $(ENCLAVE_LINK_FLAGS)
	@echo "LINK =>  $@"

$(SIGNED_ENCLAVE_NAME): $(ENCLAVE_NAME)
ifeq ($(wildcard $(ENCLAVE_TEST_KEY)),)
	@echo "There is no enclave test key<enclave_private_test.pem>."
	@echo "The project will generate a key<enclave_private_test.pem> for test."
	@openssl genrsa -out $(ENCLAVE_TEST_KEY) -3 3072
endif
	@$(SGX_ENCLAVE_SIGNER) sign -key $(ENCLAVE_TEST_KEY) -enclave $(ENCLAVE_NAME) -out $@ -config $(ENCLAVE_CONFIG_FILE)
	@echo "SIGN =>  $@"

.PHONY: clean format

clean:
	@rm -f .config_* $(APP_NAME) $(ENCLAVE_NAME) $(SIGNED_ENCLAVE_NAME) $(APP_OBJS) app/enclave_u.* $(ENCLAVE_OBJS) enclave/enclave_t.*

format:
	@for filename in $(APP_SRCS) $(APP_HEADERS) $(ENCLAVE_SRCS) $(ENCLAVE_HEADERS); do \
		indent "$${filename}" -o "$${filename}" \
        	-nbad -bap -nbc -br -bli2 -bls -ncdb -nce -cs -di2 -ndj -nfc1 --no-tabs \
        	-i4 -ip5 -lp -npcs -psl -nsc -nsob -bbo -cp1 -nfca -hnl -npsl --indent-label0; \
	done

