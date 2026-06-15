################################################################################
# Portable gmake build entry for AIC32_codec_spi_test4
#
# Usage:
#   set CG_TOOL_ROOT=C:/ti/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS
#   gmake all
################################################################################

PROJECT := AIC32_codec_spi_test4_own
BUILD_DIR ?= build/debug
DSP2833X_ROOT ?= third_party/DSP2833x

CG_TOOL_ROOT ?=
CL2000 := $(CG_TOOL_ROOT)/bin/cl2000
HEX2000 := $(CG_TOOL_ROOT)/bin/hex2000

DSP_COMMON_INC := $(DSP2833X_ROOT)/common/include
DSP_HEADERS_INC := $(DSP2833X_ROOT)/headers/include

winpath = $(subst /,\,$(1))

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
define make-dir
if not exist "$(call winpath,$(1))" mkdir "$(call winpath,$(1))"
endef
define remove-dir
if exist "$(call winpath,$(1))" rmdir /S /Q "$(call winpath,$(1))"
endef
else
SHELL := /bin/sh
define make-dir
mkdir -p "$(1)"
endef
define remove-dir
rm -rf "$(1)"
endef
endif

NEEDS_TOOLS := $(filter-out clean help,$(if $(MAKECMDGOALS),$(MAKECMDGOALS),all))
ifneq ($(NEEDS_TOOLS),)
ifeq ($(strip $(CG_TOOL_ROOT)),)
$(error CG_TOOL_ROOT is not set. Example: set CG_TOOL_ROOT=C:/ti/ccs/tools/compiler/ti-cgt-c2000_22.6.0.LTS)
endif
endif

COMMON_FLAGS := -v28 -ml -mt --float_support=fpu32 --advice:performance=all -g --c99
COMMON_FLAGS += --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi

INCLUDES := --include_path="lib/release/include"
INCLUDES += --include_path="$(CG_TOOL_ROOT)/include"
INCLUDES += --include_path="include"
INCLUDES += --include_path="$(DSP_COMMON_INC)"
INCLUDES += --include_path="$(DSP_HEADERS_INC)"

COMPILE_FLAGS := $(COMMON_FLAGS) $(INCLUDES) --preproc_with_compile

# SOURCE_ASM_SRCS := $(wildcard source/*.asm)
SOURCE_ASM_SRCS := \
	source/DSP2833x_ADC_cal.asm \
	source/DSP2833x_CodeStartBranch.asm \
	source/DSP2833x_usDelay.asm

# SOURCE_C_SRCS := $(wildcard source/*.c)
SOURCE_C_SRCS := \
	source/DSP2833x_CpuTimers.c \
	source/DSP2833x_DefaultIsr.c \
	source/DSP2833x_EPwm.c \
	source/DSP2833x_EQep.c \
	source/DSP2833x_GlobalVariableDefs.c \
	source/DSP2833x_Gpio.c \
	source/DSP2833x_I2C.c \
	source/DSP2833x_Mcbsp1.c \
	source/DSP2833x_MemCopy.c \
	source/DSP2833x_PieCtrl.c \
	source/DSP2833x_PieVect.c \
	source/DSP2833x_Sci.c \
	source/DSP2833x_Spi.c \
	source/DSP2833x_SysCtrl.c \
	source/DSP2833x_Xintf.c

USER_C_SRCS := user/student_codec_app.c

SOURCE_ASM_OBJS := $(patsubst source/%.asm,$(BUILD_DIR)/source/%.obj,$(SOURCE_ASM_SRCS))
SOURCE_C_OBJS := $(patsubst source/%.c,$(BUILD_DIR)/source/%.obj,$(SOURCE_C_SRCS))
USER_C_OBJS := $(patsubst user/%.c,$(BUILD_DIR)/user/%.obj,$(USER_C_SRCS))

OBJS := \
	$(BUILD_DIR)/source/DSP2833x_ADC_cal.obj \
	$(BUILD_DIR)/source/DSP2833x_CodeStartBranch.obj \
	$(SOURCE_C_OBJS) \
	$(BUILD_DIR)/source/DSP2833x_usDelay.obj \
	$(USER_C_OBJS)

CMD_SRCS := \
	source/DSP2833x_Headers_nonBIOS.cmd \
	source/F28335.cmd

