/*
 * 	epos/src/tdpsyn.cc
 * 	(c) 2000-2001 Petr Horak, petr.horak@click.cz
 * 	(c) 2001 Jirka Hanika, geo@cuni.cz
 *
 *	tdpsyn version 2.1 (8.11.2001)
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
#include "tdpsyn.h"
#include <math.h>

#define MAX_STRETCH	30	/* stretching beyond 30 samples per OLA frame must be done through repeating frames */
#define MAX_OLA_FRAME	1024	/* sanity check only */
#define HAMMING_PRECISION 15	/* hamming coefficient precision in bits */

int hamkoe(int winlen, unsigned short *data, int e, int e_base)
{
	int i;
	double fn;
	fn = 2 * pii / (winlen - 1);
	for (i=0; i < winlen; i++) {
		data[i] = (unsigned short)((0.53999 - 0.46 * cos(fn * i)) * e / e_base * (1 << HAMMING_PRECISION));
	}
	return 0;
}


#if 0

int hamkoe(int winlen, double *data)
{
//	printf("%d\n", winlen);

	int i;
	double fn;
	fn = 2 * pii / (winlen - 1);
	for (i=0; i < winlen; i++)
		data[i] = 0.54 - 0.46 * cos(fn * i);
	return 0;
}

int median(int prev, int curr, int next, int ibonus)
{
	int lm;
	if ((prev <= curr) == (curr <= next)) lm = curr;
	else if ((prev <= curr) == (next <= prev)) lm = prev;
	else lm = next;
	return (abs(curr - lm) > ibonus) ? lm : curr;
}

#endif


struct tdi_hdr
{
	int magic;
	int samp_rate;
	int samp_size;
	int bufpos;
	int n_segs;
	int diph_offs;
	int diph_len;
	int res1;
	int res2;
	int ppulses;
	int res3;
	int res4;
};

tdpsyn::tdpsyn(voice *v)
{
	tdi_hdr *hdr;

	difpos = 0;

	tdi = claim(v->models, v->loc, cfg->inv_base_dir, "rb", "inventory", NULL);
	hdr = (tdi_hdr *)tdi->data;
	if (v->n_segs != hdr->n_segs) shriek(463, "inconsistent n_segs");
	if (sizeof(t_samp) != hdr->samp_size) shriek(463, "inconsistent samp_size");
	tdp_buff = (t_samp *)(hdr + 1);
	diph_offs = (int *)((char *)tdp_buff + sizeof(t_samp) * hdr->bufpos);
	diph_len = diph_offs + v->n_segs;
	ppulses = diph_len + v->n_segs;

	/* allocate the maximum necessary space for Hamming windows: */	
	max_frame = 0;
	for (int k = 0; k < v->n_segs; k++) {
		int avpitch = average_pitch(diph_offs[k], diph_len[k]);
		int maxwin = avpitch + MAX_STRETCH; //(int)(w->samp_rate / 500);
		if (max_frame < maxwin) max_frame = maxwin;
	}
	max_frame++;
	if (max_frame >= MAX_OLA_FRAME || max_frame == 0) shriek(463, "Inconsistent OLA frame buffer size");
	wwin = (unsigned short *)xmalloc(sizeof(unsigned short) * (max_frame * 2));
	memset(wwin, 0, (max_frame * 2) * sizeof(*wwin));

	out_buff = (t_samp *)xmalloc(sizeof(t_samp) * max_frame * 2);
	memset(out_buff, 0, max_frame * 2 * sizeof(*out_buff));
}

tdpsyn::~tdpsyn(void)
{
	free(out_buff);
	free(wwin);
	unclaim(tdi);
}

inline int tdpsyn::average_pitch(int offs, int len)
{
	const int npitch = 145;
	int tmp;

	int total = 0;
	int i = 0;
	for (int j = 0; j <= len + 1; j++) {
		tmp = ppulses[offs + j] - ppulses[offs + j - 1];
		if (tmp < 2 * npitch) {
			total += tmp;
			i++;
		}
	}
	if (i <= 0) {
		if (cfg->paranoid) shriek(463,"pitch marks not found");
		return 160;
	}
	return total / i;
}

void tdpsyn::synseg(voice *, segment d, wavefm *w)
{
	int i, j, pitch, avpitch, pitchlen, origlen, newlen, maxwin, step, diflen;
	t_samp poms;
	
	const int max_frame = this->max_frame;

	if (diph_len[d.code] == 0) {
		DEBUG(2,9, fprintf(STDDBG, "missing diphone: %d\n", d.code);)
		if (!cfg->paranoid) return;
		shriek(463, fmt("missing diphone: %d\n",d.code));
	}

	avpitch = average_pitch(diph_offs[d.code], diph_len[d.code]);
	maxwin = avpitch + MAX_STRETCH; //(int)(w->samp_rate / 500);
	pitch = d.f;
	maxwin = (pitch > maxwin) ? maxwin : pitch;
	if (maxwin >= max_frame) shriek(461, "pitch too large");

	origlen = avpitch * diph_len[d.code];
	pitchlen = pitch * diph_len[d.code];
	newlen = origlen * 100 / d.t;
	diflen = (newlen - origlen) / diph_len[d.code] % pitch;
//	printf("\navp=%d oril=%d newl=%d difl=%d| l:%d t:%f(%f) e:%f\n",avpitch,origlen,newlen,diflen,pitch,tim_k,(double)d.t/100,ene_k);
//	printf("\navp=%d L=%d oril=%d pil=%d tk:%f newl=%d difl=%d| %d (%d)\n",avpitch,pitch,origlen,pitchlen,tim_k,newlen,diflen,diph_len[d.code],d.code);

	hamkoe(2 * maxwin + 1, wwin, d.e, 100);
//	step = 1;
	step = abs((newlen - origlen) / diph_len[d.code] / pitch) + 1;
//	diflen %= pitch;
//	while (diflen > pitch) { step++; diflen -= pitch; }	// !!
//	while (-diflen > pitch) { step++; diflen += pitch; }	// !!
//	if (step != step2) shriek(999, fmt("steps %d %d", step, step2));
	for (j = 1; j <= diph_len[d.code]; j += step) {
		memcpy(out_buff + max_frame - pitch, out_buff + max_frame, pitch * sizeof(*out_buff));
		memset(out_buff + max_frame, 0, max_frame * sizeof(*out_buff));
		for (i = -maxwin;i <= maxwin; i++) {
			poms = tdp_buff[i + ppulses[diph_offs[d.code] + j - 1]];
			poms = (t_samp)(wwin[i + pitch] * poms >> HAMMING_PRECISION);
//			poms = poms * d.e / 100;
			out_buff[max_frame + i] += poms;
		}
//		for (i = max_frame - pitch; i < max_frame; i++) w->sample(out_buff[i]);
		w->sample((SAMPLE *)out_buff + max_frame - pitch, pitch);
		difpos += diflen;
		if (difpos < -pitch) { j--; difpos += pitch; }
		if (difpos > pitch) { j++; difpos -= pitch; }
	}
}
