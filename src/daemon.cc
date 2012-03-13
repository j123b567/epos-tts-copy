/*
 *	ss/src/daemon.cc
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

#include "common.h"
#include "client.h"

#define __UNIX__
#define LOG printf

#ifdef HAVE_UNISTD_H			// fork() only
	#include <unistd.h>	// in DOS, fork(){return -1;} is supplied in interf.cc
#endif

inline int my_fork()
{
	if (!cfg->forking) return -1;
	else return fork();
}


#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
#endif

#ifdef HAVE_SYS_SELECT_H
	#include <sys/select.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
	#include <sys/ioctl.h>
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifdef HAVE_LINUX_IN_H
	#include <linux/in.h>
#endif

#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#ifndef HAVE_SOCKLEN_T
#define	socklen_t int
#endif

int session_uid = UID_SERVER;
int dispatcher_pid;

#define   SERVER_PASSWD_LEN   14
char server_passwd[SERVER_PASSWD_LEN + 2];

hash * ttscp_keywords = NULL;
// hash * settable_options = NULL;

class context
{
   public:
	FILE *stdshriek;
	FILE *stdwarn;
	FILE *stddbg;

	int uid;

	configuration *cfg;
	lang *this_lang;
	voice *this_voice;

	char *sgets_buff;	/* not touched upon context switch */
};


context *master_context = NULL;
context **context_table = (context **)malloc(sizeof(context *));
int n_contexts = 0;

void setup_master_context()
{
	lang *l;

	master_context = new context;
	master_context->cfg = cfg;
	master_context->this_lang = this_lang;
	master_context->this_voice = this_voice;
	master_context->uid = UID_ANON;
	master_context->stdshriek = stdshriek;
	master_context->stdwarn = stdwarn;
	master_context->stddbg = stddbg;
	master_context->sgets_buff = NULL;

	for (int i=0; i < cfg->n_langs; i++) {
		l = cfg->langs[i];
		l->cow = true;
		for (int j=0; j < l->n_voices; j++) {
			l->voices[j]->cow = true;
		}
	}
	cfg->cow = true;
}

void create_context(int index)
{
	LOG("create_context(%d), pid=%d\n", index, getpid());
	context *c;

	if (cfg != &master_cfg) shriek("nesting contexts");

	if (index >= n_contexts) {
		context_table = (context **)realloc(context_table, (index+1) * sizeof(context *));
		for (; n_contexts < index + 1;  n_contexts++)
			context_table[n_contexts] = NULL;
//		n_contexts = index + 1;
	}
	context_table[index] = c = new context;

	c->this_voice = this_voice;
	c->this_lang = this_lang;
	c->cfg = cfg;

	c->uid = UID_ANON;
	c->cfg->sd = index;

	c->stddbg = stddbg = fdopen(dup(index), "r+");
	c->stdshriek = stdshriek = fdopen(dup(index), "r+");
	c->stdwarn = stdwarn = fdopen(dup(index), "r+");

	c->sgets_buff = (char *)malloc(cfg->max_net_cmd);
	*c->sgets_buff = 0;
}

void forget_context(int index)
{
	LOG("forget_context(%d), pid=%d\n", index, getpid());
	context *c = context_table[index];

	close(c->cfg->sd);
	fclose(c->stddbg);
	fclose(c->stdshriek);
	fclose(c->stdwarn);

	if (!c->this_voice->cow) free(c->this_voice);
	if (!c->this_lang->cow) free(c->this_lang);
	if (!c->cfg->cow) free(c->cfg);

	free(c->sgets_buff);

	delete context_table[index];
	context_table[index] = NULL;
}

void enter_context(int index)
{
	LOG("enter_context(%d), pid=%d\n", index, getpid());
	if (cfg != &master_cfg) shriek("nesting contexts");

	context *c = context_table[index];

	if (!c) {
		LOG("Nothing to enter!");
	}

	this_voice = c->this_voice;
	this_lang = c->this_lang;
	cfg = c->cfg;

	this_voice->cow = true;
	this_lang->cow = true;
	cfg->cow = true;

	stddbg = c->stddbg;
	stdshriek = c->stdshriek;
	stdwarn = c->stdwarn;

	session_uid = c->uid;
	cfg->sd = index;	// should be unnecessary. Why not?
}

