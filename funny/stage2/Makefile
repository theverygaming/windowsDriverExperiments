CC = i686-elf-gcc
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy
CFLAGS += -O3 -nostdlib -fno-pie -mno-red-zone -ffreestanding -fno-exceptions -mno-mmx -mno-sse -mno-sse2
LDFLAGS += -nostdlib

CFILES = test.c \
		 test2.c \
		 entry.S

OBJS = $(addsuffix .o, $(basename $(CFILES)))

.PHONY: all
all: stage2.o

stage2.o: $(OBJS)
	$(LD) -T linker.ld -o $@ $(OBJS) $(LDFLAGS)
	$(OBJCOPY) -j .text -j .data -j .rodata -O binary $@ stage2.bin
	#$(LD) -T linker.ld --oformat binary -o stage2.bin $(OBJS) $(LDFLAGS)

%.o: %.S
	$(CC) -o $@ -c $< $(CFLAGS)

%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf stage2.o stage2.bin $(OBJS)
