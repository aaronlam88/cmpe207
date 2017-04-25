#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[])
{
	FILE* fp;
	char path[BUFSIZ];

	/* Open the command for reading. */
	fp = popen("date +%s | sha256sum | base64 | head -c 32 ; echo", "r");
	if (fp == NULL) {
		printf("Failed to run command\n" );
		exit(1);
	}

  	/* Read the output a line at a time - output it. */
	fgets(path, sizeof(path)-1, fp);
	printf("%s", path);

  	/* close */
	pclose(fp);

	return 0;
}