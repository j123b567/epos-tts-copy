
#ifndef SAPI_CLIENT_H
#define SAPI_CLIENT_H

// define TTSCP
#ifndef THIS_IS_A_TTSCP_CLIENT
	#include <io.h>
	#include <errno.h>
	#include <stdio.h>

	#define INITIAL_SCRATCH_SPACE 16384
	void D_PRINT(int, ...) {};
	#define xmalloc malloc
	#define xrealloc realloc
	#define THIS_IS_A_TTSCP_CLIENT
	#define TTSCP_PORT  8778

	struct pseudo_static_configuration
	{
		int asyncing;
		int scratch_size;
		int paranoid;
		int listen_port;
	};
	
	pseudo_static_configuration pseudocfg = {1, INITIAL_SCRATCH_SPACE, 0, TTSCP_PORT};

	pseudo_static_configuration *scfg = &pseudocfg;

/*
	#include "common.h"

	#ifdef HAVE_WINSVC_H
		#ifndef SERVICE_NAME
			#include "service.h"
		#endif
		bool start_service();
		bool stop_service();
	#else
		bool start_service() { return true; }
		bool stop_service() { return true; }
	#endif
*/	
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
	//#include "client.cc"
#endif
// define TTSCP

#endif