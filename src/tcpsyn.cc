/*
 *	epos/src/tcpsyn.cc
 *	(c) 1998 Jirka Hanika, geo@ff.cuni.cz
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

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#include <fcntl.h>
// #include <netdb.h>

#define TCPPORTSEP ':'

tcpsyn::tcpsyn(voice *v)
{
	int port;
	char *port_id = strchr(this_voice->remote_server, TCPPORTSEP);
	if (port_id) {
		*port_id++ = 0;
		sscanf(port_id, "%i", &port);
	} else port = EPOS_TCP_PORT;

	unsigned int a = getaddrbyname(this_voice->remote_server);
	
	cd = connect_socket(a, port);	/* adjust both, FIXME */
	dd = connect_socket(a, port);
	DEBUG(1,9,fprintf(STDDBG, "tcpsyn uses port %d ctrl fd %d data fd %d\n", port, cd, dd);)
	handle = data_conn(dd);
	sputs("strm $", cd);
	sputs(handle, cd);
	sputs(":synth:$", cd);
	sputs(handle, cd);
	sputs("\n", cd);
	sync_finish_command(cd);
}

tcpsyn::~tcpsyn()
{
	sputs("delh ", cd);
	sputs(handle, cd);
	sputs("\ndone\n", cd);
	sync_finish_command(cd);
	sync_finish_command(cd);
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
	int rec;
	void *b;

	int toread;
	void *bp;

	write(dd, d - 1,sizeof(diphone) * ++count);
	rec = send_appl(sizeof(diphone) * count, cd);
	b = malloc(rec);
	for (toread = rec, bp = b; toread; ) {
		int ret = read(dd, bp, toread);
		toread -= ret;
		*(char **)&bp += ret;
	}
	w->become(b, rec);
	free(b);
	sync_finish_command(cd);
}


