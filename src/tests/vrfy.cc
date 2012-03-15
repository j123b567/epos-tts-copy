/*
 *	epos/src/vrfy.cc
 *	(c) 1998-99 Jirka Hanika <geo@cuni.cz>
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.

 *	This file implements a single purpose TTSCP client. See
 *	doc/english/ttscp.sgml for a preliminary technical specification.
 *	The Epos developers may use this client to test a particular
 *	TTSCP implementation on a UNIX platform; using it for other
 *	purposes is discouraged: this client may deliberately violate
 *	the TTSCP specification in order to test the error recovery
 *	procedures of Epos or another TTSCP server.
 *
 *	This file is almost a plain C source file.  We compile it with a C++
 *	compiler just to avoid additional configure complexity.
 */

#define THIS_IS_A_TTSCP_CLIENT

#include "config.h"	/* You can usually remove this item */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define TTSCP_PORT	8789

const char *COMMENT_LINES = "#;\n\r";
const char *WHITESPACE = " \t\r";

// const char *output_file = "/dev/dsp";

int ctrld[10];
int datad[10];		/* file descriptors for the control and data connections */
char *data = NULL;

//#define SCRATCH_SPACE 16384
//char scratch[SCRATCH_SPACE + 2];

char *chandle[10];
char *dhandle[10];

const char *testname = "";


#define LITTLE_SPACE	   640
#define MUCH_SPACE	 16000
#define EVEN_MORE_SPACE 128000


int connect_socket(unsigned int, int);
#include <unistd.h>

void shriek(char *txt)
{
	if (!*testname) {
		fprintf(stderr, "Client side error: %s\n", txt);
		exit(1);
	}
	fprintf(stderr, "Failed test %s\n", testname);
	fprintf(stderr, "Reason: %s\n", txt);
	perror("Last error");

	sleep(1);
	testname = "";
	connect_socket(0, TTSCP_PORT);
	exit(1);
}

void shriek(int, char *txt)
{
	shriek(txt);
}


#define EPOS_COMMON_H	// this is a lie
#include "client.cc"
//#define sputs(x,y) {printf("Sent to %d: %s\n", y, x); sputs(x,y);}

int get_result(int c)
{
	char *mess;

	while (sgets(scratch, scfg->scratch, ctrld[c])) {
		scratch[scfg->scratch] = 0;
//		printf("[%-28s] Received on %d: %s\n", testname, c, scratch);
		mess = scratch+strspn(scratch, "0123456789x ");
		switch(*scratch) {
			case '1': continue;
			case '2': return 2;
			case '3': break;
			case '4': if (*mess && strcmp(mess, "interrupted")) printf("%s\n", mess);
				  return 4;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
//					exit(0);
					return 2;
				  } /* else fall through */
			case '8': if (*mess) printf("%s\n", mess);
				  exit(2);

			case '5':
			case '7':
			case '9':
			case '0': printf("%s\n", scratch); shriek("Unhandled response code");
			default : ;
		}
		if (*mess) printf("%s\n", mess);
	}
	return 8;	/* guessing */
}

void spk_strm(int c, int d)
{
	sputs("strm $", ctrld[c]);
	sputs(dhandle[d], ctrld[c]);
	sputs(":raw:rules:diphs:synth:#localsound", ctrld[c]);
	sputs("\r\n", ctrld[c]);
}

void spk_appl(int c, int d, const char *data, int data_len)
{
	sputs("appl ", ctrld[c]);
	sprintf(scratch, "%d", data_len);
	sputs(scratch, ctrld[c]);
	sputs("\r\n", ctrld[c]);
	ywrite(datad[d], data, data_len);
}

void spk_appl(int c, int d, const char *data)
{
	spk_appl(c, d, data, (int)strlen(data));
}

void spk_intr(int c, int broken)
{
	sputs("intr ", ctrld[c]);
	sputs(chandle[broken], ctrld[c]);
	sputs("\r\n", ctrld[c]);
}

