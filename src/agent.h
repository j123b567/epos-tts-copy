/*
 *	epos/src/agent.h
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

#ifdef HAVE_UNISTD_H			// fork() only
	#include <unistd.h>	// in DOS, fork(){return -1;} is supplied in interf.cc
#endif

#ifdef HAVE_NETINET_IN_H
	#include <netinet/in.h>
#endif

#ifdef HAVE_LINUX_IN_H
	#include <linux/in.h>
#endif

#ifdef HAVE_IO_H
	#include <io.h>
#endif

/*
 *	A context is a set of values for a few global variables. It is possible
 *	to switch the contexts, thus maintaining more simultaneous configurations
 *	(e.g. for unrelated active connections). Every agent has a context;
 *	a stream of connected agents may share a context.
 */

class context
{
   public:
	int uid;

	configuration *cfg;
	lang *this_lang;
	voice *this_voice;

	char *sgets_buff;	/* not touched upon context switch */

	context(int std_in, int std_out);
	~context();
	void enter();
	void leave();
};


/*
 *	An agent is "something that should work on something". Synonyms: task, process.
 *	The methods are in agent.cc; the agents are scheduled in server() in daemon.cc.
 */

class stream;

enum DATA_TYPE {T_NONE, T_ANY, T_TEXT, T_STML, T_UNITS, T_DIPHS, T_WAVEFM};

class agent
{
	friend stream;

	virtual void run() = 0;	/* run until out of input			*/
   protected:
	void *inb;
	void *outb;
	agent *next;
	virtual bool apply(int size);	/* process <size> data from input to output	*/
	virtual void finis();	/* tell the stream apply() has finished	*/
	void schedule();	/* add this agent to the run queue	*/
	void block(int fd);	/* schedule other agents until fd has more data */
	void push(int fd);	/* schedule other agents until fd can absorb more data */
	void relax();		/* free inb and outb properly		*/
	void pass();		/* called when enough outb to be passed along	*/
   public:
	context *c;
	DATA_TYPE in;
	DATA_TYPE out;
	agent(DATA_TYPE typein, DATA_TYPE typeout);
	virtual ~agent();
	virtual void brk();	/* cancel your work to do, forget inb/outb	*/
	void timeslice();	/* switch context and agent::run()	*/
};

/*
 *	A stream is a linked list of agents, one of them being the
 *	stream agent itself. stream->head is an input agent.
 */

class stream : public agent
{
	agent *callbk;	/* variable */
	agent *head;	/* fixed    */
	virtual void run();
	virtual void finis();
   public:
	stream(char *);
	virtual ~stream();
	virtual void brk();
	virtual void apply(agent *ref, int bytes);
	void release_agents();
	bool foreground() {return callbk ? true : false; };
};

class a_protocol : public agent
{
	char *sgets_buff;
	char *buffer;		// FIXME: one buffer should suffice
	virtual void run();
	virtual int run_command(char *) = 0;	// returns: reschedule, terminate or ignore
   public:
	a_protocol();
	virtual ~a_protocol();
	void disconnect();	// destructor, executes delayed. Also cleanup.
};

class a_ttscp : public a_protocol
{
	virtual int run_command(char *);

	a_ttscp *prev;
	a_ttscp *next;
   public:
	a_ttscp(int sd_in, int sd_out);
	virtual ~a_ttscp();
	virtual void brk();

	void brkall();		/* FIXME: should never be used; remove */
};

extern a_ttscp *ctrl_conns;

class a_accept : public agent
{
	virtual void run();
	sockaddr_in ia;
   public:
	a_accept();
	virtual ~a_accept();
};

struct sched_aq
{
	agent *ag;
	sched_aq *next;
	sched_aq *prev;
};

extern sched_aq *sched_head;
extern sched_aq *sched_tail;
extern int runnable_agents;

agent *sched_sel();

extern agent **sleep_table;
extern fd_set block_set;
extern fd_set push_set;
extern int select_fd_max;

inline int my_fork()
{
	if (!cfg->forking) return -1;
	else return fork();
}

void reply(const char *message);
void reply(int code, const char *message);

/* non-blocking sgets: */
int sgets(char *buffer, int space, int sd, char *partbuff);

extern char server_passwd[];

#define PA_NEXT		0
#define PA_DONE		1
#define PA_WAIT		2

struct ttscp_cmd
{
//	char name[4];
	int name;
	int(*impl)(char *param, agent *a);
	char *short_help;
	char *long_help;
};

extern ttscp_cmd ttscp_cmd_set[];
int cmd_bad(char *);

extern hash_table<char, int> *data_conns;

