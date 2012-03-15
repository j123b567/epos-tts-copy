/*
 * 	epos/src/tdpsyn.cc
 * 	(c) 2000-2002 Petr Horak, horak@petr.cz
 * 	(c) 2001-2002 Jirka Hanika, geo@cuni.cz
 *
 *	tdpsyn version 2.3.3 (25.4.2002)
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
#define MAX_OLA_FRAME	4096	/* sanity check only */
#define HAMMING_PRECISION 15	/* hamming coefficient precision in bits */
#define LP_F0_STEP 8			/* step of F0 analysis for linear prediction */
#define LP_DECIM 10				/* F0 analysis decimation coeff. for linear prediction */
#define LP_F0_ORD 4				/* order of F0 contour LP analysis */
#define F0_FILT_ORD 9			/* F0 contour filter order */
#define LP_EXC_MUL 1.33			/* LP excitation multipicator */

/* F0 contour filter coefficients */
const double a[9] = {1,-6.46921563821389,18.43727805607084,-30.21344177474595,31.11962012720199,
					-20.62061607537661,8.58111044795433,-2.04983423923570,0.21516477151414};

const double b[9] = {0.01477848982115,-0.08930749676388,0.25181616063735,-0.43840842338467,0.52230821454923,
					-0.43840842338467,0.25181616063735,-0.08930749676388,0.01477848982115};

/* lp f0 contour filter coefficients (mean of 144 sentences from speaker Machac) */
const double lp[LP_F0_ORD] = {-1.23761, 0.60009, -0.32046, 0.10699};
// FIXME! lp coefficients must be configurable

/* Hamming coefficients for TD-PSOLA algorithm */
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


/* Inventory file header structure */
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
//	max_frame = 0;
//	for (int k = 0; k < v->n_segs; k++) {
//		int avpitch = average_pitch(diph_offs[k], diph_len[k]);
//		int maxwin = avpitch + MAX_STRETCH; //(int)(w->samp_rate / 500);
//		if (max_frame < maxwin) max_frame = maxwin;
//	}
//	max_frame++;
//	if (max_frame >= MAX_OLA_FRAME || max_frame == 0) shriek(463, "Inconsistent OLA frame buffer size");
	//FIXME! max_frame=maxwin * (max_L - min_ori_L)

	max_frame = MAX_OLA_FRAME;
	
	wwin = (unsigned short *)xmalloc(sizeof(unsigned short) * (max_frame * 2));
	memset(wwin, 0, (max_frame * 2) * sizeof(*wwin));

	out_buff = (t_samp *)xmalloc(sizeof(t_samp) * max_frame * 2);
	memset(out_buff, 0, max_frame * 2 * sizeof(*out_buff));

	/* initialisation of lp prosody engine */
	if (v->lpcprosody) {
		int i;
		for (i = 0; i < LPC_PROS_ORDER; lpfilt[i++] = 0);
		for (i = 0; i < MAX_OFILT_ORDER; ofilt[i++] = 0);
		sigpos = 0;
		lppstep = LP_F0_STEP * v->samp_rate / 1000;
		lpestep = LP_DECIM * lppstep;
		basef0 = v->init_f;
		lppitch = v->samp_rate / basef0;;
	}
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

