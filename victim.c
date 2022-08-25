#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


int main() {
    srand(time(NULL));

    char* mess = (char*) malloc(sizeof(char) * 20);
    sprintf(mess, "This is a test!");

    int mystery;
    char running = 1;

    while (running) {
        mystery = rand();
    	printf("PID: %lu, Message: \"%s\", Address: 0x%p\n", GetCurrentProcessId(), mess, mess);
        printf("Mystery number: %d (size=%llu bytes, address=0x%p).\n", mystery, sizeof(mystery), &mystery);

        printf("Now waiting...\n");

        char buf[2];
        fgets(buf, 2, stdin);

        if (buf[0] == 'x') running = 0;

        // Re print the value to check if it's still the same
        else while (buf[0] == 'r') {
            printf("PID: %lu, Message: \"%s\", Address: 0x%p\n", GetCurrentProcessId(), mess, mess);
            printf("Mystery number: %d (size=%llu bytes, address=0x%p).\n", mystery, sizeof(mystery), &mystery);

            printf("Now waiting...\n");

            char buf[2];
            fgets(buf, 2, stdin);
        }
    }

    return 0;
}
