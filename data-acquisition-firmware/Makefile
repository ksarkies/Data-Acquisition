# Basic makefile K Sarkies

PROJECT		    = data-acquisition
PREFIX		    = arm-none-eabi
CC			    = $(PREFIX)-gcc
LD			    = $(PREFIX)-gcc
OBJCOPY		    = $(PREFIX)-objcopy
OBJDUMP		    = $(PREFIX)-objdump
GDB			    = $(PREFIX)-gdb
NM 			    = $(PREFIX)-nm

LIBRARY_DIR     = /home/ksarkies/Development-Software/arm-library
DRIVERS_DIR	    = $(LIBRARY_DIR)/libopencm3-examples/libopencm3
DRIVERS_SRC     = $(DRIVERS_DIR)/lib/stm32/f1
DRIVERS_INC	    = $(DRIVERS_DIR)/include
LIBS_DIR        = ../libs
FATFSDIR        = ../chan-fat-stm32-loc3

VPATH           += $(FATFSDIR)  $(LIBS_DIR)

INCLUDES	    = $(patsubst %,-I%,$(DRIVERS_INC) $(FATFSDIR) $(LIBS_DIR))

CFLAGS		    += -Os -g -Wall -Wextra -I. $(INCLUDES) -fno-common \
			       -mcpu=cortex-m3 -mthumb -msoft-float -MD -DSTM32F1

LDSCRIPT        = stm32-h103RBT6.ld

LDFLAGS		    += -I . -Wl,--start-group -lc -lgcc -lnosys -Wl,--end-group \
			       -T$(LDSCRIPT) -L$(DRIVERS_DIR)/lib -lopencm3_stm32f1 \
			       -nostartfiles -Wl,--gc-sections -mthumb -march=armv7 \
			       -mfix-cortex-m3-ldrd -msoft-float

# The libopencm3 library is assumed to exist in libopencm3/lib, otherwise add files here
CFILES		    = $(PROJECT)-stm32f103.c $(PROJECT)-objdic.c
CFILES          += buffer.c hardware.c comms.c file.c
CFILES          += ff.c fattime.c sd_spi_loc3_stm32.c

OBJS		    = $(CFILES:.c=.o)

all: $(PROJECT).elf $(PROJECT).bin $(PROJECT).hex $(PROJECT).lss \
     $(PROJECT).list $(PROJECT).sym

$(PROJECT).elf: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LDFLAGS)

$(PROJECT).hex: $(PROJECT).elf
	$(OBJCOPY) -O ihex  $< $@

$(PROJECT).bin: $(PROJECT).elf
	$(OBJCOPY) -O binary  $< $@

$(PROJECT).list: $(PROJECT).elf
	$(OBJDUMP) -S $< > $@

$(PROJECT).lss: $(PROJECT).elf
	$(OBJDUMP) -h -S $< > $@

$(PROJECT).sym: $(PROJECT).elf
	$(NM) -n $< > $@

clean:
	rm *.elf *.o *.d *.hex *.list *.lss *.sym *.bin

# Using CC and CFLAGS will cause any object files to be built implicitely if they are missing.
# We are searching an archive library opencm3_stm32f1.a which has been precompiled, so we don't need
# to recompile the DRIVERS source.
# The libopencm3 source has been added in case the archive doesn't have them, VPATH finds them.

