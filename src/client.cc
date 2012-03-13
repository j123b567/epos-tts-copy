/*
 *	(c) 1998 Jirka Hanika <geo@ff.cuni.cz>
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
 */

#ifndef EPOS_COMMON_H
#include "common.h"
#endif

#ifndef SCRATCH_SPACE
#define SCRATCH_SPACE   cfg->scratch
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
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


/*
 *	blocking sgets() 
 *	returns 0 on error (EOF), 1 on success (line read)
 */

int sgets(char *buffer, int buffer_size, int sd)
{
	int i, result;
	buffer[0] = 0;

	for (i=0; i < buffer_size; i++) {
		result = read(sd, buffer+i, 1);
		if (result == 0) return 0;
		if (result == -1) return 0;		// error
		if (buffer[i] == '\n' || !buffer[i]) {
			if (i && buffer[i-1] == '\r') buffer[i-1] = 0;
			buffer[i] = 0;
			return 1;
		}
	}
	buffer[i+1] = 0;
	return 1;	// error though
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
	if (sd == -1) shriek(672, "No socket\n");
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (!ipaddr) {
		gethostname(scratch, SCRATCH_SPACE);
		ipaddr = getaddrbyname(scratch);
	}
	addr.sin_addr.s_addr = ipaddr;

	return connect(sd, (sockaddr *)&addr, sizeof(addr)) ? (close(sd) ,-1) : sd;
}

int connect_socket(unsigned int ipaddr, int port)
{
	int sd = just_connect_socket(ipaddr, port);
	if (sd == -1) {
//		printf("Could not connect: %s\n", strerror(errno));
		shriek(673, "Cannot find epos running!\n");
	}
	sgets(scratch, SCRATCH_SPACE, sd);
	if (strncmp(scratch, "TTSCP rev ", 10)) {
		scratch[12] = 0;
//		printf(scratch);
		shriek(674, "Protocol not recognized");
	}
	return sd;
}

int sputs(const char *buffer, int sd)
{
//	printf("Sending: %s", buffer);
	if (!buffer) return 0;
	return write(sd, buffer, strlen(buffer));
}


void xmit_option(char *name, char *value, int sd)
{
	sputs("set ", sd);
	sputs(name, sd);
	sputs(" ", sd);
	sputs(value, sd);
	sputs("\n", sd);
}

char *data_conn(int sd)
{
	sputs("data\n", sd);
	do {
		sgets(scratch, SCRATCH_SPACE, sd);
	} while (strncmp(scratch, "142 ", 4));

	sgets(scratch, SCRATCH_SPACE, sd);
	char *h = strdup(scratch + 1);
	sgets(scratch, SCRATCH_SPACE, sd);
//	printf("handle is %s\n", handle);
	return h;
}

int send_appl(int bytes, int ctrld)
{
	sputs("appl ", ctrld);
	sprintf(scratch, "%d", bytes);
	sputs(scratch, ctrld);
	sputs("\n", ctrld);
	do {
		sgets(scratch, SCRATCH_SPACE, ctrld);
	} while (strncmp(scratch, "122 ", 4));
	sgets(scratch, SCRATCH_SPACE, ctrld);
	sscanf(scratch, " %d", &bytes);
	return bytes;

}

void sync_finish_command(int ctrld)
{
	while (sgets(scratch, SCRATCH_SPACE, ctrld)) {
		scratch[SCRATCH_SPACE] = 0;
//		printf("Received: %s\n", scratch);
		switch(*scratch) {
			case '1': continue;
			case '2': return;
			case '3': break;
			case '4': // printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return;
			case '6': if (!strncmp(scratch, "600 ", 4)) {
					  return;	// exit(0); FIXME
				  } /* else fall through */
			case '8': // printf("%s\n", scratch+strspn(scratch, "0123456789x "));
				  return; // exit(2); FIXME

			case '5':
			case '7':
			case '9':
			case '0': printf("%s\n", scratch); // shriek("Unhandled response code");
								// FIXME!!
			default : ;
		}
		printf("%s\n", scratch+strspn(scratch, "0123456789 "));
	}
}

void make_rnd_passwd(char *buffer, int size)
{
	int i;
	for (i = 0; i < size; i++) buffer[i] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"
			[rand() & 63];
	buffer[i] = 0;
}

