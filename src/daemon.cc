/*
 *	epos/src/daemon.cc
 *	(c) 1998 geo@ff.cuni.cz
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

#ifdef ___

parser:	stml, raw

          TTSCP 000
client: ctrl
          200 task 1
client: strm #034:synth:#034
          200 stream OK
client: skip 0
client: appl 243
client: appl 51
client: appl 55
client: fork
          200 task 2
client: set language chinese
client: strm #022:chinese-stml:rules:print:#022
client: appl 72
client: task 1
client: appl 51
client: 



#endif

/*
 *	We are running a kind of a cooperative multitasking. The tasks are
 *	represented by "agents"; they get their timeslices using agent::run().
 *	Every task is allowed to run as long as it needs, except if it had
 *	to perform some potentially (noticeably) slow i/o, such as a network
 *	commn or writing to limited sound card buffers. All file descriptors
 *	used for data transfer are marked as non-blocking and the agents
 *	yield control on -EAGAIN. The descriptor is then added to a select
 *	set and the agent gets another timeslice when new data arrives.
 *
 *	Occassionaly, there may be more than one agent with useful work to do.
 *	We queue them using agent::schedule() and select them using sched_sel().
 *	If more than SCHED_SATIATED agents are scheduled (for immediate execution),
 *	the scheduler will stop checking the file descriptors until the situation
 *	gets under control.
 *
 *	We are currently not trying to avoid DoS attacks such as sending
 *	a megabyte file for processing, then writing it to disk.
 */

#include "common.h"
#include "client.h"
#include "agent.h"

#define __UNIX__

#ifdef HAVE_SYS_IOCTL_H
	#include <sys/ioctl.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifdef HAVE_LINUX_IN_H
	#include <linux/in.h>
#endif

#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#endif

#ifdef HAVE_WAIT_H
	#include <wait.h>
#endif

#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#define DARK_ERRLOG 2	/* 2 == stderr; for global stdshriek and stddbg output */

int session_uid = UID_SERVER;


context *master_context = NULL;
// context **context_table = (context **)malloc(sizeof(context *));
// int n_contexts = 0;

context::context(int std_in, int std_out)
{
	this_voice = ::this_voice;
	this_lang = ::this_lang;
	cfg = ::cfg;

	if (/* already exists */ master_context) {
		cow((cowabilium **)&cfg, sizeof(configuration));
#ifdef DEBUG_TO_TTSCP
		cfg->stddbg =  fdopen(dup(std_out), "r+");
#else
		//	cfg->stddbg =
#endif
#ifdef SHRIEK_TO_TTSCP
		cfg->stdshriek = fdopen(dup(std_out), "r+");
#else
		//	cfg->stdshriek =
#endif
	} else {
		DEBUG(2,11,fprintf(STDDBG, "master context OK\n");)
		cow_claim(this);
	}

	uid = UID_ANON;
	cfg->sd_in = std_in;
	cfg->sd_out = std_out;
	DEBUG(1,11,fprintf(STDDBG, "new context uses fd %d and %d\n", std_in, std_out);)

	sgets_buff = (char *)malloc(cfg->max_net_cmd);
	*sgets_buff = 0;
}

context::~context()
{
	free(sgets_buff);
#ifdef DEBUG_TO_TTSCP
	if (!cfg->stddbg) shriek(862, "Reclosing in ~context");
	fclose(cfg->stddbg);
	cfg->stddbg = NULL;
#else
	// fclose(cfg->stddbg...
#endif
#ifdef SHRIEK_TO_TTSCP
	if (!cfg->stdshriek) shriek(862, "Reclosing in ~context");
	fclose(cfg->stdshriek);
	cfg->stdshriek = NULL;
#else
	// fclose(cfg->stdshriek...
#endif
}

void
context::enter()
{
	DEBUG(1,11,fprintf(STDDBG, "enter_context(%p)\n", this);)
	if (::cfg != &master_cfg) shriek(462, "nesting contexts");

	if (!this) {
		DEBUG(2,11,fprintf(STDDBG, "(nothing to enter!)\n");)
		return;
	}

	::this_voice = this_voice;
	::this_lang = this_lang;
	::cfg = cfg;

//	::this_voice->cow = true;
//	::this_lang->cow = true;
//	::cfg->cow = true;

	session_uid = uid;
//	cfg->sd = index;	// should be unnecessary. Why not?
}

void
context::leave()
{
//	context *c = context_table[index];
	if (!this) {
		DEBUG(2,11,fprintf(STDDBG, "(nothing to leave!)\n");)
		return;
	}

	this_voice = ::this_voice;
	this_lang = ::this_lang;
	cfg = ::cfg;

	uid = session_uid;

	::this_voice = master_context->this_voice;
	::this_lang = master_context->this_lang;
	::cfg = master_context->cfg;
	session_uid = master_context->uid;
	DEBUG(1,11,fprintf(STDDBG, "leave_context(%p)\n", this);)
}

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
 *	It doesn't work, however. Please FIXME.
 */