void leave_context(int index)
{
	LOG("leave_context(%d), pid=%d\n", index, getpid());
	context *c = context_table[index];
	if (!c) {
		LOG("(nothing to leave!)\n");
		return;
	}

	c->this_voice = this_voice;
	c->this_lang = this_lang;
	c->cfg = cfg;

	c->uid = session_uid;

	c->stddbg = stddbg;
	c->stdshriek = stdshriek;
	c->stdwarn = stdwarn;

	this_voice = master_context->this_voice;
	this_lang = master_context->this_lang;
	cfg = master_context->cfg;
	session_uid = master_context->uid;
	stddbg = master_context->stddbg;
	stdwarn = master_context->stdwarn;
	stdshriek = master_context->stdshriek;
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
 *	returns:      0   partial line in partbuff
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
		LOG("[core] Appending.\n");
		l = strlen(partbuff);
		if (l > space) shriek("sgets() holdback overflow");
		if (l == space) goto too_long;
		strcpy(buffer, partbuff);
		if (strchr(buffer, '\n')) goto already_enough_text;
	} else l = 0;
	result = read(sd, buffer + l, space - l); buffer[l+result] = 0;
	if (result <=  0) {
		*partbuff = 0;	/* forgetting partial line upon EOF/error. Bad? */
		*buffer = 0;
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
	LOG("[core] partial line read: %s\n", partbuff);
	*buffer = 0;
	return 0;

too_long:
	strcpy(partbuff, "remark ");
	LOG("[core] Too long command ignored");
	sputs("413 Too long", sd);
	*buffer = 0;
	return 0;
}

inline ACCESS access_level(int uid)
{
	if (uid == UID_ROOT) return A_ROOT;
	if (uid < 0) return A_PUBLIC;
	return A_AUTH;
}


void reply(const char *text)
{
	fflush(NULL);
	sputs(text, cfg->sd);
	sputs("\n", cfg->sd);
	fflush(NULL);
}

inline void register_child(int pid)
{
	static int *children = NULL;
	static int n_children = 0;
	static int lost_child = 0;
	int i;

	LOG("[core] register_child(%d)\n", pid);
	
	if (!children) children = (int *)FOREVER(malloc(sizeof(int) * cfg->max_children));
	if (n_children == cfg->max_children && pid > 0) shriek("Too many children");

	if (!pid) {
		LOG("[core] Killing %d children\n", n_children);
		for (i=0; i<n_children; i++) if (kill(children[i], SIGHUP)) 
			shriek("Problems killing child %d", children[i]);
		LOG("[core] Waiting for %d killed children\n", n_children);
		for (i=0; i<n_children; i++) LOG("[core] Child %d returned\n",wait(NULL));
		n_children = 0;
		return;
	}
	if (pid == lost_child) {
		LOG("[core] Lost child come home\n");
		lost_child = 0;
		return;
	}

	if (pid>0) children[n_children++] = pid;
	else {
		for (i=0; i<n_children; i++)
			if (-pid == children[i]) {
				children[i] = children[--n_children];
				return;
			}
		LOG("[core] Lost a child\n");
		if (lost_child) shriek("process %d returns before starting, again", -pid);
		lost_child = -pid;
	}
}

void done_child()
{
//	if (!cfg->forking) return; 	// deadlock might result. (requires further thought)

	LOG("child] All done.\n");
	int sd = connect_socket(SSD_TCP_PORT);
//	sputs("hello ", sd);
//	sputs("anonymous\n", sd);	// replace with auth
	sputs("pass ", sd);
	sputs(server_passwd, sd);
	sputs("\nreap ", sd);
	sprintf(scratch, "%d\n", getpid());
	sputs(scratch, sd);
	sputs("done\n", sd);
	sleep(200);
//	close(sd);
	exit(0);
}

