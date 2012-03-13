/*
 *	epos/src/ttscp.cc
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
#include "agent.h"
#include "client.h"


//#ifdef HAVE_UNISTD_H			// fork() only
//	#include <unistd.h>	// in DOS, fork(){return -1;} is supplied in interf.cc
//#endif
//
//#ifdef HAVE_SYS_IOCTL_H
//	#include <sys/ioctl.h>
//#endif
//
//#ifdef HAVE_NETINET_IN_H
//	#include <netinet/in.h>
//#endif
//
//#ifdef HAVE_LINUX_IN_H
//	#include <linux/in.h>
//#endif
//
#include <signal.h>
//#include <fcntl.h>
//#include <wait.h>
//#include <errno.h>

hash_table<char, int> *data_conns = new hash_table<char, int> (30);


void reply(const char *text)
{
	fflush(NULL);
	sputs(text, cfg->sd_out);
	sputs("\n", cfg->sd_out);
	fflush(NULL);
}

void reply(int code, const char *text)
{
	if (code == MUTE_EXCEPTION) return;
	fflush(NULL);
	char c[5];
	c[0] = code / 100 + '0';
	c[1] = code / 10 % 10 + '0';
	c[2] = code % 10 + '0';
	c[3] = ' ';
	c[4] = 0;
	sputs(c, cfg->sd_out);
	reply(text);
}

inline ACCESS access_level(int uid)
{
	if (uid == UID_ROOT) return A_ROOT;
	if (uid < 0) return A_PUBLIC;
	return A_AUTH;
}


int cmd_bad(char *)
{
	reply("411 He?");
	return PA_NEXT;
}

int cmd_user(char *param, agent *)
{
	if (param && !strcmp(param, "anonymous")) {
		reply("212 anonymous access allowed");
		return PA_NEXT;
	}
	reply("452 no such user");
//	reply("211 go ahead");			// 211 grant access
	return PA_NEXT;
}

int cmd_pass(char *param, agent *)
{
	if (session_uid == UID_ANON && param && !strcmp(param, server_passwd)) {
		DEBUG(2,11,fprintf(STDDBG, "[core] It's me\n");)
		session_uid = UID_SERVER;
	}
	reply("452 bad password");
//	reply("211 go ahead");			// 211 anonymous access
	return PA_NEXT;
}

int cmd_done(char *param, agent *)
{
	if (param) shriek(416, "done should have no param");
	reply("600 OK");
	return PA_DONE;
}

#ifdef SAY_IS_OBSOLETE_BUT

int cmd_say(char *param, agent *)
{
	unit *root = str2units(param);
	this_lang->ruleset->apply(root);

//	if (cfg->trans) root->fout(NULL);

/*	if (this_voice->fd == -1) {
 *		delete root;
 *		reply("421 voice busy");
 *		return;
 *	}
 *	this_voice->fd = -1;
 */
	reply("111 Text ok");

#ifdef FANCY_CHILDREN
	int child = my_fork();
	if (child > 0) register_child(child);
	else play_diphones(root, this_voice);
	if (!child) done_child();
#else
	play_diphones(root, this_voice);
#endif
	if (cfg->show_diph) show_diphones(root);

	delete root;

	reply("200 OK");
	return PA_NEXT;
}

#endif


int cmd_break(char *param, agent *a)
{
	if (param) shriek(416, "break should have no param");

	/*
	 * EXPERIMENTAL
	 *	the 401 reply is sent by the agent with inb != NULL
	 */

	a->c->leave();
	ctrl_conns->brkall();
	a->c->enter();

//	for (a_ttscp *at = ctrl_conns; at; at = at->next)
//	shriek(462, "break broken");
//	register_child(0);

	reply("200 OK");
	return PA_NEXT;
}

int cmd_transcribe(char *param, agent *)
{
	reply("121 Transcription being sent");
	unit *root = str2units(param);
	this_lang->ruleset->apply(root);
	if (cfg->trans) root->fout(NULL);
	reply("200 OK");
	return PA_NEXT;
}

int cmd_set(char *param, agent *)
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
			return PA_NEXT;
		}
		if (!strcmp("voice", param)) {
			if (voice_switch(value)) reply ("200 OK");
			else reply ("443 unknown voice");
			return PA_NEXT;
		}
		reply("442 No such option");
	}
	return PA_NEXT;
}