void syn2_test()
{
	testname="repeated appl test";
	spk_strm(0,0);
	spk_appl(0,0, "Jeden text");
	spk_appl(0,0, "Druhej text");
	if (get_result(0) > 2) shriek("Could not set up a stream");
	if (get_result(0) > 2) shriek("Could not apply a stream to the first text");
	if (get_result(0) > 2) shriek("Could not apply a stream to the second text");
}

void intr_test()
{
	spk_strm(0,0);
	spk_appl(0,0, "Jenom jednu sekundu mohu breptat. Touto dobou jest nastati tichu. Jen omyl by mohl stanovit jinak. Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Brejk je asi rozbitej. \
Nechce brejkovat tento text.");
	if (get_result(0) > 2) shriek("Could not set up a stream");
retry:	sleep(1);
	spk_intr(1, 0);
	int tmp1, tmp0;
	if ((tmp1 = get_result(1)) > 2) {
		printf("break failed, retry\n");
		goto retry;
	}
	if ((tmp0 = get_result(0)) == 2) {
		printf("got codes: speaks %d interrupts %d\n", tmp1, tmp0);
		shriek("break did not interrupt");
	}
}

void long_strm_test()
{
	char *buffer = (char *)malloc(MUCH_SPACE + 1024);
	strcpy(buffer, "strm $");
	strcat(buffer, dhandle[1]);
	int b = strlen(buffer);
	for (int i=0; i < MUCH_SPACE; i++) {
		buffer[b+i] = ":raw:print"[i % 10];
	}
	strcat (buffer + MUCH_SPACE, ":$");
	strcat (buffer + MUCH_SPACE, dhandle[1]);
	strcat (buffer + MUCH_SPACE, "\r\n");
	sputs(buffer, ctrld[1]);

	spk_appl(1,1,"Identita");

	if (get_result(1) > 2) shriek("Could not set up the long stream");
	if (get_result(1) > 2) shriek("Could not apply long stream to trivial data");
}

void long_data_test()
{
	char *buffer = (char *)malloc(EVEN_MORE_SPACE + 1024);
	strcpy(buffer, "strm $");
	strcat(buffer, dhandle[1]);
	strcat(buffer, ":raw:rules:print:$");
	strcat (buffer, dhandle[1]);
	strcat (buffer, "\r\n");
	sputs(buffer, ctrld[1]);

	memset(buffer, 'A', EVEN_MORE_SPACE);
	buffer[EVEN_MORE_SPACE] = 0;
	spk_appl(1,1,buffer);

	if (get_result(1) > 2) shriek("Could not set up the stream");
	if (get_result(1) > 2) shriek("Could not apply trivial stream to long data");
}

char *much_data()
{
	char *buffer = (char *)malloc(MUCH_SPACE + 1024);
	read(datad[1], buffer, MUCH_SPACE);
	strcpy(buffer, "strm $");
	strcat(buffer, dhandle[1]);
	strcat(buffer, ":raw:rules:print:$");
	strcat (buffer, dhandle[1]);
	strcat (buffer, "\r\n");
	sputs(buffer, ctrld[1]);

	return buffer;
}

void random_data_test()
{
	char *buffer = much_data();

	for (int i = 0; i < LITTLE_SPACE; i++)
		buffer[i] = rand() % 255 + 1;
	buffer[MUCH_SPACE] = 0;
	spk_appl(1,1,buffer, LITTLE_SPACE);

	if (get_result(1) > 2) shriek("Could not set up the stream");
	if (get_result(1) > 2) shriek("Could not apply stream to random data");
}

void hard_zero_data_test()
{
	char *buffer = much_data();

	for (int i = 0; i < MUCH_SPACE; i++)
		buffer[i] = 0;
	buffer[MUCH_SPACE] = 0;
	spk_appl(1,1,buffer, MUCH_SPACE);

	if (get_result(1) > 2) shriek("Could not set up the stream");
	if (get_result(1) > 2) shriek("Could not apply stream to hard zero data");
}