/*
bool may_change_opt(char *param)
{

	if (session_uid == UID_ROOT) return true;
	if (!settable_options) {
		char *pathname = compose_pathname(cfg->allow_file, cfg->ini_dir);
		settable_options = new hash(pathname, 40,10,200,6, "", false, ANYWAY);
		free(pathname);
	}
	if (settable_options->items == -1) return false;
	return settable_options->translate(param) ? true : false;
}
*/

void cmd_bad(char *param)
{
	unuse(param);
	reply("411 He?");
}

void cmd_hello(char *param)
{
	if (param && !strcmp(param, "anonymous")) {
		reply("212 anonymous access allowed");
		return;
	}
	reply("360 successfully ignored");
	reply("211 go ahead");			// 211 anonymous access
}

void cmd_pass(char *param)
{
	if (session_uid == UID_ANON && param && !strcmp(param, server_passwd)) {
		LOG("[core] It's me\n");
		session_uid = UID_SERVER;
	}
	reply("360 successfully ignored");
	reply("211 go ahead");			// 211 anonymous access
}

void cmd_done(char *param)
{
	reply("600 OK");
	if (param) shriek("done should have no param");
	throw new old_style_exc;
}

void cmd_say(char *param)
{
	unit *root = str2units(param);

//	if (cfg->trans) root->fout(NULL);

/*	if (this_voice->fd == -1) {
 *		delete root;
 *		reply("421 voice busy");
 *		return;
 *	}
 *	this_voice->fd = -1;
 */
	reply("111 Text ok");

	int child = my_fork();
	if (child > 0) register_child(child);
	else play_diphones(root, this_voice);
	if (!child) done_child();
	if (cfg->show_diph) show_diphones(root);

	delete root;

	reply("200 OK");
}

void cmd_break(char *param)
{
	if (param) shriek("break should have no param");
	register_child(0);
	reply("200 OK");
}

void cmd_transcribe(char *param)
{
	reply("121 Transcription being sent");
	unit *root = str2units(param);
	if (cfg->trans) root->fout(NULL);
	reply("200 OK");
}

void cmd_set(char *param)
{
	char *value = split_string(param);

	option *o = option_struct(param, this_lang->soft_options);

	if (o) {
		if (access_level(session_uid) >= o->writable) {
			if (set_option(o, value)) reply("200 OK");
			else reply("412 illegal value");
		} else reply("451 Access denied");
	} else {
		if (!param) param = "";
		if (!strcmp("language", param)) {
			if (lang_switch(value)) reply ("200 OK");
			else reply ("443 unknown language");
			return;
		}
		if (!strcmp("voice", param)) {
			if (voice_switch(value)) reply ("200 OK");
			else reply ("443 unknown voice");
			return;
		}
		reply("442 No such option");
	}
}

//	if (may_change_opt(param) && set_option(param, value)) reply("200 OK");


void cmd_help(char *param)
{
	unuse (param);
	reply ("done");
	reply ("hello <user@host> | anonymous");
	reply ("pass <password>");
	reply ("say [<text>]");
	reply ("break");
	reply ("set <option name> [<value>] | language <name> | voice <name>");
	reply ("show <option name> | language | languages | voice | voices");
	reply ("trans [<text>]");
	reply ("shutdown");
	reply ("200 OK");
}

void cmd_shutdown(char *param)
{
	if (param) shriek("shutdown should have no param");
	reply("800 shutdown OK");
	register_child(0);	// kill all children, release memory
	leave_context(cfg->sd);
	close(cfg->sd);
	for (int i=0; i<n_contexts; i++)
		if (context_table[i]) forget_context(i);
	free(context_table);
	delete ttscp_keywords;
	ss_done();
	exit(0);
}

/************ ain't work (see the next line in server() just after the dispatcher call)
void cmd_restart(char *param)
{
	if (param) shriek("shutdown should have no param");
	reply("800 will restart");	// may be mad
	register_child(0);		// kill all children, release memory
	leave_context(cfg->sd);
	close(cfg->sd);
	for (int i=0; i<n_contexts; i++)
		if (context_table[i]) forget_context(i);
	ss_reinit();
}
**************/

