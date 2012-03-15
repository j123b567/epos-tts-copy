/*
 *	epos/src/mbrsyn.cc
 *	(c) 2000 Jirka Hanika, geo@cuni.cz
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
#include "mbrsyn.h"

#ifdef HAVE_UNISTD_H
	#include <unistd.h>
#endif

#include <sys/types.h>

#ifdef HAVE_SIGNAL_H
	#include <signal.h>
#endif

#define MBROLA_PATH "/home/geo/mbrola/mbrola"
#define INV_PATH "/home/geo/mbrola/cz/cz1"

#define there there_pipe[1]
#define back  back_pipe[0]

mbrsyn::mbrsyn(voice *v)
{
	int p[2];
	int q[2];
	if (pipe(p)) shriek(463, "Failed to pipe mbrola");
	if (pipe(q)) shriek(463, "Failed to pipe mbrola");
	
	int tmp = fork();
	switch (tmp) {
	case -1: shriek(463, "Failed to fork mbrola");
	case 0:	
		if (p[0]) dup2(p[0], 0);
		dup2(q[1], 1);
		if (execl(MBROLA_PATH, MBROLA_PATH, INV_PATH, "-", "-.wav", NULL))
			shriek(463, "Failed to exec mbrola");
		break;
	default:
		there = p[1];
		back = q[0];
		pid = tmp;
		return;
	}
}

mbrsyn::~mbrsyn()
{
	kill(pid, SIGQUIT);
	close(there_pipe[0]);
	close(there_pipe[1]);
	close(back_pipe[0]);
	close(back_pipe[1]);
}

void
mbrsyn::synssif(voice *, char *b, wavefm *w)
{
	char wb[1024];
	int l;
	write(there, b, strlen(b));
	l = read(back, wb, 1024);
	w->become(wb, l);
}
