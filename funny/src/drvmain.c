#include "printf.h"
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

            printf_serial("%s\n", buffer);
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

VOID UnloadRoutine(IN PDRIVER_OBJECT DriverObject) {
    serial_init();
    printf_serial("\ntrying to take over kernel\n");

    PHYSICAL_ADDRESS phyMax;
    phyMax.QuadPart = 0xFFFFFFFF;
    void *virtual = MmAllocateContiguousMemory(10000000, phyMax);

    if (virtual == NULL) {
        printf_serial("could not allocate memory\n");
        printf_serial("returning control to NT kernel\n");
        return;
    }

    void *physical = MmGetPhysicalAddress(virtual).QuadPart;

    printf_serial("mapped 0x%p (MmGetPhysicalAddress: 0x%p)\n", virtual, physical);

    printf_serial("filesize: %u\n", getFileSize(L"\\DosDevices\\C:\\example.txt"));

    if (!readFileToMemory(L"\\DosDevices\\C:\\example.txt", (char *)virtual, 1000000)) {
        printf_serial("could not read file\n");
        printf_serial("returning control to NT kernel\n");
        return;
    }

    uint64_t cr3;
    asm volatile("mov %%cr3, %0"
                 : "=r"(cr3)
                 :);
    uint64_t cr4;
    asm volatile("mov %%cr4, %0"
                 : "=r"(cr4)
                 :);

    printf_serial("cr3: 0x%p cr4: 0x%p\n", cr3, (uintptr_t)cr4);

    printf_serial("returning control to NT kernel\n");
    return;

    while (1) {
        asm volatile("cli;hlt");
    }
}
