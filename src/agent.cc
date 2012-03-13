/*
 *	epos/src/agent.cc
 *	(c) 1998-99 geo@ff.cuni.cz
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
#include "navel.h"


#ifdef HAVE_SYS_TIME_H
	#include <sys/time.h>
#endif

#ifdef HAVE_UNIX_H
	#include <unix.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
	#include <sys/socket.h>
#endif

#ifndef HAVE_SOCKLEN_T
	#define socklen_t int
#endif

#ifdef HAVE_WINSOCK2_H
	#include <winsock2.h>
#endif

#include <fcntl.h>
#include <errno.h>

#ifndef O_NONBLOCK
	#define O_NONBLOCK 0
#endif


#define DARK_ERRLOG 2	/* 2 == stderr; for global stdshriek and stddbg output */

agent::agent(DATA_TYPE typein, DATA_TYPE typeout)
{
	in = typein, out = typeout;
	next = NULL, inb = outb = NULL;
	c = NULL;
	DEBUG(1,11,fprintf(STDDBG, "Creating a handler, intype %i, outtype %i\n", typein, typeout);)
}

agent::~agent()
{
//	DEBUG(1,11,fprintf(STDDBG, "Handler deleted.\n");)
}

void
agent::timeslice()
{
	c->enter();
	if (inb || in == T_NONE) {
		run();
	} else {
		/* We may take this path legally after an intr command */
		DEBUG(2,11,fprintf(STDDBG, "No input of type %i; shrugging off\n", in);)
	}
	c->leave();
}

bool
agent::apply(int)
{
	return false;	// except for the input agent, agents can't start a task
}

void
agent::relax()
{
	if (inb) switch (in) {
		case T_NONE:	break;
		case T_INPUT:	free(inb); break;
		case T_STML:	
		case T_TEXT:	free(inb); break;
		case T_UNITS:	delete ((unit *)inb); break;
		case T_DIPHS:	free(inb); break;
		case T_WAVEFM:	delete((wavefm *)inb); break;
	}
	inb = NULL;
	if (outb) outb = NULL, shriek(462, "outb used?!");
}

void
agent::finis()
{
	agent *a = next;
	while (a->next) a = a->next;
	a->finis();
}

void agent::brk()
{
	if (inb) reply("401 user break");
	relax();
	DEBUG(1,11,fprintf(STDDBG, "interrupting an agent, intype %i, outtype %i\n", in, out);)
}

void
agent::pass()
{
	if (!outb) shriek(862, "Nothing to pass");
	if (!next) shriek(862, "Nowhere to pass to");
		// FIXME: if (already next->inb)
	next->inb = outb;
	outb = NULL;
	next->schedule();
}

class a_ascii : public agent
{
	virtual void run();
   public:
	a_ascii() : agent(T_TEXT, T_UNITS) {};
};

void
a_ascii::run()
{
	outb = str2units((char *)inb);
	free(inb);
	inb = NULL;
	pass();
}


class a_stml : public agent
{
	virtual void run();
   public:
	a_stml() : agent(T_STML, T_UNITS) {};
};

void
a_stml::run()
{
	shriek(462, "STML parser not available");
}


class a_rules : public agent
{
	virtual void run();
   public:
	a_rules() : agent(T_UNITS, T_UNITS) {};
};

void
a_rules::run()
{
	outb = inb;
	inb = NULL;
	this_lang->ruleset->apply((unit *)outb);
	pass();
}


class a_print : public agent
{
	virtual void run();
   public:
	a_print() : agent(T_UNITS, T_TEXT) {};
};

#define UGLY_TMP_CONSTANT	65534		// FIXME

void
a_print::run()
{
	char * s = (char *)malloc(UGLY_TMP_CONSTANT + 1);
	*((unit *)inb)->gather(s, s + UGLY_TMP_CONSTANT, true /* (incl. ssegs) */ ) = 0;

	outb = s;
	delete (unit *) inb;
	inb = NULL;
	pass();
}

#undef UGLY_TMP_CONSTANT

class a_diphs : public agent
{
	virtual void run();
	int position;
   public:
	a_diphs() : agent(T_UNITS, T_DIPHS) {position = 0;};
};

