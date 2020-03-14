#include <stdio.h>
#include <stdlib.h>


int main(int argc, char const *argv[])
{
	char *p = (char*)malloc(128);

	for (int i = 0; i < 128; ++i)
		printf("%p", &p[i]);

	return 0;
}
