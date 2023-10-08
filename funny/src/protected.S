.code64

// void __attribute__((sysv_abi)) switchtoprotected(uint64_t jmp, uint64_t mem_map_ptr, uint64_t stack_ptr);
// jmp: %rdi, mem_map_ptr: %rsi, stack_ptr: %rdx
.global switchtoprotected
switchtoprotected:
cli

// load 32-bit CS
leaq .go32(%rip), %rax
pushq $0x18
push %rax
retfq

.go32:
.code32

mov %edx, %esp

// disable paging, set protected mode
mov $(1 << 0), %eax
mov %eax, %cr0
// we are now in protected mode!

mov %esi, %eax // mem_map_ptr in %eax
jmpl *%edi
.global switchtoprotected_end
switchtoprotected_end:
