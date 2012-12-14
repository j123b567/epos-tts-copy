/*
 * 	epos/src/tdpmbr.cc
 * 	(c) 2006 Zdenìk Chaloupka, chaloupka@ure.cas.cz
 *
 *	tdpmbr version 0.1 (20.9.2002)
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

#ifndef	TDPMBR_H
#define	TDPMBR_H

#ifndef LPC_PROS_ORDER
	#define LPC_PROS_ORDER 4
#endif

#ifndef MAX_OFILT_ORDER
	#define MAX_OFILT_ORDER 9
#endif

#ifndef pii
#define pii 3.141592653589793
#endif

#include "clambr.h"

struct tdi_hdr {
	int32_t  magic;
	int32_t  samp_rate;
	int32_t  samp_size;
	int32_t  bufpos;
	int32_t  n_segs;
	int32_t  diph_offs;
	int32_t  diph_len;
	int32_t  phone_ons;
	int32_t  res1;
	int32_t  ppulses;
	int32_t  res2;
	int32_t  res3;
};

//const double pii = 3.141592653589793;

//int hamkoe(int winlen, double *data);
//int median(int lold, int lact, int lnext, int ibonus);

class tdpmbr : public synth
{
   private:
	   SAMPLE *tdp_buff;
	   //SAMPLE *out_buff;
	   int32_t	*ppulses;
	   int32_t	*diph_offs;
	   int32_t	*diph_len;
	   int32_t	*num_phones;
	   int32_t	*loc_phones;
	   char	*phones;
	   int difpos;

	   /*	chaloupka */
	   int ola;
	   int srate;
	   int alloc;
	   double *win;
	   int16_t	*buffer;
	   int16_t	start;
	   int16_t	slen;
	   int16_t	length;
	   float	last_len;
	   mbrtdp	*mb;
	   /*	chaloupka */
	   
	   /* komentoval chaloupka
	   double lpfilt[LPC_PROS_ORDER];
	   double ofilt[MAX_OFILT_ORDER];
	   double smoothfilt[MAX_OFILT_ORDER];
	   int lppitch;
	   int lpestep;
	   int lppstep;
	   unsigned int sigpos;
	   int basef0;
	   int filtf0;
	   */
	
	   file *tdi;
	   
	   int average_pitch(int offs, int len);
	   
	   int	max_frame;

   public:
	   //int	convert_tab(char *chr);
	   //char		convert(char *txt, char *sampaT, char *iso);
//	   int16_t	*tri_trans(mbr_format *txt, int numPhn, char *b);
	   //void		change_mbrola(char *b, mbr_format *mbr, voice *v);
	   //void		clear_text(mbr_format *mbr, bool flag);
	   //void		preprocess(mbr_format *text, char *unit);
	   //int16_t *extract(char *b, char *text);
	   int		explosives_detection(int begin, int code);
	   void		syn(mbr_format *ph, float len, int16_t code, wavefm *w);
	   tdpmbr(voice *);
	   virtual ~tdpmbr(void);
	   void	synseg(voice *v, segment d, wavefm *w);
	   virtual void synssif(voice *v, char *, wavefm *w);
};

#endif		// TDPMBR_H
