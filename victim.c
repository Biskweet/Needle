#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>


int main() {
    srand(time(NULL));

    char* mess = malloc(sizeof(char) * 20);
    sprintf(mess, "This is a test!");

    int mystery;
    char running = 1;

    while (running) {
        mystery = rand();
    	  printf("PID: %lu, Message: \"%s\", Addresse: 0x%p\n", GetCurrentProcessId(), mess, mess);
        printf("Mystery number: %d (size=%u bytes, address=0x%p).\n", mystery, sizeof(mystery), &mystery);

        printf("Now waiting...\n");

        char buf[2];
        fgets(buf, 2, stdin);
        if (buf[0] == 'x') running = 0;
    }

    return 0;
}