void legal_data_test()
{
	char *buffer = much_data();

	for (int i = 0; i < MUCH_SPACE; i++)
		buffer[i] = 'a';
	buffer[MUCH_SPACE] = 0;
	spk_appl(1,1,buffer, MUCH_SPACE);

	if (get_result(1) > 2) shriek("Could not set up the stream");
	if (get_result(1) > 2) shriek("Could not apply stream to hard zero data");
}

void soft_zero_data_test()
{
	char *buffer = much_data();

	for (int i = 0; i < LITTLE_SPACE; i++)
		buffer[i] = '0';
	buffer[LITTLE_SPACE] = 0;
	spk_appl(1,1,buffer, LITTLE_SPACE);

	if (get_result(1) > 2) shriek("Could not set up the stream");
	if (get_result(1) > 2) shriek("Could not apply stream to soft zero data");
}

void invoke(void (*test)(), const char *name)
{
	testname = name;
	printf("%s...\n", name);
	test();
	testname = "";
}

#define strfy(x) #x
#define stringify(x) strfy(x)

char * const exec_argv[] = {
	"epos",
	"--forking=off",
	"--listen_port=" stringify(TTSCP_PORT),
	"--debug_password=make_check",
	"--base_dir=" stringify(SOURCEDIR) "/../../cfg",
	"--language=czech",
	"--voice=kubec-vq",
	NULL
};
char * const exec_envp[] = {
	NULL
};

int epos_pid = 0;
int patience = 40;

void init()
{
	/* ignore the test result if Epos is not running:    */
	if (just_connect_socket(0, TTSCP_PORT) == -1) {
		if ((epos_pid = fork())) do usleep(250000); while (just_connect_socket(0, TTSCP_PORT) == -1 && --patience);
		else execve("../epos", exec_argv, exec_envp);
	}

	ctrld[6] = connect_socket(0, TTSCP_PORT);
	chandle[6] = get_handle(ctrld[6]);
	for (int i=0; i<5; i++) {
		ctrld[i] = connect_socket(0, TTSCP_PORT);
		dhandle[i] = get_handle(ctrld[i]);
		sputs("data ", ctrld[i]);
		sputs(chandle[6], ctrld[i]);
		sputs("\r\n", ctrld[i]);
		if (get_result(i) > 2) shriek("Couldn't data");
		datad[i] = ctrld[i];
		ctrld[i] = connect_socket(0, TTSCP_PORT);
		chandle[i] = get_handle(ctrld[i]);
	}
}



int main(int argc, char **argv)
{
	if (argc != 1) shriek("No arguments allowed");
#if defined(HAVE_WINSOCK_H) || defined(HAVE_WINSOCK2_H)
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
#endif
	init();
//	sputs("intr\r\n", ctrld);

	syn2_test();
	invoke(intr_test, "intr test");
	invoke(long_strm_test, "command length overflow test");
//	invoke(long_data_test, "data length overflow test");
	invoke(hard_zero_data_test, "hard zero data test");
	invoke(soft_zero_data_test, "soft zero data test");
//	invoke(legal_data_test, "legal data test");
	invoke(random_data_test, "random data test");
	testname = "closing everything";

	for (int i = 0; i < 5; i++) {
		sputs("delh ", ctrld[i]);
		sputs(dhandle[i], ctrld[i]);
		sputs("\r\ndone\r\n", ctrld[i]);
		if (get_result(i) > 2) shriek("Could not delete a data connection handle");
		if (get_result(i) > 2) shriek("Could not shut down a control connection");
		close(datad[i]);
		close(ctrld[i]);
	}
	sputs("pass make_check\r\ndown\r\n", ctrld[6]);
	close(ctrld[6]);
//	printf("Tests successfully passed.\n");
//	if (epos_pid) kill(epos_pid, SIGTERM);
	sleep(1);
	return 0;
}


#ifndef HAVE_TERMINATE	/* the only item which needs to have included "config.h" */

void terminate(void)
{
	abort();
}

#endif

