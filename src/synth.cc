/*
 *	epos/src/synth.cc
 *	(c) 1998 geo@cuni.cz
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
//#include "ktdsyn.h"
//#include "ptdsyn.h"
#include "tdpsyn.h"
#include "lpcsyn.h"
#include "mbrsyn.h"
#include "tcpsyn.h"

#define SOUND_LABEL_BASE	(1 << SOUND_LABEL_SHIFT)
#define SOUND_LABEL_SHIFT	10

#ifdef WIN32
#define snprintf _snprintf
#endif

synth *setup_synth(voice *v)
{
	if (v->syn) shriek(862, "new v->syn - again");

	switch (v->type) {
		case S_NONE:	shriek(813, "This voice is mute");
		case S_TCP:	// shriek(462, "Network voices not implemented");
				return new tcpsyn(v);
		case S_LPC_FLOAT: return new lpcfloat(v);
		case S_LPC_INT: return new lpcint(v);
		case S_LPC_VQ:	return new lpcvq(v);
//		case S_KTD:	return new ktdsyn(v);
		case S_TDP:	return new tdpsyn(v);
//		case S_PTD:	return new ptdsyn(v);
		case S_MBROLA:  return new mbrsyn(v);
		default:	shriek(861, "Impossible synth type");
	}
	return NULL;
}

#if FROBBING_IS_EXTERNAL

void frob_segments(segment *d, int n, voice *v)
{
	for (int j=0;j<n;j++) {
		d[j].t=v->init_t * d[j].t / 100;
//		d[j].f=v->samp_rate * 100 / (v->init_f * d[j].f); //fixed 20.9.2002 by Petr
		d[j].f=v->init_f * d[j].f / 100;
		d[j].e=v->init_i * d[j].e / 100;
	}
}

#endif

#define BUFF_SIZE 1024	//every item is a quadruple of ints

void play_segments(unit *root, voice *v)
{
	static segment d[BUFF_SIZE + 1]; 
	int i=BUFF_SIZE;

	if (!v->syn) v->syn = setup_synth(v);
	if (!scfg->play_segs && !scfg->show_segs) return;
	wavefm w(v);
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_segs(d+1,k,BUFF_SIZE);
		d[0].code = i; d[0].nothing = 0; d[0].ll = 0;
//		frob_segments(d/*+1*/, i, v);		(moved to synth::synsegs)
//		w.attach();
		D_PRINT(3, "Using %s synthesis\n", enum2str(this_voice->type, STstr));
		v->syn->synsegs(v, d+1, i, &w);
//		w.detach();
	}
	w.attach(); w.detach();
}

void show_segments(unit *root)
{
	static segment d[BUFF_SIZE]; 
	int i=BUFF_SIZE;
	voice *v = this_voice;
	
	if (!scfg->show_segs) return;
	v->claim_all();
	for (int k=0; i==BUFF_SIZE; k+=BUFF_SIZE) {
		i=root->write_segs(d,k,BUFF_SIZE);
		for (int j=0;j<i;j++) {
			if (scfg->seg_raw) fprintf(STDDBG,  "%5d", d[j].code);
			fprintf(STDDBG," %.3s f=%d t=%d i=%d\n", d[j].code<v->n_segs && v->segment_names ? ((char(*)[4])v->segment_names->data)[d[j].code] : "?!", d[j].f, d[j].t, d[j].e);
		}
	}
}

#undef BUFF_SIZE

synth::synth()
{
	D_PRINT(1, "A synthesis going to initialise\n");
}

synth::~synth()
{
	D_PRINT(1, "A synthesis deconstructed\n");
}

void
synth::synseg(voice *, segment, wavefm *)
{
	shriek(861, "synth::synseg is abstract\n");
}

void
synth::synsegs(voice *v, segment *d, int n, wavefm *w)
{
	segment x;
	v->claim_all();
//	if (cfg->label_seg || cfg->label_phones) {
//		w->label(0, NULL, NULL);
//	}
	for (int i=0; i<n; i++) {
		x.code = d[i].code;
		x.t = v->init_t * d[i].t / 100;
		if ((v->bang_nnet) && (v->lpcprosody)) {
		  // excitation value from nnet
		  x.f = d[i].f; // direct value from nnet to be passed
		  // printf ("direct output value %d from nnet to %d!\n", d[i].f, x.f);
		}
		else {
		  // all other methods process the value
		x.f = v->init_f * d[i].f / 100;
		  // yes, this is too complex! make it easier, please
		  x.f = v->init_f - x.f;
		  x.f = x.f * ((double) cfg->pros_factor) / 1000.0;
		  x.f = v->init_f - x.f;
		  // printf ("x.f after %d\n", x.f);
		}
		x.e = v->init_i * d[i].e / 100;
		if (cfg->label_seg || cfg->label_phones) {
			char tmp[7];
			if (cfg->label_seg) {
				// strncpy(tmp, ((char(*)[4])v->segment_names->data)[d[i].code], 3);
				snprintf (tmp, 7, "%d", d[i].code);
				// printf ("%d is segment code\n", d[i].code);
				tmp[7] = 0;
				w->label(0, tmp, enum2str(scfg->segm_level, scfg->unit_levels));
			}
			int oi = w->get_buffer_index();
			synseg(v, x, w);
			int len = (w->get_buffer_index() - oi) / (v->samp_size >> 3);
			if (cfg->label_phones && v->sl[x.code].pos != NO_SOUND_LABEL) {
				tmp[0] = v->sl[x.code].labl;
				tmp[1] = 0;
				int negoffs = (SOUND_LABEL_BASE - v->sl[x.code].pos)
					* len >> SOUND_LABEL_SHIFT;
				int level = cfg->label_sseg ? d[i].ll : scfg->phone_level;
//				FILE *f = fopen("02tmp", "a+"); fputs(enum2str(level, scfg->unit_levels), f); fputs("\n", f); fclose(f);
				w->label(negoffs, tmp, enum2str(level, scfg->unit_levels));
			}
			// label again (this time its the 'offset', if i undestood that right)
			if (cfg->label_f0) {
				w->label (0, "-", "pitch");
			}
		} else 	synseg(v, x, w);
	}
}

void
synth::synssif(voice *v, char *, wavefm *w)
{
	shriek(462, "synth::synssif is abstract");
}

void
voidsyn::synseg(voice *, segment, wavefm *)
{
	shriek(813, "Synthesis type absent");
}


void
voidsyn::synssif(voice *, char *, wavefm *)
{
	shriek(813, "Synthesis type absent");
}