int cmd_help(char *param, agent *)
{
	if (param) {
		ttscp_cmd *cmd = ttscp_cmd_set;
		while (cmd->name && strncmp((char *)&cmd->name, param, 4)) cmd++;
		if (!cmd->name) {
			reply("441 No such command");
			return PA_NEXT;
		}
		sprintf(scratch, "%.4s %s\n%s", (char *)&cmd->name, cmd->short_help,
			cmd->long_help);
		reply(scratch);
	} else
		for (ttscp_cmd *cmd = ttscp_cmd_set; cmd->name; cmd++) {
			sprintf(scratch, "%.4s %s", (char *)&cmd->name, cmd->short_help);
			reply(scratch);
		}
	reply("200 OK");
	return PA_NEXT;
}

int cmd_shutdown(char *param, agent *)
{
	if (param) shriek(416, "shutdown should have no param");
	reply("800 shutdown OK");
//	register_child(0);	// kill all children, release memory
//	leave_context(cfg->sd);		FIXME
	close(cfg->sd_in);
	if (cfg->sd_in != cfg->sd_out)
		close(cfg->sd_out);
//	for (int i=0; i<n_contexts; i++)
//		if (context_table[i]) forget_context(i);
//	free(context_table);
//	delete ttscp_keywords;
	delete data_conns;
	epos_done();
	exit(0);
}

/************ ain't work (see the next line in server() just after the dispatcher call)
void cmd_restart(char *param)
{
	if (param) shriek(416, "shutdown should have no param");
	reply("800 will restart");	// may be mad
	register_child(0);		// kill all children, release memory
	leave_context(cfg->sd);
	close(cfg->sd);
	for (int i=0; i<n_contexts; i++)
		if (context_table[i]) forget_context(i);
	epos_reinit();
}
**************/

void cmd_reap(char *param, agent *)
{

	shriek(462, "reap broken");

/*	if (session_uid != UID_SERVER) {
 *		DEBUG(3,11,fprintf(STDDBG, "Unauthorised reap attempt, uid=%d\n", session_uid);)
 *		reply("411 no such command");
		return;
	}

	int cpid = 0;

	if (param && sscanf(param, "%d", &cpid)) {
		if (cpid) kill(cpid, SIGHUP);
		cpid = wait(NULL);
		if (cpid == -1) {
			DEBUG(2,11,fprintf(STDDBG, "[core] No child returned upon reap %d, uid=%d!\n", cpid, session_uid);)
			return;
		}
		DEBUG(1,11,fprintf(STDDBG, "[core] Suddenly, child %d returned\n",  cpid);)
//		register_child(-cpid);
 *	} else DEBUG(2,11,fprintf(STDDBG, "[core] reap with no param?\n"));
 *
 */
}

int cmd_show(char *param, agent *)
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
			reply("200 OK");
			return PA_NEXT;
		}
		if (!strcmp("voice", param)) {
			reply(this_voice->name);
			reply("200 OK");
			return PA_NEXT;
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
			reply("200 OK");
			return PA_NEXT;
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
			reply("200 OK");
			return PA_NEXT;
		}
		reply("442 No such option");
	}
	return PA_NEXT;
}

int cmd_stream(char *param, agent *)
{
	if (!param) {
		reply("415 Bad stream");
		return PA_NEXT;
	}
	DEBUG(1,11,fprintf(STDDBG, "current_strm %p\n", cfg->current_stream);)
	if (cfg->current_stream) delete cfg->current_stream;
	cfg->current_stream = NULL;
	cfg->current_stream = new stream(param);
	reply("200 OK");
	return PA_NEXT;
}

int cmd_apply(char *param, agent *a)
{
	int n;
	if (!sscanf(param, "%d", &n) || n < 0) {
		reply("414 Bad size");
		return PA_NEXT;
	}
	DEBUG(2,11,fprintf(STDDBG, "cmd_apply calls %p\n", cfg->current_stream);)
	if (!cfg->current_stream) {
		reply("415 strm command must be issued first");
		return PA_NEXT;
	}
	cfg->current_stream->apply(a, n);
	reply("112 started");
	return PA_WAIT;
}

#define DATA_HANDLE_SIZE   4

