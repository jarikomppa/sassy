#!/usr/bin/make -f
# Makefile for sassy #
# ------------------ #
# Created by falkTX
#

AR  ?= ar
CC  ?= gcc
CXX ?= g++

# ---------------------------------------------------------------------------------------------------------------------
# Protect against multiple inclusion

ifneq ($(MAKEFILE_BASE_INCLUDED),true)

MAKEFILE_BASE_INCLUDED = true

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect OS if not defined

TARGET_MACHINE := $(shell $(CC) -dumpmachine)

ifneq ($(BSD),true)
ifneq ($(HAIKU),true)
ifneq ($(HURD),true)
ifneq ($(LINUX),true)
ifneq ($(MACOS),true)
ifneq ($(WINDOWS),true)

ifneq (,$(findstring bsd,$(TARGET_MACHINE)))
BSD=true
else ifneq (,$(findstring haiku,$(TARGET_MACHINE)))
HAIKU=true
else ifneq (,$(findstring linux,$(TARGET_MACHINE)))
LINUX=true
else ifneq (,$(findstring gnu,$(TARGET_MACHINE)))
HURD=true
else ifneq (,$(findstring apple,$(TARGET_MACHINE)))
MACOS=true
else ifneq (,$(findstring mingw,$(TARGET_MACHINE)))
WINDOWS=true
else ifneq (,$(findstring windows,$(TARGET_MACHINE)))
WINDOWS=true
endif

endif
endif
endif
endif
endif
endif

# ---------------------------------------------------------------------------------------------------------------------
# Auto-detect the processor

TARGET_PROCESSOR := $(firstword $(subst -, ,$(TARGET_MACHINE)))

ifneq (,$(filter i%86,$(TARGET_PROCESSOR)))
CPU_I386=true
CPU_I386_OR_X86_64=true
endif
ifneq (,$(filter x86_64,$(TARGET_PROCESSOR)))
CPU_X86_64=true
CPU_I386_OR_X86_64=true
endif
ifneq (,$(filter arm%,$(TARGET_PROCESSOR)))
CPU_ARM=true
CPU_ARM_OR_AARCH64=true
endif
ifneq (,$(filter arm64%,$(TARGET_PROCESSOR)))
CPU_ARM64=true
CPU_ARM_OR_AARCH64=true
endif
ifneq (,$(filter aarch64%,$(TARGET_PROCESSOR)))
CPU_AARCH64=true
CPU_ARM_OR_AARCH64=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set PKG_CONFIG (can be overridden by environment variable)

ifeq ($(WINDOWS),true)
# Build statically on Windows by default
PKG_CONFIG ?= pkg-config --static
else
PKG_CONFIG ?= pkg-config
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set LINUX_OR_MACOS

ifeq ($(LINUX),true)
LINUX_OR_MACOS=true
endif

ifeq ($(MACOS),true)
LINUX_OR_MACOS=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set MACOS_OR_WINDOWS and HAIKU_OR_MACOS_OR_WINDOWS

ifeq ($(HAIKU),true)
HAIKU_OR_MACOS_OR_WINDOWS=true
endif

ifeq ($(MACOS),true)
MACOS_OR_WINDOWS=true
HAIKU_OR_MACOS_OR_WINDOWS=true
endif

ifeq ($(WINDOWS),true)
MACOS_OR_WINDOWS=true
HAIKU_OR_MACOS_OR_WINDOWS=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set UNIX

ifeq ($(BSD),true)
UNIX=true
endif

ifeq ($(HURD),true)
UNIX=true
endif

ifeq ($(LINUX),true)
UNIX=true
endif

ifeq ($(MACOS),true)
UNIX=true
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set build and link flags

BASE_FLAGS = -Wall -Wextra -pipe -MD -MP
BASE_OPTS  = -O3 -ffast-math -fno-finite-math-only -fdata-sections -ffunction-sections

ifeq ($(CPU_I386_OR_X86_64),true)
BASE_OPTS += -mtune=generic -msse -msse2 -mfpmath=sse
endif

ifeq ($(CPU_ARM),true)
ifneq ($(CPU_ARM64),true)
BASE_OPTS += -mfpu=neon-vfpv4 -mfloat-abi=hard
endif
endif

