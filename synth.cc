/*
 *	ss/src/synth.cc
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
#include "lpcsyn.h"

synth *setup_synth(voice *v)		//FIXME
{
//	DEBUG(3,9,fprintf(stddbg, "Setting up syntheses\n");)
//	if (syn[S_KTD]) delete syn[S_KTD];
//	syn[S_KTD] = new ktdsyn(cfg->ktd_pitch, cfg->ktd_speed);
/*		dsynth *ds = new dsynth(cfg->inv_type, cfg->inv_counts,
			cfg->inv_models, cfg->inv_book, cfg->inv_dpt,
			cfg->inv_f0, cfg->inv_i0, cfg->inv_t0, cfg->inv_hz);
*/	// FIXME!!!

	if (v->syn) shriek("new v->syn - again");

	switch (v->type) {
		case S_NONE:	shriek("This voice is mute");
		case S_TCP:	shriek("Network voices not implemented");
#ifdef LPC_H
		case S_LPC_FLOAT: return new lpcfloat(v);
		case S_LPC_INT: return new lpcint(v);
		case S_LPC_VQ:	return new lpcvq(v);
#else
		case S_LPC_FLOAT:
		case S_LPC_INT:
		case S_LPC_VQ:	return new voidsyn;
#endif
//		case S_KTD:	return new ktdsyn(cfg->ktd_pitch, cfg->ktd_speed);
		case S_KTD:	return new ktdsyn(v);
		default:	shriek("Impossible synth type");
	}
	return NULL;
}

#define BUFF_SIZE 1024	//every item is a quadruple of ints

void play_diphones(unit *root, voice *v)
{
	static diphone d[BUFF_SIZE]; 
	int i=BUFF_SIZE;
	int j;

	if (!v->syn) v->syn = setup_synth(v);
	if (!cfg->play_diph && !cfg->show_diph) return;
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_diphs(d,k,BUFF_SIZE);
		for (j=0;j<i;j++) {
			d[j].f=this_voice->samp_rate * 100 / (d[j].f*cfg->inv_f0);
		}
		v->attach();
		DEBUG(3,9,fprintf(stddbg,"Using %s synthesis\n", enum2str(this_voice->type, STstr));)
		v->syn->syndiphs(v, d, i);
		v->detach();
	}
}

void show_diphones(unit *root)
{
	static diphone d[BUFF_SIZE]; 
	int i=BUFF_SIZE;
	
	if (!cfg->show_diph) return;
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_diphs(d,k,BUFF_SIZE);
		for (int j=0;j<i;j++) {
			if (cfg->diph_raw) fprintf(stddbg,  "%5d", d[j].code);
			fprintf(stddbg," %.3s f=%d t=%d i=%d\n", d[j].code<441 ? this_voice->diphone_names[d[j].code] : "?!", d[j].f, d[j].t, d[j].e);
		}
	}
}

#undef BUFF_SIZE

synth::synth()
{
	// printf("FIXME synth::synth\n");   but do fix me!
}

synth::~synth()
{
//	printf("FIXME synth::~synth\n"); but do fix me!
}

void
synth::syndiph(voice *v, diphone d)
{
	unuse (v); unuse(d.code);
	shriek("synth::syndiph is abstract\n");
}

void
synth::syndiphs(voice *v, diphone *d, int n)
{
	for (int i=0; i<n; i++) syndiph(v, d[i]);
}


void
voidsyn::syndiph(voice *, diphone)
{
	shriek("Synthesis type absent");
}




//--------------------------------------

/*		dsynth *ds = new dsynth(cfg->inv_type, cfg->inv_counts,
			cfg->inv_models, cfg->inv_book, cfg->inv_dpt,
			cfg->inv_f0, cfg->inv_i0, cfg->inv_t0, cfg->inv_hz);
*/	// FIXME!!!
