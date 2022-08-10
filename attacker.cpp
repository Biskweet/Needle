#include <memoryapi.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <handleapi.h>
#include <errhandlingapi.h>

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>


using namespace std;


void printProcess(DWORD processId) {
    int processNameSize = 300;
    CHAR processName[processNameSize];
    HANDLE processHandle;

    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, false, processId);

    if (GetModuleFileNameExA(processHandle, NULL, processName, processNameSize) == 0) {
        return;  // Skip unsuccessfull reads
    }

    CloseHandle(processHandle);

    cout << "------- @" << processId << "\t" << processName << endl;
}


int printAllProcesses() {
    DWORD processList[1024], amountReturned;  // 1024 DWORD x 4 = 4.096 kB, !! 1 DWORD = 4 bytes !!

    if (EnumProcesses(processList, sizeof(processList), &amountReturned) == false) {
        return false;
    }

    int found = amountReturned / sizeof(DWORD);

    cout << "End of scan, " << found << " processes found:" << endl;

    for (DWORD i = 0; i < (DWORD) found; i++) {
        DWORD processId = processList[i];
        printProcess(processId);
    }

    return true;
}


// TODO (not working yet)
int* getValueAddress(DWORD targetProcessId) {
    HANDLE processHandle;
    MEMORY_BASIC_INFORMATION resultContainer = {};

    // Opening the process to get the starting memory address
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, false, targetProcessId);

    int processNameSize = 300;
    CHAR processName[processNameSize];
    if (GetModuleFileNameExA(processHandle, NULL, processName, processNameSize) != 0) {
        cout << "Found process " << processName << " at address " << targetProcessId << " OK" << endl;
    }

    if (VirtualQueryEx(processHandle, NULL, &resultContainer, sizeof(resultContainer)) == 0) {
        return NULL;
    }

    CloseHandle(processHandle);

    // Returning random non-null pointer
    return (int*) malloc(sizeof(int));
}


int main()
{
    if (printAllProcesses() == false) {
        return 1;
    }

    printf("Please enter the process ID you want to analyze the memory from\n>>> ");

    // Reading input and parsing it to a `long long`
    char input[16];
    fgets(input, 16, stdin);
    unsigned long long targetProcessId = atoll(input);

    // While the user input is not numeric
    while (targetProcessId == 0) {
        printf("Wrong input.\n>>> ");
        fgets(input, 16, stdin);
        targetProcessId = atoi(input);
    }

    int* address = getValueAddress((DWORD) targetProcessId);

    // Get address: failed case
    if (address == NULL) {
        printf("Failed to get the base memory address of the process %llu", targetProcessId);
        printf(" (error code: %d).\n", GetLastError());
        return 1;
    }

    // Else: do stuff here


    return 0;
}
