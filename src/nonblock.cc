/*
 *	epos/src/nonblock.cc
 *	(c) 2002 geo@cuni.cz
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
 */

#include "common.h"
#include "client.h"
#include "agent.h"

/*
 *	nonblocking sgets - returns immediately.
 *	tries to get a line into buffer; if it can't,
 *	returns zero and partbuff will contain some (undefined)
 *	data, which should be passed to the next call to
 *	sgets() with this, but not another socket.
 *	The "space" argument limits both buffers.
 *
 *	Upon the first call with this socket, *partbuff must == 0.
 *
 *	returns:      0   partial line in partbuff or nothing to do
 *		positive  full line in buffer
 *		negative  error reading socket
 *
 *	Our policy is not to read the socket when we've got
 *	a partial line acquired in an earlier invocation.
 *	This is to avoid starvation by an over-active session.
 *	Such a session would however cause a lot of shifting
 *	strings back and forth between the buffers.
 */

int sgets(char *buffer, int space, int sd, char *partbuff)
{
	int i, l;
	int result = 0;

	if (*partbuff) {
		DEBUG(1,11,fprintf(STDDBG, "[core] Appending.\n");)
		l = strlen(partbuff);
		if (l > space) shriek(862, "sgets() holdback overflow"); // was: shriek(664)
		if (l == space) goto too_long;
		strcpy(buffer, partbuff);
		if (strchr(buffer, '\n')) goto already_enough_text;
	} else l = 0;
	result = yread(sd, buffer + l, space - l);
	if (result >= 0) buffer[l+result] = 0; else buffer[l] = 0;
	if (result <= 0) {
		if (result == -1 && errno == EAGAIN) {
			DEBUG(2,11,fprintf(STDDBG, "Nothing to do: %d\n", sd);)
			*buffer = 0;
			return 0;
		}
		*partbuff = 0;	/* forgetting partial line upon EOF/error. Bad? */
		*buffer = 0;
		DEBUG(2,11,fprintf(STDDBG, "Error on socket: %d\n", sd);)
		return -1;
	}
	l += result;

already_enough_text:
	for (i=0; i<l; i++) {
		if (buffer[i] == '\n' || !buffer[i]) {
			if (i && buffer[i-1] == '\r') buffer[i-1] = 0;
			buffer[i] = 0;
			if (++i < l) strcpy(partbuff, buffer+i);
			else *partbuff = 0;
			return 1;
		}
	}
	if (i >= space) goto too_long;
	buffer[i] = 0;
	strcpy(partbuff, buffer);
	DEBUG(1,11,fprintf(STDDBG, "[core] partial line read: %s\n", partbuff);)
	*buffer = 0;
	return 0;

too_long:
	strcpy(partbuff, "remark ");
	DEBUG(2,11,fprintf(STDDBG, "[core] Too long command ignored");)
	sputs("413 Too long\n", sd);
	*buffer = 0;
	return 0;
}

/*
 *	class a_replier handles control connection congestion.
 *	It is constructed at the moment of the first partial
 *	or EWOULDBLOCK/EAGAIN write in async_sputs().
 *
 *	As soon as the a_replier is attached to a control connection,
 *	it is never released and imposes its overhead over it forever.
 *
 *	There is no real flow control with repliers, so that their
 *	activity can become a memory hog.
 */

class a_replier : public agent
{
	int sd;
	char *buffer;
	int len;
	virtual const char *name() { return "replier"; };
	virtual void run();
   public:
	a_replier(socky int s);
	virtual ~a_replier();
	void write(const char *buffer, int len);
};

a_replier **replier_table = (a_replier **)xmalloc(1);
int n_repliers = 0;

a_replier::a_replier(socky int s) : agent(T_NONE, T_NONE)
{
	buffer = NULL;
	len = 0;
	sd = s;
}

a_replier::~a_replier()
{
	if (buffer) free(buffer);
	buffer = NULL;
	replier_table[sd] = NULL;
}

void
a_replier::run()
{
	int result = ywrite(sd, buffer, len);
	if (result == len || result == -1 && errno == EPIPE) {
		len = 0;
		free(buffer);
		buffer = NULL;
		return;
	}
	if (result == -1) {
		push(sd);
		return;
	}
	char *tmp = (char *)xmalloc(len - result);
	memcpy(tmp, buffer + result, len - result);
	free(buffer);
	buffer = tmp;
	len -= result;
	push(sd);
}

void
a_replier::write(const char *str, int l)
{
	if (!buffer) buffer = (char *)xmalloc(l);
	else buffer = (char *)xrealloc(buffer, len + l);

	memcpy(buffer + len, str, l);
	len += l;

	if (len == l) push(sd);
}



int async_sputs(socky int sd, const char *buffer, int len)
{
	if (sd < 0) return -1;
	if (sd < n_repliers && replier_table[sd]) {
		replier_table[sd]->write(buffer, len);
		return len;
	}
	int result = ywrite(sd, buffer, len);
	if (result == len) return len;
	if (result == -1 && errno == EAGAIN)
		result = 0;
	if (result != -1) {
		if (sd >= n_repliers) {
			n_repliers = sd + 1;
			replier_table = (a_replier **)xrealloc(replier_table, n_repliers * sizeof(a_replier));
		}
		replier_table[sd] = new a_replier(sd);
		replier_table[sd]->write(buffer + result, len - result);
		return len;
	}
	if (errno == EPIPE || errno == ECONNRESET) {
		return -1;
	}
	shriek(861, "sputs failed in an unknown way");
	return -1;
}

void use_async_sputs()
{
	sputs_replacement = &async_sputs;
}

