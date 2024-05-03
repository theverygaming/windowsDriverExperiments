#define outb(port, value) asm volatile("outb %%al, %%dx" ::"d"(port), "a"(value))

int _c_entry() {
    outb(0xE9, 'H');
    outb(0xE9, 'i');
    outb(0xE9, '!');
    outb(0xE9, '\n');
}
