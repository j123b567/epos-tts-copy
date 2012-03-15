/*
 *	(c) 1998-01 Jirka Hanika <geo@cuni.cz>
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

struct pseudo_static_configuration
{
	int asyncing;
	int scratch;
	int paranoid;
	int listen_port;
};

pseudo_static_configuration pseudocfg = {1, SCRATCH_SPACE, 0, TTSCP_PORT};

pseudo_static_configuration *scfg = &pseudocfg;

char scratch[SCRATCH_SPACE + 2];

#endif

#define PUBLIC_TTSCP_SERVER   "epos.ure.cas.cz"

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
	#define HAVE_WINSOCK
#else
	#ifdef HAVE_WINSOCK_H
		#include <winsock.h>
		#define HAVE_WINSOCK
	#endif
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_SYS_TYPES_H
	#include <sys/types.h>
#endif

#ifdef HAVE_SIGNAL_H
	#include <signal.h>
#endif

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
		if (result == -1) {
			i--;
			continue;		// error
		}
		if (buffer[i] == '\n' || !buffer[i]) {
			if (i && buffer[i-1] == '\r') buffer[i-1] = 0;
			buffer[i] = 0;
			return 1;
		}
	}
	buffer[i+1] = 0;
	return 1;	// error though - FIXME
}

int (*sputs_replacement)(int sd, const char *, int) = NULL;

int sputs(const char *buffer, int sd)
{

	int total;
	int len = total = strlen(buffer);
	int result;

	if (!buffer) return 0;
	if (sputs_replacement)
		return sputs_replacement(sd, buffer, len);
	else do {
		result = ywrite(sd, buffer, len);
		if (result == -1 && errno == EPIPE) return -1;
//		if (result == -1 && errno == EAGAIN && ctrl_enque)
//			ctrl_enque(sd, buffer, len);
		if (result == -1) result = 0;
		buffer += result;
		len -= result;
	} while (len);
	return total;
}



int getaddrbyname(const char *inet_name)
{
#ifdef WANT_DMALLOC
	return htonl(INADDR_LOOPBACK);
#endif
	hostent *he = gethostbyname(inet_name);
	if (!he || he->h_addrtype != AF_INET || !he->h_addr_list[0])
		return -1;
	return ((in_addr *)he->h_addr_list[0])->s_addr;
}

int just_connect_socket(unsigned int ipaddr, int port)
{
	sockaddr_in addr;
	int sd;

	if (!port) {
		sd = just_connect_socket(ipaddr, TTSCP_PORT);
		if (sd == -1) sd = just_connect_socket(ipaddr, TTSCP_PORT + 1);
		if (sd != -1) return sd;
		int public_addr = getaddrbyname(PUBLIC_TTSCP_SERVER);
		if (public_addr == -1) return -1;
		if (sd == -1) sd = just_connect_socket(public_addr, TTSCP_PORT + 1);
		if (sd == -1) sd = just_connect_socket(public_addr, TTSCP_PORT);
		return sd;
	}

	sd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sd == -1) shriek(464, "No socket\n");
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (!ipaddr) {
//		gethostname(scratch, scfg->scratch);	// can be used instead of localhost
		strcpy(scratch, "localhost");
		ipaddr = getaddrbyname(scratch);
		if (ipaddr == -1) return -1;
	}
	addr.sin_addr.s_addr = ipaddr;

	return connect(sd, (sockaddr *)&addr, sizeof(addr)) ? (close(sd) ,-1) : sd;
}

int connect_socket(unsigned int ipaddr, int port)
{
	int sd = just_connect_socket(ipaddr, port);
	if (sd == -1) {
		shriek(473, "Server unreachable (Epos not running?)\n");
	}
	if (!sgets(scratch, scfg->scratch, sd)) shriek(474, "Remote server listens but discards\n");
	if (strncmp(scratch, "TTSCP spoken here", 18)) {
		scratch[15] = 0;
		shriek(474, "Protocol not recognized");
	}
	return sd;
}

bool running_at_localhost()
{
	int j = just_connect_socket(0, scfg->listen_port);
	if (j == -1) return false;
	close(j);
	return true;
}

char *get_handle(int sd)
{
	do {
		sgets(scratch, scfg->scratch, sd);
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
	while (sgets(scratch, scfg->scratch, ctrld)) {
		scratch[scfg->scratch] = 0;
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