# LIB_SRCS := source/IQmath_fpu32.lib $(wildcard lib/release/lib/*.lib)
LIB_SRCS := \
	source/IQmath_fpu32.lib \
	lib/release/lib/audio_lib.lib \
	lib/release/lib/codec_service_lib.lib \
	lib/release/lib/exint_lib.lib \
	lib/release/lib/led_lib.lib \
	lib/release/lib/spi_lib.lib \
	lib/release/lib/timer0_lib.lib \
	lib/release/lib/uarta_lib.lib \
	lib/release/lib/key_lib.lib

# LIB_NAMES := -llibc.a $(LIB_SRCS:/lib/release/lib/%.lib=-l%.lib)
LIB_NAMES := \
	-llibc.a \
	-lcodec_service_lib.lib \
	-laudio_lib.lib \
	-luarta_lib.lib \
	-ltimer0_lib.lib \
	-lspi_lib.lib \
	-lled_lib.lib \
	-lexint_lib.lib \
	-lkey_lib.lib

OUT := $(BUILD_DIR)/$(PROJECT).out
TXT := $(BUILD_DIR)/$(PROJECT).txt
MAP := $(BUILD_DIR)/$(PROJECT).map
LINKINFO := $(BUILD_DIR)/$(PROJECT)_linkInfo.xml

.PHONY: all baseline clean help libs profile

all: $(TXT)

baseline:
	"$(MAKE)" -C lib clean
	"$(MAKE)" -C lib all CG_TOOL_ROOT="$(CG_TOOL_ROOT)" AMR_PROFILE=0
	"$(MAKE)" clean
	"$(MAKE)" all CG_TOOL_ROOT="$(CG_TOOL_ROOT)"

profile:
	"$(MAKE)" -C lib clean
	"$(MAKE)" -C lib all CG_TOOL_ROOT="$(CG_TOOL_ROOT)" AMR_PROFILE=1
	"$(MAKE)" clean
	"$(MAKE)" all CG_TOOL_ROOT="$(CG_TOOL_ROOT)"

libs:
	"$(MAKE)" -C lib all CG_TOOL_ROOT="$(CG_TOOL_ROOT)" AMR_PROFILE=0

$(BUILD_DIR)/source/%.obj: source/%.c
	@$(call make-dir,$(@D))
	@echo Building file: "$<"
	"$(CL2000)" $(COMPILE_FLAGS) --preproc_dependency="$(@:.obj=.d_raw)" --obj_directory="$(@D)" "$<"

$(BUILD_DIR)/source/%.obj: source/%.asm
	@$(call make-dir,$(@D))
	@echo Building file: "$<"
	"$(CL2000)" $(COMPILE_FLAGS) --preproc_dependency="$(@:.obj=.d_raw)" --obj_directory="$(@D)" "$<"

$(BUILD_DIR)/user/%.obj: user/%.c
	@$(call make-dir,$(@D))
	@echo Building file: "$<"
	"$(CL2000)" $(COMPILE_FLAGS) --preproc_dependency="$(@:.obj=.d_raw)" --obj_directory="$(@D)" "$<"

$(OUT): $(OBJS) $(CMD_SRCS) $(LIB_SRCS)
	@$(call make-dir,$(@D))
	@echo Building target: "$@"
	"$(CL2000)" -v28 -ml -mt --float_support=fpu32 --advice:performance=all -g --c99 --diag_warning=225 \
	--diag_wrap=off --display_error_number --abi=coffabi -z -m"$(MAP)" --stack_size=0x6000 --warn_sections \
	-i"$(CG_TOOL_ROOT)/lib" -i"lib/release/lib" -i"$(CG_TOOL_ROOT)/include" --reread_libs --diag_wrap=off \
	--display_error_number --xml_link_info="$(LINKINFO)" --rom_model -o "$(OUT)" $(OBJS) $(CMD_SRCS) \
	$(LIB_SRCS) $(LIB_NAMES)

$(TXT): $(OUT)
	@echo Building hex output: "$@"
	"$(HEX2000)" "$(OUT)" -boot -sci8 -a -o "$(TXT)"

clean:
	@$(call remove-dir,$(BUILD_DIR))
	@echo Cleaned $(BUILD_DIR)

help:
	@echo Portable build targets:
	@echo   gmake all       Build $(OUT) and $(TXT) from current release libs
	@echo   gmake baseline  Rebuild release libs with AMR_PROFILE=0, then build firmware
	@echo   gmake profile   Rebuild release libs with AMR_PROFILE=1, then build firmware
	@echo   gmake clean     Remove $(BUILD_DIR)
