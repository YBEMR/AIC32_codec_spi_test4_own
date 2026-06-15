################################################################################
# Portable gmake build entry for AIC32_codec_spi libraries
#
# This Makefile builds the DSP support libraries directly and does not depend on
# CCS-generated Debug/makefile or subdir_*.mk files.
#
# Usage:
#   set CG_TOOL_ROOT=F:/ti/CCS8.3/ccsv8/tools/compiler/ti-cgt-c2000_22.6.0.LTS
#   gmake all
#   gmake codec_service_lib
#   gmake student-release
################################################################################

BUILD_DIR ?= build
CG_TOOL_ROOT ?=

CL2000 := $(CG_TOOL_ROOT)/bin/cl2000
AR2000 := $(CG_TOOL_ROOT)/bin/ar2000

LIBS := audio_lib spi_lib uarta_lib timer0_lib exint_lib led_lib key_lib codec_service_lib

DSP_COMMON_INC := ../third_party/DSP2833x/common/include
DSP_HEADERS_INC := ../third_party/DSP2833x/headers/include

winpath = $(subst /,\,$(1))

ifeq ($(OS),Windows_NT)
SHELL := cmd.exe
define make-dir
if not exist "$(call winpath,$(1))" mkdir "$(call winpath,$(1))"
endef
define remove-dir
if exist "$(call winpath,$(1))" rmdir /S /Q "$(call winpath,$(1))"
endef
define remove-file
if exist "$(call winpath,$(1))" del /F /Q "$(call winpath,$(1))"
endef
else
SHELL := /bin/sh
define make-dir
mkdir -p "$(1)"
endef
define remove-dir
rm -rf "$(1)"
endef
define remove-file
rm -f "$(1)"
endef
endif

NEEDS_TOOLS := $(filter-out clean help sync student-release,$(if $(MAKECMDGOALS),$(MAKECMDGOALS),all))
ifneq ($(NEEDS_TOOLS),)
ifeq ($(strip $(CG_TOOL_ROOT)),)
$(error CG_TOOL_ROOT is not set. Example: set CG_TOOL_ROOT=F:/ti/CCS8.3/ccsv8/tools/compiler/ti-cgt-c2000_22.6.0.LTS)
endif
endif

COMMON_FLAGS := -v28 -ml -mt --float_support=fpu32 --advice:performance=all -g --c99
COMMON_FLAGS += --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi

AUDIO_DEBUG_FLAGS ?=
CODEC_SERVICE_DEBUG_FLAGS ?=
AMR_ENCODE_MODE ?= MR515
AMR_PROFILE ?= 0
SPEED_COMMON_FLAGS := $(filter-out -g,$(COMMON_FLAGS))
OPT_SPEED_FLAGS := --opt_level=3 --opt_for_speed=5

AUDIO_FLAGS := $(SPEED_COMMON_FLAGS) $(OPT_SPEED_FLAGS) --define=AMR_FAST_BASIC_OPS=1 --define=AMR_ENCODE_MODE=$(AMR_ENCODE_MODE) $(AUDIO_DEBUG_FLAGS)
ifeq ($(AMR_PROFILE),1)
AUDIO_FLAGS += --define=AMR_PROFILE=1
endif
CODEC_SERVICE_FLAGS := $(SPEED_COMMON_FLAGS) $(OPT_SPEED_FLAGS) $(CODEC_SERVICE_DEBUG_FLAGS)
SPI_FLAGS := $(COMMON_FLAGS)
UARTA_FLAGS := $(COMMON_FLAGS)
TIMER0_FLAGS := $(COMMON_FLAGS)
EXINT_FLAGS := $(COMMON_FLAGS)
LED_FLAGS := $(COMMON_FLAGS)
KEY_FLAGS := $(COMMON_FLAGS)

