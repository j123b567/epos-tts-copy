/*
 *	epos/src/tdpsyn.cc
 * 	(c) 2000-2001 Petr Horak, petr.horak@click.cz
 * 	(c) 2001 Jirka Hanika, geo@cuni.cz
 *
 *	tdpsyn version 1.1 (31.10.2001)
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

#ifndef	EPOS_TDP_H
#define	EPOS_TDP_H

#define t_samp short

const double pii = 3.141592653589793;

int hamkoe(int winlen, double *data);
int median(int lold, int lact, int lnext, int ibonus);

class tdpsyn : public synth
{
   private:
	t_samp *tdp_buff;
	t_samp *out_buff;
	int *ppulses;
	int *diph_offs;
	int *diph_len;
	int difpos;
	unsigned short *wwin;
	
	file *tdi;
	
	int average_pitch(int offs, int len);
	
	int max_frame;
   public:
	tdpsyn(voice *);
	virtual ~tdpsyn(void);
	void synseg(voice *v, segment d, wavefm *w);
};

#endif		// EPOS_TDP_H