void
a_diphs::run()
{
	diphone *d = (diphone *)malloc((cfg->db_size + 1) * sizeof(diphone));
	int n;

	unit *root = *(unit **)&inb;
	n = root->write_diphs(d + 1, position, cfg->db_size);
	position += n;
	if (n < cfg->db_size) {
		delete root;
		inb = NULL;
		position = 0;
	}
	d->code = n;
	DEBUG(1,11,fprintf(STDDBG, "agent diphs generated %d diphones\n", n);)
	outb = d;
	pass();
}


class a_synth : public agent
{
	virtual void run();
   public:
	a_synth() : agent(T_DIPHS, T_WAVEFM) {};
};

void
a_synth::run()
{
	if (!this_voice) shriek(861, "No current voice");
	if (!this_voice->syn) {
		try {
			this_voice->syn = setup_synth(this_voice);
		} catch (command_failed *e) {
			if (e->code / 10 == 47 && this_lang->fallback_voice) {	// FIXME: ugly
				voice_switch(this_lang->fallback_voice);
				delete e;
				run();
				return;
			} else throw e;
		}
	}
	wavefm *wfm = new wavefm(this_voice);
	this_voice->syn->syndiphs(this_voice, (diphone *)inb + 1, ((diphone*)inb)->code, wfm);
	DEBUG(1,11,fprintf(STDDBG, "a_synth processes %d diphones\n", ((diphone *)inb)->code);)

	outb = wfm;
	free(inb);
	inb = NULL;
	pass();
}

/******** not finished, misconcepted

class a_label : public agent
{
	virtual void run();
   public:
	a_label() : agent(T_DIPHS, T_TEXT) {};
};

#define UGLY_TMP_CONSTANT	65534		// FIXME

void
a_label::run()
{
	char * s = (char *)malloc(UGLY_TMP_CONSTANT + 1);
	diphone *d = ((diphone *)inb);
	int offs = 0;
	for (int i = 1; i < d->code; i++) {
		offs += sprintf(s + offs, "%d %d %s\n",,,);
		if (offs > UGLY_TMP_CONSTANT - 32) shriek(461, "Out of buffer space");
	}

	outb = s;
	delete (unit *) inb;
	inb = NULL;
	pass();
}

******/

template <DATA_TYPE TYPE> class a_type : public agent
{
	virtual void run();
   public:
	a_type() : agent(TYPE, TYPE) {};
};

template <DATA_TYPE TYPE> void
a_type<TYPE>::run()
{
	outb = inb;
	inb = NULL;
	pass();
}

class a_io : public agent
{
	virtual void run() = 0;
   protected:
	int socket;
	a_ttscp *dc;
//	bool close_upon_exit;
   public:
	a_io(const char *, DATA_TYPE, DATA_TYPE);
	virtual ~a_io();
};

a_io::a_io(const char *par, DATA_TYPE in, DATA_TYPE out) : agent(in, out)
{
//	a_ttscp *tmp;
	char *filename;

//	close_upon_exit = false;
	dc = NULL;

	switch(*par) {
		case '$': dc = data_conns->translate(par + 1);
			  if (!dc) shriek(444, "Not a known data connection handle");
			  else socket = (in == T_INPUT ? dc->c->config->sd_in : dc->c->config->sd_out);
			  break;
		case '/': filename = limit_pathname(par, cfg->pseudo_root_dir);
			  socket = open(filename, in == T_INPUT ? O_RDONLY | O_NONBLOCK
						: O_WRONLY | O_CREAT | O_TRUNC | O_NONBLOCK);
			  free(filename);
			  if (socket == -1) shriek(445, fmt("Cannot open file %s", par));
//			  else close_upon_exit = true;
			  break;
		default:  shriek(462, "unimplemented i/o agent class");
			/* if ever adding classes, take care of closing/nonclosing
			 * the socket upon exit        */
	}
	DEBUG(0,11,fprintf(STDDBG, "I/O agent is %p\n", this);)
}

a_io::~a_io()
{
	DEBUG(0,11,fprintf(STDDBG, "~a_io\n");)
//	if (close_upon_exit)
	if (!dc) async_close(socket);
}

class a_input : public a_io
{
	int toread;
	int offset;

	virtual void run();
   protected:
	virtual bool apply(int size);
   public:
	a_input(const char *);
};

a_input::a_input(const char *par) : a_io(par, T_INPUT, T_TEXT)
{
}


void a_input::run()
{
	int res;
	DEBUG(0,11,fprintf(STDDBG, "Entering input agent\n");)
	res = yread(socket, (char *)inb + offset, toread - offset);
	if (res <= 0) {
		if (!dc) {
			if (res == 0) shriek(438, "end of file");
			else shriek(437, "read error");
		}
		if (dc->ctrl) dc->ctrl->deps->remove(dc->handle);
		c->leave();
		delete data_conns->remove(dc->handle);
		c->enter();
		shriek(436, fmt("data conn %d lost", socket));
	}
	offset += res;
	if (offset == toread) {
		switch (out) {
			case T_DIPHS:
				if ((((diphone *)inb)->code + 1) * (int)sizeof(diphone) != offset)
					shriek(432, fmt("Received bad diphones: %d diphs, %d bytes",
						((diphone *)inb)->code, offset));
				break;
			case T_WAVEFM:
				wavefm *w;
				w = new wavefm(this_voice);
				w->become(inb, offset);
				free(inb);
				outb = w;
				inb = NULL; toread = offset = 0;
				pass();
				return;
			case T_TEXT:
			case T_STML:
			case T_UNITS:
			default: ;	/* otherwise no problem, FIXME: shorten */
		}
		outb = inb;
		((char *)outb)[offset] = 0;
		DEBUG(2,11,fprintf(STDDBG, "Read and about to process %s\n", (char *)outb);)
		inb = NULL;
		toread = 0;	// superfluous
		offset = 0;	// superfluous
		pass();
	} else block(socket);
}

bool
a_input::apply(int size)
{
	DEBUG(2,11,fprintf(STDDBG, "%d bytes to be read\n", size);)
	if (inb) return false;	// busy
	toread = size;
	inb = malloc(size + 1);
	offset = 0;
//	schedule();
	block(socket);
	DEBUG(1,11,fprintf(STDDBG, "Apply task has been scheduled\n");)
	return true;
}


class a_output : public a_io
{
	virtual int insize() = 0;
	virtual void run();
	bool foreground() {return ((stream *)next)->foreground(); };
	int written;
   protected:
	void report(bool total, int written);
   public:
	a_output(const char *par, DATA_TYPE i) : a_io(par, i, T_NONE) {written = 0;};
};

void
a_output::run()
{
	int size = insize();
	if (written) {
		int now_written = ywrite(socket, (char *)inb + written, size - written);
		report(false, now_written);
		written += now_written;
	} else {
		written = ywrite(socket, (char *)inb, size);
		if (written) report(true, size);
	}
	if (written == size) {
		written = 0;
		reply("200 output OK");
		relax();
		finis();
	} else push(socket);
}

void
a_output::report(bool total, int written)
{
//	inb = NULL;
	if (foreground()) {
		reply(total ? "122 total bytes" : "123 written bytes");
		sprintf(scratch, " %d", written);
		reply(scratch);
	}
}

class oa_ascii : public a_output
{
	virtual int insize() {
		return strlen((char *)inb);
	}
   public:
	oa_ascii(const char *s): a_output(s, T_TEXT) {};
};

class oa_stml : public a_output
{
	virtual int insize() {
		return strlen((char *)inb);
	}
   public:
	oa_stml(const char *s): a_output(s, T_STML) {};
};

class oa_diph : public a_output
{
	virtual int insize() {
		DEBUG(1,11,fprintf(STDDBG, "Sending %d diphones\n", ((diphone *)inb)->code);)
		return ((diphone *)inb)->code * sizeof(diphone);
	}
   public:
	oa_diph(const char *s): a_output(s, T_DIPHS) {};
};

class oa_wavefm : public a_output
{
	virtual int insize() {
		shriek(462, "abstract oa_wavefm::insize"); return 0;
	}
	virtual void run();
	virtual void brk();
	bool attached;
   public:
	oa_wavefm(const char *s): a_output(s, T_WAVEFM) {attached = false;};
};

