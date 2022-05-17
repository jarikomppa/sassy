#!/usr/bin/make -f
# Makefile for sassy #
# ------------------ #
# Created by falkTX
#

include Makefile.mk

DESTDIR =
PREFIX  = /usr

# ---------------------------------------------------------------------------------------------------------------------

BUILD_CXX_FLAGS += -Isrc/fftreal
BUILD_CXX_FLAGS += -Isrc/libsamplerate
BUILD_CXX_FLAGS += -Isrc/ocornut_imgui
BUILD_CXX_FLAGS += -Isrc/ocornut_imgui/backends
BUILD_CXX_FLAGS += -Isrc/rtmidi
BUILD_CXX_FLAGS += -Isrc/tinyfiledialogs

BUILD_CXX_FLAGS += -Wno-shadow
BUILD_CXX_FLAGS += -Wno-unused-parameter

BUILD_CXX_FLAGS += $(shell pkg-config --cflags sdl2) -pthread
LINK_FLAGS      += $(shell pkg-config --libs sdl2) -pthread

ifeq ($(MACOS),true)
BUILD_CXX_FLAGS += -D__MACOSX_CORE__
LINK_FLAGS      += -framework CoreMIDI
else ifeq ($(WINDOWS),true)
BUILD_CXX_FLAGS += -D__WINDOWS_MM__
LINK_FLAGS      += -lwinmm
else
BUILD_CXX_FLAGS += -D__LINUX_ALSA__
BUILD_CXX_FLAGS += $(shell pkg-config --cflags alsa gl) -pthread
LINK_FLAGS      += $(shell pkg-config --libs alsa gl) -pthread
endif

# ---------------------------------------------------------------------------------------------------------------------

FILES  = src/sassy.cpp
FILES += src/akwfdata.c
FILES += src/well512.cpp
FILES += src/ayumi/ayumi.c
FILES += src/klatt/darray.cpp
FILES += src/klatt/klatt.cpp
FILES += src/klatt/resonator.cpp
FILES += src/klatt/tts.cpp
FILES += src/padsynth/PADsynth.cpp
FILES += src/rtmidi/RtMidi.cpp
FILES += src/stb/stb_vorbis.c
FILES += src/tinyfiledialogs/tinyfiledialogs.c
FILES += $(wildcard src/eval_impl_*.cpp)
FILES += $(wildcard src/sassy_*.cpp)
FILES += $(wildcard src/libsamplerate/*.c)
FILES += $(wildcard src/ocornut_imgui/*.cpp)
FILES += $(wildcard src/ocornut_imgui/backends/*.cpp)

OBJS = $(FILES:%=build-tmp/%.o)

# ---------------------------------------------------------------------------------------------------------------------

all: sassy$(APP_EXT)

clean:
	rm -f $(OBJS)
	rm -rf build-tmp

# ---------------------------------------------------------------------------------------------------------------------

sassy$(APP_EXT): $(OBJS)
	@echo "Linking $@"
	$(SILENT)$(CXX) $^ $(LINK_FLAGS) -o $@

# ---------------------------------------------------------------------------------------------------------------------

build-tmp/%.c.o: %.c
	-@mkdir -p $(shell dirname $@)
	@echo "Compiling $<"
	$(SILENT)$(CC) $< $(BUILD_C_FLAGS) -c -o $@

build-tmp/%.cpp.o: %.cpp
	-@mkdir -p $(shell dirname $@)
	@echo "Compiling $<"
	$(SILENT)$(CXX) $< $(BUILD_CXX_FLAGS) -c -o $@

# ---------------------------------------------------------------------------------------------------------------------

-include $(OBJS:%.o=%.d)

# ---------------------------------------------------------------------------------------------------------------------
