/*
 *	epos/src/lpcfest.h
 *	(c) 1994-2000 Petr Horak, horak@ure.cas.cz
 *		Czech Academy of Sciences (URE AV CR)
 *	(c) 1997-2000 Jirka Hanika, geo@cuni.cz
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

#ifndef LPCFEST_H
#define LPCFEST_H

#include <math.h>
#include "clambr.h"

#ifndef uchar
	#define uchar unsigned char
#endif

#ifndef pii
    #define pii 3.141592653589793
#endif

#define SEGM 2048

struct label_hdr
{
	//char	*label;
	int16_t length, frames;
	float	*coeffs;
	char	*residual;
};

struct snd_hdr {
   char		magic[4];       /* magic number */
   int32_t	hdr_size;    /* size of this header */
   int32_t	data_size;   /* length of data (optional) */
   int32_t	encoding;    /* data encoding format */
   int32_t	sample_rate; /* samples per second */
   int32_t	channels;    /* number of interleaved channels */
   //__int16	*data;
};

struct lpc_coeff_hdr {
	float	p_mark;
	float	lpccoff[];
};

/*template <class T, int i> struct lpc_struct {
public:
	T p_mark;
	T lpccoff[i];
};*/

void CompRepFact(int time, label_hdr *pv_unit, label_hdr *df_unit, label_hdr *nx_unit, int *rep);
int hamkoe(int winlen, double *data);
void hann(double *x, int n);
void hannAs(double *x, int n1, int n2);

class lpcfest : public synth
{
	//int ipitch, lastl;
	//int iyold, ifilt[8], kyu;
   protected:
	   file			*models;
	   //label_hdr	*def_units;
	   char			*coeffs, *lpc_units;
	   int			nmbr_unit, order;
	   char			*index_name;
       voice        *vc;

	public:
	   inline int	reverse32 (int N);
	   inline int	ulaw2linear(uchar ulawbyte);
	   char			*convert_tab(char *chr);
	   //void			change_mbrola(char *b, mbr_format *mbr, int n, voice *v);
       //void			clear_text(mbr_format *mbr, bool flag);
	   void			get_unit(label_hdr* lpc_unit, char* this_unit, char* next_unit);	   
       int          prepare_units(label_hdr **lpc_unit, mbr_format *mb_text, int num);
	   lpcfest(voice *);
	   ~lpcfest(void);
	   virtual void synseg(voice *v, segment d, wavefm *w);
       virtual void synssif(voice *v, char *, wavefm *w);
	   //virtual void frobmod(int imodel, segment d, model *m, int &incrl, int &znely) = 0;
};

#endif		// LPC_H

