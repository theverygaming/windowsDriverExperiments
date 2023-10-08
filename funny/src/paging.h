#pragma once
#include <stdint.h>

// nonzero return value on error
int map_addr(uint64_t virt, uint64_t phys);
int map_addr_n(uint64_t virt, uint64_t phys, size_t pages);