void
oa_wavefm::run()
{
	wavefm *w = (wavefm *)inb;
	if (!attached) {
		report(true, w->written_bytes() + sizeof(wave_header));
		w->attach(socket);
		report(false, w->written + sizeof(wave_header));
		attached = true;
	}
	if (w->flush()) {
		if (w->written) report(false, w->written);
		push(socket);
	} else {
		if (w->written) report(false, w->written);
		w->detach(socket);
		DEBUG(1,11,fprintf(STDDBG, "oa_wavefm wrote %d bytes\n, ", w->written_bytes());)
		attached = false;
		delete w;
		inb = NULL;
		reply("200 output OK");
		finis();
	}
}

void
oa_wavefm::brk()
{
	if (inb) {

		wavefm *w = (wavefm *)inb;
		
		report(false, w->written_bytes());
		w->brk();
		if (attached) w->detach(socket);
		DEBUG(1,11,fprintf(STDDBG, "oa_wavefm wrote %d bytes\n, ", w->written_bytes());)
		attached = false;
		delete w;
		inb = NULL;
		reply("401 user break");
	}
	finis();
}


/*
 *	A stream is a linked list of agents, one of them being the
 *	stream agent itself. stream->head is an input agent.
 */

enum agent_type {AT_UNKNOWN, AT_ASCII, AT_DIPHS, AT_PRINT, AT_RULES, AT_STML, AT_SYNTH,
		 AT_T_TEXT, AT_T_STML, AT_T_UNITS, AT_T_DIPHS, AT_T_WAVEFM};
const char *agent_type_str = ":raw:diphs:print:rules:stml:synth:[t]:[s]:[i]:[d]:[w]:";

agent *make_agent(char *s, agent *preceding)
{
	if (strchr("@#/.$", *s)) {
		if (!preceding) return new a_input(s);
		switch (preceding->out) {
			case T_TEXT:   return new oa_ascii(s);
			case T_STML:   return new oa_stml(s);
			case T_UNITS: shriek(448, "Units are hard to output");
			case T_DIPHS:  return new oa_diph(s);
			case T_WAVEFM: return new oa_wavefm(s);
			default: shriek(462, "unimplmd oa");
		}
	}
	switch ((agent_type)str2enum(s, agent_type_str, AT_UNKNOWN))
	{
		case AT_UNKNOWN: shriek(861, "Agent type bug.");
		case AT_ASCII: return new a_ascii;
		case AT_DIPHS: return new a_diphs;
		case AT_PRINT: return new a_print;
		case AT_RULES: return new a_rules;
		case AT_STML:  return new a_stml;
		case AT_SYNTH: return new a_synth;

		case AT_T_TEXT:  return new a_type<T_TEXT>;
		case AT_T_STML:  return new a_type<T_STML>;
		case AT_T_UNITS: return new a_type<T_UNITS>;
		case AT_T_DIPHS: return new a_type<T_DIPHS>;
		case AT_T_WAVEFM:return new a_type<T_WAVEFM>;

		default:       shriek(415, "Unknown agent type"); return NULL;
	}
}


stream::stream(char *s, context *pc) : agent(T_NONE, T_NONE)
{
	char *tmp;
	agent *a;
	agent *l = head = NULL;

	callbk = NULL;
//	c = new context(cfg->sd_in, cfg->sd_out);	// a little redundant
	c = pc;
//	navelcord<context> ncc(c);

	tmp = strchr(s, LIST_DELIM);
	if (!tmp) shriek(415, "Bad stream syntax");

	do {
		*tmp = 0;
		DEBUG(1,11,fprintf(STDDBG, "Making agent out of %s\n", s);)
		try {
			a = make_agent(s, NULL); a->c = c;
		} catch (command_failed *e) {
			release_agents();
			throw e;
		}
		*tmp = LIST_DELIM;
		s = ++tmp;
		if (!l) head = a;
		else l->next = a;
		l = a;
	} while((tmp = strchr(s, LIST_DELIM)));
	a = make_agent(s, a); a->c = c;
	l->next = a;
	a->next = this;
	if (head->next != this) head->out = head->next->in;	/* adjust a_input type */
//	ncc.release();
}

