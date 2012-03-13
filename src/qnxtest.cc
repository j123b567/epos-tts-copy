#include <stdio.h>
#include <string.h>
#include <sys/kernel.h>
#include <sys/name.h>

#define BUFF_SIZE 8192

char buff[BUFF_SIZE + 1];

void main( int argc, char *argv[] )
{
	int pid = qnx_name_locate(0, "epos-spk-proxy", 0, NULL);
	if (pid == -1) {
		fprintf(stderr, "Cannot locate the proxy\n");
		abort();
	}
	strcpy(buff, argv[1]);
	int r = Send(pid, buff, buff, strlen(buff)+1, BUFF_SIZE);
	if (r == -1) {
		fprintf(stderr, "Cannot send text\n");
		abort();
	}
	fprintf(stderr, "Said\n");
}

