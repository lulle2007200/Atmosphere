ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/base_rules

################################################################################

IPL_LOAD_ADDR := 0x40010000

################################################################################

THIS_MAKEFILE     := $(abspath $(lastword $(MAKEFILE_LIST)))
CURRENT_DIRECTORY := $(abspath $(dir $(THIS_MAKEFILE)))

BUILDDIR := build
OUTPUTDIR := out
SOURCEDIR = fatal_handler
BDKDIR := bdk
GFXDIR := fatal_handler/gfx
TARGET := fatal_handler

BDKINC := -I$(BDKDIR)
GFXINC   := -I$(GFXDIR)

GFX_INC := '"gfx.h"'

VPATH = $(dir ./$(SOURCEDIR)/) $(dir $(wildcard ./$(SOURCEDIR)/*/)) $(dir $(wildcard ./$(SOURCEDIR)/*/*/))
VPATH += $(dir $(wildcard ./$(BDKDIR)/)) $(dir $(wildcard ./$(BDKDIR)/*/)) $(dir $(wildcard ./$(BDKDIR)/*/*/))

OBJS = $(addprefix $(BUILDDIR)/$(TARGET)/, \
	start.o exception_handlers.o irq.o main.o \
	timer.o heap.o hw_init.o clock.o i2c.o gpio.o \
	max7762x.o di.o util.o fuse.o pinmux.o \
	secmon_exo.o gfx.o bpmp.o sdram.o minerva.o btn.o max77620-rtc.o se.o mc.o)



################################################################################


# BDK defines.
CUSTOMDEFINES := -DGFX_INC=$(GFX_INC) 
CUSTOMDEFINES += -DBDK_MALLOC_NO_DEFRAG -DBDK_MC_ENABLE_AHB_REDIRECT -DBDK_EMUMMC_ENABLE
CUSTOMDEFINES += -DBDK_WATCHDOG_FIQ_ENABLE -DBDK_RESTART_BL_ON_WDT

INCDIRS := $(BDKINC) $(GFXINC)

WARNINGS := -Wall -Wsign-compare -Wno-array-bounds -Wno-stringop-overflow

ARCH := -march=armv4t -mtune=arm7tdmi -mthumb-interwork
CFLAGS = $(ARCH) -O2 -g -nostdlib -ffunction-sections -fdata-sections -fomit-frame-pointer -std=gnu11 $(WARNINGS) $(CUSTOMDEFINES)
LDFLAGS = $(ARCH) -nostartfiles -lgcc -Wl,--nmagic,--gc-sections -Xlinker --defsym=IPL_LOAD_ADDR=$(IPL_LOAD_ADDR)

################################################################################

.PHONY: all clean 

all: $(OUTPUTDIR)/$(TARGET).bin

clean:
	@rm -rf $(OBJS)

$(OUTPUTDIR)/$(TARGET).bin: $(BUILDDIR)/$(TARGET)/$(TARGET).elf
	$(OBJCOPY) -S -O binary $< $(OUTPUTDIR)/$(TARGET).bin

$(BUILDDIR)/$(TARGET)/$(TARGET).elf: $(OBJS)
	@$(CC) $(LDFLAGS) -T $(SOURCEDIR)/link.ld $^ -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.c
	@$(CC) $(CFLAGS) $(INCDIRS) -c $< -o $@

$(BUILDDIR)/$(TARGET)/%.o: %.S
	@$(CC) $(CFLAGS) -c $< -o $@

$(OBJS): $(BUILDDIR)/$(TARGET)

$(BUILDDIR)/$(TARGET):
	@mkdir -p "$(BUILDDIR)"
	@mkdir -p "$(BUILDDIR)/$(TARGET)"
	@mkdir -p "$(OUTPUTDIR)"


