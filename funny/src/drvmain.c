#define __INTRINSIC_DEFINED__InterlockedAdd64
#include "paging.h"
#include "printf.h"
#include "protected.h"
#include "serial.h"
#include <ntddk.h>
#include <stdbool.h>
#include <stdint.h>
#include <wdm.h>

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath);
VOID UnloadRoutine(IN PDRIVER_OBJECT DriverObject);

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath) {
    DriverObject->DriverUnload = UnloadRoutine;
    return STATUS_SUCCESS;
}

static size_t getFileSize(const char *path) {
    size_t filesize = 0;

    UNICODE_STRING uniName;
    OBJECT_ATTRIBUTES objAttr;

    RtlInitUnicodeString(&uniName, path); // path like L"\\DosDevices\\C:\\example.txt"
    InitializeObjectAttributes(&objAttr, &uniName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE handle;
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK ioStatusBlock;

    ntstatus = ZwCreateFile(&handle,
                            GENERIC_READ,
                            &objAttr,
                            &ioStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            0,
                            FILE_OPEN,
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0);
    if (NT_SUCCESS(ntstatus)) {
        FILE_STANDARD_INFORMATION fileInfo = {0};
        ntstatus = ZwQueryInformationFile(
            handle,
            &ioStatusBlock,
            &fileInfo,
            sizeof(fileInfo),
            FileStandardInformation);
        if (NT_SUCCESS(ntstatus)) {
            filesize = fileInfo.EndOfFile.QuadPart;
        } else {
            printf_serial("could not read from file\n");
            return filesize;
        }
        ZwClose(handle);
        return filesize;
    } else {
        printf_serial("could not create file handle\n");
        return filesize;
    }
}

static bool readFileToMemory(const char *path, char *buffer, size_t size) {
    UNICODE_STRING uniName;
    OBJECT_ATTRIBUTES objAttr;

    RtlInitUnicodeString(&uniName, path); // path like L"\\DosDevices\\C:\\example.txt"
    InitializeObjectAttributes(&objAttr, &uniName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    HANDLE handle;
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK ioStatusBlock;

    LARGE_INTEGER byteOffset;

    ntstatus = ZwCreateFile(&handle,
                            GENERIC_READ,
                            &objAttr,
                            &ioStatusBlock,
                            NULL,
                            FILE_ATTRIBUTE_NORMAL,
                            0,
                            FILE_OPEN,
                            FILE_SYNCHRONOUS_IO_NONALERT,
                            NULL,
                            0);
    if (NT_SUCCESS(ntstatus)) {
        byteOffset.LowPart = byteOffset.HighPart = 0;
        ntstatus = ZwReadFile(handle, NULL, NULL, NULL, &ioStatusBlock, buffer, size, &byteOffset, NULL);
        if (NT_SUCCESS(ntstatus)) {
            buffer[size - 1] = '\0';

            printf_serial("FILE: %s\n", buffer);
        } else {
            printf_serial("could not read from file\n");
            return false;
        }
        ZwClose(handle);
        return true;
    } else {
        printf_serial("could not create file handle\n");
        return false;
    }
}

// osdev discord gdt
static uint64_t gdt[] = {
    0x0000000000000000, // null
    0x00009a000000ffff, // 16-bit code
    0x000093000000ffff, // 16-bit data
    0x00cf9a000000ffff, // 32-bit code
    0x00cf93000000ffff, // 32-bit data
    0x00af9b000000ffff, // 64-bit code
    0x00af93000000ffff, // 64-bit data
    0x00affb000000ffff, // usermode 64-bit code
    0x00aff3000000ffff  // usermode 64-bit data
    /* at some stage (presumably when starting to work on usermode) you'll need at least one TSS desriptor
     * in order to set that up, you'll need some knowledge of the segment descriptor and TSS layout */
};

void loadgdt() {
    struct __attribute__((packed)) {
        uint16_t size;
        uint64_t *base;
    } gdtr = {sizeof(gdt) - 1, gdt};

    asm volatile("lgdt %0" ::"m"(gdtr));
}

extern uint8_t switchtoprotected_end;

VOID UnloadRoutine(IN PDRIVER_OBJECT DriverObject) {
    serial_init();
    printf_serial("\n:trolley:\n");

    PHYSICAL_ADDRESS phy32Max;
    phy32Max.QuadPart = 0xFFFFFFFF;
    void *stage1 = MmAllocateContiguousMemory(10000000, phy32Max);
    void *stage2 = MmAllocateContiguousMemory(10000000, phy32Max);

    if (stage1 == NULL || stage2 == NULL) {
        printf_serial("could not allocate memory\n");
        printf_serial("returning control to NT kernel\n");
        return;
    }

    printf_serial("mapped 0x%p (MmGetPhysicalAddress: 0x%p)\n", stage2, MmGetPhysicalAddress(stage2).QuadPart);

    printf_serial("filesize: %u\n", getFileSize(L"\\DosDevices\\C:\\Documents and Settings\\Administrator\\Desktop\\Downloads\\stage2.bin"));

    if (!readFileToMemory(L"\\DosDevices\\C:\\Documents and Settings\\Administrator\\Desktop\\Downloads\\stage2.bin", (char *)stage2, 1000000)) {
        printf_serial("could not read stage 2 file\n");
        printf_serial("returning control to NT kernel\n");
        return;
    }

    // build memory map
    PPHYSICAL_MEMORY_RANGE phys_mem_ranges = MmGetPhysicalMemoryRanges();
    printf_serial("memory map:\n");
    int map_count = 0;
    while ((phys_mem_ranges[map_count].BaseAddress.QuadPart != 0) && (phys_mem_ranges[map_count].NumberOfBytes.QuadPart != 0)) {
        printf_serial("    -> [%d] b: 0x%p len: 0x%p\n", map_count, phys_mem_ranges[map_count].BaseAddress.QuadPart, phys_mem_ranges[map_count].NumberOfBytes.QuadPart);
        map_count++;
    }
    struct __attribute__((packed)) memmap_entry {
        uint64_t base;
        uint64_t size;
    };
    struct memmap_entry *memmap = MmAllocateContiguousMemory(sizeof(struct memmap_entry) * (map_count + 1), phy32Max);
    if (memmap == NULL) {
        printf_serial("could not allocate memory\n");
        printf_serial("returning control to NT kernel\n");
        return;
    }
    // terminating entry
    memmap[map_count].base = 0;
    memmap[map_count].size = 0;
    for (int i = 0; i < map_count; i++) {
        memmap[i].base = phys_mem_ranges[map_count].BaseAddress.QuadPart;
        memmap[i].size = phys_mem_ranges[map_count].NumberOfBytes.QuadPart;
    }

    uint64_t stage1_phys = MmGetPhysicalAddress(stage1).QuadPart;
    printf_serial("physicall address of stage1: 0x%p\n", stage1_phys);
    memcpy(stage1, switchtoprotected, (uint64_t)&switchtoprotected_end - (uint64_t)switchtoprotected);
    void __attribute__((sysv_abi)) (*switchtoprotected_ptr)(uint64_t jmp, uint64_t mem_map_ptr, uint64_t stack_ptr) = stage1_phys;

    uint64_t jmp_addr = MmGetPhysicalAddress(stage2).QuadPart;
    uint64_t mem_map_ptr = MmGetPhysicalAddress(memmap).QuadPart;
    uint64_t stack_ptr = 0x2000;
    printf_serial("entering protected mode (jump: 0x%p, memmap: 0x%p, stack: 0x%p)\n", jmp_addr, mem_map_ptr, stack_ptr);
    map_addr_n(stage1_phys, stage1_phys, (((uint64_t)&switchtoprotected_end - (uint64_t)switchtoprotected) / 4096) + 1);
    // jump to protected mode
    printf_serial("loading GDT\n");
    loadgdt();
    printf_serial("loaded GDT\n");
    printf_serial("switching to protected mode\n");
    switchtoprotected_ptr(jmp_addr, mem_map_ptr, stack_ptr);
}
