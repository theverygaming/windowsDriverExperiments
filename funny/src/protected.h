#pragma once

void __attribute__((sysv_abi)) switchtoprotected(uint64_t jmp, uint64_t mem_map_ptr, uint64_t stack_ptr);
