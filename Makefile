MIPS_BINUTILS_PREFIX ?= mips-linux-gnu-

BUILD_DIR := build
TOOLS_DIR := tools

#TODO
CC := COMPILER_PATH=$(TOOLS_DIR)/gcc_kmc/linux/2.7.2 $(TOOLS_DIR)/gcc_kmc/linux/2.7.2/gcc

AS      := $(MIPS_BINUTILS_PREFIX)as
LD      := $(MIPS_BINUTILS_PREFIX)ld
NM      := $(MIPS_BINUTILS_PREFIX)nm
OBJCOPY := $(MIPS_BINUTILS_PREFIX)objcopy
OBJDUMP := $(MIPS_BINUTILS_PREFIX)objdump
AR := ar

SPLAT             ?= python3.9 -m splat split

SPLAT_FLAGS       ?=

# Use relocations and abi fpr names in the dump
#TODO test
OBJDUMP_FLAGS := --disassemble --reloc --disassemble-zeroes -Mreg-names=32

RM_MDEBUG = $(OBJCOPY) --remove-section .mdebug $@

OBJDUMP_CMD = $(OBJDUMP) $(OBJDUMP_FLAGS) $@ > $(@:.o=.s)
OBJCOPY_BIN = $(OBJCOPY) -O binary $@ $@.bin

#### Files ####

# ROM image
ROM      := $(BUILD_DIR)/SLUS_203.12.rom
ELF      := $(ROM:.rom=.elf)
MAP      := $(ROM:.rom=.map)
LDSCRIPT := SLUS_203.12.ld

SRC_DIRS := $(shell find src -type d)
ASM_DIRS := $(shell find asm -type d -not -path "asm/nonmatchings*")
ASSET_DIRS := $(shell find assets -type d)
LIB_DIRS      := $(foreach f,$(LIBULTRA_DIR),$f)

C_FILES        := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.c))
S_FILES        := $(foreach dir,$(ASM_DIRS) $(SRC_DIRS),$(wildcard $(dir)/*.s))
O_FILES        := $(foreach f,$(C_FILES:.c=.o),$(BUILD_DIR)/$f) \
                  $(foreach f,$(S_FILES:.s=.o),$(BUILD_DIR)/$f)

# create build directories
$(shell mkdir -p $(foreach dir,$(SRC_DIRS) $(ASM_DIRS) $(ASSET_DIRS) compressed files $(LIB_DIRS),$(BUILD_DIR)/$(dir)))

AS_FLAGS := -no-pad-sections -EL -march=5900 -mabi=eabi -I include
C_FLAGS :=
D_FLAGS := -D_LANGUAGE_C
C_FLAGS_INCLUDE := -I. -Iinclude
AS_FLAGS_INCLUDE := -I. -Iinclude

OPT_FLAGS := -O2

#### Main Targets ###

all: rom

rom: $(ROM)
	md5sum -c checksum.md5

$(ROM): $(ELF)
	$(OBJCOPY) --gap-fill=0x00 -O binary $< $@

$(ELF): $(LDSCRIPT) $(O_FILES) undefined_funcs_auto.txt undefined_syms_auto.txt
	$(LD) -T $(LDSCRIPT) -T undefined_funcs_auto.txt -T undefined_syms_auto.txt --no-check-sections --accept-unknown-input-arch --emit-relocs -EL -Map $(MAP) -o $@

diff-init: all
	$(RM) -rf expected/$(BUILD_DIR)
	mkdir -p expected/$(BUILD_DIR)
	cp -r $(BUILD_DIR)/* expected/$(BUILD_DIR)

extract:
	$(RM) -r asm/$(VERSION) bin/$(VERSION)
	$(SPLAT) SLUS_203.12.yaml $(SPLAT_FLAGS)

init:
	$(MAKE) distclean
	$(MAKE) setup
	$(MAKE) extract
	$(MAKE) all
	$(MAKE) diff-init

setup:
	$(MAKE) -C tools

#### Main commands ####

## Cleaning ##

clean: libclean
	$(RM) -r $(BUILD_DIR)/asm $(BUILD_DIR)/assets $(BUILD_DIR)/files $(BUILD_DIR)/compressed $(BUILD_DIR)/src $(ROM) $(ROMC) $(ELF) $(BUILD_DIR)/*.map

libclean:
	$(RM) -r $(BUILD_DIR)/lib

distclean: clean
	$(RM) -r $(BUILD_DIR) asm/ assets/ files/ .splat/
	$(MAKE) -C tools distclean

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

$(BUILD_DIR)/compressed/%.o: $(BUILD_DIR)/files/%
	tools/gzip-1.2.4/gzip -c --no-name -6 < $< | python3 tools/compress_trim.py > $@.bin
	$(OBJCOPY) -I binary -O elf32-big $@.bin $@

$(BUILD_DIR)/files/game: $(BUILD_DIR)/files/game.o
	$(OBJCOPY) --dump-section .main=$@ $<

$(BUILD_DIR)/files/%: files/%
	cp $< $@

$(BUILD_DIR)/files/game.o: $(O_FILES) $(LIBULTRA_LIB) game.ld game_syms2.txt
	$(LD) -T game.ld -T game_syms2.txt -T files/syms/undefined_funcs_auto.game.txt -T files/syms/undefined_syms_auto.game.txt --no-check-sections --accept-unknown-input-arch --emit-relocs -Map $(BUILD_DIR)/game.map -o $@

#mips-linux-gnu-objcopy -O binary --gap-fill=0x00 --pad-to 0x56A300 SLUS_203.12 SLUS_203.12.rom
