CC=clang
CFLAGS=-ggdb -fblocks -Icortex-m0p -Iperipherals -Icore -std=c11
SRC=core/mcu.c core/gdb.c peripherals/ram.c peripherals/flash.c peripherals/uart.c peripherals/unittest.c cortex-m0p/mcu.c simulator.c
OBJS=$(SRC:.c=.o)

simulator: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

.c.o:
	$(CC) -c $(CFLAGS) -MMD -MF $<.d -o $@ $<

clean:
	rm -f simulator $(OBJS) $(SRC:.c=.c.d)

-include $(SRC:.c=.c.d)