stream::~stream()
{
	release_agents();
//	delete c;
}

void
stream::release_agents()
{
	for (agent *a = head; a && a != this; ) {
		agent *b = a;
		a = a->next;
		delete b;
	}
}

void
stream::apply(agent *ref, int bytes)
{
	DEBUG(2,11,fprintf(STDDBG, "In stream::apply %p %p %d\n", head, ref, bytes);)
	callbk = ref;
	head->apply(bytes);
}

void
stream::brk()
{
	if (!callbk) return;	/* break only if running */
	for (agent *a = head; a && a != this; a = a->next)
		a->brk();
}

void
stream::run()
{
	shriek(861, "scheduled a stream");
}

void
stream::finis()
{
	if (callbk) callbk->schedule();
	else shriek(862, "no callback");
	callbk = NULL;
}

class a_disconnector : public agent
{
	virtual void run();
	a_protocol **to_delete;
	int last;
	int max;
   public:
	void disconnect(a_protocol *);
	a_disconnector();
	virtual ~a_disconnector();
};

a_disconnector::a_disconnector() : agent(T_NONE, T_NONE)
{
	to_delete = (a_protocol **)malloc(sizeof(void *));
	last = 0;
	max = 1;
}

a_disconnector::~a_disconnector()
{
	if (last) shriek(861, "Forgot to disconnect a protocol agent!");
	free(to_delete);
}

void a_disconnector::run()
{
	if (!last) shriek(861, "Spurious disconnect");
	delete to_delete[--last];
	to_delete[last] = NULL;
}

void a_disconnector::disconnect(a_protocol *moriturus)
{
	if (last == max && ! (max & max - 1)) {
		max <<= 1;
		if (max > 8) shriek(861, "Too many concurrent disconnections"); // FIXME
		to_delete = (a_protocol **)realloc(to_delete, max * sizeof(void *));
	}
	to_delete[last++] = moriturus;
	schedule();
}

a_disconnector disconnector;

a_protocol::a_protocol() : agent(T_NONE, T_NONE)
{
	sgets_buff = (char *)malloc(cfg->max_net_cmd + 2);
	*sgets_buff = 0;
	buffer = (char *)malloc(cfg->max_net_cmd + 2);
}

a_protocol::~a_protocol()
{
	free(sgets_buff);
	free(buffer);
}

void a_protocol::run()
{
	int res;
	res = sgets(buffer, cfg->max_net_cmd, cfg->sd_in, sgets_buff);
	if (res < 0) {
		disconnect();
		return;
	}

	if ((int)strlen(buffer) >= cfg->max_net_cmd)
		shriek(413, "Received command is too looong");
	if (res > 0 && *buffer) switch (run_command(buffer)) {
		case PA_NEXT:
			DEBUG(0,11,fprintf(STDDBG, "PA_NEXT\n");)
			if (strchr(sgets_buff, '\n')) schedule();
			else block(cfg->sd_in);
			return;
		case PA_DONE:
			DEBUG(0,11,fprintf(STDDBG, "PA_DONE\n");)
			disconnect();
			return;
		case PA_WAIT:
			DEBUG(0,11,fprintf(STDDBG, "PA_WAIT\n");)
			return;
		default:
			shriek(861, "Bad protocol action\n");
	}
	block(cfg->sd_in);		/* partial line read */

//	leave_context(i);

//	non-blocking get_line etc.
}

a_ttscp::a_ttscp(int sd_in, int sd_out) : a_protocol()
{
	c = new context(sd_in, sd_out);
	c->enter();
//	cow_claim();	//FIXME: rethink 
	
	do make_rnd_passwd(handle, HANDLE_SIZE);
		while (data_conns->translate(handle));
	ctrl_conns->add(handle, this);

	sputs(
		"TTSCP spoken here\r\n"
		"protocol: 0\r\n"
		"extensions:\r\n"
		"server: Epos\r\n"
		"release: " VERSION "\r\n"
		"handle: ", cfg->sd_out);
	reply(handle);
	reply("");
	ctrl = NULL;
	deps = new hash_table<char, a_ttscp>(4);
	deps->dupdata = deps->dupkey = false;
	c->leave();
	block(sd_in);
}

