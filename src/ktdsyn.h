/*
 *	epos/src/ktdsyn.h
 *	(c) 1996-98 Zdenek Kadlec, kadlec@phil.muni.cz
 *	(c) 1997-98 Martin Petriska, petriska@decef.elf.stuba.sk
 *	(c) 1997-98 Petr Horak, horak@ure.cas.cz
 *		Czech Academy of Sciences (URE AV CR)
 *	(c) 1998 Jirka Hanika, geo@cuni.cz
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

#ifndef EPOS_KTD_H
#define EPOS_KTD_H

#define  sample2_t	unsigned char

typedef struct
  {
    char jm[3];
    char imp;
    int delk;
  } USEK;

class ktdsyn : public synth
{
   public:
	double buf[9];
	USEK U[1000];
	int uind, pocimp, peri, pocp;
	double delka, smer, pomr;
	int po_u;			//pocet usekov

	int dif2psl (char *, char *);
	int op_psl (char *, sample2_t *);

//	     ktdsyn (int, int);
	     ktdsyn (voice *);
	    ~ktdsyn ();
	virtual void syndiph (voice *v, diphone d, wavefm *w);
	
};

#endif		// EPOS_KTD_H