BASE_INCLUDES := --include_path="$(CG_TOOL_ROOT)/include"
BASE_INCLUDES += --include_path="../include"
BASE_INCLUDES += --include_path="$(DSP_COMMON_INC)"
BASE_INCLUDES += --include_path="$(DSP_HEADERS_INC)"

AUDIO_INCLUDES := $(BASE_INCLUDES)
AUDIO_INCLUDES += --include_path="audio_lib"
AUDIO_INCLUDES += --include_path="audio_lib/gsm-codec-lib-r3/libtest"
AUDIO_INCLUDES += --include_path="audio_lib/gsm-codec-lib-r3/libtwamr"

SPI_INCLUDES := $(BASE_INCLUDES) --include_path="spi_lib"
UARTA_INCLUDES := $(BASE_INCLUDES) --include_path="uarta_lib"
TIMER0_INCLUDES := $(BASE_INCLUDES) --include_path="timer0_lib"
EXINT_INCLUDES := $(BASE_INCLUDES) --include_path="exint_lib"
LED_INCLUDES := $(BASE_INCLUDES) --include_path="led_lib"
KEY_INCLUDES := $(BASE_INCLUDES) --include_path="key_lib"
CODEC_SERVICE_INCLUDES := $(BASE_INCLUDES)
CODEC_SERVICE_INCLUDES += --include_path="codec_service_lib"
CODEC_SERVICE_INCLUDES += --include_path="audio_lib"
CODEC_SERVICE_INCLUDES += --include_path="audio_lib/gsm-codec-lib-r3/libtest"
CODEC_SERVICE_INCLUDES += --include_path="audio_lib/gsm-codec-lib-r3/libtwamr"
CODEC_SERVICE_INCLUDES += --include_path="spi_lib"

