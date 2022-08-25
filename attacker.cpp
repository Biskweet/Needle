#include <windows.h>
// #include <memoryapi.h>
#include <psapi.h>
// #include <processthreadsapi.h>
// #include <handleapi.h>
// #include <errhandlingapi.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>

using namespace std;


typedef struct _infoNode {
    MEMORY_BASIC_INFORMATION* info;
    struct _infoNode* next;
} InfoNode;


typedef struct _addressNode {
    LPCVOID address;
    struct _addressNode* next;
} AddressNode;



char* formatBytes(unsigned long long bytes)
{
    char* result = (char*) malloc(sizeof(char) * 16);
    if (result == NULL) return NULL;

    if (bytes == 0) {
        sprintf(result, "0 Bytes");
    } else {
        char sizes[][6] = { "Bytes", "kB", "MB", "GB", "TB", "PB" };
        int i = (int) (log(bytes) / log(1024.0));

        sprintf(result, "%.2f %s", bytes / pow(1024, i), sizes[i]);
    }

    return result;
}



void printProcessName(DWORD processId)
{
    int i;
    const int processPathSize = 300;
    CHAR processPath[processPathSize];
    HANDLE processHandle;

    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processId);

    if (GetModuleFileNameExA(processHandle, NULL, processPath, processPathSize) == 0) {
        return;  // Skip unsuccessfull reads
    }

    CloseHandle(processHandle);

    // Finding the first occurence of `\`
    for (i = strlen(processPath) - 1; processPath[i] != '\\' && i >= 0; i--);

    printf("------- @%ld\t%s\n", processId, processPath + i + 1);
}


