/*
 *	epos/src/synth.cc
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
#include "ktdsyn.h"
#include "ptdsyn.h"
#include "lpcsyn.h"
#include "tcpsyn.h"

synth *setup_synth(voice *v)
{
	if (v->syn) shriek(862, "new v->syn - again");

	switch (v->type) {
		case S_NONE:	shriek(813, "This voice is mute");
		case S_TCP:	// shriek(462, "Network voices not implemented");
				return new tcpsyn(v);
#ifdef LPC_H
		case S_LPC_FLOAT: return new lpcfloat(v);
		case S_LPC_INT: return new lpcint(v);
		case S_LPC_VQ:	return new lpcvq(v);
#else
		case S_LPC_FLOAT:
		case S_LPC_INT:
		case S_LPC_VQ:	return new voidsyn;
#endif
		case S_KTD:	return new ktdsyn(v);
		case S_PTD:	return new ptdsyn(v);
		default:	shriek(861, "Impossible synth type");
	}
	return NULL;
}

#if FROBBING_IS_EXTERNAL

void frob_diphones(diphone *d, int n, voice *v)
{
	for (int j=0;j<n;j++) {
//		d[j].f=v->samp_rate * 100 / (d[j].f*cfg->inv_f0);  - fixed 8.7.98 by Petr
		d[j].t=v->init_t * d[j].t / 100;            //corrections for t,f,i by Petr
		d[j].f=v->samp_rate * 100 / (v->init_f * d[j].f);
		d[j].e=v->init_i * d[j].e / 100;
	}
}

#endif

#define BUFF_SIZE 1024	//every item is a quadruple of ints

void play_diphones(unit *root, voice *v)
{
	static diphone d[BUFF_SIZE + 1]; 
	int i=BUFF_SIZE;

	if (!v->syn) v->syn = setup_synth(v);
	if (!cfg->play_diph && !cfg->show_diph) return;
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_diphs(d+1,k,BUFF_SIZE);
		d[0].code = i;
//		frob_diphones(d/*+1*/, i, v);		(moved to synth::syndiphs)
		wavefm w(v);
		w.attach();
		DEBUG(3,9,fprintf(STDDBG,"Using %s synthesis\n", enum2str(this_voice->type, STstr));)
		v->syn->syndiphs(v, d+1, i, &w);
		w.detach();
	}
}

void show_diphones(unit *root)
{
	static diphone d[BUFF_SIZE]; 
	int i=BUFF_SIZE;
	voice *v = this_voice;
	
	if (!cfg->show_diph) return;
	v->claim_diphone_names();
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_diphs(d,k,BUFF_SIZE);
		for (int j=0;j<i;j++) {
			if (cfg->diph_raw) fprintf(STDDBG,  "%5d", d[j].code);
			fprintf(STDDBG," %.3s f=%d t=%d i=%d\n", d[j].code<441 && v->diphone_names ? ((char(*)[4])v->diphone_names->data)[d[j].code] : "?!", d[j].f, d[j].t, d[j].e);
		}
	}
}

#undef BUFF_SIZE

synth::synth()
{
	DEBUG(1,9,fprintf(STDDBG,"A synthesis going to initialise\n");)
}

synth::~synth()
{
	DEBUG(1,9,fprintf(STDDBG,"A synthesis deconstructed\n");)
}

void
synth::syndiph(voice *, diphone, wavefm *)
{
	shriek(861, "synth::syndiph is abstract\n");
}

void
synth::syndiphs(voice *v, diphone *d, int n, wavefm *w)
{
	diphone x;
#ifndef NO_HACKS		// FIXME
	v->claim_diphone_names();
#endif
	for (int i=0; i<n; i++) {
		x.code = d[i].code;
		x.t = v->init_t * d[i].t / 100;            // fixed 8.7.98 by Petr
		x.f = v->samp_rate * 100 / (v->init_f * d[i].f);
		x.e = v->init_i * d[i].e / 100;
#ifndef NO_HACKS		// FIXME
		int hackint = w->offset();
#endif
		syndiph(v, x, w);
#ifndef NO_HACKS
		if (cfg->show_labels) fprintf(STDDBG, "%d %d %.3s\n", hackint, w->offset() - 1,
			(((char(*)[4])v->diphone_names->data)[d[i].code]));
#endif
	}
}

void
voidsyn::syndiph(voice *, diphone, wavefm *)
{
	shriek(813, "Synthesis type absent");
}