void tdpsyn::synseg(voice *v, segment d, wavefm *w)
{
	int i, j, k, l, m, slen, nlen, pitch, avpitch, origlen, newlen, maxwin, skip, reply, diflen;
	double outf0, synf0, exc;
	t_samp poms;
	
	const int max_frame = this->max_frame;

	if (diph_len[d.code] == 0) {
		DEBUG(2,9, fprintf(STDDBG, "missing speech unit No: %d\n", d.code);)
		if (!cfg->paranoid) return;
		shriek(463, fmt("missing speech unit No: %d\n",d.code));
	}

	/* lp prosody reconstruction filter excitation signal computing */
	if (v->lpcprosody) { 	// in d.f is excitation signal value
		exc = LP_EXC_MUL * (100 * v->samp_rate / d.f / v->init_f - 100);
		pitch = lppitch;
	}
	else					// in d.f is f0 contour value
	pitch = d.f;
	slen = diph_len[d.code];
	avpitch = average_pitch(diph_offs[d.code], slen);
	maxwin = avpitch + MAX_STRETCH;
	maxwin = (pitch > maxwin) ? maxwin : pitch;
	if (maxwin >= max_frame) shriek(461, "pitch too large");

	if (d.t > 0) origlen = avpitch * slen * d.t / 100; else origlen = avpitch * slen;
	newlen = pitch * slen;
	//diflen = (newlen - origlen) / slen;
	//printf("\navp=%d L=%d oril=%d newl=%d | %d (%d)\n",avpitch,pitch,origlen,newlen,diph_len[d.code],d.code);
	//printf("unit:%4d f=%3d i=%3d t=%3d - pitch=%d\n",d.code,d.f,d.e,d.t,pitch);

	hamkoe(2 * maxwin + 1, wwin, d.e, 100);
	skip = 1; reply = 1;
	if (newlen > origlen) skip = newlen / origlen;
	if (origlen > newlen) reply = origlen / newlen;
	//printf("dlen=%d p:%d avp=%d oril=%d newl=%d difl=%d",diph_len[d.code],pitch,avpitch,origlen,newlen,diflen);
	nlen = slen - (skip - 1) * slen / skip + (reply - 1) * slen;
	diflen = (newlen - origlen - (skip - 1) * slen * pitch / skip + (reply - 1) * slen * pitch) / nlen;
	//printf(" -> diflen:%d sk:%d rp:%d\n",diflen,skip,reply);
	for (j = 1; j <= diph_len[d.code]; j += skip) for (k = 0; k < reply; k++) {
		memcpy(out_buff + max_frame - pitch, out_buff + max_frame, pitch * sizeof(*out_buff));
		memset(out_buff + max_frame, 0, max_frame * sizeof(*out_buff));
		for (i = -maxwin;i <= maxwin; i++) {
			poms = tdp_buff[i + ppulses[diph_offs[d.code] + j - 1]];
			poms = (t_samp)(wwin[i + pitch] * poms >> HAMMING_PRECISION);
//			poms = poms * d.e / 100;
			out_buff[max_frame + i] += poms;
		}

		/* lpc synthesis of F0 contour */
		if (v->lpcprosody) {
			synf0 = 0; outf0 = 0;
			for (l = 0; l < 2 * pitch; l++) {
				sigpos++;
				if (!(sigpos % lppstep)) {	// new pitch value into f0 output filter
					//printf("LPP position point %d, exc=%.2f synf0=%d otf0=%.2f L=%d\n",sigpos,exc,synf0,outf0,lppitch);
					synf0 = 0;
					if (!(sigpos % lpestep)) {	// new excitation value into recontruction filter
						//printf("   >> LPE position point %d (exc=%.4f) <<   \n",sigpos,exc);
						//printf("lp=[%.3f %.3f %.3f %.3f] lpfilt=[%.3f %.3f %.3f %.3f]\n",lp[0],lp[1],lp[2],lp[3],lpfilt[0],lpfilt[1],lpfilt[2],lpfilt[3]);
						synf0 = exc - lpfilt[0]*lp[0];
						exc = 0;
						for (m = LP_F0_ORD - 1; m > 0; m--) {
							synf0 -= lp[m] * lpfilt[m];
							lpfilt[m] = lpfilt[m-1];
						}
						lpfilt[0] = synf0;
					}
					ofilt[0] = synf0;
					synf0 = 0;
					for (m = 1; m < F0_FILT_ORD; m++) ofilt[0] -= a[m] * ofilt[m];
					outf0 = 0;
					for (m = 0; m < F0_FILT_ORD; m++) outf0 += b[m] * ofilt[m];
					//printf("of=[%.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f %.3f]\n",ofilt[0],ofilt[1],
					//	ofilt[2],ofilt[3],ofilt[4],ofilt[5],ofilt[6],ofilt[7],ofilt[8]);
					lppitch = (int)(v->samp_rate / (basef0 + outf0));
					outf0 = 0;
					for (m = F0_FILT_ORD - 1; m > 0; m--) ofilt[m] = ofilt[m - 1];
				}
			}
		}
		w->sample((SAMPLE *)out_buff + max_frame - pitch, pitch);
		//printf("  j:%d difpos:%d diflen:%d",j,difpos,diflen);
		difpos += diflen;
		if (difpos < -pitch) {
			if (reply == 1) j--; else k--;
			difpos += pitch;
		}
		if (difpos > pitch) {
			if (reply == 1) j++;
			else if (k == reply - 1) { j++; k = 1; }
			else k++;
			difpos -= pitch;
		}
		//printf(" -> j:%d difpos:%d\n",j,difpos);
	}
}
