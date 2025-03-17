ifeq ($(OS),Windows_NT)
	ifeq ($(MSYSTEM), MINGW32)
		UNAME := MSYS
	else
		UNAME := Windows
	endif
else
	UNAME := $(shell uname)
endif

TARGET = jordandemoscene.elf

.DEFAULT: $(TARGET)

# Directories (relative to src/)
src_dir = .
corelib_dir = ../../../SDK
core_dir = $(src_dir)/core
render_dir = $(src_dir)/rendering
scenes_dir = $(src_dir)/scenes
utils_dir = $(src_dir)/utils
assets_dir = $(src_dir)/assets
zlib_dir = $(src_dir)/zlib

# Rules
ifeq ($(UNAME), Windows)
RISCV_PREFIX ?= riscv32-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)g++
RISCV_GCC_OPTS ?= -mcmodel=medany -std=c++20 --param "l1-cache-line-size=64" --param "l1-cache-size=16" -Wall -Ofast -march=rv32im_zicsr_zifencei_zfinx -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -Wl,--strip-all -lgcc -lm
else
RISCV_PREFIX ?= riscv64-unknown-elf-
RISCV_GCC ?= $(RISCV_PREFIX)g++
RISCV_GCC_OPTS ?= -mcmodel=medany -std=c++20 --param "min-pagesize=0" --param "l1-cache-line-size=64" --param "l1-cache-size=16" -Wall -Ofast -march=rv32im_zicsr_zifencei_zfinx -mabi=ilp32 -ffunction-sections -fdata-sections -Wl,-gc-sections -Wl,--strip-all -lgcc -lm
endif

RISCV_OBJDUMP ?= $(RISCV_PREFIX)objdump

# Include Directories (Add utils/ or wherever common.h is)
incs  += -I$(src_dir) -I$(corelib_dir) -I$(core_dir) -I$(render_dir) -I$(scenes_dir) -I$(utils_dir) -I$(assets_dir)


# Find Source Files
SRCS := $(wildcard $(src_dir)/*.cpp) \
        $(wildcard $(core_dir)/*.cpp) \
        $(wildcard $(render_dir)/*.cpp) \
        $(wildcard $(scenes_dir)/*.cpp) \
	$(wildcard $(utils_dir)/*.cpp) \
	$(wildcard $(zlib_dir)/*.cpp)

# Libraries (SDK)
libs += $(wildcard $(corelib_dir)/*.S) $(wildcard $(corelib_dir)/*.c)

# Object Files (Convert .cpp to .o)
OBJS := $(patsubst %.cpp, %.o, $(SRCS))

# Compilation Rule
$(TARGET): $(OBJS)
	$(RISCV_GCC) $(incs) -o $(TARGET) $(OBJS) $(libs) $(RISCV_GCC_OPTS)

# Individual Compilation Rule
%.o: %.cpp
	$(RISCV_GCC) $(incs) -c -o $@ $< $(RISCV_GCC_OPTS)

# Disassembly Dump
dump: $(TARGET)
	$(RISCV_OBJDUMP) $(TARGET) -x -D -S >> $(TARGET).txt

# Clean Build
.PHONY: clean
clean:
ifeq ($(UNAME), Windows)
	del *.elf *.o *.txt
else
	rm -rf *.elf *.o *.txt
endif