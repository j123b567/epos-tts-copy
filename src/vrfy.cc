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

#define TTSCP_PORT	8778

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

	while (sgets(scratch, SCRATCH_SPACE, ctrld[c])) {
		scratch[SCRATCH_SPACE] = 0;
		printf("Received on %d: %s\n", c, scratch);
		mess = scratch+strspn(scratch, "0123456789x ");
		switch(*scratch) {
			case '1': continue;
			case '2': return 2;
			case '3': break;
			case '4': if (*mess && strcmp(mess, "user break")) printf("%s\n", mess);
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

void spk_appl(int c, int d, const char *data)
{
	sputs("appl ", ctrld[c]);
	sprintf(scratch, "%d", (int)strlen(data));
	sputs(scratch, ctrld[c]);
	sputs("\r\n", ctrld[c]);
	sputs(data, datad[d]);
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
	testname="intr test";
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
	sleep(1);
	spk_intr(1, 0);
	if (get_result(1) > 2)
		shriek("break failed");
	if (get_result(0) == 2)
		shriek("break did not interrupt");
}

void long_strm_test()
{
	testname = "command length overflow test";
	char *buffer = (char *)malloc(16384);
	strcpy(buffer, "strm $");
	strcat(buffer, dhandle[1]);
	int b = strlen(buffer);
	for (int i=0; i < 16000; i++) {
		buffer[b+i] = ":raw:print"[i % 10];
	}
	strcat (buffer + 16000, ":$");
	strcat (buffer + 16000, dhandle[1]);
	strcat (buffer + 16000, "\r\n");
	sputs(buffer, ctrld[1]);

	spk_appl(1,1,"Identita");

	if (get_result(1) > 2) shriek("Could not set up the long stream");
	if (get_result(1) > 2) shriek("Could not apply long stream to trivial data");
}

void init()
{
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
//	sleep(2);
	intr_test();
//	sleep(2);
	long_strm_test();

	for (int i=0; i<5; i++) {
		sputs("delh ", ctrld[i]);
		sputs(dhandle[i], ctrld[i]);
		sputs("\r\ndone\r\n", ctrld[i]);
		if (get_result(i) > 2) shriek("Could not delete a data connection handle");
		if (get_result(i) > 2) shriek("Could not shut down a control connection");
		close(datad[i]);
		close(ctrld[i]);
	}
	sputs("done\r\n", ctrld[6]);
	close(ctrld[6]);
	printf("Tests successfully passed.\n");
	return 0;
}


#ifndef HAVE_TERMINATE	/* the only item which needs to have included "config.h" */

void terminate(void)
{
	abort();
}

#endif