int cmd_data(char *param, agent *)
{
	if (cfg->sd_in != cfg->sd_out) shriek(862, "Data connection halved");
	if (param) {
		if (data_conns->translate(param)) {
			reply("422 Not unique");
			return PA_NEXT;
		}
		data_conns->add(param, &cfg->sd_in);
	} else {
		*scratch = ' ';		/* in TTSCP, the handle must be preceded by a ' ' */
		do make_rnd_passwd(scratch + 1, DATA_HANDLE_SIZE);
			while (data_conns->translate(scratch));
		reply("142 data connection handle follows");
		reply(scratch);
		data_conns->add(scratch + 1, &cfg->sd_in);
	}
	reply("200 OK");
	cfg->sd_in = -1;	/* protect the socket from closing */
	cfg->sd_out = -1;
	return PA_DONE;
}

int cmd_delhandle(char *param, agent *)
{
	if (!param) shriek(416, "delete handle needs param");
	int *fd = data_conns->remove(param);
	if (!fd) shriek(444, "invalid data connection handle");
	async_close(*fd);
	delete fd;
	reply("200 OK");
	return PA_NEXT;
}

/******
	if (param && (value = format_option(param))) {
		reply("141 here you are");
		reply(value);
		reply("200 OK");
		free(value);
	} else reply("442 No such option");

#define keyword_list "user:pass:done:help:say:set:trans:break:shutdown:reap:show:strm:"
void (*dispatch[])(char *param) = {cmd_user, cmd_pass, cmd_done, cmd_help, cmd_say, cmd_set,
		cmd_transcribe, cmd_break, cmd_shutdown, cmd_reap, cmd_show,
		cmd_stream};

********/


/*
 *	The commands below are ordered by their estimated relative frequency
 *	in real world TTSCP sessions, for efficiency reasons.
 */


#define APPLY_HELP	"Process n bytes by the current stream."

#define DATA_HELP	"Turn this control connection to a data connection.\n"\
			"The server will reply with a data connection handle\n"\
			"to be referred to in subsequent commands."

#define SET_HELP	"Set an operating parameter to the value specified.\n"\
			"It is also possible to set the current language or\n"\
			"the current voice. Note that some parameters may have\n"\
			"access restrictions configured for security reasons."

#define SHOW_HELP	"Show the current value for an operating parameter.\n"\
			"It is also possible to view the current language or voice\n"\
			"as well as the list of all available languages or voices.\n"\
			"set language\nset voice\nset languages\nset voices"

#define STRM_HELP	"Create a new current stream. The stream is described\n"\
			"as a colon separated list of interconnected modules.\n"

#define USER_HELP	"Authenticate as a specified user. The authentication stays\n"\
			"incomplete until a valid \"pass\" command is issued.\n"\
			"The control connection is initially in an \"incomplete superuser\"\n"\
			"authentication state. A successful authentication may have\n"\
			"effect on access restrictions, particularly the \"set\" command."
	

#define TTSCP_COMMAND(x,y,s,l) {*(int *)x, &y, s, *(l) ? (l) : "(no help)", },

ttscp_cmd ttscp_cmd_set[] = {
	TTSCP_COMMAND("appl", cmd_apply, "n", APPLY_HELP)
	TTSCP_COMMAND("set",  cmd_set, "parameter value", SET_HELP)
	TTSCP_COMMAND("show", cmd_show, "parameter", SHOW_HELP)
	TTSCP_COMMAND("strm", cmd_stream, "stream", STRM_HELP)
	TTSCP_COMMAND("data", cmd_data, "", DATA_HELP)
	TTSCP_COMMAND("delh", cmd_delhandle, "handle", "Terminate a specified data connection.\n")
	TTSCP_COMMAND("pass", cmd_pass, "password", "See \"help user\"")
	TTSCP_COMMAND("user", cmd_user, "username", USER_HELP)
	TTSCP_COMMAND("done", cmd_done, "", "Terminate this control connection.")
	TTSCP_COMMAND("brk",  cmd_break, "", "Cancel all running appl commands.")
//	TTSCP_COMMAND("say",  cmd_say, "text", "")
//	TTSCP_COMMAND("xscr", cmd_trans, "", "Transcribe the text and return it in the control connection.")
//	TTSCP_COMMAND("trns", cmd_trans, "", "")
	TTSCP_COMMAND("help", cmd_help, "[command]", "")
	TTSCP_COMMAND("down", cmd_shutdown, "", "Shutdown the daemon.")
	{0, NULL, "", ""}
};
