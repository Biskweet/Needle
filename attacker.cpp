#include <memoryapi.h>
#include <psapi.h>
#include <processthreadsapi.h>
#include <handleapi.h>

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

    // Opening the process to get the starting memory address
    processHandle = OpenProcess(PROCESS_QUERY_INFORMATION, false, processId);

    if (ReadProcessMemory(processHandle, NULL, processName, processNameSize) == 0) {
        return NULL;
    }

    CloseHandle(processHandle);
}


int main()
{
    if (printAllProcesses() == false) {
        return 1;
    }

    char input[16];
    cout << "Please enter the process ID you want to analyze the memory from\n>>> " << endl;

    while (fgets(input, 16, stdin) == NULL) {
        cout << "Wrong input." << endl;
    }

    int targetProcessId;
    do {
        fgets(input, 16, stdin);
        targetProcessId = atoi(input);
    } while (targetProcessId == 0);

    int* address = getValueAddress(targetProcessId);


    return 0;
}
