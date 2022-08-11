#include <memoryapi.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <handleapi.h>
#include <errhandlingapi.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

using namespace std;


char* formatBytes(unsigned long long bytes)
{
    char* result = (char*) malloc(sizeof(char) * 12);
    if (bytes == 0) {
        sprintf(result, "0 Bytes");
    } else {
        char sizes[][6] = {"Bytes", "kB", "MB", "GB", "TB", "PB"};
        int i = (int) (log(bytes) / log(1024.0));
        sprintf(result, "%.2f %s", bytes / pow(1024, i), sizes[i]);
    }

    return result;
}



void printProcessName(DWORD processId)
{
    int i;
    int processPathSize = 300;
    CHAR processPath[processPathSize];
    HANDLE processHandle;

    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processId);

    if (GetModuleFileNameExA(processHandle, NULL, processPath, processPathSize) == 0) {
        return;  // Skip unsuccessfull reads
    }

    CloseHandle(processHandle);

    // Finding the first occurence of `\`
    for (i = strlen(processPath) - 1; processPath[i] != '\\' && i >= 0; i--);

    printf("------- @%ld\t%s\n", processId, &processPath[i+1]);
}


int printAllProcesses()
{
    DWORD processList[1024], amountReturned;  // 1024 DWORD x 4 = 4.096 kB, !! 1 DWORD = 4 bytes !!

    if (EnumProcesses(processList, sizeof(processList), &amountReturned) == false) {
        return false;
    }

    int found = amountReturned / sizeof(DWORD);

    cout << "End of scan, " << found << " processes found:" << endl;

    for (DWORD i = 0; i < (DWORD) found; i++) {
        DWORD processId = processList[i];
        printProcessName(processId);
    }

    return true;
}



unsigned long printProcessModule(MEMORY_BASIC_INFORMATION info)
{
    unsigned long memUsage = 0;

    char* size = formatBytes(info.RegionSize / 1024.0);
    printf("0x%p (%s)      \t", info.BaseAddress, size);
    free(size);

    switch (info.State)
    {
        case MEM_COMMIT:
            printf("Committed"); break;

        case MEM_RESERVE:
            printf("Reserved"); break;

        case MEM_FREE:
            printf("Free    "); break;
    }

    printf("\t");

    switch (info.Type)
    {
        case MEM_IMAGE:
            printf("Image  "); break;

        case MEM_MAPPED:
            printf("Section"); break;

        case MEM_PRIVATE:
            printf("Private"); break;

        default:
            printf("%lx", info.Type); break;
    }

    if (info.State == MEM_COMMIT && (info.AllocationProtect == PAGE_READWRITE || info.AllocationProtect == PAGE_READONLY))
        memUsage += info.RegionSize;

    printf("\n");

    return memUsage;
}


unsigned long printProcessMemory(DWORD targetPid, unsigned long* totalMemUsage)
{
    HANDLE processHandle;
    MEMORY_BASIC_INFORMATION resultContainer = {};
    unsigned char* startingPoint;
    unsigned long modulesCount = 0;

    *totalMemUsage = 0;


    // Opening the process to get the starting memory address
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, false, targetPid);

    for (startingPoint = NULL;
        VirtualQueryEx(processHandle, startingPoint, &resultContainer, sizeof(resultContainer)) != 0;
        startingPoint += resultContainer.RegionSize)
    {
        *totalMemUsage += printProcessModule(resultContainer);
        modulesCount++;
    }

    CloseHandle(processHandle);

    return modulesCount;
}


// TODO (not working yet)
/* 
int* scanForValueAddress(DWORD targetPid)
{
    HANDLE processHandle;

    // Opening the process to get the starting memory address
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, false, targetPid);

    int processPathSize = 300;
    CHAR processPath[processPathSize];

    if (VirtualQueryEx(processHandle, NULL, &resultContainer, sizeof(resultContainer)) == 0) {
        return NULL;
    }

    CloseHandle(processHandle);

    printf("Base address: %p\nAllocation base address (?): %p\nAllocation protection: %lu\nPartition ID: %d\nRegion size: %llu\nState: 0x%lx\nProtect: %lx\nType: %lx\n",
            resultContainer.BaseAddress, resultContainer.AllocationBase, resultContainer.AllocationProtect, resultContainer.PartitionId,
            resultContainer.RegionSize, resultContainer.State, resultContainer.Protect, resultContainer.Type);

    // Returning random non-null pointer
    return (int*) malloc(sizeof(int));
}
*/



unsigned long long readLLUserInput(const char text[])
{
    printf("%s\n>>> ", text);

    // Reading input and parsing it to a `long long`
    char input[16];
    fgets(input, 16, stdin);
    unsigned long long parsed = atoll(input);

    // While the user input is not numeric
    while (parsed == 0) {
        printf("Wrong input.\n>>> ");
        fgets(input, 16, stdin);
        parsed = atoi(input);
    }

    return parsed;
}


int main()
{
    if (printAllProcesses() == false) {
        return 1;
    }

    // Getting the target PID
    long long targetPid = readLLUserInput("Please enter the PID you want to analyze the memory from.");

    // long long value = readLLUserInput("Please enter the value to be located in the process memory.");

    // int* address = getValueAddress((DWORD) targetPid);
    unsigned long totalMemUsage;
    unsigned long modulesCount = printProcessMemory(targetPid, &totalMemUsage);

    char* humanReadableMemoryUsage = formatBytes(totalMemUsage);

    printf("-------------------------------------------------------------");
    printf("\nTOTAL MEMORY USAGE FOR PID %llu: %s in %lu modules.\n\n", targetPid, humanReadableMemoryUsage, modulesCount);
    free(humanReadableMemoryUsage);

    // Get address: failed case
    // if (address == NULL) {
    //     printf("Failed to get the base memory address of the process %llu", targetPid);
    //     printf(" (error code: %ld).\n", GetLastError());
    //     return 1;
    // }

    // Else: do stuff here


    return 0;
}