void cmd_reap(char *param)
{
	int cpid = 0;

	if (session_uid != UID_SERVER) {
		LOG("Unauthorised reap attempt, uid=%d\n", session_uid);
		reply("411 no such command");
		return;
	}

	if (param && sscanf(param, "%d", &cpid)) {
		if (cpid) kill(cpid, SIGHUP);
		cpid = wait(NULL);
		if (cpid == -1) {
			LOG("[core] No child returned upon reap %d, uid=%d!\n", cpid, session_uid);
			return;
		}
		LOG("[core] Suddenly, child %d returned\n",  cpid);
		register_child(-cpid);
	} else LOG("[core] reap with no param?\n");
}

void cmd_show(char *param)
{
	int i;

	option *o = option_struct(param, this_lang->soft_options);

	if (o) {
		if (access_level(session_uid) >= o->readable) {
			char *tmp = format_option(o);
			reply(tmp);
			free(tmp);
			reply("200 OK");
		} else reply("451 Access denied");
	} else {
		if (!param) param = "";
		if (!strcmp("language", param)) {
			reply(this_lang->name);
			reply ("200 OK");
			return;
		}
		if (!strcmp("voice", param)) {
			reply(this_voice->name);
			reply ("200 OK");
			return;
		}
		if (!strcmp("languages", param)) {
			int bufflen = 0;
			for (i=0; i < cfg->n_langs; i++) bufflen += strlen(cfg->langs[i]->name) + strlen(cfg->comma);
			char *result = (char *)malloc(bufflen + 1);
			strcpy(result, cfg->n_langs ? cfg->langs[0]->name : "(empty list)");
			for (i=1; i < cfg->n_langs; i++) {
				strcat(result, cfg->comma);
				strcat(result, cfg->langs[i]->name);
			}
			reply(result);
			free(result);
			return;
		}
		if (!strcmp("voices", param)) {
			int bufflen = 0;
			for (i=0; i < this_lang->n_voices; i++)
				bufflen += strlen(this_lang->voices[i]->name) + strlen(cfg->comma);
			char *result = (char *)malloc(bufflen + 1);
			strcpy(result, this_lang->n_voices ? this_lang->voices[0]->name : "(empty list)");
			for (i=1; i < this_lang->n_voices; i++) {
				strcat(result, cfg->comma);
				strcat(result, this_lang->voices[i]->name);
			}
			reply(result);
			free(result);
			return;
		}
		reply("442 No such option");
	}
}


/******
	if (param && (value = format_option(param))) {
		reply("141 here you are");
		reply(value);
		reply("200 OK");
		free(value);
	} else reply("442 No such option");
********/

#define keyword_list "hello:pass:done:help:say:set:trans:break:shutdown:reap:show:"
void (*dispatch[])(char *param) = {cmd_hello, cmd_pass, cmd_done, cmd_help, cmd_say, cmd_set,
		cmd_transcribe, cmd_break, cmd_shutdown, cmd_reap, cmd_show};


void run_command(char *cmd)
{
	char *keyword;
	char *param;
	int cmd_index;

	LOG("[ cmd] %s\n", cmd);

	keyword = cmd + strspn (cmd, WHITESPACE);
	param = keyword + strcspn(keyword, WHITESPACE);
	if (!*param) param = NULL;
	else *param++ = 0;

	cmd_index = ttscp_keywords->translate_int(keyword);
	if (cmd_index == INT_NOT_FOUND) {
		cmd_bad(cmd);
		return;
	};

	if (param) param += strspn(param, WHITESPACE);
	dispatch[cmd_index](param);
}

