#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>


int main() {
    char* mess = malloc(sizeof(char) * 20);
    sprintf(mess, "This is a test!");
	printf("PID: %d, Message: \"%s\", Addresse: 0x%p\n", GetCurrentProcessId(), mess, mess);

    int _;
    printf("Now waiting..."); scanf("%d", &_);

    return 0;
}
