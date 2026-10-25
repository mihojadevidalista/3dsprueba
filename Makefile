#---------------------------------------------------------------------------------
# Makefile minimo para homebrew de 3DS (devkitARM + libctru)
#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Define DEVKITARM en tu entorno: export DEVKITARM=$(DEVKITPRO)/devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := hola3ds
BUILD       := build
SOURCES     := source
INCLUDES    := include

APP_TITLE   := Hola 3DS
APP_DESCRIPTION := Mi primer homebrew
APP_AUTHOR  := Yo

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -g -Wall -O2 -mword-relocations -ffunction-sections $(ARCH) \
               $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
               -I$(CTRULIB)/include -D__3DS__
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
LIBS        := -lctru -lm
LIBDIRS     := $(CTRULIB)

export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES      := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OFILES := $(CFILES:.c=.o)
export LD   := $(CC)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile build

$(BUILD):
	@mkdir -p $@

clean:
	@echo limpiando ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(OUTPUT).smdh $(TARGET).elf

ifeq ($(MAKECMDGOALS),build)
build: $(OUTPUT).3dsx

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh
$(OUTPUT).elf: $(OFILES)

-include $(DEPSDIR)/*.d
endif
