CC = x86_64-w64-mingw32-gcc
CFLAGS += -I/usr/i686-w64-mingw32/include/ddk/ -I../mingw-w64/mingw-w64-headers/ddk/include/ddk -O3 -Wall -Wextra
LDFLAGS += -nostdlib -nostartfiles -shared -lntoskrnl -Wl,--entry,DriverEntry # -s  

CFILES = src/drvmain.c \
		 src/cpu.c \
		 src/serial.c \
		 src/printf.c \
		 src/paging.c \
		 src/protected.S

OBJS = $(addsuffix .o, $(basename $(CFILES)))

.PHONY: all
all: drv.sys stage2.bin

stage2.bin: stage2.nasm
	nasm -f bin $< -o $@

drv.sys: $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf drv.sys $(OBJS)