int sgets(char *buffer, int space, int sd, char *partbuff)
{
	int i, l;
	int result = 0;

	if (*partbuff) {
		DEBUG(2,11,fprintf(STDDBG, "[core] Appending.\n");)
		l = strlen(partbuff);
		if (l > space) shriek(664, "sgets() holdback overflow");
		if (l == space) goto too_long;
		strcpy(buffer, partbuff);
		if (strchr(buffer, '\n')) goto already_enough_text;
	} else l = 0;
	result = read(sd, buffer + l, space - l); buffer[l+result] = 0;
	if (result <=  0) {
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
	DEBUG(2,11,fprintf(STDDBG, "[core] partial line read: %s\n", partbuff);)
	*buffer = 0;
	return 0;

too_long:
	strcpy(partbuff, "remark ");
	DEBUG(2,11,fprintf(STDDBG, "[core] Too long command ignored");)
	sputs("413 Too long", sd);
	*buffer = 0;
	return 0;
}


#define   SERVER_PASSWD_LEN   14
char server_passwd[SERVER_PASSWD_LEN + 2];

// hash * ttscp_keywords = NULL;

#ifdef HAVE_UNISTD_H
	#define UNIX(x) x
#else
	#define UNIX(x)
#endif

static void detach()
{
	int i;
        UNIX (ioctl(0, TIOCNOTTY);)          //Release the control terminal
	for (i=0; i<3; i++) close(i);
	if (cfg->daemon_log && open(cfg->daemon_log, O_RDWR|O_CREAT|O_APPEND
					UNIX( |O_NOCTTY)) != -1) {
		for (i=1; i<3; i++) dup(0);
		DEBUG(3,11,fprintf(STDDBG, "\n\n\n\nEpos restarted at ");)
		fflush(stdout);
		system("/bin/date");
	}
}

static void daemonize()
{
	UNIX(signal(SIGPIPE, SIG_IGN);)	// possibly dangerous

//	dispatcher_pid = getpid();

	make_rnd_passwd(server_passwd, SERVER_PASSWD_LEN);

	DEBUG(0,11,fprintf(STDDBG, "[core] server internal password is %s\n", server_passwd);)

	FD_ZERO(&block_set);
	FD_ZERO(&push_set);
	master_context = new context(-1, DARK_ERRLOG);
	unuse (new a_accept());
}

#undef UNIX

static fd_set rd_set;
static fd_set wr_set;

static bool select_socket(bool sleep)
{
	rd_set = block_set;
	wr_set = push_set;
	timeval tv; tv.tv_sec = tv.tv_usec = 0;
	int n;

	n = select(select_fd_max, &rd_set, &wr_set, NULL, sleep ? (timeval *)NULL : &tv);
	if (n == 0 && !sleep) return false;
	if (n < 0) shriek(871, "select() failed");
	if (n <= 0) shriek(871, "select() failed to sleep");
	return true;
}

#define SCHED_SATIATED	64
#define SCHED_WARN	 2

bool server_shutting_down = false;

static void server()
{
	int fd;
	daemonize();

	while (!server_shutting_down) {
		while (runnable_agents > SCHED_SATIATED
					|| runnable_agents && !select_socket(false)) {
			DEBUG(3,11,if (runnable_agents > SCHED_WARN) fprintf(STDDBG,"Busy! %d runnable agents\n", runnable_agents);)
			sched_sel()->timeslice();
		}
		select_socket(true);
		for (fd=0; fd < select_fd_max; fd++) if (FD_ISSET(fd, &rd_set) || FD_ISSET(fd, &wr_set)) {
			agent *a = sleep_table[fd];
			DEBUG(2,11,fprintf(STDDBG, a ? "Scheduling select(%d)ed agent\n"
					: "sche sche scheduler\n", fd);)
			sleep_table[fd] = NULL;
			FD_CLR(fd, &block_set);
			FD_CLR(fd, &push_set);
			a->timeslice();
		}
	}
}

int main(int argc, char **argv)
{
	try {
		epos_init(argc, argv);
		if (just_connect_socket(0, cfg->listen_port) > -1)
			shriek(872, "Already running\n");
//		ttscp_keywords = str2hash(keyword_list, 0);
		switch (my_fork()) {
			case -1: server();
				 return 0;	/* foreground process */
			case 0:  detach();
				 server();
				 return 0;	/* child  */
			default: return 0;	/* parent */
		}

	} catch (exception *) {

		/* handle all uncatched exceptions here */
		return 4;
	}
}

