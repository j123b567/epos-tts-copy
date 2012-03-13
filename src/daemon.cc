/*
 *	epos/src/daemon.cc
 *	(c) 1998-01 geo@cuni.cz
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

#ifdef HAVE_SYS_STAT_H
	#include <sys/stat.h>
#endif

#ifdef HAVE_SIGNAL_H
	#include <signal.h>
#endif

#ifdef HAVE_SYS_TERMIOS_H
	#include <sys/termios.h>
#endif

#include <fcntl.h>
#include <errno.h>

#ifdef HAVE_QNX_NAME_ATTACH
	#include "qnxipc.cc"
#else
	inline void qipc_proxy_init() {};
#endif


#ifdef HAVE_WINSOCK2_H
class wsa_init			/* initialise winsock before main() is entered  */
{
   public:
	wsa_init()		/* global constructor */
	{
		char scratch[14227];
		if (WSAStartup(MAKEWORD(2,0), (LPWSADATA)scratch))
			shriek(464, "No winsock");
	};
} wsa_init_instance;

#endif

#ifdef HAVE_WINSVC_H
int start_nt_service();
#endif

const bool is_monolith = 0;


#define DARK_ERRLOG 2	/* 2 == stderr; for global stdshriek and stddbg output */

// int session_uid = UID_SERVER;


context *master_context = NULL;
context *this_context = NULL;

context::context(int std_in, int std_out)
{
	config = cfg;

	if (/* already exists */ master_context) {
		cow_claim();
		cow_configuration(&config);
#ifdef DEBUG_TO_TTSCP
		config->stddbg =  fdopen(dup(std_out), "r+");
#else
		//	config->stddbg =
#endif
#ifdef SHRIEK_TO_TTSCP
		config->stdshriek = fdopen(dup(std_out), "r+");
#else
		//	config->stdshriek =
#endif
	} else {
		DEBUG(2,11,fprintf(STDDBG, "master context OK\n");)
		cow_claim();
		this_context = this;
	}

	uid = UID_ANON;
	config->sd_in = std_in;
	config->sd_out = std_out;
	DEBUG(1,11,fprintf(STDDBG, "new context uses fd %d and %d\n", std_in, std_out);)

	sgets_buff = (char *)xmalloc(config->max_net_cmd);
	*sgets_buff = 0;
}

context::~context()
{
#ifdef DEBUG_TO_TTSCP
	if (!config->stddbg) shriek(862, "Reclosing in ~context");
	fclose(config->stddbg);
	config->stddbg = NULL;
#else
	// fclose(config->stddbg...
#endif
#ifdef SHRIEK_TO_TTSCP
	if (!config->stdshriek) shriek(862, "Reclosing in ~context");
	fclose(config->stdshriek);
	config->stdshriek = NULL;
#else
	// fclose(config->stdshriek...
#endif
	if (this == this_context) leave();  // shriek(862, "Deleting active context");
	cow_unclaim(config);
	free(sgets_buff);
}

void
context::enter()
{
	DEBUG(1,11,fprintf(STDDBG, "enter_context(%p)\n", this);)
	if (!this) {
		DEBUG(2,11,fprintf(STDDBG, "(nothing to enter!)\n");)
		return;
	}

	if (this_context != master_context) shriek(462, "nesting contexts");

//	::this_voice = this_voice;
//	::this_lang = this_lang;
	master_context->config = cfg;
	cfg = config;
	::this_context = this;

//	session_uid = uid;
//	config->sd = index;	// should be unnecessary. Why not?
}