void server()
{
	static int sk;
	static socklen_t sia = sizeof(sockaddr);	// __QNX__ wants signed :-(
	static sockaddr_in sa;
	static sockaddr_in ia;
	
	int f, i, max;
	int one = 1;

	sk = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(0x7f000001);
	sa.sin_port = htons(cfg->listen_port);
	LOG("[core] Trying to bind...\n");
	while (bind(sk, (sockaddr *)&sa, sizeof (sa)) && --cfg->persistence) 
		sleep(1);
	setsockopt(f, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int)); // not necessary
	if (!cfg->persistence) shriek("Could not bind: %s", strerror(errno));
	if (listen(sk, 5)) shriek("Could not listen");
	ia.sin_family = AF_INET;
	ia.sin_addr.s_addr = INADDR_ANY;
	ia.sin_port = 0;

	char *buffer = (char *)FOREVER(malloc(cfg->max_net_cmd + 2));

	setup_master_context();
	fd_set thefds;
	FD_ZERO(&thefds);
	FD_SET(sk, &thefds);
	max = sk + 1;

	while (1) {
		fd_set rdfds = thefds;
//		LOG("[core] fdset: ");
//		for (i=0; i<=max; i++) LOG(FD_ISSET(i, &rdfds) ? "X" : "-"); LOG("\n");
		LOG("[core] Ready....\n");
		if (select(max, &rdfds, NULL, NULL, NULL) <= 0) shriek("select() failed");
//		LOG("[core] ...got a job\n");
		if (FD_ISSET(sk, &rdfds)) {
			f = accept(sk, (sockaddr *)&ia, &sia);
//			fcntl(f, F_SETFL, O_NONBLOCK);
			LOG("[core] Accepted.\n");
			FD_SET(f, &thefds);
			if (f >= max) max = f+1;
			create_context(f);
			enter_context(f);
			reply("preTTSCP spoken here");
			leave_context(f);
			continue;
		}
		for (i=0; i<max; i++) if (FD_ISSET(i, &rdfds)) {
			try {
				LOG("[core] serving job %d (of %d)\n", i, max);
				enter_context(i);
more_lines:
				if (sgets(buffer, cfg->max_net_cmd, cfg->sd,
				     context_table[cfg->sd]->sgets_buff) == -1) {
					LOG("[core] client unreachable\n");
					leave_context(i);
					FD_CLR(i, &thefds);
					forget_context(i);
					continue;
				}
				if ((int)strlen(buffer) >= cfg->max_net_cmd)
					shriek("Received command is too looong");
				if (*buffer) run_command(buffer);
				if (strchr(context_table[cfg->sd]->sgets_buff, '\n'))
					goto more_lines;
				leave_context(i);
			} catch (old_style_exc *e) {
				free(e);
				reply("690 session terminated");
				if (dispatcher_pid != getpid()) done_child();
				LOG("[core] session terminated\n");
				if (context_table[i]) {
					leave_context(i);
					FD_CLR(i, &thefds);
					forget_context(i);
					close(i);
				}
			}
		}
	}
}

void daemonize(const char *logfile)
{
	int i;

        ioctl(0, TIOCNOTTY);          //Release the control terminal
	for (i=0; i<3; i++) close(i);
	if (logfile && open(logfile, O_RDWR|O_CREAT|O_APPEND|O_NOCTTY) != -1) {
		for (i=1; i<3; i++) dup(0);
		LOG("\n\n\n\nssd restarted at ");
		fflush(stdout);
		system("/bin/date");
	}
	signal(SIGPIPE, SIG_IGN);	// possibly dangerous

	dispatcher_pid = getpid();

	for (i = 0; i < SERVER_PASSWD_LEN; i++) server_passwd[i] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-"
			[rand() & 63];
	server_passwd[i] = 0;
//	LOG("[core] server internal password is %s\n", server_passwd);
}


int main(int argc, char **argv)
{
	try {
		ss_init(argc, argv);
		if (just_connect_socket(cfg->listen_port) > -1)
			shriek("Already running\n");
		ttscp_keywords = str2hash(keyword_list, 0);
		switch (my_fork()) {
			case -1: server();
				 return 0;	/* foreground process */
			case 0:  daemonize(cfg->daemon_log);
				 server();
				 return 0;	/* child  */
			default: return 0;	/* parent */
		}

	} catch (exception *e) {

		/* handle all uncatched exceptions here */
		unuse(e);
		return 4;
	}
}

