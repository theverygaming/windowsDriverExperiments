#define __INTRINSIC_DEFINED__InterlockedAdd64
#include "paging.h"
#include "printf.h"
#include <ntddk.h>
#include <wdm.h>

static void *nt_map_phys(void *paddr, size_t size) {
    PHYSICAL_ADDRESS addr;
    addr.QuadPart = (uint64_t)paddr;
    void *mapped = MmMapIoSpace(addr, size, MmNonCached);
    if (mapped == NULL) {
        printf_serial("nt_map_phys failed... retrying cached\n");
        mapped = MmMapIoSpace(addr, size, MmCached);
        if (mapped == NULL) {
            printf_serial("nt_map_phys failed\n");
        }
    }
    return mapped;
}

static void nt_unmap_phys(void *vaddr, size_t size) {
    MmUnmapIoSpace(vaddr, size);
}

static uint64_t nt_read_phys_u64(uint64_t addr) {
    printf_serial("nt_read_phys_u64 0x%p\n", addr);
    uint64_t *ptr = (uint64_t *)nt_map_phys((void *)addr, 8);
    uint64_t val = *ptr;
    nt_unmap_phys(ptr, 8);
    return val;
}

static void nt_write_phys_u64(uint64_t addr, uint64_t data) {
    printf_serial("nt_write_phys_u64 0x%p\n", addr);
    uint64_t *ptr = (uint64_t *)nt_map_phys((void *)addr, 8);
    *ptr = data;
    nt_unmap_phys(ptr, 8);
}

union __attribute__((packed)) pml5to2_entry {
    uint64_t n;
    struct __attribute__((packed)) {
        uint64_t present : 1;
        uint64_t write_enable : 1;
        uint64_t supervisor : 1;
        uint64_t write_through : 1;
        uint64_t cache_disable : 1;
        uint64_t accessed : 1;
        uint64_t _unused1 : 1;
        uint64_t page_size : 1;
        uint64_t _unused2 : 4;
        uint64_t addr : 40;
        uint64_t _unused3 : 11;
        uint64_t execute_disable : 1;
    } s;
};

union __attribute__((packed)) pt_entry {
    uint64_t n;
    struct __attribute__((packed)) {
        uint64_t present : 1;
        uint64_t write_enable : 1;
        uint64_t supervisor : 1;
        uint64_t write_through : 1;
        uint64_t cache_disable : 1;
        uint64_t accessed : 1;
        uint64_t dirty : 1;
        uint64_t page_attr_table : 1;
        uint64_t global : 1;
        uint64_t _unused1 : 3;
        uint64_t addr : 40;
        uint64_t _unused2 : 7;
        uint64_t protection_key : 4;
        uint64_t execute_disable : 1;
    } s;
};

int map_addr(uint64_t virt, uint64_t phys) {
    // FIXME: check for 5-level paging (apparently used on newer windows server editions)
    _Static_assert(sizeof(union pml5to2_entry) == 8, "pml5to2_entry size is wrong");
    _Static_assert(sizeof(union pt_entry) == 8, "pt_entry size is wrong");

    uint64_t l4_addr;
    asm volatile("movq %%cr3, %0"
                 : "=r"(l4_addr));
    printf_serial("L4 pt addr: 0x%p\n", l4_addr);

    uint64_t next_addr = l4_addr;
    for (int i = 4; i > 1; i--) {
        size_t idx = (virt & (0x1ffull << (12 + (9 * (i - 1))))) >> (12 + (9 * (i - 1)));
        printf_serial("@L%d idx: %u\n", i, idx);
        union pml5to2_entry ent;
        ent.n = nt_read_phys_u64(next_addr + (idx * 8));
        uint64_t last_addr = next_addr;
        next_addr = ent.s.addr << 12;
        if (ent.s.present == 0) {
            printf_serial("L%d entry not present (creating)\n", i);
            PHYSICAL_ADDRESS phy64Max;
            phy64Max.QuadPart = MAXULONG64;
            uint64_t *n_pt_v = (uint64_t *)MmAllocateContiguousMemory(512 * 8, phy64Max);
            next_addr = MmGetPhysicalAddress(n_pt_v).QuadPart;
            memset(n_pt_v, 0, 512 * 8);
            ent.n = 0;
            ent.s.present = 1;
            ent.s.write_enable = 1;
            ent.s.supervisor = 1;
            ent.s.cache_disable = 1;
            ent.s.addr = next_addr >> 12;
            nt_write_phys_u64(last_addr + (idx * 8), ent.n);
            continue;
        }
        if (ent.s.page_size == 1) {
            printf_serial("error: L%d entry is mapped\n", i);
            return 1;
        }
        printf_serial("got L%d (L%d @0x%p)\n", i, i - 1, next_addr);
    }
    size_t pml1_idx = (virt & (0x1ffull << 12)) >> 12;
    union pt_entry ent;
    ent.n = 0;
    ent.s.present = 1;
    ent.s.write_enable = 1;
    ent.s.cache_disable = 1;
    ent.s.addr = phys >> 12;
    nt_write_phys_u64(next_addr + (pml1_idx * 8), ent.n);
    printf_serial("mapped virtual address 0x%p to physical address 0x%p\n", virt, phys);
    return 0;
}

int map_addr_n(uint64_t virt, uint64_t phys, size_t pages) {
    printf_serial("map_addr_n(0x%p, 0x%p, %u)\n", virt, phys, pages);
    while (pages > 0) {
        if (map_addr(virt, phys) != 0) {
            return 1;
        }
        virt += 4096;
        phys += 4096;
        pages--;
    }
    return 0;
}