void
context::leave()
{
//	context *c = context_table[index];
	if (!this) {
		DEBUG(2,11,fprintf(STDDBG, "(nothing to leave!)\n");)
		return;
	}
	if (this_context != this) shriek(462, "leaving unentered context");

//	this_voice = ::this_voice;
//	this_lang = ::this_lang;
	config = cfg;

//	uid = session_uid;

//	::this_voice = master_context->this_voice;
//	::this_lang = master_context->this_lang;
	cfg = master_context->config;
	::this_context = master_context;
//	session_uid = master_context->uid;
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



void make_rnd_passwd(char *buffer, int size)
{
	int i;
	for (i = 0; i < size; i++) buffer[i] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"
			[rand() & 63];
	buffer[i] = 0;
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
	if (!cfg->daemon_log || !*cfg->daemon_log)
		return;
	for (i=0; i<3; i++) close(i);
	if (!open(cfg->daemon_log, O_RDWR|O_CREAT|O_APPEND  UNIX( |O_NOCTTY), MODE_MASK)
			|| !open (NULL_FILE, O_RDWR|O_APPEND, MODE_MASK)) {
		for (i=1; i<3; i++) dup(0);
		cfg->colored = false;
		DEBUG(3,11,fprintf(STDDBG, "\n\n\n\nEpos restarted at ");)
		fflush(stdout);
		system("/bin/date");
	} else /* deep OS level trouble, contact authors */ abort();
}

static inline void make_server_passwd()
{
	make_rnd_passwd(server_passwd, SERVER_PASSWD_LEN);
	DEBUG(0,11,fprintf(STDDBG, "[core] server internal password is %s\n", server_passwd);)
	if (cfg->listen_port != TTSCP_PORT) return;

	FILE *f;
	char *filename = compose_pathname(cfg->pwdfile, "");
	if (filename && *filename && (f = fopen(filename, "w", NULL))) {
		UNIX(chmod(filename, S_IRUSR));
		fwrite(server_passwd, SERVER_PASSWD_LEN, 1, f);
		fwrite("\n", 1, 1, f);
		fclose(f);
		free((char *)cfg->pwdfile);
		cfg->pwdfile = filename;
	}
}

volatile bool server_shutting_down = false;

void server_shutdown()
{
	while (ctrl_conns->items) {
		a_ttscp *tmp = ctrl_conns->translate(ctrl_conns->get_random());
		sputs("800 shutdown requested\r\n", tmp->c->config->sd_out);
		delete ctrl_conns->remove(tmp->handle);
	}
	if (cfg->pwdfile) remove(cfg->pwdfile);
	delete accept_conn;
	delete ctrl_conns;
	delete data_conns;
	delete master_context;
	free(sleep_table);
	epos_done();
	exit(0);
}


static void unix_sigterm(int)
{
	if (server_shutting_down) shriek(466, "forcing shutdown");
	server_shutting_down = true;
}

static void daemonize()
{
	UNIX(signal(SIGPIPE, SIG_IGN);)		// possibly dangerous
	UNIX(signal(SIGCHLD, SIG_IGN);)		// automatic child reaping
	UNIX(signal(SIGTERM, unix_sigterm);)

//	dispatcher_pid = getpid();

	make_server_passwd();

	FD_ZERO(&block_set);
	FD_ZERO(&push_set);
	master_context = new context(-1, DARK_ERRLOG);
	accept_conn = new a_accept();
	qipc_proxy_init();

	data_conns->dupkey = data_conns->dupdata = ctrl_conns->dupkey
		= ctrl_conns->dupdata = false;
}

#undef UNIX

/*

fd_set data_conn_set;

void update_data_conn_set(char *, socky int *fd)
{
	FD_SET(*fd, &data_conn_set);
}

*/

static void idle()
{
//	FD_ZERO(&data_conn_set);
//	data_conns->forall(update_data_conn_set);

//	while (waitpid(-1, NULL, WNOHANG) > 0) ;
}

static fd_set rd_set;
static fd_set wr_set;

static bool select_socket(bool sleep)
{
	rd_set = block_set;
	wr_set = push_set;
	timeval tv; tv.tv_sec = tv.tv_usec = 0;
	int n;

	n = select(select_fd_max, &rd_set, &wr_set, NULL, &tv);
	if (n > 0) return true;
	if (!sleep) return false;
	if (n < 0) shriek(871, "select() failed");

   restart:
	if (server_shutting_down) return false;

	idle();

	rd_set = block_set;
	wr_set = push_set;

	n = select(select_fd_max, &rd_set, &wr_set, NULL, (timeval *)NULL);
	if (n <= 0) {
		if (n == -1 && errno == EINTR) goto restart;
		shriek(871, n ? "select() failed" : "select() failed to sleep");
	}
	return true;
}

#define SCHED_SATIATED	64
#define SCHED_WARN	30


void server()
{
	socky int fd;
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
			int pushing = FD_ISSET(fd, &push_set);
			FD_CLR(fd, &block_set);
			FD_CLR(fd, &push_set);
			a->timeslice();
			if (a->dep && sleep_table[fd] == NULL) {
				sleep_table[fd] = a->dep;
				FD_SET(fd, pushing ? &push_set : &block_set);
				a->dep = NULL;
			}
		}
	}
	server_shutdown();
}

void server_crashed(char *, a_ttscp *a, int why_we_crashed)
{
	int w = why_we_crashed;
	char code[]= "865";
	if (w / 100 == 8) code[1] = (w % 100) / 10 + '0', code[2] = w % 10 + '0';
	sputs(code, a->c->config->sd_out);
	sputs(" shutdown, not your problem\n", a->c->config->sd_out);
}

inline void lest_already_running()
{
	if (running_at_localhost()) {
		cfg->pwdfile = NULL;
		shriek(872, "Already running\n");
	}
#ifdef ENETUNREACH
	if (errno == ENETUNREACH)
		shriek(871, "Network unreachable\n");
#endif
}

void init_thrown_exception(int errcode)
{
	ctrl_conns->forall(server_crashed, errcode);
	if (cfg->pwdfile) remove(cfg->pwdfile);
}

int start_unix_daemon()
{
	try {
		epos_init();
		lest_already_running();
		switch (my_fork()) {
			case -1: server();
				 return 0;	/* foreground process */
			case 0:  detach();
				 server();
				 return 0;	/* child  */
			default: return 0;	/* parent */
		}

	} catch (any_exception *e) {
		/* handle all known uncatched exceptions here */
		init_thrown_exception(e->code);
		return 1;
	} catch (...) {
		/* handle all unknown uncatched exceptions here */
		init_thrown_exception(869);
		return 2;
	}
}


int main(int argc, char **argv)
{
	argc_copy = argc, argv_copy = argv;
#ifdef HAVE_WINSVC_H
	int k = start_nt_service();
	if (!k) return 0;
	if (k != ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) return k;
#endif
	return start_unix_daemon();
}