ifeq ($(MACOS),true)
# MacOS linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,-dead_strip -Wl,-dead_strip_dylibs
ifneq ($(SKIP_STRIPPING),true)
LINK_OPTS += -Wl,-x
endif
else
# Common linker flags
LINK_OPTS  = -fdata-sections -ffunction-sections -Wl,--gc-sections -Wl,-O1 -Wl,--as-needed
ifneq ($(SKIP_STRIPPING),true)
LINK_OPTS += -Wl,--strip-all
endif
endif

ifeq ($(SKIP_STRIPPING),true)
BASE_FLAGS += -g
endif

ifeq ($(NOOPT),true)
# Non-CPU-specific optimization flags
BASE_OPTS  = -O2 -ffast-math -fno-finite-math-only -fdata-sections -ffunction-sections
endif

ifneq ($(MACOS_OR_WINDOWS),true)
BASE_FLAGS += -fno-gnu-unique
endif

ifeq ($(WINDOWS),true)
# Assume we want posix
BASE_FLAGS += -posix -D__STDC_FORMAT_MACROS
# Needed for windows, see https://github.com/falkTX/Carla/issues/855
BASE_FLAGS += -mstackrealign
else
# Not needed for Windows
BASE_FLAGS += -fPIC -DPIC
endif

ifeq ($(DEBUG),true)
BASE_FLAGS += -DDEBUG -O0 -g
LINK_OPTS   =
else
BASE_FLAGS += -DNDEBUG $(BASE_OPTS) -fvisibility=hidden
CXXFLAGS   += -fvisibility-inlines-hidden
endif

ifeq ($(WITH_LTO),true)
BASE_FLAGS += -fno-strict-aliasing -flto
LINK_OPTS  += -fno-strict-aliasing -flto -Werror=odr -Werror=lto-type-mismatch
endif

BUILD_C_FLAGS   = $(BASE_FLAGS) -std=gnu99 $(CFLAGS)
BUILD_CXX_FLAGS = $(BASE_FLAGS) -std=gnu++11 $(CXXFLAGS)
LINK_FLAGS      = $(LINK_OPTS) $(LDFLAGS)

ifneq ($(MACOS),true)
# Not available on MacOS
LINK_FLAGS     += -Wl,--no-undefined
endif

ifeq ($(MACOS_OLD),true)
BUILD_CXX_FLAGS = $(BASE_FLAGS) $(CXXFLAGS) -DHAVE_CPP11_SUPPORT=0
endif

ifeq ($(WINDOWS),true)
# Always build statically on windows
LINK_FLAGS     += -static -static-libgcc -static-libstdc++
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set app extension

ifeq ($(WINDOWS),true)
APP_EXT = .exe
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set shared lib extension

LIB_EXT = .so

ifeq ($(MACOS),true)
LIB_EXT = .dylib
endif

ifeq ($(WINDOWS),true)
LIB_EXT = .dll
endif

# ---------------------------------------------------------------------------------------------------------------------
# Set shared library CLI arg

ifeq ($(MACOS),true)
SHARED = -dynamiclib
else
SHARED = -shared
endif

# ---------------------------------------------------------------------------------------------------------------------
# Handle the verbosity switch

SILENT =

ifeq ($(VERBOSE),1)
else ifeq ($(VERBOSE),y)
else ifeq ($(VERBOSE),yes)
else ifeq ($(VERBOSE),true)
else
SILENT = @
endif

# ---------------------------------------------------------------------------------------------------------------------
# all needs to be first

all:

# ---------------------------------------------------------------------------------------------------------------------
# helper to print what is available/possible to build

print_available = @echo $(1): $(shell echo $($(1)) | grep -q true && echo Yes || echo No)

features:
	@echo === Detected CPU
	$(call print_available,CPU_AARCH64)
	$(call print_available,CPU_ARM)
	$(call print_available,CPU_ARM64)
	$(call print_available,CPU_ARM_OR_AARCH64)
	$(call print_available,CPU_I386)
	$(call print_available,CPU_I386_OR_X86_64)
	@echo === Detected OS
	$(call print_available,BSD)
	$(call print_available,HAIKU)
	$(call print_available,HURD)
	$(call print_available,LINUX)
	$(call print_available,MACOS)
	$(call print_available,WINDOWS)
	$(call print_available,HAIKU_OR_MACOS_OR_WINDOWS)
	$(call print_available,LINUX_OR_MACOS)
	$(call print_available,MACOS_OR_WINDOWS)
	$(call print_available,UNIX)

# ---------------------------------------------------------------------------------------------------------------------
# Protect against multiple inclusion

endif # MAKEFILE_BASE_INCLUDED

# ---------------------------------------------------------------------------------------------------------------------
