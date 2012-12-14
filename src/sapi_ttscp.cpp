/*
 *	(c) 1998-01 Jirka Hanika <geo@cuni.cz>
 *	(c) 2006	Zdenìk Chaloupka <chaloupka@ure.cas.cz>
 *
 *	This single source file src/startsrv.cc, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precedence.
 *
 *	This file supplements "say", a simple TTSCP client, with
 *	the ability to start the TTSCP service on Windows NT.
 *	Epos must already be installed as the TTSCP service.
 *	You are expected to happily ignore this file.
 */

#define THIS_IS_A_TTSCP_CLIENT

#include "common.h"

#ifdef HAVE_WINSVC_H
	bool start_service();
	bool stop_service();
#else
	bool start_service() { return true; }
	bool stop_service() { return true; }
#endif

#ifdef SERVICE_BINARY
	void report(char *caption, char *message);
#else
	void report(char *caption, char *message){ return; }
#endif

#ifndef HAVE_TERMINATE

void terminate(void)
{
	abort();
}

#endif

void shriek(char *txt)
{
	fprintf(stderr, "Client side error: %s\n", txt);
	exit(1);
}

void shriek(int, char *txt)
{
	shriek(txt);
}

#include "client.cc"

void main( int argc, char *argv[ ], char *envp[ ] ) {
/*#ifdef HAVE_WINSOCK
	if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch)) shriek(464, "No winsock");
	charset = "cp1250";
#endif*/

	int ctrld, datad;
	if (!start_service()) {
		report("Error", "EPOS system has not been started (maybe not installed?)\n");
		return;
	}
	ctrld = connect_socket(0, 0);
	datad = connect_socket(0, 0);
}