int listAllProcesses()
{
    DWORD processList[1024], amountReturned;  //!! 1 DWORD = 4 bytes !! 1024 DWORD x 4 bytes = 4.096 kB !!

    if (EnumProcesses(processList, sizeof(processList), &amountReturned) == false) {
        return false;
    }

    int found = amountReturned / sizeof(DWORD);

    printf("End of scan, %d processes found:\n", found);

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
    printf("0x%p (%s)     \t", info.BaseAddress, size);
    free(size);

    switch (info.State)
    {
    case MEM_COMMIT:
        printf("Committed"); break;

    case MEM_RESERVE:
        printf("Reserved "); break;

    case MEM_FREE:
        printf("Free     "); break;
    }

    printf("\t");

    switch (info.Type)
    {
    case MEM_IMAGE:
        printf("Image    "); break;

    case MEM_MAPPED:
        printf("Section  "); break;

    case MEM_PRIVATE:
        printf("Private  "); break;

    default:
        printf("0x%lx", info.Type); break;
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


void printSelectedModules(InfoNode* info)
{
    while (info != NULL) {
        printProcessModule(*(info->info));
        info = info->next;
    }
}


InfoNode* createProcessInfoNode(MEMORY_BASIC_INFORMATION info)
{
    InfoNode* node = (InfoNode*) malloc(sizeof(InfoNode));
    if (node == NULL) return NULL;

    node->info = (MEMORY_BASIC_INFORMATION*) malloc(sizeof(MEMORY_BASIC_INFORMATION));
    if (node->info == NULL) return NULL;

    // Copying the MEM._BASIC_INFO block
    node->info->BaseAddress = info.BaseAddress;
    node->info->AllocationBase = info.AllocationBase;
    node->info->AllocationProtect = info.AllocationProtect;
    node->info->PartitionId = info.PartitionId;
    node->info->RegionSize = info.RegionSize;
    node->info->State = info.State;
    node->info->Protect = info.Protect;
    node->info->Type = info.Type;

    node->next = NULL;

    return node;
}

AddressNode* createAddressNode(LPCVOID address)
{
    AddressNode* node = (AddressNode*) malloc(sizeof(AddressNode));

    if (node != NULL) {
        node->next = NULL;
    }

    return node;
}


void freeInfoNode(InfoNode* node)
{
    free(node->info);
    free(node);
}


long long readLLUserInput(const char text[])
{
    printf("%s\n>>> ", text);

    // Reading input and parsing it to a `long long`
    char input[16];
    fgets(input, 16, stdin);
    long long parsed = atoll(input);

    // While the user input is not numeric
    while (parsed == 0) {
        printf("Wrong input.\n>>> ");
        fgets(input, 16, stdin);
        parsed = atoi(input);
    }

    return parsed;
}




void printAddresses(AddressNode* addr)
{
    printf("Possible addresses:\n");
    while (addr != NULL) {
        printf("\t- 0x%p\n", addr->address);
        addr = addr->next;
    }
}


AddressNode* scanMemoryForValue(DWORD targetPid, InfoNode* modules, int value, int readableModulesCount)
{
    AddressNode* addresses = NULL;
    AddressNode* newAddress;

    HANDLE processHandle = OpenProcess(PROCESS_VM_READ, false, targetPid);
    int buffer[12];
    size_t NumberOfBytesRead;

    int count = 0;

    // Iterating over all readable modules
    while (modules != NULL) {
        for (LPVOID address = modules->info->BaseAddress; ReadProcessMemory(processHandle, (LPVOID) address[i], (LPVOID) buffer, sizeof(value), &NumberOfBytesRead) != 0; address = (LPVOID) (address + sizeof(int))) {            
            // address < ((uint8_t*) modules->info->BaseAddress + modules->info->RegionSize);

            // Found the value somewhere!
            if (value == *buffer) {
                // Append to linked list
                newAddress = createAddressNode(address);
                newAddress->next = addresses;
                addresses = newAddress;
            }
        }

        printf("Scanned module %d/%d\r", count, readableModulesCount);
        count++;

        modules = modules->next;
    }
    printf("\n");


    CloseHandle(processHandle);

    return addresses;
}



InfoNode* getReadableModules(DWORD targetPid, unsigned long* modulesCount, unsigned long* totalMemUsage, unsigned long* readableModulesCount)
{
    HANDLE processHandle;
    MEMORY_BASIC_INFORMATION resultContainer = {};

    unsigned char* startingPoint;
    InfoNode* newNode, * node = NULL;

    *readableModulesCount = 0;
    *modulesCount = 0;
    *totalMemUsage = 0;

    // Opening the process to get the starting memory address
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, false, targetPid);
    int code = 1;

    for (startingPoint = NULL; code != 0; startingPoint += resultContainer.RegionSize) {
        code = VirtualQueryEx(processHandle, startingPoint, &resultContainer, sizeof(MEMORY_BASIC_INFORMATION));
        *totalMemUsage += printProcessModule(resultContainer);
        *modulesCount += 1;

        if (resultContainer.State == MEM_COMMIT) {
            newNode = createProcessInfoNode(resultContainer);
            newNode->next = node;
            node = newNode;
            *readableModulesCount += 1;
        }
    }


    CloseHandle(processHandle);

    return node;
}



int main()
{
    if (listAllProcesses() == false) {
        return 1;
    }

    // Getting the target PID
    long long targetPid = readLLUserInput("Please enter the PID you want to analyze the memory from.");

    unsigned long modulesCount, readableModulesCount, totalMemUsage;

    InfoNode* readableModules = getReadableModules(targetPid, &modulesCount, &totalMemUsage, &readableModulesCount);

    char* humanReadableMemoryUsage = formatBytes(totalMemUsage);
    printf("-------------------------------------------------------------");
    printf("\nTOTAL MEMORY USAGE FOR PID %llu: %s in %lu modules (%lu readable).\n\n", targetPid, humanReadableMemoryUsage, modulesCount, readableModulesCount);
    free(humanReadableMemoryUsage);

    printf("Selected memory blocks:\n");
    printSelectedModules(readableModules);


    long long value = readLLUserInput("Please enter the value to be located in the process memory.");

    AddressNode* addresses = scanMemoryForValue(targetPid, readableModules, value, readableModulesCount);

    printAddresses(addresses);


    return 0;
}
