APP = phil-phaselock

CC = $(CROSS_COMPILE)gcc
AS = $(CROSS_COMPILE)as

BSPDIR = ../../bsp

CFLAGS  = -I$(BSPDIR) -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard -O0 -g3 -Wall -Werror -Wpedantic
ASFLAGS  = -mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard
LDFLAGS += -specs=$(BSPDIR)/Xilinx.spec -T$(BSPDIR)/ldscript.ld -L$(BSPDIR) -l:libxil.a

VPATH = $(BSPDIR)	

$(APP): _exit.o

clean:
	rm $(APP)