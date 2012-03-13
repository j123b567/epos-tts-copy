/*
 *	(c) 1998-99 Jirka Hanika <geo@cuni.cz>
 *
 *	This single source file src/client.cc, but NOT THE REST OF THIS PACKAGE,
 *	is considered to be in Public Domain. Parts of this single source file may be
 *	freely incorporated into any commercial or other software.
 *
 *	Most files in this package are strictly covered by the General Public
 *	License version 2, to be found in doc/COPYING. Should GPL and the paragraph
 *	above come into any sort of legal conflict, GPL takes precendence.
 *
 *	This file implements support routines for a simple TTSCP client.
 *	See doc/english/ttscp.doc for a preliminary technical specification.
 *
 *	This file can be included with cfg pointing to two very different
 *	structures.  The usual interpretation, the one compiled into client.o,
 *	is a few hundred bytes long structure.  However, when the "say" client
 *	is compiled, this file is #included directly and now cfg points to
 *	a fake constant structure with only a few items needed to compile
 *	this file.  This scheme is probably too clever to keep, but anyway,
 *	at the moment it prevents using client.o for actual client stuff.
 */

#ifndef EPOS_COMMON_H
#include "common.h"
#endif


#ifdef THIS_IS_A_TTSCP_CLIENT

#define SCRATCH_SPACE 16384

struct pseudoconfiguration
{
	int asyncing;
	int scratch;
	int paranoid;
};

pseudoconfiguration pseudocfg = {1, SCRATCH_SPACE, 0};

pseudoconfiguration *cfg = &pseudocfg;

char scratch[SCRATCH_SPACE + 2];

#endif



#include "client.h"

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_UNIX_H
	#include <unix.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifdef HAVE_NETDB_H
	#include <netdb.h>
#endif

#ifdef HAVE_IO_H
	#include <io.h>
#endif

#ifdef HAVE_WINSOCK2_H
	#include <winsock2.h>
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#include <sys/types.h>
#include <signal.h>

/*
 *	blocking sgets() 
 *	returns 0 on error (EOF), 1 on success (line read)
 */

int sgets(char *buffer, int buffer_size, int sd)
{
	int i, result;
	buffer[0] = 0;

	for (i=0; i < buffer_size; i++) {
		result = yread(sd, buffer+i, 1);
		if (result == 0) return 0;
		if (result == -1) return 0;		// error
		if (buffer[i] == '\n' || !buffer[i]) {
			if (i && buffer[i-1] == '\r') buffer[i-1] = 0;
			buffer[i] = 0;
			return 1;
		}
	}
	buffer[i+1] = 0;
	return 1;	// error though - FIXME
}

int getaddrbyname(const char *inet_name)
{
	hostent *he = gethostbyname(inet_name);
	if (!he || he->h_addrtype != AF_INET || !he->h_addr_list[0])
		shriek(472, "Unknown remote tcpsyn server");
	return ((in_addr *)he->h_addr_list[0])->s_addr;
}

int just_connect_socket(unsigned int ipaddr, int port)
{
	sockaddr_in addr;
	int sd;


	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sd == -1) shriek(464, "No socket\n");
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (!ipaddr) {
//		gethostname(scratch, cfg->scratch);	// can be used instead of localhost
		strcpy(scratch, "localhost");
		ipaddr = getaddrbyname(scratch);
	}
	addr.sin_addr.s_addr = ipaddr;

	return connect(sd, (sockaddr *)&addr, sizeof(addr)) ? (close(sd) ,-1) : sd;
}

int connect_socket(unsigned int ipaddr, int port)
{
	int sd = just_connect_socket(ipaddr, port);
	if (sd == -1) {
		shriek(473, "Server unreachable\n");
	}
	if (!sgets(scratch, cfg->scratch, sd)) shriek(474, "Remote server listens but discards\n");
	if (strncmp(scratch, "TTSCP spoken here", 18)) {
		scratch[15] = 0;
		shriek(474, "Protocol not recognized");
	}
	return sd;
}

char *get_handle(int sd)
{
	do {
		sgets(scratch, cfg->scratch, sd);
	} while (*scratch && strncmp(scratch, "handle: ", 8));
	if (!*scratch) {
		printf("NULL handle\n");
		return NULL;
	}
	return strdup(scratch + 8);
}

void xmit_option(const char *name, const char *value, int sd)
{
	sputs("setl ", sd);
	sputs(name, sd);
	sputs(" ", sd);
	sputs(value, sd);
	sputs("\r\n", sd);
}

#define ERROR_CODE ((scratch[0]-'0')*100+(scratch[1]-'0')*10+(scratch[2]-'0'))

int sync_finish_command(int ctrld)
{
	while (sgets(scratch, cfg->scratch, ctrld)) {
		scratch[cfg->scratch] = 0;
//		printf("Received: %s\n", scratch);
		switch(*scratch) {
			case '1': continue;
			case '2': return 0;
			case '3': break;
			case '4': // printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return ERROR_CODE;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
					  return 0;
				  } /* else fall through */
			case '8': // printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return ERROR_CODE;

			case '5':
			case '7':
			case '9':
			case '0': // printf("%s\n", scratch);
				  shriek(474, "Unhandled response code");
			default : ;
		}
		printf("%s\n", scratch+strspn(scratch, "0123456789 "));
	}
	return 649;
}

#undef ERROR_CODE