AUDIO_ROOT_SRCS := audio_lib/audio.c
AUDIO_LIBTEST_SRCS := $(wildcard audio_lib/gsm-codec-lib-r3/libtest/*.c)
AUDIO_LIBTWAMR_SRCS := $(wildcard audio_lib/gsm-codec-lib-r3/libtwamr/*.c)
AUDIO_SRCS := $(AUDIO_ROOT_SRCS) $(AUDIO_LIBTEST_SRCS) $(AUDIO_LIBTWAMR_SRCS)
AUDIO_HDRS := audio_lib/audio.h
AUDIO_HDRS += $(wildcard audio_lib/gsm-codec-lib-r3/libtest/*.h)
AUDIO_HDRS += $(wildcard audio_lib/gsm-codec-lib-r3/libtwamr/*.h)

AUDIO_OBJS := $(patsubst audio_lib/%.c,$(BUILD_DIR)/audio_lib/%.obj,$(AUDIO_SRCS))
AUDIO_ROOT_OBJS := $(patsubst audio_lib/%.c,$(BUILD_DIR)/audio_lib/%.obj,$(AUDIO_ROOT_SRCS))
AUDIO_LIBTEST_OBJS := $(patsubst audio_lib/%.c,$(BUILD_DIR)/audio_lib/%.obj,$(AUDIO_LIBTEST_SRCS))
AUDIO_LIBTWAMR_OBJS := $(patsubst audio_lib/%.c,$(BUILD_DIR)/audio_lib/%.obj,$(AUDIO_LIBTWAMR_SRCS))
AUDIO_LIBTWAMR_OBJS_1 := $(wordlist 1,45,$(AUDIO_LIBTWAMR_OBJS))
AUDIO_LIBTWAMR_OBJS_2 := $(wordlist 46,90,$(AUDIO_LIBTWAMR_OBJS))
AUDIO_LIBTWAMR_OBJS_3 := $(wordlist 91,200,$(AUDIO_LIBTWAMR_OBJS))

SPI_SRCS := spi_lib/spi.c
UARTA_SRCS := uarta_lib/uart.c
TIMER0_SRCS := timer0_lib/timer.c
EXINT_SRCS := exint_lib/exint.c
LED_SRCS := led_lib/leds.c
KEY_SRCS := key_lib/key.c
CODEC_SERVICE_SRCS := codec_service_lib/codec_service.c
SPI_HDRS := spi_lib/spi.h
UARTA_HDRS := uarta_lib/uart.h
TIMER0_HDRS := timer0_lib/timer.h
EXINT_HDRS := exint_lib/exint.h
LED_HDRS := led_lib/leds.h
KEY_HDRS := key_lib/key.h
CODEC_SERVICE_HDRS := codec_service_lib/codec_service.h audio_lib/audio.h spi_lib/spi.h

SPI_OBJS := $(patsubst spi_lib/%.c,$(BUILD_DIR)/spi_lib/%.obj,$(SPI_SRCS))
UARTA_OBJS := $(patsubst uarta_lib/%.c,$(BUILD_DIR)/uarta_lib/%.obj,$(UARTA_SRCS))
TIMER0_OBJS := $(patsubst timer0_lib/%.c,$(BUILD_DIR)/timer0_lib/%.obj,$(TIMER0_SRCS))
EXINT_OBJS := $(patsubst exint_lib/%.c,$(BUILD_DIR)/exint_lib/%.obj,$(EXINT_SRCS))
LED_OBJS := $(patsubst led_lib/%.c,$(BUILD_DIR)/led_lib/%.obj,$(LED_SRCS))
KEY_OBJS := $(patsubst key_lib/%.c,$(BUILD_DIR)/key_lib/%.obj,$(KEY_SRCS))
CODEC_SERVICE_OBJS := $(patsubst codec_service_lib/%.c,$(BUILD_DIR)/codec_service_lib/%.obj,$(CODEC_SERVICE_SRCS))

AUDIO_LIB_OUT := $(BUILD_DIR)/audio_lib/audio_lib.lib
AUDIO_AR_CMD := $(BUILD_DIR)/audio_lib/audio_lib.arcmd
SPI_LIB_OUT := $(BUILD_DIR)/spi_lib/spi_lib.lib
UARTA_LIB_OUT := $(BUILD_DIR)/uarta_lib/uarta_lib.lib
TIMER0_LIB_OUT := $(BUILD_DIR)/timer0_lib/timer0_lib.lib
EXINT_LIB_OUT := $(BUILD_DIR)/exint_lib/exint_lib.lib
LED_LIB_OUT := $(BUILD_DIR)/led_lib/led_lib.lib
KEY_LIB_OUT := $(BUILD_DIR)/key_lib/key_lib.lib
CODEC_SERVICE_LIB_OUT := $(BUILD_DIR)/codec_service_lib/codec_service_lib.lib

.PHONY: all clean help sync student-release $(LIBS)

all: $(LIBS) sync

define compile-c
	@$(call make-dir,$(@D))
	@echo Building file: "$<"
	"$(CL2000)" $($(1)_FLAGS) $($(1)_INCLUDES) --preproc_with_compile --preproc_dependency="$(@:.obj=.d_raw)" --obj_directory="$(call winpath,$(@D))" "$<"
	@echo Finished building: "$<"
	@echo.
endef

$(BUILD_DIR)/audio_lib/%.obj: audio_lib/%.c $(AUDIO_HDRS)
	$(call compile-c,AUDIO)

$(BUILD_DIR)/spi_lib/%.obj: spi_lib/%.c $(SPI_HDRS)
	$(call compile-c,SPI)

$(BUILD_DIR)/uarta_lib/%.obj: uarta_lib/%.c $(UARTA_HDRS)
	$(call compile-c,UARTA)

$(BUILD_DIR)/timer0_lib/%.obj: timer0_lib/%.c $(TIMER0_HDRS)
	$(call compile-c,TIMER0)

$(BUILD_DIR)/exint_lib/%.obj: exint_lib/%.c $(EXINT_HDRS)
	$(call compile-c,EXINT)

$(BUILD_DIR)/led_lib/%.obj: led_lib/%.c $(LED_HDRS)
	$(call compile-c,LED)

$(BUILD_DIR)/key_lib/%.obj: key_lib/%.c $(KEY_HDRS)
	$(call compile-c,KEY)

$(BUILD_DIR)/codec_service_lib/%.obj: codec_service_lib/%.c $(CODEC_SERVICE_HDRS)
	$(call compile-c,CODEC_SERVICE)

$(AUDIO_LIB_OUT): $(AUDIO_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	@echo r $(call winpath,$@)> "$(call winpath,$(AUDIO_AR_CMD))"
	@echo $(AUDIO_ROOT_OBJS) $(AUDIO_LIBTEST_OBJS)>> "$(call winpath,$(AUDIO_AR_CMD))"
	@echo $(AUDIO_LIBTWAMR_OBJS_1)>> "$(call winpath,$(AUDIO_AR_CMD))"
	@echo $(AUDIO_LIBTWAMR_OBJS_2)>> "$(call winpath,$(AUDIO_AR_CMD))"
	@echo $(AUDIO_LIBTWAMR_OBJS_3)>> "$(call winpath,$(AUDIO_AR_CMD))"
	"$(AR2000)" @"$(call winpath,$(AUDIO_AR_CMD))"
	@echo Finished building: "$@"
	@echo.

$(SPI_LIB_OUT): $(SPI_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	"$(AR2000)" r "$@" $(SPI_OBJS)
	@echo Finished building: "$@"
	@echo.

$(UARTA_LIB_OUT): $(UARTA_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	"$(AR2000)" r "$@" $(UARTA_OBJS)
	@echo Finished building: "$@"
	@echo.

$(TIMER0_LIB_OUT): $(TIMER0_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	"$(AR2000)" r "$@" $(TIMER0_OBJS)
	@echo Finished building: "$@"
	@echo.

$(EXINT_LIB_OUT): $(EXINT_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	"$(AR2000)" r "$@" $(EXINT_OBJS)
	@echo Finished building: "$@"
	@echo.

$(LED_LIB_OUT): $(LED_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	"$(AR2000)" r "$@" $(LED_OBJS)
	@echo Finished building: "$@"
	@echo.

$(KEY_LIB_OUT): $(KEY_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	"$(AR2000)" r "$@" $(KEY_OBJS)
	@echo Finished building: "$@"
	@echo.

$(CODEC_SERVICE_LIB_OUT): $(CODEC_SERVICE_OBJS)
	@$(call make-dir,$(@D))
	@$(call remove-file,$@)
	@echo Building library: "$@"
	"$(AR2000)" r "$@" $(CODEC_SERVICE_OBJS)
	@echo Finished building: "$@"
	@echo.

audio_lib: $(AUDIO_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y audio_lib\audio.h release\include\audio.h >nul
	@copy /Y "$(call winpath,$(AUDIO_LIB_OUT))" release\lib\audio_lib.lib >nul

spi_lib: $(SPI_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y spi_lib\spi.h release\include\spi.h >nul
	@copy /Y "$(call winpath,$(SPI_LIB_OUT))" release\lib\spi_lib.lib >nul

uarta_lib: $(UARTA_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y uarta_lib\uart.h release\include\uart.h >nul
	@copy /Y "$(call winpath,$(UARTA_LIB_OUT))" release\lib\uarta_lib.lib >nul

timer0_lib: $(TIMER0_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y timer0_lib\timer.h release\include\timer.h >nul
	@copy /Y "$(call winpath,$(TIMER0_LIB_OUT))" release\lib\timer0_lib.lib >nul

exint_lib: $(EXINT_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y exint_lib\exint.h release\include\exint.h >nul
	@copy /Y "$(call winpath,$(EXINT_LIB_OUT))" release\lib\exint_lib.lib >nul

led_lib: $(LED_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y led_lib\leds.h release\include\leds.h >nul
	@copy /Y "$(call winpath,$(LED_LIB_OUT))" release\lib\led_lib.lib >nul

key_lib: $(KEY_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y key_lib\key.h release\include\key.h >nul
	@copy /Y "$(call winpath,$(KEY_LIB_OUT))" release\lib\key_lib.lib >nul

codec_service_lib: $(CODEC_SERVICE_LIB_OUT)
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@copy /Y codec_service_lib\codec_service.h release\include\codec_service.h >nul
	@copy /Y "$(call winpath,$(CODEC_SERVICE_LIB_OUT))" release\lib\codec_service_lib.lib >nul

sync:
	@if not exist release\include mkdir release\include
	@if not exist release\lib mkdir release\lib
	@if exist audio_lib\audio.h copy /Y audio_lib\audio.h release\include\audio.h >nul
	@if exist spi_lib\spi.h copy /Y spi_lib\spi.h release\include\spi.h >nul
	@if exist uarta_lib\uart.h copy /Y uarta_lib\uart.h release\include\uart.h >nul
	@if exist timer0_lib\timer.h copy /Y timer0_lib\timer.h release\include\timer.h >nul
	@if exist exint_lib\exint.h copy /Y exint_lib\exint.h release\include\exint.h >nul
	@if exist led_lib\leds.h copy /Y led_lib\leds.h release\include\leds.h >nul
	@if exist key_lib\key.h copy /Y key_lib\key.h release\include\key.h >nul
	@if exist codec_service_lib\codec_service.h copy /Y codec_service_lib\codec_service.h release\include\codec_service.h >nul
	@if exist "$(call winpath,$(AUDIO_LIB_OUT))" copy /Y "$(call winpath,$(AUDIO_LIB_OUT))" release\lib\audio_lib.lib >nul
	@if exist "$(call winpath,$(SPI_LIB_OUT))" copy /Y "$(call winpath,$(SPI_LIB_OUT))" release\lib\spi_lib.lib >nul
	@if exist "$(call winpath,$(UARTA_LIB_OUT))" copy /Y "$(call winpath,$(UARTA_LIB_OUT))" release\lib\uarta_lib.lib >nul
	@if exist "$(call winpath,$(TIMER0_LIB_OUT))" copy /Y "$(call winpath,$(TIMER0_LIB_OUT))" release\lib\timer0_lib.lib >nul
	@if exist "$(call winpath,$(EXINT_LIB_OUT))" copy /Y "$(call winpath,$(EXINT_LIB_OUT))" release\lib\exint_lib.lib >nul
	@if exist "$(call winpath,$(LED_LIB_OUT))" copy /Y "$(call winpath,$(LED_LIB_OUT))" release\lib\led_lib.lib >nul
	@if exist "$(call winpath,$(KEY_LIB_OUT))" copy /Y "$(call winpath,$(KEY_LIB_OUT))" release\lib\key_lib.lib >nul
	@if exist "$(call winpath,$(CODEC_SERVICE_LIB_OUT))" copy /Y "$(call winpath,$(CODEC_SERVICE_LIB_OUT))" release\lib\codec_service_lib.lib >nul

student-release: sync
	@if not exist ..\student_release\include mkdir ..\student_release\include
	@if not exist ..\student_release\lib mkdir ..\student_release\lib
	@xcopy /Y /I release\include\* ..\student_release\include\ >nul
	@xcopy /Y /I release\lib\*.lib ..\student_release\lib\ >nul

clean:
	@$(call remove-dir,$(BUILD_DIR))
	@echo Cleaned $(BUILD_DIR)

help:
	@echo Portable library build targets:
	@echo   gmake all              Build all libraries and sync release files
	@echo   gmake codec_service_lib Build only codec_service_lib
	@echo   gmake student-release  Sync release files to ../student_release
	@echo   gmake clean            Remove $(BUILD_DIR)
