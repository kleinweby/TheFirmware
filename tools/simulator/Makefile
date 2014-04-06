CC=clang
LLVM_CONFIG=/usr/local/Cellar/llvm/3.4/bin/llvm-config
LLVM_CFLAGS=$(shell $(LLVM_CONFIG) --cflags)
LLVM_LDFLAGS=$(shell $(LLVM_CONFIG) --ldflags --libs)
CFLAGS=-g -ggdb -fblocks -Icortex-m0p -Iperipherals -Icore -std=c11 -Wall $(LLVM_CFLAGS)
LDFLAGS=-lev $(LLVM_LDFLAGS)
SRC=core/mcu.c core/mcu_kernel.c core/gdb.c core/elf.c peripherals/ram.c peripherals/flash.c peripherals/uart.c peripherals/unittest.c cortex-m0p/mcu.c simulator.c
OBJS=$(SRC:.c=.o)

simulator: $(OBJS)
	$(CC)++ $(CFLAGS) $(LDFLAGS) -o $@ $^

.c.o:
	$(CC) -c $(CFLAGS) -MMD -MF $<.d -o $@ $<

clean:
	rm -f simulator $(OBJS) $(SRC:.c=.c.d)

-include $(SRC:.c=.c.d)
