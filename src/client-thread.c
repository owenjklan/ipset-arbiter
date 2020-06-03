
#include <stdio.h>

void client_main(void *args)
{
	printf("I am thread %d\n", (int)args);
	return (int)args;
}