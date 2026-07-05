#include<stdio.h>
#include<unistd.h>

int main(void)
{
	int i = 0;

	while(1)
	{
		printf("i = %d\n", i++);
		sleep(2);
	}

	return 0;
}

