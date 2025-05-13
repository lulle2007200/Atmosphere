.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

include $(DEVKITPRO)/libnx/switch_rules

TARGET		:=	emummc
BUILD		:=	build
SOURCES		:=	source/nx source source/utils source/emmc source/soc source/power source/emuMMC source/FS source/libs/fatfs
DATA		:=	data
INCLUDES	:=	include
EXEFS_SRC	:=	exefs_src


EMUMMC_INCLUDE_FATAL_PAYLOAD ?= 1

ifneq ($(BUILD),$(notdir $(CURDIR)))
EMUMMCDIR ?= $(CURDIR)
else
EMUMMCDIR ?= $(CURDIR)/../
endif


ARCH	:=	-march=armv8-a -mtune=cortex-a57 -mtp=soft -fPIE

# Current max usage is 0x4600. (512 * 34 FatFS file objects + 1 fsync buffer).
DEFINES := -DINNER_HEAP_SIZE=0x8000

CFLAGS	:=	-Wall -O2 -ffunction-sections -fdata-sections -Wno-unused-function \
			$(ARCH) $(DEFINES)

CFLAGS	+=	$(INCLUDE) -D__SWITCH__ -DEMUMMC_HAS_FATAL_PAYLOAD=$(EMUMMC_INCLUDE_FATAL_PAYLOAD)

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=$(EMUMMCDIR)/emummc.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lnx
LIBDIRS := $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(EMUMMCDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			        $(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
			        ../source/fatal/out

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))
ifeq ($(EMUMMC_INCLUDE_FATAL_PAYLOAD), 1)
	BINFILES += fatal_handler.bin
endif

ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES))
export OFILES_SRC	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES 	:=	$(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN	:=	$(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export BUILD_EXEFS_SRC := $(TOPDIR)/$(EXEFS_SRC)

ifeq ($(strip $(CONFIG_JSON)),)
	jsons := $(wildcard *.json)
	ifneq (,$(findstring $(TARGET).json,$(jsons)))
		export APP_JSON := $(TOPDIR)/$(TARGET).json
	else
		ifneq (,$(findstring config.json,$(jsons)))
			export APP_JSON := $(TOPDIR)/config.json
		endif
	endif
else
	export APP_JSON := $(TOPDIR)/$(CONFIG_JSON)
endif

.PHONY: $(BUILD) clean all

all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).kip

else
.PHONY:	all fatal_handler

all	:	fatal_handler $(OUTPUT)_unpacked.kip

ifeq ($(EMUMMC_INCLUDE_FATAL_PAYLOAD), 1) 
fatal_handler:
	@$(MAKE) --no-print-directory -C $(CURDIR)/../source/fatal
else
fatal_handler: ;

endif

DEPENDS	:=	$(OFILES:.o=.d)


$(OUTPUT)_unpacked.kip	:	$(OUTPUT).kip
	@hactool -t kip --uncompressed=$(OUTPUT)_unpacked.kip $(OUTPUT).kip

$(OUTPUT).kip	:	$(OUTPUT).elf

$(OUTPUT).elf	:	$(OFILES)

$(OFILES_SRC)	: $(HFILES_BIN)

fatal_handler.bin.o	fatal_handler_bin.h : fatal_handler.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

endif
