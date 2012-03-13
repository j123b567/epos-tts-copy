/*
 *	epos/src/???.cc
 *	(c) 1998 Jirka Hanika <geo@ff.cuni.cz>
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
 *	purposes is discouraged. This client may deliberately violate
 *	the TTSCP specification in order to test the error recovery
 *	procedures of Epos or another TTSCP server.
 *
 *	This file is almost a plain C source file.  We compile it with a C++
 *	compiler just to avoid additional configure complexity.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "config.h"	/* You can usually remove this item */

#define TTSCP_PORT	8778

const char *COMMENT_LINES = "#;\n";
const char *WHITESPACE = " \t";

const char *output_file = "/dev/dsp";

int ctrld[10];
int datad[10];		/* file descriptors for the control and data connections */
char *data = NULL;

#define SCRATCH_SPACE 16384
char scratch[SCRATCH_SPACE + 2];

char *dhandle[10];

char *testname = "";


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
//		printf("Received: %s\n", scratch);
		mess = scratch+strspn(scratch, "0123456789x ");
		switch(*scratch) {
			case '1': continue;
			case '2': return 2;
			case '3': break;
			case '4': if (*mess) printf("%s\n", mess);
				  return 4;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
					exit(0);
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

#ifdef ___

// #ifdef HAVE_GETENV
	FILE *f = fopen(getenv("TTSCP_USER"), "rt");
	if (f) {
		while (!feof(f)) {
			*scratch = 0;
			fgets(scratch, SCRATCH_SPACE, f);
			if (*scratch && !strchr(COMMENT_LINES, scratch[strspn(scratch, WHITESPACE)])) {
				sputs(scratch, ctrld);
				get_result();
			}
		}
		fclose(f);
	}
/// #endif

void say_data()
{
	if (!data) data = "No.";
	sputs("strm $", ctrld);
	sputs(handle, ctrld);
	sputs(":raw:rules:diphs:synth:", ctrld);
	sputs(output_file, ctrld);
	sputs("\n", ctrld);
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", strlen(data));
	sputs(scratch, ctrld);
	sputs("\n", ctrld);
	sputs(data, datad);
	if (get_result() > 2) shriek("Could not set up a stream");
	if (get_result() > 2) shriek("Could not apply a stream to text");
}

void trans_data()
{
	if (!data) data = "No.";
	sputs("strm $", ctrld);
	sputs(handle, ctrld);
	sputs(":raw:rules:print:$", ctrld);
	sputs(handle, ctrld);
	sputs("\n", ctrld);
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", strlen(data));
	sputs(scratch, ctrld);
	sputs("\n", ctrld);
	sputs(data, datad);
	get_result();
	get_result();
}


void send_option(char *name, char *value)
{
	xmit_option(name, value, ctrld);
	get_result();
}

#define CMD_LINE_OPT "-"
#define CMD_LINE_VAL '='


void dump_help()
{
	printf("usage: say [options] ['text to be processed']\n");
	printf(" -b  bare format (no frills)\n");
	printf(" -c  casual pronunciation\n");
	printf(" -i  ironic pronunciation\n");
	printf(" -k  shutdown Epos\n");
	printf(" -l  list available languages and voices\n");
	printf(" -w  write output to said.wav (in the \"pseudo root\" directory)\n");
}

#endif

void spk_strm(int c, int d)
{
	sputs("strm $", ctrld[c]);
	sputs(dhandle[d], ctrld[c]);
	sputs(":raw:rules:diphs:synth:/dev/dsp", ctrld[c]);
	sputs("\n", ctrld[c]);
}

void spk_appl(int c, int d, char *data)
{
	sputs("appl ", ctrld[c]);
	sprintf(scratch, "%d", strlen(data));
	sputs(scratch, ctrld[c]);
	sputs("\n", ctrld[c]);
	sputs(data, datad[d]);
}

void spk_brk(int c)
{
	sputs("brk\n", ctrld[c]);
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

void brk_test()
{
	testname="brk test";
	spk_strm(0,0);
	spk_appl(0,0, "Brejk je asi rozbitej");
	if (get_result(0) > 2) shriek("Could not set up a stream");
	sleep(1);
	spk_brk(1);
	if (get_result(1) > 2 || get_result(0) == 2)
		shriek("Failed to cancel the current jobs");
}


int main(int argc, char **argv)
{
	if (argc != 1) shriek("No arguments allowed");
	for (int i=0; i<5; i++) {
		datad[i] = connect_socket(0, TTSCP_PORT);
		dhandle[i] = data_conn(datad[i]);
		ctrld[i] = connect_socket(0, TTSCP_PORT);
	}
//	sputs("brk\n", ctrld);

	syn2_test();
	brk_test();

	for (int i=0; i<5; i++) {
		sputs("delh ", ctrld[i]);
		sputs(dhandle[i], ctrld[i]);
		sputs("\ndone\n", ctrld[i]);
		if (get_result(0) > 2) shriek("Could not delete a data connection handle");
		if (get_result(0) > 2) shriek("Could not shut down a control connection");
		close(datad[i]);
		close(ctrld[i]);
	}
	return 0;
}


#ifndef HAVE_TERMINATE	/* the only item which needs to have included "config.h" */

void terminate(void)
{
	abort();
}

#endif