/*
 *	Warning: the following destructor runs in the master context;
 *	therefore, be careful with using cfg etc.
 */

//static void kill_my_data_conns(char *, a_ttscp *a, void *me)
//{
//	if (a->ctrl == me) {
//		disconnector.disconnect(a);
//	}
//}

a_ttscp::~a_ttscp()
{
	c->enter();
	if (cfg->current_stream) delete cfg->current_stream;
	cfg->current_stream = NULL;
	DEBUG(2,11,fprintf(STDDBG, "deleted context closes fd %d and %d\n", cfg->sd_in, cfg->sd_out);)
	c->leave();
	while (deps->items) {
		a_ttscp *tmp = deps->translate(deps->get_random());
		deps->remove(tmp->handle);
		delete data_conns->remove(tmp->handle);
	}
	delete deps;
	c->enter();
	if (cfg->sd_in != -1)
		async_close(cfg->sd_in);
	if (cfg->sd_out != -1 && cfg->sd_out != cfg->sd_in)
		async_close(cfg->sd_out);
	if (data_conns->translate(handle) || ctrl_conns->translate(handle))
		shriek(862, "Forgot to forget a_ttscp");
	c->leave();
	delete c;
	// close the descriptor? no, the ~context does that
}

void
a_ttscp::brk()
{
	if (c->config->current_stream)
		c->config->current_stream->brk();
}

/*

void
a_ttscp::brkall()		// FIXME: replace this method
{
	c->enter();
	brk();
	c->leave();
	if (next) next->brkall();
}

*/

int
a_ttscp::run_command(char *cmd)
{
	char *keyword;
	char *param;

	DEBUG(2,11,fprintf(STDDBG, "[ cmd] %s\n", cmd);)

	keyword = cmd + strspn (cmd, WHITESPACE);
	param = keyword + strcspn(keyword, WHITESPACE);
	if (param - keyword > 4) goto bad;	/* all cmds are 3 or 4 chars */

	if (!*param) param = NULL;
	else *param++ = 0;

	if (param) param += strspn(param, WHITESPACE);

	int i;
	for (i=0; ttscp_cmd_set[i].name &&
		(*(const int *)keyword != *(const int *)&ttscp_cmd_set[i].name);)
			i++;
	if (!ttscp_cmd_set[i].name) goto bad;
	
	if (!param && ttscp_cmd_set[i].param == PAR_REQ) {
		reply("417 parameter missing");
		return PA_NEXT;
	}
	if (param && ttscp_cmd_set[i].param == PAR_FORBIDDEN) {
		reply("416 parameter not allowed");
		return PA_NEXT;
	}

	try {
		return ttscp_cmd_set[i].impl(param, this);
	} catch (command_failed *e) {
		DEBUG(2,11,fprintf(STDDBG, "Command failed, %d, %.60s\n", e->code, e->msg);)
		reply(e->code, e->msg);
		delete e;
		return PA_NEXT;
	} catch (connection_lost *e) {
		DEBUG(2,11,fprintf(STDDBG, "Releasing a TTSCP control connection, %d, %.60s\n", e->code, e->msg);)
		reply(e->code, e->msg);		/* FIXME: doesn't make any sense, does it? */
		reply(201, fmt("debug %d", cfg->sd_in));
		delete e;
		return PA_DONE;
	}

   bad:
	cmd_bad(cmd);
	return PA_NEXT;
}

void
a_ttscp::disconnect()
{
	DEBUG(2,11,fprintf(STDDBG, "ctrl conn %d lost\n", cfg->sd_in);)
	if (this != ctrl_conns->remove(handle) /* && this != data_conns->remove(handle) */ )
		shriek(862, "Failed to disconnect a ctrl connection");
	disconnector.disconnect(this);
}

