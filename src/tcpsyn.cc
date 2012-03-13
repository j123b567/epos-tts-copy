/*
 *	epos/src/tcpsyn.cc
 *	(c) 1998-99 Jirka Hanika, geo@ff.cuni.cz
 *
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License in doc/COPYING for more details.
 *	
 *
 */

#include "common.h"
#include "tcpsyn.h"
#include "client.h"

#ifdef HAVE_IO_H
	#include <io.h>		/* open, write, (ioctl,) ... */
#endif

#ifdef HAVE_ERRNO_H
	#include <errno.h>
#endif

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#include <fcntl.h>
// #include <netdb.h>

#define TCPPORTSEP ':'

/*
 *	tcpsyn_appl() sends an apply command and blocks until it is processed.
 *	It reads and returns the data sent by the server.  The size argument
 *	is actually an output argument: the number of bytes written into the
 *	buffer returned.  The buffer should be freed with free() by the caller.
 *
 *	This function should be called only if the strm command uses datad
 *	as both the input and the output module.
 */

void *tcpsyn_appl(int bytes, int ctrld, int datad, int *size)
{
	char *rec = NULL;
	int offset = 0;
	int bs = 0;
	int sum = 0;

	sputs("appl ", ctrld);
	sprintf(scratch, "%d", bytes);
	sputs(scratch, ctrld);
	sputs("\r\n", ctrld);
	do {
		sgets(scratch, cfg->scratch, ctrld);
		if (!strncmp("122 ", scratch, 4)) {
			sgets(scratch, cfg->scratch, ctrld);
			sscanf(scratch, " %d", &bytes);
			if (bs != bytes) rec = rec ? (char *)realloc(rec, bytes)
						   : (char *)malloc(bytes);
		}
		if (!strncmp("123 ", scratch, 4)) {
			sgets(scratch, cfg->scratch, ctrld);
			sscanf(scratch, " %d", &bytes);
			sum += bytes;
			if (sum > bs) rec = rec ? (char *)realloc(rec, sum)
						: (char *)malloc(sum);
			bytes = yread(datad, rec + offset, sum - offset);
			if (bytes == -1) {
				if (errno == EAGAIN || errno == EINTR) bytes = 0;
				else shriek(473, "Connection lost");
			}
			offset += bytes;
		}
	} while (!strchr("2468", scratch[0]));
	if (scratch[0] != '2' && scratch[1] != '0') shriek(475, fmt("Remote returned %.3s for appl", scratch));
	while (sum - offset) {
		bytes = yread(datad, rec + offset, sum - offset);
		if (bytes == -1) {
			if (errno == EAGAIN || errno == EINTR) bytes = 0;
			else shriek(473, "Connection lost");
		}
		offset += bytes;
	}
	*size = offset;
	return rec;

}

/*
 *	For tcpsyn, loc denotes the remote server name and port
 */

static int tcpsyn_connect_socket(unsigned int ipaddr, int port)
{
	int sd = just_connect_socket(ipaddr, port);
	if (sd == -1) {
		shriek(473, "Server unreachable\n");
	}
	sgets(scratch, cfg->scratch, sd);
	if (strncmp(scratch, "TTSCP spoken here", 18)) {
		scratch[15] = 0;
		shriek(474, "Protocol not recognized");
	}
	return sd;
}

tcpsyn::tcpsyn(voice *v)
{
//	throw new command_failed (471, "Bus");
	int port;
	char *remote_server = strdup(v->loc);
	char *port_id = strchr(remote_server, TCPPORTSEP);
	if (port_id) {
		*port_id++ = 0;
		sscanf(port_id, "%i", &port);
	} else port = TTSCP_PORT;

	unsigned int a = getaddrbyname(remote_server);
	free(remote_server);
	
//	shriek(472, "bus");
	cd = tcpsyn_connect_socket(a, port);	/* adjust both, FIXME */
	dd = tcpsyn_connect_socket(a, port);
	DEBUG(1,9,fprintf(STDDBG, "tcpsyn uses port %d ctrl fd %d data fd %d\n", port, cd, dd);)

	char *ctrl_handle = get_handle(cd);
	sputs("data ", dd);
	sputs(ctrl_handle, dd);
	sputs("\r\n", dd);
	free(ctrl_handle);
	handle = get_handle(dd);
	int err;
	err = sync_finish_command(dd);
	if (err) shriek(475, fmt("Remote returned %d for data", err));

	sputs("strm $", cd);
	sputs(handle, cd);
	sputs(":synth:$", cd);
	sputs(handle, cd);
	sputs("\r\n", cd);
	err = sync_finish_command(cd);
	if (err) shriek(475, fmt("Remote returned %d for strm", err));
}

tcpsyn::~tcpsyn()
{
	int err;

	sputs("delh ", cd);
	sputs(handle, cd);
	sputs("\r\ndone\r\n", cd);
	err = sync_finish_command(cd);
	if (err) shriek(475, fmt("Remote returned %d for delh", err));
	err = sync_finish_command(cd);
	if (err) shriek(475, fmt("Remote returned %d for done", err));
	async_close(cd);
	async_close(dd);
	free(handle);
}

void
tcpsyn::syndiph(voice *v, diphone d, wavefm *w)
{
	shriek(861, "abstract tcpsyn::syndiph");
}

void
tcpsyn::syndiphs(voice *v, diphone *d, int count, wavefm *w)
{
	int size;
	void *b;

	ywrite(dd, d - 1,sizeof(diphone) * ++count);
	b = tcpsyn_appl(sizeof(diphone) * count, cd, dd, &size);
	w->become(b, size);
	free(b);
}


