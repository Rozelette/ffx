MIPS_BINUTILS_PREFIX ?= mips-linux-gnu-
VERSION ?= intl
TOOLS_DIR := tools

#TODO
CC := COMPILER_PATH=$(TOOLS_DIR)/cc/ee-gcc2.9-991111/lib/gcc-lib/ee/2.9-ee-991111 $(TOOLS_DIR)/cc/ee-gcc2.9-991111/bin/ee-gcc

AS      := $(MIPS_BINUTILS_PREFIX)as
LD      := $(MIPS_BINUTILS_PREFIX)ld
NM      := $(MIPS_BINUTILS_PREFIX)nm
OBJCOPY := $(MIPS_BINUTILS_PREFIX)objcopy
OBJDUMP := $(MIPS_BINUTILS_PREFIX)objdump
AR := ar
VENV := .venv
PYTHON         ?= $(VENV)/bin/python3

SPLAT             ?= $(PYTHON) -m splat split

SPLAT_FLAGS       ?=

# Use relocations and abi fpr names in the dump
#TODO test
OBJDUMP_FLAGS := --disassemble --reloc --disassemble-zeroes -Mreg-names=32

RM_MDEBUG = $(OBJCOPY) --remove-section .mdebug $@

OBJDUMP_CMD = $(OBJDUMP) $(OBJDUMP_FLAGS) $@ > $(@:.o=.s)
OBJCOPY_BIN = $(OBJCOPY) -O binary $@ $@.bin

# Each version gets its own asm, build output, linker inputs and checksum. src/ is shared, and the C
# files use VERSION_INTL / VERSION_US_ALPHA where the versions differ.
ifeq ($(VERSION), intl)
  GAME_CODE := SLPS_250.88
  VERSION_DEFINE := -DVERSION_INTL
  ROM_PAD :=
else ifeq ($(VERSION), us_alpha)
  GAME_CODE := SLUS_203.12
  VERSION_DEFINE := -DVERSION_US_ALPHA
  # Pad the target ROM up to .sbss so it ends where the zerofill segment starts
  ROM_PAD := --pad-to 0x56A300
else
  $(error Unsupported version $(VERSION))
endif

BUILD_DIR := build/$(VERSION)
ASM_DIR := asm/$(VERSION)
UNDEFINED_FUNCS := undefined_funcs_auto_$(VERSION).txt
UNDEFINED_SYMS := undefined_syms_auto_$(VERSION).txt

#### Files ####

# ROM image
ROM      := $(BUILD_DIR)/$(GAME_CODE).rom
ELF      := $(ROM:.rom=.elf)
MAP      := $(ROM:.rom=.map)
LDSCRIPT := $(GAME_CODE).ld

SRC_DIRS := $(shell find src -type d)
ASM_DIRS := $(shell find $(ASM_DIR) -type d -not -path "$(ASM_DIR)/nonmatchings*" 2>/dev/null)
ASSET_DIRS := $(shell find assets -type d)
LIB_DIRS      := $(foreach f,$(LIBULTRA_DIR),$f)

C_FILES        := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
S_FILES        := $(foreach dir,$(ASM_DIRS) $(SRC_DIRS),$(wildcard $(dir)/*.s))
O_FILES        := $(foreach f,$(C_FILES:.c=.o),$(BUILD_DIR)/$f) \
                  $(foreach f,$(S_FILES:.s=.o),$(BUILD_DIR)/$f)

# create build directories
$(shell mkdir -p $(foreach dir,$(SRC_DIRS) $(ASM_DIRS) $(ASSET_DIRS) compressed files $(LIB_DIRS),$(BUILD_DIR)/$(dir)))

AS_FLAGS := -no-pad-sections -EL -march=5900 -mabi=eabi -I include
C_FLAGS := -G0 -I include
D_FLAGS := -D_LANGUAGE_C $(VERSION_DEFINE)
C_FLAGS_INCLUDE := -I. -Iinclude
AS_FLAGS_INCLUDE := -I. -Iinclude

OPT_FLAGS := -O2

#### Main Targets ###

all: rom

rom: $(ROM)
	md5sum -c checksum_$(VERSION).md5

$(ROM): $(ELF)
	$(OBJCOPY) --gap-fill=0x00 -O binary $< $@

# The single LOAD segment is RWX by design (code and data share one segment, like the original ELF)
$(ELF): $(LDSCRIPT) $(O_FILES) $(UNDEFINED_FUNCS) $(UNDEFINED_SYMS)
	$(LD) -T $(LDSCRIPT) -T $(UNDEFINED_FUNCS) -T $(UNDEFINED_SYMS) -EL --no-warn-rwx-segments -Map $(MAP) -o $@

diff-init: all
	$(RM) -rf expected/$(BUILD_DIR)
	mkdir -p expected/$(BUILD_DIR)
	cp -r $(BUILD_DIR)/* expected/$(BUILD_DIR)

extract:
	$(RM) -r $(ASM_DIR)
	$(SPLAT) $(GAME_CODE).yaml $(SPLAT_FLAGS)

init:
	$(MAKE) distclean
	$(MAKE) setup
	$(MAKE) extract
	$(MAKE) all
	$(MAKE) diff-init

#TODO better padding than hard-coded end address
setup: venv
	#$(MAKE) -C tools
	$(OBJCOPY) -O binary --gap-fill=0x00 $(ROM_PAD) $(GAME_CODE) $(GAME_CODE).rom

venv:
# Create the virtual environment if it doesn't exist.
# Delete the virtual environment directory if creation fails.
	test -d $(VENV) || python3 -m venv $(VENV) || { rm -rf $(VENV); false; }
	$(PYTHON) -m pip install -U pip
	$(PYTHON) -m pip install -U -r requirements.txt

#### Main commands ####

## Cleaning ##

clean: libclean
	$(RM) -r $(BUILD_DIR)/asm $(BUILD_DIR)/assets $(BUILD_DIR)/files $(BUILD_DIR)/compressed $(BUILD_DIR)/src $(ROM) $(ELF) $(MAP)

libclean:
	$(RM) -r $(BUILD_DIR)/lib

distclean: clean
	$(RM) -r $(BUILD_DIR) $(ASM_DIR) .splache_$(VERSION)
#	$(MAKE) -C tools distclean


.PHONY: all rom clean libclean diff-init extract init distclean setup

#### Various Recipes ####

$(BUILD_DIR)/%.o: %.c
	$(CC) -c $(C_FLAGS) $(OPT_FLAGS) $(D_FLAGS) $(C_FLAGS_INCLUDE) -o $@ $<
	$(OBJDUMP_CMD)
	$(RM_MDEBUG)

$(BUILD_DIR)/%.o: %.s
	$(AS) $(AS_FLAGS) $(AS_FLAGS_INCLUDE) -o $@ $<
	$(OBJDUMP_CMD)
	$(RM_MDEBUG)

$(BUILD_DIR)/%.o: %.bin
	$(OBJCOPY) -I binary -O elf32-big $< $@