a_accept::a_accept() : agent(T_NONE, T_NONE)
{
//	static socklen_t sia = sizeof(sockaddr);	// __QNX__ wants signed :-(
	static sockaddr_in sa;
//	static sockaddr_in ia;
	
	char one = 1;

	c = new context(-1, /*** dark errors ***/ DARK_ERRLOG);	//FIXME
	c->enter();

	cfg->sd_in = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	memset(&sa, 0, sizeof(sa));
	gethostname(scratch, cfg->scratch - 1);
	sa.sin_family = AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	sa.sin_port = htons(cfg->listen_port);
	setsockopt(cfg->sd_in, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	DEBUG(2,11,fprintf(STDDBG, "[core] Trying to bind...\n");)
	if (bind(cfg->sd_in, (sockaddr *)&sa, sizeof (sa))) shriek(871, "Could not bind");
	if (listen(cfg->sd_in, 64)) shriek(871, "Could not listen");

	ia.sin_family = AF_INET;
	ia.sin_addr.s_addr = htonl(INADDR_ANY);
	ia.sin_port = 0;

	block(cfg->sd_in);
	c->leave();
}

a_accept::~a_accept()
{
	close (cfg->sd_in);
	delete c;
}

void
a_accept::run()
{
	static socklen_t sia = sizeof(sockaddr);	// Will __QNX__ complain?
	int f = accept(cfg->sd_in, (sockaddr *)&ia, &sia);
	if (f == -1) shriek(871, "Cannot accept() - network problem");
#ifdef HAVE_WINSOCK2_H
	ioctlsocket((unsigned long)f, FIONBIO, (unsigned long *)&sia);	// &sia is a dummy non-NULL pointer
#else
	fcntl(f, F_SETFL, O_NONBLOCK);
#endif
	DEBUG(2,11,fprintf(STDDBG, "[core] Accepted %d (on %d).\n", f, cfg->sd_in);)
	c->leave();
	unuse(new a_ttscp(f, f));
	c->enter();
	block(cfg->sd_in);
}

sched_aq *sched_head = NULL;
sched_aq *sched_tail = NULL;

int runnable_agents = 0;

void
agent::schedule()
{
	if (!this) shriek(862, "scheduling garbage");
	runnable_agents++;
	sched_aq *tmp = new sched_aq;
	tmp->ag = this;
	tmp->prev = NULL;
	tmp->next = sched_head;
	if (sched_head) sched_head->prev = tmp;
	else sched_tail = tmp;
	sched_head = tmp;
}

agent *sched_sel()
{
	agent *r;
	sched_aq *tmp;
	if (!sched_tail) shriek(862, "agent queue empty");
	if (!sched_tail->prev) sched_head = NULL;
	else sched_tail->prev->next = NULL;
	r = sched_tail->ag;
	tmp = sched_tail;
	sched_tail = sched_tail->prev;
	delete tmp;
	runnable_agents--;
	DEBUG(1,11,fprintf(STDDBG, "Running a queued agent\n");)
	return r;
}

agent **sleep_table = (agent **)malloc(1);
fd_set block_set;
fd_set push_set;
int select_fd_max = 0;

/*
 *	agent::run() should return after calling block() or push()
 */

void
agent::block(int fd)
{
	DEBUG(1,11,fprintf(STDDBG, "Sleeping on %d\n", fd);)
	if (select_fd_max <= fd) {
		sleep_table = (agent **)realloc(sleep_table, (fd + 1) * sizeof(agent *));
		for ( ; select_fd_max <= fd; select_fd_max++)
			sleep_table[select_fd_max] = NULL;
	}
	if (sleep_table[fd]) shriek(861, fmt("%ssleeping on %d", this == sleep_table[fd]
			? "Re" : "Cross", fd));
	sleep_table[fd] = this;
	FD_SET(fd, &block_set);
	return;
}

void
agent::push(int fd)
{
	DEBUG(1,11,fprintf(STDDBG, "Pushing on %d\n", fd);)
	if (select_fd_max <= fd) {
		sleep_table = (agent **)realloc(sleep_table, (fd + 1) * sizeof(agent *));
		for ( ; select_fd_max <= fd; select_fd_max++)
			sleep_table[select_fd_max] = NULL;
	}
	if (sleep_table[fd]) shriek(861, fmt("%ssleeping on %d", this == sleep_table[fd]
			? "Re" : "Cross", fd));
	sleep_table[fd] = this;
	FD_SET(fd, &push_set);
	return;
}

void free_sleep_table()
{
	free(sleep_table);
}
